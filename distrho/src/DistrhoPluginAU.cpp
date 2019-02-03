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

#if ((DISTRHO_PLUGIN_NUM_INPUTS == 1) && (DISTRHO_PLUGIN_NUM_OUTPUTS == 1))
# define CLONE_MONO(x, y) x
#else
# define CLONE_MONO(x, y) y
#endif

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
          fPluginA(this, writeMidiCallback),
#if CLONE_MONO(1,0)
          fPluginB(this, writeMidiCallback),
#endif
          fLastParameterValues(nullptr)
    {

        CreateElements();

        AUElement* const globals = Globals();
        DISTRHO_SAFE_ASSERT_RETURN(globals != nullptr,);

        if (const uint32_t paramCount = fPluginA.getParameterCount())
        {
            fLastParameterValues = new float[paramCount];
            globals->UseIndexedParameters(paramCount);

            for (uint32_t i=0; i < paramCount; ++i)
            {
                fLastParameterValues[i] = fPluginA.getParameterValue(i);
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
            const String& name = fPluginA.getParameterName(inParameterID);
            CFStringRef cfname = CFStringCreateWithCString(kCFAllocatorDefault, name.buffer(), kCFStringEncodingUTF8);

            AUBase::FillInParameterName(outParameterInfo, cfname, false);
        }

        // Hints
        {
            const uint32_t hints(fPluginA.getParameterHints(inParameterID));

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
            const ParameterRanges& ranges(fPluginA.getParameterRanges(inParameterID));

            outParameterInfo.defaultValue = ranges.def;
            outParameterInfo.minValue = ranges.min;
            outParameterInfo.maxValue = ranges.max;
        }

        // Unit
        {
            const String& unit = fPluginA.getParameterUnit(inParameterID);

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
#if DISTRHO_PLUGIN_HAS_UI
        if (inID == kAudioUnitProperty_CocoaUI && inScope == kAudioUnitScope_Global)
        {
            d_stdout("GetPropertyInfo asked for CocoaUI");
            outDataSize = sizeof(AudioUnitCocoaViewInfo);
            outWritable = false;
            return noErr;
        }
#endif

        return AUEffectBase::GetPropertyInfo(inID, inScope, inElement, outDataSize, outWritable);
    }

    OSStatus GetProperty(AudioUnitPropertyID inID,
                         AudioUnitScope inScope,
                         AudioUnitElement inElement,
                         void* outData) override
    {
#if DISTRHO_PLUGIN_HAS_UI
        if (inID == kAudioUnitProperty_CocoaUI && inScope == kAudioUnitScope_Global && outData != nullptr)
        {
            d_stdout("GetProperty asked for CocoaUI");
            AudioUnitCocoaViewInfo* const info = (AudioUnitCocoaViewInfo*)outData;
            info->mCocoaAUViewBundleLocation = nullptr; // NSURL, CFURLRef
            info->mCocoaAUViewClass[0] = nullptr; // NSString, CFStringRef
            // return noErr;
        }
#endif

        return AUEffectBase::GetProperty (inID, inScope, inElement, outData);
    }

#if 0
    void SetParameter(AudioUnitParameterID index,
                      AudioUnitParameterValue value) override
    {
        d_stdout("SetParameter %u %f", index, value);

        const uint32_t hints(fPluginA.getParameterHints(index));
        const ParameterRanges& ranges(fPluginA.getParameterRanges(index));

        if (hints & kParameterIsBoolean)
        {
            const float midRange = ranges.min + (ranges.max - ranges.min) / 2.0f;
            value = value > midRange ? ranges.max : ranges.min;
        }
        else if (hints & kParameterIsInteger)
        {
            value = std::round(value);
        }

        fPluginA.setParameterValue(index, value);
        CLONE_MONO(fPluginB.setParameterValue(index, value),);
        //printf("SET: id=%d val=%f\n", index, value);
    }
#endif

    bool SupportsTail() override
    {
        return false;
    }

    OSStatus Version() override
    {
        return fPluginA.getVersion();
    }

    OSStatus ProcessBufferLists(AudioUnitRenderActionFlags& ioActionFlags,
                                const AudioBufferList& inBuffer,
                                AudioBufferList& outBuffer,
                                UInt32 inFramesToProcess) override
    {
        const float* srcBuffer[numCh];
        /* */ float* destBuffer[numCh];

        for (uint32_t i = 0; i < numCh; ++i) {
            srcBuffer[i] = (const float*)inBuffer.mBuffers[i].mData;
            destBuffer[i] = (float *)outBuffer.mBuffers[i].mData;
        }

        updateParameterInputs();

        fPluginA.run(&srcBuffer[0], &destBuffer[0], inFramesToProcess);
        if (numCh == 2) {
	    CLONE_MONO(fPluginB.run(&srcBuffer[1], &destBuffer[1], inFramesToProcess),);
        }

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

        numCh = GetNumberOfChannels();

        // make sure things are in sync
        fPluginA.setBufferSize(GetMaxFramesPerSlice(), true);
        CLONE_MONO(fPluginB.setBufferSize(GetMaxFramesPerSlice(), true),);
        fPluginA.setSampleRate(GetSampleRate(), true);
        CLONE_MONO(fPluginB.setSampleRate(GetSampleRate(), true),);

        fPluginA.activate();
        CLONE_MONO(fPluginB.activate(),);

        return noErr;
    }

    void Cleanup() override
    {
        fPluginA.deactivate();
        CLONE_MONO(fPluginB.deactivate(),);
        AUEffectBase::Cleanup();
    }

    // -------------------------------------------------------------------

    UInt32 SupportedNumChannels(const AUChannelInfo** outInfo) override
    {
#if CLONE_MONO(1,0) 
        static const AUChannelInfo sChannels[2] = {{2,2}, { DISTRHO_PLUGIN_NUM_INPUTS, DISTRHO_PLUGIN_NUM_OUTPUTS }};
        if (outInfo != nullptr)
            *outInfo = sChannels;

        return 2;
#else
        static const AUChannelInfo sChannels[1] = {{2,2}};
        if (outInfo != nullptr)
            *outInfo = sChannels;

        return 1;
#endif
    }

    void SetMaxFramesPerSlice(UInt32 nFrames) override
    {
        fPluginA.setBufferSize(nFrames, true);
        CLONE_MONO(fPluginB.setBufferSize(nFrames, true),);
        AUEffectBase::SetMaxFramesPerSlice(nFrames);
    }

    // -------------------------------------------------------------------

private:
    uint16_t numCh;
    LastValuesInit fLastValuesInit;
    PluginExporter fPluginA;
#if CLONE_MONO(1,0)
    PluginExporter fPluginB;
#endif 

    // Temporary data
    float* fLastParameterValues;

    void updateParameterInputs()
    {
        float value;

        for (uint32_t i=0, count=fPluginA.getParameterCount(); i < count; ++i)
        {
            if (! fPluginA.isParameterInput(i))
                continue;

            value = GetParameter(i);

            if (d_isEqual(fLastParameterValues[i], value))
                continue;

            fLastParameterValues[i] = value;
            fPluginA.setParameterValue(i, value);
            CLONE_MONO(fPluginB.setParameterValue(i, value),);
        }
    }

    void updateParameterOutputsAndTriggers()
    {
        float value;

        for (uint32_t i=0, count=fPluginA.getParameterCount(); i < count; ++i)
        {
            if (fPluginA.isParameterOutput(i))
            {
                value = fLastParameterValues[i] = fPluginA.getParameterValue(i);
                SetParameter(i, value);
            }
            else if ((fPluginA.getParameterHints(i) & kParameterIsTrigger) == kParameterIsTrigger)
            {
                // NOTE: no trigger support in AU, simulate it here
                value = fPluginA.getParameterRanges(i).def;

                if (d_isEqual(value, fPluginA.getParameterValue(i)))
                    continue;

                fLastParameterValues[i] = value;
                fPluginA.setParameterValue(i, value);
                CLONE_MONO(fPluginB.setParameterValue(i, value),);
                SetParameter(i, value);
            }
        }
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
