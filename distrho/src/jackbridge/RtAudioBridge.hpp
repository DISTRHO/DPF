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
# define RTMIDI_API_TYPE MACOSX_CORE
#elif defined(DISTRHO_OS_WINDOWS) && !defined(_MSC_VER)
# define __WINDOWS_DS__
# define __WINDOWS_MM__
# define RTAUDIO_API_TYPE WINDOWS_DS
# define RTMIDI_API_TYPE WINDOWS_MM
#else
# if defined(HAVE_PULSEAUDIO)
#  define __LINUX_PULSE__
#  define RTAUDIO_API_TYPE LINUX_PULSE
# elif defined(HAVE_ALSA)
#  define RTAUDIO_API_TYPE LINUX_ALSA
# endif
# ifdef HAVE_ALSA
#  define __LINUX_ALSA__
#  define RTMIDI_API_TYPE LINUX_ALSA
# endif
#endif

#ifdef RTAUDIO_API_TYPE
# include "rtaudio/RtAudio.h"
# include "rtmidi/RtMidi.h"
# include "../../extra/ScopedPointer.hpp"
# include "../../extra/String.hpp"

using DISTRHO_NAMESPACE::ScopedPointer;
using DISTRHO_NAMESPACE::String;

struct RtAudioBridge : NativeBridge {
    // pointer to RtAudio instance
    ScopedPointer<RtAudio> handle;
    bool captureEnabled = false;
   #if defined(RTMIDI_API_TYPE) && DISTRHO_PLUGIN_WANT_MIDI_INPUT
    std::vector<RtMidiIn> midiIns;
   #endif
   #if defined(RTMIDI_API_TYPE) && DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    std::vector<RtMidiOut> midiOuts;
   #endif

    // caching
    String name;
    uint nextBufferSize = 512;

    RtAudioBridge()
    {
       #if defined(RTMIDI_API_TYPE) && (DISTRHO_PLUGIN_WANT_MIDI_INPUT || DISTRHO_PLUGIN_WANT_MIDI_OUTPUT)
        midiAvailable = true;
       #endif
    }

    const char* getVersion() const noexcept
    {
        return RTAUDIO_VERSION;
    }

