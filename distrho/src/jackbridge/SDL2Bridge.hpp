/*
 * SDL Bridge for DPF
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

#ifndef SDL_BRIDGE_HPP_INCLUDED
#define SDL_BRIDGE_HPP_INCLUDED

#include "NativeBridge.hpp"

#include <SDL.h>

#ifndef SDL_HINT_AUDIO_DEVICE_APP_NAME
# define SDL_HINT_AUDIO_DEVICE_APP_NAME "SDL_AUDIO_DEVICE_APP_NAME"
#endif

#ifndef SDL_HINT_AUDIO_DEVICE_STREAM_NAME
# define SDL_HINT_AUDIO_DEVICE_STREAM_NAME "SDL_AUDIO_DEVICE_STREAM_NAME"
#endif

#if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS == 0
# error SDL without audio does not make sense
#endif

struct SDL2Bridge : NativeBridge {
   #if DISTRHO_PLUGIN_NUM_INPUTS > 0
    SDL_AudioDeviceID captureDeviceId;
   #endif
   #if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
    SDL_AudioDeviceID playbackDeviceId;
   #endif

    SDL2Bridge()
        : NativeBridge()
       #if DISTRHO_PLUGIN_NUM_INPUTS > 0
        , captureDeviceId(0)
       #endif
       #if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        , playbackDeviceId(0)
       #endif
    {}

    bool open(const char* const clientName) override
    {
        SDL_InitSubSystem(SDL_INIT_AUDIO);

        SDL_AudioSpec requested;
        std::memset(&requested, 0, sizeof(requested));
        requested.format = AUDIO_F32SYS;
        requested.freq = 48000;
        requested.samples = 512;
        requested.userdata = this;

        SDL_SetHint(SDL_HINT_AUDIO_DEVICE_APP_NAME, clientName);
        // SDL_SetHint(SDL_HINT_AUDIO_RESAMPLING_MODE, "1");

       #if DISTRHO_PLUGIN_NUM_INPUTS > 0
        SDL_SetHint(SDL_HINT_AUDIO_DEVICE_STREAM_NAME, "Capure");
        requested.channels = DISTRHO_PLUGIN_NUM_INPUTS;
        requested.callback = AudioInputCallback;

        SDL_AudioSpec receivedCapture;
        captureDeviceId = SDL_OpenAudioDevice(nullptr, 1, &requested, &receivedCapture,
                                              SDL_AUDIO_ALLOW_FREQUENCY_CHANGE|SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
        if (captureDeviceId == 0)
        {
            d_stderr2("Failed to open SDL playback device, error was: %s", SDL_GetError());
            return false;
        }

        if (receivedCapture.channels != DISTRHO_PLUGIN_NUM_INPUTS)
        {
            SDL_CloseAudioDevice(captureDeviceId);
            captureDeviceId = 0;
            d_stderr2("Invalid or missing audio input channels");
            return false;
        }
       #endif

       #if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        SDL_AudioSpec receivedPlayback;
        SDL_SetHint(SDL_HINT_AUDIO_DEVICE_STREAM_NAME, "Playback");
        requested.channels = DISTRHO_PLUGIN_NUM_OUTPUTS;
        requested.callback = AudioOutputCallback;

        playbackDeviceId = SDL_OpenAudioDevice(nullptr, 0, &requested, &receivedPlayback,
                                               SDL_AUDIO_ALLOW_FREQUENCY_CHANGE|SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
        if (playbackDeviceId == 0)
        {
            d_stderr2("Failed to open SDL playback device, error was: %s", SDL_GetError());
            return false;
        }

        if (receivedPlayback.channels != DISTRHO_PLUGIN_NUM_OUTPUTS)
        {
            SDL_CloseAudioDevice(playbackDeviceId);
            playbackDeviceId = 0;
            d_stderr2("Invalid or missing audio output channels");
            return false;
        }
       #endif

       #if DISTRHO_PLUGIN_NUM_INPUTS > 0 && DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        // if using both input and output, make sure they match
        if (receivedCapture.samples != receivedPlayback.samples)
        {
            SDL_CloseAudioDevice(captureDeviceId);
            SDL_CloseAudioDevice(playbackDeviceId);
            captureDeviceId = playbackDeviceId = 0;
            d_stderr2("Mismatch buffer size %u vs %u", receivedCapture.samples, receivedCapture.samples);
            return false;
        }
        if (receivedCapture.freq != receivedPlayback.freq)
        {
            SDL_CloseAudioDevice(captureDeviceId);
            SDL_CloseAudioDevice(playbackDeviceId);
            captureDeviceId = playbackDeviceId = 0;
            d_stderr2("Mismatch sample rate %u vs %u", receivedCapture.freq, receivedCapture.freq);
            return false;
        }
       #endif

       #if DISTRHO_PLUGIN_NUM_INPUTS > 0
        bufferSize = receivedCapture.samples;
        sampleRate = receivedCapture.freq;
       #else
        bufferSize = receivedPlayback.samples;
        sampleRate = receivedPlayback.freq;
       #endif

        allocBuffers(true, false);
        return true;
    }

    bool close() override
    {
       #if DISTRHO_PLUGIN_NUM_INPUTS > 0
        DISTRHO_SAFE_ASSERT_RETURN(captureDeviceId != 0, false);
        SDL_CloseAudioDevice(captureDeviceId);
        captureDeviceId = 0;
       #endif
       #if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        DISTRHO_SAFE_ASSERT_RETURN(playbackDeviceId != 0, false);
        SDL_CloseAudioDevice(playbackDeviceId);
        playbackDeviceId = 0;
       #endif

        freeBuffers();
        return true;
    }

    bool activate() override
    {
       #if DISTRHO_PLUGIN_NUM_INPUTS > 0
        DISTRHO_SAFE_ASSERT_RETURN(captureDeviceId != 0, false);
        SDL_PauseAudioDevice(captureDeviceId, 0);
       #endif
       #if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        DISTRHO_SAFE_ASSERT_RETURN(playbackDeviceId != 0, false);
        SDL_PauseAudioDevice(playbackDeviceId, 0);
       #endif
        return true;
    }

    bool deactivate() override
    {
       #if DISTRHO_PLUGIN_NUM_INPUTS > 0
        DISTRHO_SAFE_ASSERT_RETURN(captureDeviceId != 0, false);
        SDL_PauseAudioDevice(captureDeviceId, 1);
       #endif
       #if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        DISTRHO_SAFE_ASSERT_RETURN(playbackDeviceId != 0, false);
        SDL_PauseAudioDevice(playbackDeviceId, 1);
       #endif
        return true;
    }

   #if DISTRHO_PLUGIN_NUM_INPUTS > 0
    static void AudioInputCallback(void* const userData, uchar* const stream, const int len)
    {
        NativeBridge* const self = static_cast<NativeBridge*>(userData);

        // safety checks
        DISTRHO_SAFE_ASSERT_RETURN(stream != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(len > 0,);

        if (self->jackProcessCallback == nullptr)
            return;

        const uint numFrames = static_cast<uint>(len / sizeof(float) / DISTRHO_PLUGIN_NUM_INPUTS);
        DISTRHO_SAFE_ASSERT_UINT2_RETURN(numFrames == self->bufferSize, numFrames, self->bufferSize,);

        const float* const fstream = (const float*)stream;

        for (uint i=0; i<DISTRHO_PLUGIN_NUM_INPUTS; ++i)
        {
            for (uint j=0; j<numFrames; ++j)
                self->audioBuffers[i][j] = fstream[j * DISTRHO_PLUGIN_NUM_INPUTS + i];
        }

       #if DISTRHO_PLUGIN_NUM_OUTPUTS == 0
        // if there are no outputs, run process callback now
        self->jackProcessCallback(numFrames, self->jackProcessArg);
       #endif
    }
   #endif

   #if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
    static void AudioOutputCallback(void* const userData, uchar* const stream, const int len)
    {
        NativeBridge* const self = static_cast<NativeBridge*>(userData);

        // safety checks
        DISTRHO_SAFE_ASSERT_RETURN(stream != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(len > 0,);

        if (self->jackProcessCallback == nullptr)
        {
            std::memset(stream, 0, len);
            return;
        }

        const uint numFrames = static_cast<uint>(len / sizeof(float) / DISTRHO_PLUGIN_NUM_OUTPUTS);
        DISTRHO_SAFE_ASSERT_UINT2_RETURN(numFrames == self->bufferSize, numFrames, self->bufferSize,);

        self->jackProcessCallback(numFrames, self->jackProcessArg);

        float* const fstream = (float*)stream;

        for (uint i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
        {
            for (uint j=0; j < numFrames; ++j)
                fstream[j * DISTRHO_PLUGIN_NUM_OUTPUTS + i] = self->audioBuffers[DISTRHO_PLUGIN_NUM_INPUTS + i][j];
        }
    }
   #endif
};

#endif // SDL_BRIDGE_HPP_INCLUDED
