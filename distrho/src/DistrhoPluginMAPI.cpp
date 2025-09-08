/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2025 Filipe Coelho <falktx@falktx.com>
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

#include "DistrhoPluginInternal.hpp"

#ifndef DISTRHO_NO_WARNINGS
# if DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
#  error Cannot use parameter value change request with MAPI
# endif
# if DISTRHO_PLUGIN_WANT_MIDI_INPUT || DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
#  error Cannot use MIDI with MAPI
# endif
# if DISTRHO_PLUGIN_WANT_FULL_STATE
#  error Cannot use full state with MAPI
# endif
# if DISTRHO_PLUGIN_WANT_TIMEPOS
#  error Cannot use time position with MAPI
# endif
#endif

#include "mapi/mapi.h"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class PluginMAPI
{
public:
    PluginMAPI()
        : fPlugin(nullptr, nullptr, nullptr, nullptr)
    {
        fPlugin.activate();
    }

    ~PluginMAPI() noexcept
    {
        fPlugin.deactivate();
    }

    // ----------------------------------------------------------------------------------------------------------------

    void process(const float* const* ins, float** outs, unsigned int frames)
    {
       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        fPlugin.run(const_cast<const float**>(ins), outs, frames, nullptr, 0);
       #else
        fPlugin.run(const_cast<const float**>(ins), outs, frames);
       #endif

        updateParameterOutputsAndTriggers();
    }

    void setParameter(unsigned int index, float value)
    {
        fPlugin.setParameterValue(index, fPlugin.getParameterRanges(index).getFixedValue(value));
    }

    void setState(const char* key, const char* value)
    {
       #if DISTRHO_PLUGIN_WANT_STATE
        fPlugin.setState(key, value);
       #else
        // unused
        (void)key;
        (void)value;
       #endif
    }

    // ----------------------------------------------------------------------------------------------------------------

private:
    PluginExporter fPlugin;

    void updateParameterOutputsAndTriggers()
    {
        float value;

        for (uint32_t i = 0, count = fPlugin.getParameterCount(); i < count; ++i)
        {
            if ((fPlugin.getParameterHints(i) & kParameterIsTrigger) == kParameterIsTrigger)
            {
                // NOTE: no trigger support in MAPI, simulate it here
                value = fPlugin.getParameterRanges(i).def;

                if (d_isEqual(value, fPlugin.getParameterValue(i)))
                    continue;

                fPlugin.setParameterValue(i, value);
            }
        }
    }
};

// --------------------------------------------------------------------------------------------------------------------

MAPI_EXPORT
mapi_handle_t mapi_create(unsigned int sample_rate)
{
    if (d_nextBufferSize == 0)
    {
       #if defined(_DARKGLASS_DEVICE_PABLITO)
        d_nextBufferSize = 16;
       #elif defined(__MOD_DEVICES__)
        d_nextBufferSize = 128;
       #else
        d_nextBufferSize = 2048;
       #endif
    }

    d_nextSampleRate = sample_rate;

    return new PluginMAPI();
}

MAPI_EXPORT
void mapi_process(mapi_handle_t handle,
                  const float* const* ins,
                  float** outs,
                  unsigned int frames)
{
    static_cast<PluginMAPI*>(handle)->process(ins, outs, frames);
}

MAPI_EXPORT
void mapi_set_parameter(mapi_handle_t handle, unsigned int index, float value)
{
    static_cast<PluginMAPI*>(handle)->setParameter(index, value);
}

MAPI_EXPORT
void mapi_set_state(mapi_handle_t handle, const char* key, const char* value)
{
    static_cast<PluginMAPI*>(handle)->setState(key, value);
}

MAPI_EXPORT
void mapi_destroy(mapi_handle_t handle)
{
    delete static_cast<PluginMAPI*>(handle);
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