    bool open(const char* const clientName) override
    {
        name = clientName;
        return _open(false);
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

       #if DISTRHO_PLUGIN_NUM_INPUTS > 0
        freeBuffers();
       #endif
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

    bool isAudioInputEnabled() const override
    {
       #if DISTRHO_PLUGIN_NUM_INPUTS > 0
        return captureEnabled;
       #else
        return false;
       #endif
    }

    bool requestAudioInput() override
    {
       #if DISTRHO_PLUGIN_NUM_INPUTS > 0
        // stop audio first
        deactivate();
        close();

        // try to open with capture enabled
        const bool ok = _open(true);

        if (ok)
            captureEnabled = true;
        else
            _open(false);

        activate();
        return ok;
       #else
        return false;
       #endif
    }

    bool isMIDIEnabled() const override
    {
        d_stdout("%s %d", __PRETTY_FUNCTION__, __LINE__);
       #if defined(RTMIDI_API_TYPE) && DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if (!midiIns.empty())
            return true;
       #endif
       #if defined(RTMIDI_API_TYPE) && DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        if (!midiOuts.empty())
            return true;
       #endif
        return false;
    }

    bool requestMIDI() override
    {
        d_stdout("%s %d", __PRETTY_FUNCTION__, __LINE__);
        // clear ports in use first
       #if defined(RTMIDI_API_TYPE) && DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if (!midiIns.empty())
        {
            try {
                midiIns.clear();
            } catch (const RtMidiError& err) {
                d_safe_exception(err.getMessage().c_str(), __FILE__, __LINE__);
                return false;
            } DISTRHO_SAFE_EXCEPTION_RETURN("midiIns.clear()", false);
        }
       #endif
       #if defined(RTMIDI_API_TYPE) && DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        if (!midiOuts.size())
        {
            try {
                midiOuts.clear();
            } catch (const RtMidiError& err) {
                d_safe_exception(err.getMessage().c_str(), __FILE__, __LINE__);
                return false;
            } DISTRHO_SAFE_EXCEPTION_RETURN("midiOuts.clear()", false);
        }
       #endif

        // query port count
       #if defined(RTMIDI_API_TYPE) && DISTRHO_PLUGIN_WANT_MIDI_INPUT
        uint midiInCount;
        try {
            RtMidiIn midiIn(RtMidi::RTMIDI_API_TYPE, name.buffer());
            midiInCount = midiIn.getPortCount();
        } catch (const RtMidiError& err) {
            d_safe_exception(err.getMessage().c_str(), __FILE__, __LINE__);
            return false;
        } DISTRHO_SAFE_EXCEPTION_RETURN("midiIn.getPortCount()", false);
       #endif
       #if defined(RTMIDI_API_TYPE) && DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        uint midiOutCount;
        try {
            RtMidiOut midiOut(RtMidi::RTMIDI_API_TYPE, name.buffer());
            midiOutCount = midiOut.getPortCount();
        } catch (const RtMidiError& err) {
            d_safe_exception(err.getMessage().c_str(), __FILE__, __LINE__);
            return false;
        } DISTRHO_SAFE_EXCEPTION_RETURN("midiOut.getPortCount()", false);
       #endif

        // open all possible ports
       #if defined(RTMIDI_API_TYPE) && DISTRHO_PLUGIN_WANT_MIDI_INPUT
        for (uint i=0; i<midiInCount; ++i)
        {
            try {
                RtMidiIn midiIn(RtMidi::RTMIDI_API_TYPE, name.buffer());
                midiIn.setCallback(RtMidiCallback, this);
                midiIn.openPort(i);
                midiIns.push_back(std::move(midiIn));
            } catch (const RtMidiError& err) {
                d_safe_exception(err.getMessage().c_str(), __FILE__, __LINE__);
            } DISTRHO_SAFE_EXCEPTION("midiIn.openPort()");
        }
       #endif
       #if defined(RTMIDI_API_TYPE) && DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        for (uint i=0; i<midiOutCount; ++i)
        {
            try {
                RtMidiOut midiOut(RtMidi::RTMIDI_API_TYPE, name.buffer());
                midiOut.openPort(i);
                midiOuts.push_back(std::move(midiOut));
            } catch (const RtMidiError& err) {
                d_safe_exception(err.getMessage().c_str(), __FILE__, __LINE__);
            } DISTRHO_SAFE_EXCEPTION("midiOut.openPort()");
        }
       #endif

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

        const bool ok = _open(captureEnabled);

        if (!ok)
        {
            // revert to old buffer size if new one failed
            nextBufferSize = bufferSize;
            _open(captureEnabled);
        }

        if (bufferSizeCallback != nullptr)
            bufferSizeCallback(bufferSize, jackBufferSizeArg);

        activate();
        return ok;
    }
   #endif

    bool _open(const bool withInput)
    {
        ScopedPointer<RtAudio> rtAudio;

        try {
            rtAudio = new RtAudio(RtAudio::RTAUDIO_API_TYPE);
        } DISTRHO_SAFE_EXCEPTION_RETURN("new RtAudio()", false);

        uint rtAudioBufferFrames = nextBufferSize;

       #if DISTRHO_PLUGIN_NUM_INPUTS > 0
        RtAudio::StreamParameters inParams;
       #endif
        RtAudio::StreamParameters* inParamsPtr = nullptr;

       #if DISTRHO_PLUGIN_NUM_INPUTS > 0
        if (withInput)
        {
            inParams.deviceId = rtAudio->getDefaultInputDevice();
            inParams.nChannels = DISTRHO_PLUGIN_NUM_INPUTS;
            inParamsPtr = &inParams;
        }
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
        opts.streamName = name.buffer();

        try {
            rtAudio->openStream(outParamsPtr, inParamsPtr, RTAUDIO_FLOAT32, 48000, &rtAudioBufferFrames,
                                RtAudioCallback, this, &opts, nullptr);
        } catch (const RtAudioError& err) {
            d_safe_exception(err.getMessage().c_str(), __FILE__, __LINE__);
            return false;
        } DISTRHO_SAFE_EXCEPTION_RETURN("rtAudio->openStream()", false);

        handle = rtAudio;
        bufferSize = rtAudioBufferFrames;
        sampleRate = handle->getStreamSampleRate();
        allocBuffers(!withInput, true);
        return true;
    }

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

   #if defined(RTMIDI_API_TYPE) && DISTRHO_PLUGIN_WANT_MIDI_INPUT
    static void RtMidiCallback(double /*timeStamp*/, std::vector<uchar>* message, void* userData)
    {
        const size_t len = message->size();
        DISTRHO_SAFE_ASSERT_RETURN(len > 0 && len <= kMaxMIDIInputMessageSize,);

        RtAudioBridge* const self = static_cast<RtAudioBridge*>(userData);

        // TODO timestamp handling
        self->midiInBufferPending.writeByte(static_cast<uint8_t>(len));
        self->midiInBufferPending.writeCustomData(message->data(), len);
        for (uint8_t i=0; i<len; ++i)
            self->midiInBufferPending.writeByte(0);
        self->midiInBufferPending.commitWrite();
    }
   #endif
};

#endif // RTAUDIO_API_TYPE
#endif // RTAUDIO_BRIDGE_HPP_INCLUDED
