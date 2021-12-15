/*
 * RtAudioBridge for DPF
 * Copyright (C) 2021 Filipe Coelho <falktx@falktx.com>
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

#ifndef RTAUDIOBRIDGE_HPP_INCLUDED
#define RTAUDIOBRIDGE_HPP_INCLUDED

#include "JackBridge.hpp"

#if defined(DISTRHO_OS_MAC)
# define __MACOSX_CORE__
# define RTAUDIO_API_TYPE MACOSX_CORE
#elif defined(DISTRHO_OS_WINDOWS) && !defined(_MSC_VER)
# define __WINDOWS_DS__
# define RTAUDIO_API_TYPE WINDOWS_DS
#elif defined(HAVE_PULSEAUDIO)
# define __LINUX_PULSE__
# define RTAUDIO_API_TYPE LINUX_PULSE
#elif defined(HAVE_ALSA)
# define __LINUX_ALSA__
# define RTAUDIO_API_TYPE LINUX_ALSA
#endif

#ifdef RTAUDIO_API_TYPE
# define Point CorePoint /* fix conflict between DGL and macOS Point name */
# include "rtaudio/RtAudio.h"
# undef Point
# include "../../extra/RingBuffer.hpp"
# include "../../extra/ScopedPointer.hpp"

using DISTRHO_NAMESPACE::HeapRingBuffer;
using DISTRHO_NAMESPACE::ScopedPointer;

struct RtAudioBridge {
    // pointer to RtAudio instance
    ScopedPointer<RtAudio> handle;

    // RtAudio information
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
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    HeapRingBuffer midiInBuffer;
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    HeapRingBuffer midiOutBuffer;
#endif

    bool open()
    {
        ScopedPointer<RtAudio> rtAudio;

        try {
            rtAudio = new RtAudio(RtAudio::RTAUDIO_API_TYPE);
        } DISTRHO_SAFE_EXCEPTION_RETURN("new RtAudio()", false);

        uint rtAudioBufferFrames = 512;

#if DISTRHO_PLUGIN_NUM_INPUTS > 0
        RtAudio::StreamParameters inParams;
        RtAudio::StreamParameters* const inParamsPtr = &inParams;
        inParams.deviceId = rtAudio->getDefaultInputDevice();
        inParams.nChannels = DISTRHO_PLUGIN_NUM_INPUTS;
#else
        RtAudio::StreamParameters* const inParamsPtr = nullptr;
#endif

        RtAudio::StreamParameters outParams;
        outParams.deviceId = rtAudio->getDefaultOutputDevice();
        outParams.nChannels = DISTRHO_PLUGIN_NUM_OUTPUTS;

        RtAudio::StreamOptions opts;
        opts.flags = RTAUDIO_NONINTERLEAVED | RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_ALSA_USE_DEFAULT;

        try {
            rtAudio->openStream(&outParams, inParamsPtr, RTAUDIO_FLOAT32, 48000, &rtAudioBufferFrames, RtAudioCallback, this, &opts, nullptr);
        } catch (const RtAudioError& err) {
            d_safe_exception(err.getMessage().c_str(), __FILE__, __LINE__);
            return false;
        } DISTRHO_SAFE_EXCEPTION_RETURN("rtAudio->openStream()", false);

        handle = rtAudio;
        bufferSize = rtAudioBufferFrames;
        sampleRate = handle->getStreamSampleRate();

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        midiInBuffer.createBuffer(128);
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        midiOutBuffer.createBuffer(128);
#endif
        return true;
    }

    bool close()
    {
        DISTRHO_SAFE_ASSERT_RETURN(handle != nullptr, false);

        if (handle->isStreamRunning())
        {
            try {
                handle->abortStream();
            } DISTRHO_SAFE_EXCEPTION("handle->abortStream()");
        }

        handle = nullptr;
        return true;
    }

    bool activate()
    {
        DISTRHO_SAFE_ASSERT_RETURN(handle != nullptr, false);

        try {
            handle->startStream();
        } DISTRHO_SAFE_EXCEPTION("handle->startStream()");

        return true;
    }

    bool deactivate()
    {
        DISTRHO_SAFE_ASSERT_RETURN(handle != nullptr, false);

        try {
            handle->stopStream();
        } DISTRHO_SAFE_EXCEPTION("handle->stopStream()");

        return true;
    }

    jack_port_t* registerPort(const char* const type, const ulong flags)
    {
        bool isAudio, isInput;

        if (std::strcmp(type, JACK_DEFAULT_AUDIO_TYPE) == 0)
            isAudio = true;
        else if (std::strcmp(type, JACK_DEFAULT_MIDI_TYPE) == 0)
            isAudio = false;
        else
            return nullptr;

        if (flags & JackPortIsInput)
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

    static int RtAudioCallback(void* const outputBuffer,
                               void* const inputBuffer,
                               const uint numFrames,
                               const double /* streamTime */,
                               const RtAudioStreamStatus /* status */,
                               void* const userData)
    {
        RtAudioBridge* const self = (RtAudioBridge*)userData;

        if (self->jackProcessCallback == nullptr)
        {
            if (outputBuffer != nullptr)
                std::memset((float*)outputBuffer, 0, sizeof(float)*numFrames*DISTRHO_PLUGIN_NUM_OUTPUTS);
            return 0;
        }

#if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        float** const selfAudioBuffers = self->audioBuffers;

        uint i = 0;
# if DISTRHO_PLUGIN_NUM_INPUTS > 0
        if (float* const insPtr  = (float*)inputBuffer)
        {
            for (uint j=0; j<DISTRHO_PLUGIN_NUM_INPUTS; ++j, ++i)
                selfAudioBuffers[i] = insPtr + (j * numFrames);
        }
# endif
# if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        if (float* const outsPtr = (float*)outputBuffer)
        {
            for (uint j=0; j<DISTRHO_PLUGIN_NUM_OUTPUTS; ++j, ++i)
                selfAudioBuffers[i] = outsPtr + (j * numFrames);
        }
# endif
#endif

        self->jackProcessCallback(numFrames, self->jackProcessArg);
        return 0;

#if DISTRHO_PLUGIN_NUM_INPUTS == 0
        // unused
        (void)inputBuffer;
#endif
    }
};

#endif // RTAUDIO_API_TYPE
#endif // RTAUDIOBRIDGE_HPP_INCLUDED
