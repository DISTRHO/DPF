/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2022 Filipe Coelho <falktx@falktx.com>
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

#include "DistrhoUIInternal.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

#if ! DISTRHO_PLUGIN_WANT_STATE
static constexpr const setStateFunc setStateCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_MIDI_INPUT
static constexpr const sendNoteFunc sendNoteCallback = nullptr;
#endif

// --------------------------------------------------------------------------------------------------------------------

/**
 * Stub UI class, does nothing but serving as example code for other implementations.
 */
class UIStub
{
public:
    UIStub(const intptr_t winId,
           const double sampleRate,
           const char* const bundlePath,
           void* const dspPtr,
           const float scaleFactor)
        : fUI(this, winId, sampleRate,
              editParameterCallback,
              setParameterCallback,
              setStateCallback,
              sendNoteCallback,
              setSizeCallback,
              fileRequestCallback,
              bundlePath, dspPtr, scaleFactor)
    {
    }

    // ----------------------------------------------------------------------------------------------------------------

private:
    // Stub stuff here

    // Plugin UI (after Stub stuff so the UI can call into us during its constructor)
    UIExporter fUI;

    // ----------------------------------------------------------------------------------------------------------------
    // DPF callbacks

    void editParameter(uint32_t, bool) const
    {
    }

    static void editParameterCallback(void* const ptr, const uint32_t rindex, const bool started)
    {
        static_cast<UIStub*>(ptr)->editParameter(rindex, started);
    }

    void setParameterValue(uint32_t, float)
    {
    }

    static void setParameterCallback(void* const ptr, const uint32_t rindex, const float value)
    {
        static_cast<UIStub*>(ptr)->setParameterValue(rindex, value);
    }

    void setSize(uint, uint)
    {
    }

    static void setSizeCallback(void* const ptr, const uint width, const uint height)
    {
        static_cast<UIStub*>(ptr)->setSize(width, height);
    }

   #if DISTRHO_PLUGIN_WANT_STATE
    void setState(const char*, const char*)
    {
    }

    static void setStateCallback(void* const ptr, const char* key, const char* value)
    {
        static_cast<UIStub*>(ptr)->setState(key, value);
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    void sendNote(const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
    }

    static void sendNoteCallback(void* const ptr, const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        static_cast<UIStub*>(ptr)->sendNote(channel, note, velocity);
    }
   #endif

    bool fileRequest(const char*)
    {
        return true;
    }

    static bool fileRequestCallback(void* const ptr, const char* const key)
    {
        return static_cast<UIStub*>(ptr)->fileRequest(key);
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
