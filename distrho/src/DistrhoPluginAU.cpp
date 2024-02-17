/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2024 Filipe Coelho <falktx@falktx.com>
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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#include "au/AUBase.h"

#pragma clang diagnostic pop

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

class PluginAU : public AUBase
{
public:
    PluginAU(AudioUnit component)
        : AUBase(component, 0, 0)
    {}

protected:
    bool CanScheduleParameters() const override
    {
        return false;
    }

    bool StreamFormatWritable (AudioUnitScope, AudioUnitElement) override
    {
        return false;
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginAU)
};

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

#ifndef verify
# define verify(assertion) __Verify(assertion)
#endif
#ifndef verify_noerr
# define verify_noerr(errorCode)  __Verify_noErr(errorCode)
#endif

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wunused-private-field"

#include "au/AUBase.cpp"
#include "au/AUBuffer.cpp"
#include "au/AUDispatch.cpp"
#include "au/AUInputElement.cpp"
#include "au/AUMIDIBase.cpp"
#include "au/AUOutputBase.cpp"
#include "au/AUOutputElement.cpp"
#include "au/AUScopeElement.cpp"
#include "au/CAAUParameter.cpp"
#include "au/CAAudioChannelLayout.cpp"
#include "au/CAMutex.cpp"
#include "au/CAStreamBasicDescription.cpp"
#include "au/CAVectorUnit.cpp"
#include "au/CarbonEventHandler.cpp"
#include "au/ComponentBase.cpp"
#include "au/MusicDeviceBase.cpp"

#pragma clang diagnostic pop

#undef verify
#undef verify_noerr

// --------------------------------------------------------------------------------------------------------------------

DISTRHO_PLUGIN_EXPORT
OSStatus PluginAUEntry(ComponentParameters*, PluginAU*);

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT || DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
AUDIOCOMPONENT_ENTRY(AUMIDIEffectFactory, PluginAU)
#else
AUDIOCOMPONENT_ENTRY(AUBaseFactory, PluginAU)
#endif

// --------------------------------------------------------------------------------------------------------------------
