/*
 * SDLBridge for DPF
 * Copyright (C) 2021-2022 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef SDLBRIDGE_HPP_INCLUDED
#define SDLBRIDGE_HPP_INCLUDED

#include "JackBridge.hpp"
#include "../../extra/RingBuffer.hpp"

#include <SDL.h>

struct SDLBridge {
    SDL_AudioDeviceID deviceId = 0;

    // SDL information
    uint bufferSize = 0;
    uint sampleRate = 0;

    // Port caching information
    uint numAudioIns = 0;
    uint numAudioOuts = 0;
    uint numMidiIns = 0;
    uint numMidiOuts = 0;

    // JACK callbacks
    JackProcessCallback jackProcessCallback = nullptr;
    void* jackProcessArg = nullptr;

    // Runtime buffers
    enum PortMask {
        kPortMaskAudio = 0x1000,
        kPortMaskMIDI = 0x2000,
        kPortMaskInput = 0x4000,
        kPortMaskOutput = 0x8000,
        kPortMaskInputMIDI = kPortMaskInput|kPortMaskMIDI,
        kPortMaskOutputMIDI = kPortMaskOutput|kPortMaskMIDI,
    };
#if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
    float* audioBuffers[DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS];
    float* audioBufferStorage;
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    HeapRingBuffer midiInBuffer;
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    HeapRingBuffer midiOutBuffer;
#endif

    bool open(const char* const clientName)
    {
        SDL_AudioSpec requested, received;
        std::memset(&requested, 0, sizeof(requested));
        requested.format = AUDIO_F32SYS;
        requested.channels = DISTRHO_PLUGIN_NUM_OUTPUTS;
        requested.freq = 48000;
        requested.samples = 512;
        requested.callback = SDLCallback;
        requested.userdata = this;

        SDL_SetHint(SDL_HINT_AUDIO_DEVICE_APP_NAME, clientName);
        // SDL_SetHint(SDL_HINT_AUDIO_DEVICE_STREAM_NAME, );
        SDL_SetHint(SDL_HINT_AUDIO_RESAMPLING_MODE, "2");

        /*
        deviceId = SDL_OpenAudioDevice("PulseAudio", 0,
                                       &requested, &received,
                                       SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
        */
        deviceId = SDL_OpenAudio(&requested, &received) == 0 ? 1 : 0;
        DISTRHO_SAFE_ASSERT_RETURN(deviceId != 0, false);

        if (received.channels != DISTRHO_PLUGIN_NUM_OUTPUTS)
        {
            SDL_CloseAudioDevice(deviceId);
            deviceId = 0;
            d_stderr2("Invalid or missing audio output channels");
            return false;
        }

        bufferSize = received.samples;
        sampleRate = received.freq;

#if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        audioBufferStorage = new float[bufferSize*(DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS)];
#if DISTRHO_PLUGIN_NUM_INPUTS > 0
        std::memset(audioBufferStorage, 0, sizeof(float)*bufferSize*DISTRHO_PLUGIN_NUM_INPUTS);
#endif

        for (uint i=0; i<DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
            audioBuffers[i] = audioBufferStorage + (bufferSize * i);
#endif

        return true;
    }

    bool close()
    {
        DISTRHO_SAFE_ASSERT_RETURN(deviceId != 0, false);

        // SDL_CloseAudioDevice(deviceId);
        SDL_CloseAudio();
        deviceId = 0;

#if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        delete[] audioBufferStorage;
        audioBufferStorage = nullptr;
#endif

        return true;
    }

    bool activate()
    {
        DISTRHO_SAFE_ASSERT_RETURN(deviceId != 0, false);

        SDL_PauseAudioDevice(deviceId, 0);
        return true;
    }

    bool deactivate()
    {
        DISTRHO_SAFE_ASSERT_RETURN(deviceId != 0, false);

        SDL_PauseAudioDevice(deviceId, 1);
        return true;
    }

    jack_port_t* registerPort(const char* const type, const ulong flags)
    {
        bool isAudio, isInput;

        /**/ if (std::strcmp(type, JACK_DEFAULT_AUDIO_TYPE) == 0)
            isAudio = true;
        else if (std::strcmp(type, JACK_DEFAULT_MIDI_TYPE) == 0)
            isAudio = false;
        else
            return nullptr;

        /**/ if (flags & JackPortIsInput)
            isInput = true;
        else if (flags & JackPortIsOutput)
            isInput = false;
        else
            return nullptr;

        const uintptr_t ret = (isAudio ? kPortMaskAudio : kPortMaskMIDI)
                            | (isInput ? kPortMaskInput : kPortMaskOutput);

        return (jack_port_t*)(ret + (isAudio ? (isInput ? numAudioIns++ : numAudioOuts++)
                                             : (isInput ? numMidiIns++ : numMidiOuts++)));
    }

    void* getPortBuffer(jack_port_t* const port)
    {
        const uintptr_t portMask = (uintptr_t)port;
        DISTRHO_SAFE_ASSERT_RETURN(portMask != 0x0, nullptr);

#if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        if (portMask & kPortMaskAudio)
            return audioBuffers[(portMask & kPortMaskInput ? 0 : DISTRHO_PLUGIN_NUM_INPUTS) + (portMask & 0x0fff)];
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if ((portMask & kPortMaskInputMIDI) == kPortMaskInputMIDI)
            return &midiInBuffer;
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        if ((portMask & kPortMaskOutputMIDI) == kPortMaskOutputMIDI)
            return &midiOutBuffer;
#endif

        return nullptr;
    }

    static void SDLCallback(void* const userData, uchar* const stream, const int len)
    {
        SDLBridge* const self = (SDLBridge*)userData;

        // safety checks
        DISTRHO_SAFE_ASSERT_RETURN(stream != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(len > 0,);

        float* const fstream = (float*)stream;

// #if DISTRHO_PLUGIN_NUM_OUTPUTS == 0
        if (self->jackProcessCallback == nullptr)
// #endif
        {
            std::memset(fstream, 0, len);
            return;
        }

// #if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        const uint numFrames = static_cast<uint>(static_cast<uint>(len) / sizeof(float) / DISTRHO_PLUGIN_NUM_OUTPUTS);

        self->jackProcessCallback(numFrames, self->jackProcessArg);

        for (uint i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
        {
            for (uint j=0; j < numFrames; ++j)
                fstream[j * DISTRHO_PLUGIN_NUM_OUTPUTS + i] = self->audioBuffers[DISTRHO_PLUGIN_NUM_INPUTS+i][j];
        }
// #endif
    }

};

#endif
