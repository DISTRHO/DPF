/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2019 Filipe Coelho <falktx@falktx.com>
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

#include "CoreAudio106/AudioUnits/AUPublic/OtherBases/AUEffectBase.h"

// -----------------------------------------------------------------------

START_NAMESPACE_DISTRHO

// #if ! DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
static const writeMidiFunc writeMidiCallback = nullptr;
// #endif

// -----------------------------------------------------------------------
// AU Plugin

class PluginAU : public AUEffectBase
{
public:
    PluginAU(AudioUnit component)
        : AUEffectBase(component),
          fPlugin(this, writeMidiCallback)
    {
        CreateElements();

        AUElement* const globals = Globals();
        DISTRHO_SAFE_ASSERT_RETURN(globals != nullptr,);

        if (const uint32_t paramCount = fPlugin.getParameterCount())
        {
            globals->UseIndexedParameters(paramCount);

            for (uint32_t i=0; i < paramCount; ++i)
                globals->SetParameter(i, fPlugin.getParameterValue(i));
        }
    }

    ~PluginAU() override
    {
    }

    AUKernelBase* NewKernel() override
    {
        return new PluginKernel(this, fPlugin);
    }

    OSStatus GetParameterValueStrings(AudioUnitScope inScope,
                                      AudioUnitParameterID inParameterID,
                                      CFArrayRef* outStrings) override
    {
        // TODO scalepoints support via kAudioUnitParameterFlag_ValuesHaveStrings
        return kAudioUnitErr_InvalidProperty;
    }

    OSStatus GetParameterInfo(AudioUnitScope inScope,
                              AudioUnitParameterID inParameterID,
                              AudioUnitParameterInfo& outParameterInfo) override
    {
        DISTRHO_SAFE_ASSERT_RETURN(inScope == kAudioUnitScope_Global, kAudioUnitErr_InvalidParameter);

        // Name
        {
            // FIXME
            // const String& name = fPlugin.getParameterUnit(inParameterID);

            // AUBase::FillInParameterName(outParameterInfo, name.buffer(), true);
        }

        // Hints
        {
            const uint32_t hints(fPlugin.getParameterHints(inParameterID));

            outParameterInfo.flags = 0x0;

            // other interesting bits:
            // kAudioUnitParameterFlag_OmitFromPresets for outputs?
            // kAudioUnitParameterFlag_MeterReadOnly for outputs?
            // kAudioUnitParameterFlag_CanRamp for log?
            // kAudioUnitParameterFlag_IsHighResolution ??

            if (hints & kParameterIsOutput)
                outParameterInfo.flags = kAudioUnitParameterFlag_IsReadable;
            else
                outParameterInfo.flags = kAudioUnitParameterFlag_IsWritable;

            if ((hints & kParameterIsAutomable) == 0x0)
                outParameterInfo.flags |= kAudioUnitParameterFlag_NonRealTime;

            /* TODO: there doesn't seem to be a hint for this
            if (hints & kParameterIsBoolean)
            {}
            else if (hints & kParameterIsInteger)
            {}
            */

            if (hints & kParameterIsLogarithmic)
                outParameterInfo.flags |= kAudioUnitParameterFlag_DisplayLogarithmic;
        }

        // Ranges
        {
            const ParameterRanges& ranges(fPlugin.getParameterRanges(inParameterID));

            outParameterInfo.defaultValue = ranges.def;
            outParameterInfo.minValue = ranges.min;
            outParameterInfo.maxValue = ranges.max;
        }

        // Unit
        {
            const String& unit = fPlugin.getParameterUnit(inParameterID);

            // TODO: map all AU unit types

            if (unit == "dB")
            {
                outParameterInfo.unit = kAudioUnitParameterUnit_LinearGain;
            }
            else
            {
                // FIXME
                // outParameterInfo.unit = kAudioUnitParameterUnit_CustomUnit;
                // outParameterInfo.unitName = unit.buffer();
            }
        }

        return noErr;
    }

    OSStatus GetPropertyInfo(AudioUnitPropertyID inID,
                             AudioUnitScope inScope,
                             AudioUnitElement inElement,
                             UInt32& outDataSize,
                             Boolean& outWritable) override
    {
        return AUEffectBase::GetPropertyInfo(inID, inScope, inElement, outDataSize, outWritable);
    }

    OSStatus GetProperty(AudioUnitPropertyID inID,
                         AudioUnitScope inScope,
                         AudioUnitElement inElement,
                         void* outData) override
    {
        return AUEffectBase::GetProperty (inID, inScope, inElement, outData);
    }

    bool SupportsTail() override
    {
        return false;
    }

    OSStatus Version() override
    {
        return fPlugin.getVersion();
    }

    // -------------------------------------------------------------------

private:
    PluginExporter fPlugin;

    // most of the real work happens here
    class PluginKernel : public AUKernelBase
    {
    public:
        PluginKernel(AUEffectBase* const inAudioUnit, PluginExporter& plugin)
            : AUKernelBase(inAudioUnit),
              fPlugin(plugin) {}

        // *Required* overides for the process method for this effect
        // processes one channel of interleaved samples
        void Process(const Float32* const inSourceP,
                     Float32* const inDestP,
                     UInt32 inFramesToProcess,
                     UInt32 inNumChannels,
                     bool& ioSilence)
        {
            DISTRHO_SAFE_ASSERT_RETURN(inNumChannels == DISTRHO_PLUGIN_NUM_INPUTS,);

            // FIXME
            // fPlugin.run(inSourceP, inDestP, inFramesToProcess);
        }

        void Reset() override
        {
        }

    private:
        PluginExporter& fPlugin;

        DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginKernel)
    };

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginAU)
};

END_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

COMPONENT_ENTRY(PluginAU)

// -----------------------------------------------------------------------

#include "CoreAudio106/AudioUnits/AUPublic/AUBase/AUBase.cpp"
#include "CoreAudio106/AudioUnits/AUPublic/AUBase/AUDispatch.cpp"
#include "CoreAudio106/AudioUnits/AUPublic/AUBase/AUInputElement.cpp"
#include "CoreAudio106/AudioUnits/AUPublic/AUBase/AUOutputElement.cpp"
#include "CoreAudio106/AudioUnits/AUPublic/AUBase/AUScopeElement.cpp"
#include "CoreAudio106/AudioUnits/AUPublic/AUBase/ComponentBase.cpp"
#include "CoreAudio106/AudioUnits/AUPublic/OtherBases/AUEffectBase.cpp"
#include "CoreAudio106/AudioUnits/AUPublic/Utility/AUBaseHelper.cpp"
#include "CoreAudio106/AudioUnits/AUPublic/Utility/AUBuffer.cpp"
#include "CoreAudio106/PublicUtility/CAAudioChannelLayout.cpp"
#include "CoreAudio106/PublicUtility/CAAUParameter.cpp"
#include "CoreAudio106/PublicUtility/CAMutex.cpp"
#include "CoreAudio106/PublicUtility/CAStreamBasicDescription.cpp"
#include "CoreAudio106/PublicUtility/CAVectorUnit.cpp"

// -----------------------------------------------------------------------
