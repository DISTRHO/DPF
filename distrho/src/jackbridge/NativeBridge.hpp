/*
 * Native Bridge for DPF
 * Copyright (C) 2021-2023 Filipe Coelho <falktx@falktx.com>
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

#ifndef NATIVE_BRIDGE_HPP_INCLUDED
#define NATIVE_BRIDGE_HPP_INCLUDED

#include "JackBridge.hpp"

#include "../../extra/RingBuffer.hpp"

#if DISTRHO_PLUGIN_NUM_INPUTS > 2
# define DISTRHO_PLUGIN_NUM_INPUTS_2 2
#else
# define DISTRHO_PLUGIN_NUM_INPUTS_2 DISTRHO_PLUGIN_NUM_INPUTS
#endif

#if DISTRHO_PLUGIN_NUM_OUTPUTS > 2
# define DISTRHO_PLUGIN_NUM_OUTPUTS_2 2
#else
# define DISTRHO_PLUGIN_NUM_OUTPUTS_2 DISTRHO_PLUGIN_NUM_OUTPUTS
#endif

using DISTRHO_NAMESPACE::HeapRingBuffer;

struct NativeBridge {
    // Current status information
    uint bufferSize;
    uint sampleRate;

    // Port caching information
    uint numAudioIns;
    uint numAudioOuts;
    uint numCvIns;
    uint numCvOuts;
    uint numMidiIns;
    uint numMidiOuts;

    // JACK callbacks
    JackProcessCallback jackProcessCallback;
    JackBufferSizeCallback bufferSizeCallback;
    void* jackProcessArg;
    void* jackBufferSizeArg;

    // Runtime buffers
    enum PortMask {
        kPortMaskAudio = 0x1000,
        kPortMaskCV = 0x2000,
        kPortMaskMIDI = 0x4000,
        kPortMaskInput = 0x10000,
        kPortMaskOutput = 0x20000,
        kPortMaskInputMIDI = kPortMaskInput|kPortMaskMIDI,
        kPortMaskOutputMIDI = kPortMaskOutput|kPortMaskMIDI,
    };
#if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
    float* audioBuffers[DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS];
    float* audioBufferStorage;
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT || DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    bool midiAvailable;
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    static constexpr const uint32_t kMaxMIDIInputMessageSize = 3;
    static constexpr const uint32_t kRingBufferMessageSize = 1u /*+ sizeof(double)*/ + kMaxMIDIInputMessageSize;
    uint8_t midiDataStorage[kMaxMIDIInputMessageSize];
    HeapRingBuffer midiInBufferCurrent;
    HeapRingBuffer midiInBufferPending;
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    HeapRingBuffer midiOutBuffer;
#endif

    NativeBridge()
        : bufferSize(0),
          sampleRate(0),
          numAudioIns(0),
          numAudioOuts(0),
          numCvIns(0),
          numCvOuts(0),
          numMidiIns(0),
          numMidiOuts(0),
          jackProcessCallback(nullptr),
          bufferSizeCallback(nullptr),
          jackProcessArg(nullptr),
          jackBufferSizeArg(nullptr)
       #if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        , audioBuffers()
        , audioBufferStorage(nullptr)
       #endif
       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT || DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
       , midiAvailable(false)
       #endif
    {
       #if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        std::memset(audioBuffers, 0, sizeof(audioBuffers));
       #endif
    }

    virtual ~NativeBridge() {}
    virtual bool open(const char* const clientName) = 0;
    virtual bool close() = 0;
    virtual bool activate() = 0;
    virtual bool deactivate() = 0;

    virtual bool supportsAudioInput() const
    {
       #if DISTRHO_PLUGIN_NUM_INPUTS > 0
        return true;
       #else
        return false;
       #endif
    }

    virtual bool isAudioInputEnabled() const
    {
       #if DISTRHO_PLUGIN_NUM_INPUTS > 0
        return true;
       #else
        return false;
       #endif
    }

    virtual bool supportsBufferSizeChanges() const { return false; }
    virtual bool isMIDIEnabled() const { return false; }
    virtual bool requestAudioInput() { return false; }
    virtual bool requestBufferSizeChange(uint32_t) { return false; }
    virtual bool requestMIDI() { return false; }

    uint32_t getBufferSize() const noexcept
    {
        return bufferSize;
    }

    bool supportsMIDI() const noexcept
    {
       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT || DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        return midiAvailable;
       #else
        return false;
       #endif
    }

    uint32_t getEventCount()
    {
       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if (midiAvailable)
        {
            // NOTE: this function is only called once per run
            midiInBufferCurrent.copyFromAndClearOther(midiInBufferPending);
            return midiInBufferCurrent.getReadableDataSize() / kRingBufferMessageSize;
        }
       #endif

        return 0;
    }

    bool getEvent(jack_midi_event_t* const event)
    {
       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        // NOTE: this function is called for all events in index succession
        if (midiAvailable && midiInBufferCurrent.getReadableDataSize() >= kRingBufferMessageSize)
        {
            event->size = midiInBufferCurrent.readByte();
            // TODO timestamp
            // const double timestamp = midiInBufferCurrent.readDouble();
            event->time = 0;
            event->buffer = midiDataStorage;
            return midiInBufferCurrent.readCustomData(midiDataStorage, kMaxMIDIInputMessageSize);
        }
       #endif
        return false;
        // maybe unused
        (void)event;
    }

    void clearEventBuffer()
    {
       #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        if (midiAvailable)
            midiOutBuffer.flush();
       #endif
    }
    
    bool writeEvent(const jack_nframes_t time, const jack_midi_data_t* const data, const uint32_t size)
    {
        if (size > 3)
            return false;

       #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        if (midiAvailable)
        {
            if (midiOutBuffer.writeByte(size) && midiOutBuffer.writeCustomData(data, size))
            {
                bool fail = false;
                // align
                switch (size)
                {
                case 1: fail |= !midiOutBuffer.writeByte(0);
                // fall-through
                case 2: fail |= !midiOutBuffer.writeByte(0);
                }
                fail |= !midiOutBuffer.writeUInt(time);
                midiOutBuffer.commitWrite();
                return !fail;
            }
            midiOutBuffer.commitWrite();
        }
       #endif

        return false;
        // maybe unused
        (void)data;
        (void)time;
    }

    void allocBuffers(const bool audio, const bool midi)
    {
        DISTRHO_SAFE_ASSERT_RETURN(bufferSize != 0,);

        if (audio)
        {
           #if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
            audioBufferStorage = new float[bufferSize*(DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS)];

            for (uint i=0; i<DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
                audioBuffers[i] = audioBufferStorage + (bufferSize * i);
           #endif

           #if DISTRHO_PLUGIN_NUM_INPUTS > 0
            std::memset(audioBufferStorage, 0, sizeof(float)*bufferSize*DISTRHO_PLUGIN_NUM_INPUTS);
           #endif
        }

        if (midi)
        {
           #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
            midiInBufferCurrent.createBuffer(kMaxMIDIInputMessageSize * 512);
            midiInBufferPending.createBuffer(kMaxMIDIInputMessageSize * 512);
           #endif
           #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
            midiOutBuffer.createBuffer(2048);
           #endif
        }
    }

    void freeBuffers()
    {
       #if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        delete[] audioBufferStorage;
        audioBufferStorage = nullptr;
       #endif
       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        midiInBufferCurrent.deleteBuffer();
        midiInBufferPending.deleteBuffer();
       #endif
       #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        midiOutBuffer.deleteBuffer();
       #endif
    }

    jack_port_t* registerPort(const char* const type, const ulong flags)
    {
        uintptr_t ret = 0;

        /**/ if (flags & JackPortIsInput)
            ret |= kPortMaskInput;
        else if (flags & JackPortIsOutput)
            ret |= kPortMaskOutput;
        else
            return nullptr;

        /**/ if (std::strcmp(type, JACK_DEFAULT_AUDIO_TYPE) == 0)
        {
            if (flags & JackPortIsControlVoltage)
            {
                ret |= kPortMaskAudio;
                ret += flags & JackPortIsInput ? numAudioIns++ : numAudioOuts++;
            }
            else
            {
                ret |= kPortMaskCV;
                ret += flags & JackPortIsInput ? numCvIns++ : numCvOuts++;
            }
        }
        else if (std::strcmp(type, JACK_DEFAULT_MIDI_TYPE) == 0)
        {
            ret |= kPortMaskMIDI;
            ret += flags & JackPortIsInput ? numMidiIns++ : numMidiOuts++;
        }
        else
        {
            return nullptr;
        }

        return (jack_port_t*)ret;
    }

    void* getPortBuffer(jack_port_t* const port)
    {
        const uintptr_t portMask = (uintptr_t)port;
        DISTRHO_SAFE_ASSERT_RETURN(portMask != 0x0, nullptr);

       #if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        if (portMask & (kPortMaskAudio|kPortMaskCV))
            return audioBuffers[(portMask & kPortMaskInput ? 0 : DISTRHO_PLUGIN_NUM_INPUTS) + (portMask & 0x0fff)];
       #endif
       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if ((portMask & kPortMaskInputMIDI) == kPortMaskInputMIDI)
            return (void*)0x1;
       #endif
       #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        if ((portMask & kPortMaskOutputMIDI) == kPortMaskOutputMIDI)
            return (void*)0x2;
       #endif

        return nullptr;
    }
};

#endif // NATIVE_BRIDGE_HPP_INCLUDED
