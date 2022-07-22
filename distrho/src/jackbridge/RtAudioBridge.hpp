/*
 * RtAudio Bridge for DPF
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

#ifndef RTAUDIO_BRIDGE_HPP_INCLUDED
#define RTAUDIO_BRIDGE_HPP_INCLUDED

#include "NativeBridge.hpp"

#if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS == 0
# error RtAudio without audio does not make sense
#endif

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
# include "../../extra/ScopedPointer.hpp"
# include "../../extra/String.hpp"

using DISTRHO_NAMESPACE::ScopedPointer;

struct RtAudioBridge : NativeBridge {
    // pointer to RtAudio instance
    ScopedPointer<RtAudio> handle;

    // for buffer size changes
    String name;
    uint nextBufferSize = 512;

    const char* getVersion() const noexcept
    {
        return RTAUDIO_VERSION;
    }

    bool open(const char* const clientName) override
    {
        ScopedPointer<RtAudio> rtAudio;

        try {
            rtAudio = new RtAudio(RtAudio::RTAUDIO_API_TYPE);
        } DISTRHO_SAFE_EXCEPTION_RETURN("new RtAudio()", false);

        uint rtAudioBufferFrames = nextBufferSize;

       #if DISTRHO_PLUGIN_NUM_INPUTS > 0
        RtAudio::StreamParameters inParams;
        inParams.deviceId = rtAudio->getDefaultInputDevice();
        inParams.nChannels = DISTRHO_PLUGIN_NUM_INPUTS;
        RtAudio::StreamParameters* const inParamsPtr = &inParams;
       #else
        RtAudio::StreamParameters* const inParamsPtr = nullptr;
       #endif

       #if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        RtAudio::StreamParameters outParams;
        outParams.deviceId = rtAudio->getDefaultOutputDevice();
        outParams.nChannels = DISTRHO_PLUGIN_NUM_OUTPUTS;
        RtAudio::StreamParameters* const outParamsPtr = &outParams;
       #else
        RtAudio::StreamParameters* const outParamsPtr = nullptr;
       #endif

        RtAudio::StreamOptions opts;
        opts.flags = RTAUDIO_NONINTERLEAVED | RTAUDIO_MINIMIZE_LATENCY | RTAUDIO_ALSA_USE_DEFAULT;
        opts.streamName = clientName;

        try {
            rtAudio->openStream(outParamsPtr, inParamsPtr, RTAUDIO_FLOAT32, 48000, &rtAudioBufferFrames,
                                RtAudioCallback, this, &opts, nullptr);
        } catch (const RtAudioError& err) {
            d_safe_exception(err.getMessage().c_str(), __FILE__, __LINE__);
            return false;
        } DISTRHO_SAFE_EXCEPTION_RETURN("rtAudio->openStream()", false);

        name = clientName;
        handle = rtAudio;
        bufferSize = rtAudioBufferFrames;
        sampleRate = handle->getStreamSampleRate();
        return true;
    }

    bool close() override
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

    bool activate() override
    {
        DISTRHO_SAFE_ASSERT_RETURN(handle != nullptr, false);

        try {
            handle->startStream();
        } DISTRHO_SAFE_EXCEPTION_RETURN("handle->startStream()", false);

        return true;
    }

    bool deactivate() override
    {
        DISTRHO_SAFE_ASSERT_RETURN(handle != nullptr, false);

        try {
            handle->stopStream();
        } DISTRHO_SAFE_EXCEPTION_RETURN("handle->stopStream()", false);

        return true;
    }

    /* RtAudio in macOS uses a different than usual way to handle audio block size,
     * where RTAUDIO_MINIMIZE_LATENCY makes CoreAudio use very low latencies (around 15 samples).
     * As such, dynamic buffer sizes are meaningless there.
     */
   #ifndef DISTRHO_OS_MAC
    bool supportsBufferSizeChanges() const override
    {
        return true;
    }

    bool requestBufferSizeChange(const uint32_t newBufferSize) override
    {
        // stop audio first
        deactivate();
        close();

        // try to open with new buffer size
        nextBufferSize = newBufferSize;

        const bool ok = open(name);

        if (!ok)
        {
            // revert to old buffer size if new one failed
            nextBufferSize = bufferSize;
            open(name);
        }

        if (bufferSizeCallback != nullptr)
            bufferSizeCallback(bufferSize, jackBufferSizeArg);

        activate();
        return ok;
    }
   #endif

    static int RtAudioCallback(void* const outputBuffer,
                              #if DISTRHO_PLUGIN_NUM_INPUTS > 0
                               void* const inputBuffer,
                              #else
                               void*,
                              #endif
                               const uint numFrames,
                               const double /* streamTime */,
                               const RtAudioStreamStatus /* status */,
                               void* const userData)
    {
        RtAudioBridge* const self = static_cast<RtAudioBridge*>(userData);

        if (self->jackProcessCallback == nullptr)
        {
            if (outputBuffer != nullptr)
                std::memset((float*)outputBuffer, 0, sizeof(float)*numFrames*DISTRHO_PLUGIN_NUM_OUTPUTS);
            return 0;
        }

       #if DISTRHO_PLUGIN_NUM_INPUTS > 0
        if (float* const insPtr = static_cast<float*>(inputBuffer))
        {
            for (uint i=0; i<DISTRHO_PLUGIN_NUM_INPUTS; ++i)
                self->audioBuffers[i] = insPtr + (i * numFrames);
        }
       #endif

       #if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        if (float* const outsPtr = static_cast<float*>(outputBuffer))
        {
            for (uint i=0; i<DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
                self->audioBuffers[DISTRHO_PLUGIN_NUM_INPUTS + i] = outsPtr + (i * numFrames);
        }
       #endif

        self->jackProcessCallback(numFrames, self->jackProcessArg);

        return 0;
    }
};

#endif // RTAUDIO_API_TYPE
#endif // RTAUDIO_BRIDGE_HPP_INCLUDED
