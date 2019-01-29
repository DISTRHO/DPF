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

struct LastValuesInit {
    LastValuesInit()
    {
        if (d_lastBufferSize == 0)
            d_lastBufferSize = kAUDefaultMaxFramesPerSlice;

        if (d_isZero(d_lastSampleRate))
            d_lastSampleRate = kAUDefaultSampleRate;
    };
};

// -----------------------------------------------------------------------
// AU Plugin

class PluginAU : public AUEffectBase
{
public:
    PluginAU(AudioUnit component)
        : AUEffectBase(component),
          fLastValuesInit(),
          fPlugin(this, writeMidiCallback),
          fNumChannels(0),
          fLastParameterValues(nullptr)
    {
        CreateElements();

        AUElement* const globals = Globals();
        DISTRHO_SAFE_ASSERT_RETURN(globals != nullptr,);

        if (const uint32_t paramCount = fPlugin.getParameterCount())
        {
            fLastParameterValues = new float[paramCount];
            globals->UseIndexedParameters(paramCount);

            for (uint32_t i=0; i < paramCount; ++i)
            {
                fLastParameterValues[i] = fPlugin.getParameterValue(i);
                globals->SetParameter(i, fLastParameterValues[i]);
            }
        }
    }

    ~PluginAU() override
    {
        if (fLastParameterValues)
        {
            delete[] fLastParameterValues;
            fLastParameterValues = nullptr;
        }
    }

protected:
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

        outParameterInfo.flags = kAudioUnitParameterFlag_IsReadable | kAudioUnitParameterFlag_IsWritable;

        // Name
        {
            const String& name = fPlugin.getParameterName(inParameterID);
            CFStringRef cfname = CFStringCreateWithCString(kCFAllocatorDefault, name.buffer(), kCFStringEncodingUTF8);

            AUBase::FillInParameterName(outParameterInfo, cfname, false);
        }

        // Hints
        {
            const uint32_t hints(fPlugin.getParameterHints(inParameterID));

            // other interesting bits:
            // kAudioUnitParameterFlag_OmitFromPresets for outputs?
            // kAudioUnitParameterFlag_MeterReadOnly for outputs?
            // kAudioUnitParameterFlag_CanRamp for log?
            // kAudioUnitParameterFlag_IsHighResolution ??

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
                outParameterInfo.unit = kAudioUnitParameterUnit_CustomUnit;
                outParameterInfo.unitName = CFStringCreateWithCString(kCFAllocatorDefault, unit.buffer(), kCFStringEncodingUTF8);
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

#if 0
    void SetParameter(AudioUnitParameterID index,
                      AudioUnitParameterValue value) override
    {
        d_stdout("SetParameter %u %f", index, value);

        const uint32_t hints(fPlugin.getParameterHints(index));
        const ParameterRanges& ranges(fPlugin.getParameterRanges(index));

        if (hints & kParameterIsBoolean)
        {
            const float midRange = ranges.min + (ranges.max - ranges.min) / 2.0f;
            value = value > midRange ? ranges.max : ranges.min;
        }
        else if (hints & kParameterIsInteger)
        {
            value = std::round(value);
        }

        fPlugin.setParameterValue(index, value);
        //printf("SET: id=%d val=%f\n", index, value);
    }
#endif

    bool SupportsTail() override
    {
        return false;
    }

    OSStatus Version() override
    {
        return fPlugin.getVersion();
    }

    OSStatus ProcessBufferLists(AudioUnitRenderActionFlags& ioActionFlags,
                                const AudioBufferList& inBuffer,
                                AudioBufferList& outBuffer,
                                UInt32 inFramesToProcess) override
    {
        const float* srcBuffer[fNumChannels];
        /* */ float* destBuffer[fNumChannels];

        for (uint32_t i = 0; i < fNumChannels; ++i) {
            srcBuffer[i] = (const float*)inBuffer.mBuffers[i].mData;
            destBuffer[i] = (float *)outBuffer.mBuffers[i].mData;
        }

        updateSampleRate();

        updateParameterInputs();

        fPlugin.run(srcBuffer, destBuffer, inFramesToProcess);

        updateParameterOutputsAndTriggers();

        ioActionFlags &= ~kAudioUnitRenderAction_OutputIsSilence;

        return noErr;
    }

    // -------------------------------------------------------------------

    ComponentResult Initialize() override
    {
        ComponentResult err;

        if ((err = AUEffectBase::Initialize()) != noErr)
            return err;

        updateSampleRate();

        fPlugin.activate();

        // FIXME this does not seem right
        fNumChannels = GetNumberOfChannels();
        d_stdout("fNumChannels %u", fNumChannels);
        DISTRHO_SAFE_ASSERT(fNumChannels == DISTRHO_PLUGIN_NUM_INPUTS);

        return noErr;
    }

    void Cleanup() override
    {
        AUEffectBase::Cleanup();
        fPlugin.deactivate();
    }

    // -------------------------------------------------------------------

private:
    LastValuesInit fLastValuesInit;
    PluginExporter fPlugin;
    uint32_t fNumChannels;

    // Temporary data
    float* fLastParameterValues;

    void updateParameterInputs()
    {
        float value;

        for (uint32_t i=0, count=fPlugin.getParameterCount(); i < count; ++i)
        {
            if (! fPlugin.isParameterInput(i))
                continue;

            value = GetParameter(i);

            if (d_isEqual(fLastParameterValues[i], value))
                continue;

            fLastParameterValues[i] = value;
            fPlugin.setParameterValue(i, value);
        }
    }

    void updateParameterOutputsAndTriggers()
    {
        float value;

        for (uint32_t i=0, count=fPlugin.getParameterCount(); i < count; ++i)
        {
            if (fPlugin.isParameterOutput(i))
            {
                value = fLastParameterValues[i] = fPlugin.getParameterValue(i);
                SetParameter(i, value);
            }
            else if ((fPlugin.getParameterHints(i) & kParameterIsTrigger) == kParameterIsTrigger)
            {
                // NOTE: no trigger support in AU, simulate it here
                value = fPlugin.getParameterRanges(i).def;

                if (d_isEqual(value, fPlugin.getParameterValue(i)))
                    continue;

                fLastParameterValues[i] = value;
                fPlugin.setParameterValue(i, value);
                SetParameter(i, value);
            }
        }
    }

    void updateSampleRate()
    {
        d_lastSampleRate = GetSampleRate();
    }

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
