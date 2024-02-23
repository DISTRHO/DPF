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

// TODO
// - g_nextBundlePath vs d_nextBundlePath cleanup
// - scale points to kAudioUnitParameterFlag_ValuesHaveStrings

#include "DistrhoPluginInternal.hpp"
#include "../DistrhoPluginUtils.hpp"

#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioUnitUtilities.h>
#include <vector>

#ifndef DISTRHO_PLUGIN_AU_SUBTYPE
# error DISTRHO_PLUGIN_AU_SUBTYPE undefined!
#endif

#ifndef DISTRHO_PLUGIN_AU_MANUFACTURER
# error DISTRHO_PLUGIN_AU_MANUFACTURER undefined!
#endif

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

static const char* AudioUnitPropertyID2Str(const AudioUnitPropertyID prop) noexcept
{
    switch (prop)
    {
    #define PROP(s) case s: return #s;
    PROP(kAudioUnitProperty_ClassInfo)
    PROP(kAudioUnitProperty_MakeConnection)
    PROP(kAudioUnitProperty_SampleRate)
    PROP(kAudioUnitProperty_ParameterList)
    PROP(kAudioUnitProperty_ParameterInfo)
   #if !TARGET_OS_IPHONE
    PROP(kAudioUnitProperty_FastDispatch)
   #endif
    PROP(kAudioUnitProperty_CPULoad)
    PROP(kAudioUnitProperty_StreamFormat)
    PROP(kAudioUnitProperty_ElementCount)
    PROP(kAudioUnitProperty_Latency)
    PROP(kAudioUnitProperty_SupportedNumChannels)
    PROP(kAudioUnitProperty_MaximumFramesPerSlice)
    PROP(kAudioUnitProperty_ParameterValueStrings)
    PROP(kAudioUnitProperty_AudioChannelLayout)
    PROP(kAudioUnitProperty_TailTime)
    PROP(kAudioUnitProperty_BypassEffect)
    PROP(kAudioUnitProperty_LastRenderError)
    PROP(kAudioUnitProperty_SetRenderCallback)
    PROP(kAudioUnitProperty_FactoryPresets)
    PROP(kAudioUnitProperty_RenderQuality)
    PROP(kAudioUnitProperty_HostCallbacks)
    PROP(kAudioUnitProperty_InPlaceProcessing)
    PROP(kAudioUnitProperty_ElementName)
    PROP(kAudioUnitProperty_SupportedChannelLayoutTags)
    PROP(kAudioUnitProperty_PresentPreset)
    PROP(kAudioUnitProperty_DependentParameters)
    PROP(kAudioUnitProperty_InputSamplesInOutput)
    PROP(kAudioUnitProperty_ShouldAllocateBuffer)
    PROP(kAudioUnitProperty_FrequencyResponse)
    PROP(kAudioUnitProperty_ParameterHistoryInfo)
    PROP(kAudioUnitProperty_NickName)
    PROP(kAudioUnitProperty_OfflineRender)
    PROP(kAudioUnitProperty_ParameterIDName)
    PROP(kAudioUnitProperty_ParameterStringFromValue)
    PROP(kAudioUnitProperty_ParameterClumpName)
    PROP(kAudioUnitProperty_ParameterValueFromString)
    PROP(kAudioUnitProperty_PresentationLatency)
    PROP(kAudioUnitProperty_ClassInfoFromDocument)
    PROP(kAudioUnitProperty_RequestViewController)
    PROP(kAudioUnitProperty_ParametersForOverview)
    PROP(kAudioUnitProperty_SupportsMPE)
    PROP(kAudioUnitProperty_RenderContextObserver)
    PROP(kAudioUnitProperty_LastRenderSampleTime)
    PROP(kAudioUnitProperty_LoadedOutOfProcess)
   #if !TARGET_OS_IPHONE
    PROP(kAudioUnitProperty_SetExternalBuffer)
    PROP(kAudioUnitProperty_GetUIComponentList)
    PROP(kAudioUnitProperty_CocoaUI)
    PROP(kAudioUnitProperty_IconLocation)
    PROP(kAudioUnitProperty_AUHostIdentifier)
   #endif
    PROP(kAudioUnitProperty_MIDIOutputCallbackInfo)
    PROP(kAudioUnitProperty_MIDIOutputCallback)
    PROP(kAudioUnitProperty_MIDIOutputEventListCallback)
    PROP(kAudioUnitProperty_AudioUnitMIDIProtocol)
    PROP(kAudioUnitProperty_HostMIDIProtocol)
    PROP(kAudioUnitProperty_MIDIOutputBufferSizeHint)
    #undef PROP
    }
    return "[unknown]";
}

static const char* AudioUnitScope2Str(const AudioUnitScope scope) noexcept
{
    switch (scope)
    {
    #define SCOPE(s) case s: return #s;
    SCOPE(kAudioUnitScope_Global)
    SCOPE(kAudioUnitScope_Input)
    SCOPE(kAudioUnitScope_Output)
    SCOPE(kAudioUnitScope_Group)
    SCOPE(kAudioUnitScope_Part)
    SCOPE(kAudioUnitScope_Note)
    SCOPE(kAudioUnitScope_Layer)
    SCOPE(kAudioUnitScope_LayerItem)
    #undef SCOPE
    }
    return "[unknown]";
}

static const char* AudioUnitSelector2Str(const SInt16 selector) noexcept
{
    switch (selector)
    {
    #define SEL(s) case s: return #s;
    SEL(kAudioUnitInitializeSelect)
    SEL(kAudioUnitUninitializeSelect)
    SEL(kAudioUnitGetPropertyInfoSelect)
    SEL(kAudioUnitGetPropertySelect)
    SEL(kAudioUnitSetPropertySelect)
    SEL(kAudioUnitAddPropertyListenerSelect)
    SEL(kAudioUnitRemovePropertyListenerSelect)
    SEL(kAudioUnitRemovePropertyListenerWithUserDataSelect)
    SEL(kAudioUnitAddRenderNotifySelect)
    SEL(kAudioUnitRemoveRenderNotifySelect)
    SEL(kAudioUnitGetParameterSelect)
    SEL(kAudioUnitSetParameterSelect)
    SEL(kAudioUnitScheduleParametersSelect)
    SEL(kAudioUnitRenderSelect)
    SEL(kAudioUnitResetSelect)
    SEL(kAudioUnitComplexRenderSelect)
    SEL(kAudioUnitProcessSelect)
    SEL(kAudioUnitProcessMultipleSelect)
    SEL(kMusicDeviceMIDIEventSelect)
    SEL(kMusicDeviceSysExSelect)
    SEL(kMusicDevicePrepareInstrumentSelect)
    SEL(kMusicDeviceReleaseInstrumentSelect)
    SEL(kMusicDeviceStartNoteSelect)
    SEL(kMusicDeviceStopNoteSelect)
    SEL(kMusicDeviceMIDIEventListSelect)
    SEL(kAudioOutputUnitStartSelect)
    SEL(kAudioOutputUnitStopSelect)
    #undef SEL
    }
    return "[unknown]";
}

static constexpr FourCharCode getFourCharCodeFromString(const char str[4])
{
    return (str[0] << 24) + (str[1] << 16) + (str[2] << 8) + str[3];
}

// --------------------------------------------------------------------------------------------------------------------

#define MACRO_STR2(s) #s
#define MACRO_STR(s) MACRO_STR2(s)

static constexpr const char kTypeStr[] = MACRO_STR(DISTRHO_PLUGIN_AU_TYPE);
static constexpr const char kSubTypeStr[] = MACRO_STR(DISTRHO_PLUGIN_AU_SUBTYPE);
static constexpr const char kManufacturerStr[] = MACRO_STR(DISTRHO_PLUGIN_AU_MANUFACTURER);

#undef MACRO_STR
#undef MACRO_STR2

static constexpr const FourCharCode kType = getFourCharCodeFromString(kTypeStr);
static constexpr const FourCharCode kSubType = getFourCharCodeFromString(kSubTypeStr);
static constexpr const FourCharCode kManufacturer = getFourCharCodeFromString(kManufacturerStr);

// --------------------------------------------------------------------------------------------------------------------

struct PropertyListener {
    AudioUnitPropertyID	prop;
    AudioUnitPropertyListenerProc proc;
    void* userData;
};

typedef std::vector<PropertyListener> PropertyListeners;

// --------------------------------------------------------------------------------------------------------------------

#if ! DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
static constexpr const writeMidiFunc writeMidiCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
static constexpr const requestParameterValueChangeFunc requestParameterValueChangeCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_STATE
static constexpr const updateStateValueFunc updateStateValueCallback = nullptr;
#endif

class PluginAU
{
public:
    PluginAU(const AudioUnit component)
        : fPlugin(this, writeMidiCallback, requestParameterValueChangeCallback, updateStateValueCallback),
          fComponent(component),
          fLastRenderError(noErr),
          fPropertyListeners(),
         #if DISTRHO_PLUGIN_NUM_INPUTS != 0
          fSampleRateForInput(d_nextSampleRate),
         #endif
         #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
          fSampleRateForOutput(d_nextSampleRate),
         #endif
          fParameterCount(fPlugin.getParameterCount()),
          fLastParameterValues(nullptr),
          fBypassParameterIndex(UINT32_MAX)
    {
	    if (fParameterCount != 0)
        {
            fLastParameterValues = new float[fParameterCount];
            std::memset(fLastParameterValues, 0, sizeof(float) * fParameterCount);

            for (uint32_t i=0; i<fParameterCount; ++i)
            {
                fLastParameterValues[i] = fPlugin.getParameterValue(i);

                if (fPlugin.getParameterDesignation(i) == kParameterDesignationBypass)
                    fBypassParameterIndex = i;
            }
        }
    }

    ~PluginAU()
    {
        delete[] fLastParameterValues;
    }

    OSStatus auInitialize()
    {
        fPlugin.activate();
        return noErr;
    }

    OSStatus auUninitialize()
    {
        fPlugin.deactivateIfNeeded();
        return noErr;
    }

    OSStatus auGetPropertyInfo(const AudioUnitPropertyID inProp,
                               const AudioUnitScope inScope,
                               const AudioUnitElement inElement,
                               UInt32& outDataSize,
                               Boolean& outWritable)
    {
        switch (inProp)
        {
        case kAudioUnitProperty_ClassInfo:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            outDataSize = sizeof(CFPropertyListRef);
            outWritable = true;
            return noErr;

        case kAudioUnitProperty_MakeConnection:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global || inScope == kAudioUnitScope_Input, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            outDataSize = sizeof(AudioUnitConnection);
            outWritable = true;
            return noErr;

       #if DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS != 0
        case kAudioUnitProperty_SampleRate:
           #if DISTRHO_PLUGIN_NUM_INPUTS != 0 && DISTRHO_PLUGIN_NUM_OUTPUTS != 0
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Input || inScope == kAudioUnitScope_Output, inScope, kAudioUnitErr_InvalidScope);
           #elif DISTRHO_PLUGIN_NUM_INPUTS != 0
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Input, inScope, kAudioUnitErr_InvalidScope);
           #else
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Output, inScope, kAudioUnitErr_InvalidScope);
           #endif
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            outDataSize = sizeof(Float64);
            outWritable = true;
            return noErr;
       #endif

        case kAudioUnitProperty_ParameterList:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            outDataSize = inScope == kAudioUnitScope_Global ? sizeof(AudioUnitParameterID) * fParameterCount : 0;
            outWritable = false;
            return noErr;

        case kAudioUnitProperty_ParameterInfo:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement < fParameterCount, inElement, kAudioUnitErr_InvalidElement);
            outDataSize = sizeof(AudioUnitParameterInfo);
            outWritable = false;
            return noErr;

       #if DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS != 0
        case kAudioUnitProperty_StreamFormat:
           #if DISTRHO_PLUGIN_NUM_INPUTS != 0 && DISTRHO_PLUGIN_NUM_OUTPUTS != 0
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Input || inScope == kAudioUnitScope_Output, inScope, kAudioUnitErr_InvalidScope);
           #elif DISTRHO_PLUGIN_NUM_INPUTS != 0
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Input, inScope, kAudioUnitErr_InvalidScope);
           #else
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Output, inScope, kAudioUnitErr_InvalidScope);
           #endif
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            outDataSize = sizeof(AudioStreamBasicDescription);
            outWritable = true;
            return noErr;
       #endif

        case kAudioUnitProperty_ElementCount:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            outDataSize = sizeof(UInt32);
            outWritable = false;
            return noErr;

        case kAudioUnitProperty_Latency:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
           #if DISTRHO_PLUGIN_WANT_LATENCY
            if (inScope == kAudioUnitScope_Global)
            {
                outDataSize = sizeof(Float64);
                outWritable = false;
                return noErr;
            }
           #endif
            return kAudioUnitErr_InvalidProperty;

        case kAudioUnitProperty_SupportedNumChannels:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            outDataSize = sizeof(AUChannelInfo);
            outWritable = false;
            return noErr;

        case kAudioUnitProperty_MaximumFramesPerSlice:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            outDataSize = sizeof(UInt32);
            outWritable = true;
            return noErr;

        case kAudioUnitProperty_BypassEffect:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            if (inScope == kAudioUnitScope_Global && fBypassParameterIndex != UINT32_MAX)
            {
                outDataSize = sizeof(UInt32);
                outWritable = false;
                return noErr;
            }
            return kAudioUnitErr_InvalidProperty;

        case kAudioUnitProperty_LastRenderError:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            outDataSize = sizeof(OSStatus);
            outWritable = false;
            return noErr;

        case kAudioUnitProperty_SetRenderCallback:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Input, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            outDataSize = sizeof(AURenderCallbackStruct);
            outWritable = true;
            return noErr;

        case kAudioUnitProperty_PresentPreset:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            outDataSize = sizeof(AUPreset);
            outWritable = true;
            return noErr;

        case kAudioUnitProperty_CocoaUI:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
           #if DISTRHO_PLUGIN_HAS_UI
            outDataSize = sizeof(AudioUnitCocoaViewInfo);
            outWritable = false;
            return noErr;
           #else
            return kAudioUnitErr_InvalidProperty;
           #endif

       #if DISTRHO_PLUGIN_HAS_UI && DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
        case 'DPFa':
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            outDataSize = sizeof(void*);
            outWritable = false;
            return noErr;
       #endif

        case 'DPFp':
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement < fParameterCount, inElement, kAudioUnitErr_InvalidElement);
            outDataSize = sizeof(float);
            outWritable = true;
            return noErr;

        case 'DPFt':
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement < fParameterCount, inElement, kAudioUnitErr_InvalidElement);
            outDataSize = sizeof(bool);
            outWritable = true;
            return noErr;

        // unwanted properties
        case kAudioUnitProperty_CPULoad:
        case kAudioUnitProperty_RenderContextObserver:
        case kAudioUnitProperty_TailTime:
       #if DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS == 0
        case kAudioUnitProperty_SampleRate:
        case kAudioUnitProperty_StreamFormat:
       #endif
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            return kAudioUnitErr_InvalidProperty;
        }

        d_stdout("TODO GetPropertyInfo(%2d:%s, %d:%s, %d, ...)", inProp, AudioUnitPropertyID2Str(inProp), inScope, AudioUnitScope2Str(inScope), inElement);
        return kAudioUnitErr_InvalidProperty;
    }

    OSStatus auGetProperty(const AudioUnitPropertyID inProp,
                           const AudioUnitScope inScope,
                           const AudioUnitElement inElement,
                           void* const outData)
    {
        switch (inProp)
        {
        case kAudioUnitProperty_ClassInfo:
            {
                CFPropertyListRef* const propList = static_cast<CFPropertyListRef*>(outData);

                CFMutableDictionaryRef dict = CFDictionaryCreateMutable(nullptr, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
                SInt32 value;

                value = 0;
                if (const CFNumberRef num = CFNumberCreate(nullptr, kCFNumberSInt32Type, &value))
                {
                    CFDictionarySetValue(dict, CFSTR(kAUPresetVersionKey), num);
                    CFRelease(num);
                }

                value = kType;
                if (const CFNumberRef num = CFNumberCreate(nullptr, kCFNumberSInt32Type, &value))
                {
                    CFDictionarySetValue(dict, CFSTR(kAUPresetTypeKey), num);
                    CFRelease(num);
                }

                value = kSubType;
                if (const CFNumberRef num = CFNumberCreate(nullptr, kCFNumberSInt32Type, &value))
                {
                    CFDictionarySetValue(dict, CFSTR(kAUPresetSubtypeKey), num);
                    CFRelease(num);
                }

                value = kManufacturer;
                if (const CFNumberRef num = CFNumberCreate(nullptr, kCFNumberSInt32Type, &value))
                {
                    CFDictionarySetValue(dict, CFSTR(kAUPresetManufacturerKey), num);
                    CFRelease(num);
                }

	            if (const CFMutableDataRef data = CFDataCreateMutable(nullptr, 0))
                {
                    // TODO save plugin state here
                    d_stdout("WIP GetProperty(%d:%s, %d:%s, %d, ...)", inProp, AudioUnitPropertyID2Str(inProp), inScope, AudioUnitScope2Str(inScope), inElement);

                    CFDictionarySetValue(dict, CFSTR(kAUPresetDataKey), data);
                    CFRelease(data);
                }

	            CFDictionarySetValue(dict, CFSTR(kAUPresetNameKey), CFSTR("Default"));

                *propList = dict;
            }
            return noErr;

       #if DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS != 0
        case kAudioUnitProperty_SampleRate:
           #if DISTRHO_PLUGIN_NUM_INPUTS != 0
            if (inScope == kAudioUnitScope_Input)
            {
                *static_cast<Float64*>(outData) = fSampleRateForInput;
            }
            else
           #endif
           #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
            if (inScope == kAudioUnitScope_Output)
            {
                *static_cast<Float64*>(outData) = fSampleRateForOutput;
            }
            else
           #endif
            {
                return kAudioUnitErr_InvalidScope;
            }
            return noErr;
       #endif

        case kAudioUnitProperty_ParameterList:
            {
                AudioUnitParameterID* const paramList = static_cast<AudioUnitParameterID*>(outData);

                for (uint32_t i=0; i<fParameterCount; ++i)
                    paramList[i] = i;
            }
            return noErr;

        case kAudioUnitProperty_ParameterInfo:
            {
                AudioUnitParameterInfo* const info = static_cast<AudioUnitParameterInfo*>(outData);
                std::memset(info, 0, sizeof(*info));

                const ParameterRanges& ranges(fPlugin.getParameterRanges(inElement));

                info->flags = kAudioUnitParameterFlag_IsHighResolution
                            | kAudioUnitParameterFlag_IsReadable
                            | kAudioUnitParameterFlag_HasCFNameString;

                if (fPlugin.getParameterDesignation(inElement) == kParameterDesignationBypass)
                {
                    info->flags |= kAudioUnitParameterFlag_IsWritable;
                    info->unit = kAudioUnitParameterUnit_Generic;

                    d_strncpy(info->name, "Bypass", sizeof(info->name));
                    info->cfNameString = CFSTR("Bypass");
                }
                else
                {
                    const uint32_t hints = fPlugin.getParameterHints(inElement);

                    info->flags |= kAudioUnitParameterFlag_CFNameRelease;

                    if (hints & kParameterIsOutput)
                    {
                        info->flags |= kAudioUnitParameterFlag_MeterReadOnly;
                    }
                    else
                    {
                        info->flags |= kAudioUnitParameterFlag_IsWritable;

                        if ((hints & kParameterIsAutomatable) == 0x0)
                            info->flags |= kAudioUnitParameterFlag_NonRealTime;
                    }

                    if (hints & kParameterIsBoolean)
                        info->unit = kAudioUnitParameterUnit_Boolean;
                    else if (hints & kParameterIsInteger)
                        info->unit = kAudioUnitParameterUnit_Indexed;
                    else
                        info->unit = kAudioUnitParameterUnit_Generic;

                    // | kAudioUnitParameterFlag_ValuesHaveStrings;

                    const String& name(fPlugin.getParameterName(inElement));
                    d_strncpy(info->name, name, sizeof(info->name));
                    info->cfNameString = CFStringCreateWithCString(nullptr, name, kCFStringEncodingUTF8);
                }

                info->minValue = ranges.min;
                info->maxValue = ranges.max;
                info->defaultValue = ranges.def;
            }
            return noErr;

       #if DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS != 0
        case kAudioUnitProperty_StreamFormat:
            {
                AudioStreamBasicDescription* const desc = static_cast<AudioStreamBasicDescription*>(outData);
                std::memset(desc, 0, sizeof(*desc));

                if (inElement != 0)
                    return kAudioUnitErr_InvalidElement;

               #if DISTRHO_PLUGIN_NUM_INPUTS != 0
                if (inScope == kAudioUnitScope_Input)
                {
                    desc->mChannelsPerFrame = DISTRHO_PLUGIN_NUM_INPUTS;
                }
                else
               #endif
               #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
                if (inScope == kAudioUnitScope_Output)
                {
                    desc->mChannelsPerFrame = DISTRHO_PLUGIN_NUM_OUTPUTS;
                }
                else
               #endif
                {
                    return kAudioUnitErr_InvalidScope;
                }

                desc->mFormatID         = kAudioFormatLinearPCM;
                desc->mFormatFlags      = kAudioFormatFlagsNativeFloatPacked | kAudioFormatFlagIsNonInterleaved;
                desc->mSampleRate       = fPlugin.getSampleRate();
                desc->mBitsPerChannel   = 32;
                desc->mBytesPerFrame    = sizeof(float);
                desc->mBytesPerPacket   = sizeof(float);
                desc->mFramesPerPacket  = 1;
            }
            return noErr;
       #endif

        case kAudioUnitProperty_ElementCount:
            switch (inScope)
            {
            case kAudioUnitScope_Global:
                *static_cast<UInt32*>(outData) = 1;
                break;
            case kAudioUnitScope_Input:
                *static_cast<UInt32*>(outData) = DISTRHO_PLUGIN_NUM_INPUTS != 0 ? 1 : 0;
                break;
            case kAudioUnitScope_Output:
                *static_cast<UInt32*>(outData) = DISTRHO_PLUGIN_NUM_OUTPUTS != 0 ? 1 : 0;
                break;
            default:
                *static_cast<UInt32*>(outData) = 0;
                break;
            }
            return noErr;

       #if DISTRHO_PLUGIN_WANT_LATENCY
        case kAudioUnitProperty_Latency:
            *static_cast<Float64*>(outData) = static_cast<double>(fPlugin.getLatency()) / fPlugin.getSampleRate();
            return noErr;
       #endif

        case kAudioUnitProperty_SupportedNumChannels:
            *static_cast<AUChannelInfo*>(outData) = { DISTRHO_PLUGIN_NUM_INPUTS, DISTRHO_PLUGIN_NUM_OUTPUTS };
            return noErr;

        case kAudioUnitProperty_MaximumFramesPerSlice:
            *static_cast<UInt32*>(outData) = fPlugin.getBufferSize();
            return noErr;

        case kAudioUnitProperty_LastRenderError:
            *static_cast<OSStatus*>(outData) = fLastRenderError;
            fLastRenderError = noErr;
            return noErr;

        case kAudioUnitProperty_BypassEffect:
            *static_cast<OSStatus*>(outData) = fPlugin.getParameterValue(fBypassParameterIndex) > 0.5f ? 1 : 0;
            return noErr;

        case kAudioUnitProperty_SetRenderCallback:
            d_stdout("WIP GetProperty(%d:%s, %d:%s, %d, ...)", inProp, AudioUnitPropertyID2Str(inProp), inScope, AudioUnitScope2Str(inScope), inElement);
            // TODO
            break;

        case kAudioUnitProperty_PresentPreset:
            {
                AUPreset* const preset = static_cast<AUPreset*>(outData);
                std::memset(preset, 0, sizeof(*preset));

                preset->presetName = CFStringCreateWithCString(nullptr, "Default", kCFStringEncodingUTF8);
            }
            return noErr;

       #if DISTRHO_PLUGIN_HAS_UI
        case kAudioUnitProperty_CocoaUI:
            {
                AudioUnitCocoaViewInfo* const info = static_cast<AudioUnitCocoaViewInfo*>(outData);
                std::memset(info, 0, sizeof(*info));

                NSString* const bundlePathString = [[NSString alloc]
                    initWithBytes:d_nextBundlePath
                           length:strlen(d_nextBundlePath)
                         encoding:NSUTF8StringEncoding];

                info->mCocoaAUViewBundleLocation = static_cast<CFURLRef>([[NSURL fileURLWithPath: bundlePathString] retain]);

                #define MACRO_STR3(a, b, c) a "_" b "_" c
                #define MACRO_STR2(a, b, c) MACRO_STR3(#a, #b, #c)
                #define MACRO_STR(a, b, c) MACRO_STR2(a, b, c)

                info->mCocoaAUViewClass[0] = CFSTR("CocoaAUView_" MACRO_STR(DISTRHO_PLUGIN_AU_TYPE,
                                                                            DISTRHO_PLUGIN_AU_SUBTYPE,
                                                                            DISTRHO_PLUGIN_AU_MANUFACTURER));

                #undef MACRO_STR
                #undef MACRO_STR2
                #undef MACRO_STR3

                [bundlePathString release];
            }
            return noErr;
       #endif

       #if DISTRHO_PLUGIN_HAS_UI && DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
        case 'DPFa':
            *static_cast<void**>(outData) = fPlugin.getInstancePointer();
            return noErr;
       #endif
        }

        d_stdout("TODO GetProperty(%d:%s, %d:%s, %d, ...)", inProp, AudioUnitPropertyID2Str(inProp), inScope, AudioUnitScope2Str(inScope), inElement);
        return kAudioUnitErr_InvalidProperty;
    }

    OSStatus auSetProperty(const AudioUnitPropertyID inProp,
                           const AudioUnitScope inScope,
                           const AudioUnitElement inElement,
                           const void* const inData,
                           const UInt32 inDataSize)
    {
        switch (inProp)
        {
        case kAudioUnitProperty_ClassInfo:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(CFPropertyListRef), inDataSize, kAudioUnitErr_InvalidPropertyValue);
            d_stdout("WIP SetProperty(%d:%s, %d:%s, %d, %p, %u)", inProp, AudioUnitPropertyID2Str(inProp), inScope, AudioUnitScope2Str(inScope), inElement, inData, inDataSize);
            // TODO
            return noErr;

        case kAudioUnitProperty_MakeConnection:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global || inScope == kAudioUnitScope_Input, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(AudioUnitConnection), inDataSize, kAudioUnitErr_InvalidPropertyValue);
            d_stdout("WIP SetProperty(%d:%s, %d:%s, %d, %p, %u)", inProp, AudioUnitPropertyID2Str(inProp), inScope, AudioUnitScope2Str(inScope), inElement, inData, inDataSize);
            // TODO
            return noErr;

       #if DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS != 0
        case kAudioUnitProperty_SampleRate:
           #if DISTRHO_PLUGIN_NUM_INPUTS != 0 && DISTRHO_PLUGIN_NUM_OUTPUTS != 0
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Input || inScope == kAudioUnitScope_Output, inScope, kAudioUnitErr_InvalidScope);
           #elif DISTRHO_PLUGIN_NUM_INPUTS != 0
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Input, inScope, kAudioUnitErr_InvalidScope);
           #else
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Output, inScope, kAudioUnitErr_InvalidScope);
           #endif
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(Float64), inDataSize, kAudioUnitErr_InvalidPropertyValue);
            {
                const Float64 sampleRate = *static_cast<const Float64*>(inData);

               #if DISTRHO_PLUGIN_NUM_INPUTS != 0
                if (inScope == kAudioUnitScope_Input)
                {
                    if (d_isNotEqual(fSampleRateForInput, sampleRate))
                    {
                        fSampleRateForInput = sampleRate;

                       #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
                        if (d_isEqual(fSampleRateForOutput, sampleRate))
                       #endif
                        {
                            fPlugin.setSampleRate(sampleRate, true);
                        }

                        notifyListeners(inProp, inScope, inElement);
                    }
                    return noErr;
                }
               #endif

               #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
                if (inScope == kAudioUnitScope_Output)
                {
                    if (d_isNotEqual(fSampleRateForOutput, sampleRate))
                    {
                        fSampleRateForOutput = sampleRate;

                       #if DISTRHO_PLUGIN_NUM_INPUTS != 0
                        if (d_isEqual(fSampleRateForInput, sampleRate))
                       #endif
                        {
                            fPlugin.setSampleRate(sampleRate, true);
                        }

                        notifyListeners(inProp, inScope, inElement);
                    }
                    return noErr;
                }
               #endif
            }
            return kAudioUnitErr_InvalidScope;
       #endif

       #if DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS != 0
        case kAudioUnitProperty_StreamFormat:
           #if DISTRHO_PLUGIN_NUM_INPUTS != 0 && DISTRHO_PLUGIN_NUM_OUTPUTS != 0
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Input || inScope == kAudioUnitScope_Output, inScope, kAudioUnitErr_InvalidScope);
           #elif DISTRHO_PLUGIN_NUM_INPUTS != 0
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Input, inScope, kAudioUnitErr_InvalidScope);
           #else
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Output, inScope, kAudioUnitErr_InvalidScope);
           #endif
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(AudioStreamBasicDescription), inDataSize, kAudioUnitErr_InvalidPropertyValue);
            {
                const AudioStreamBasicDescription* const desc = static_cast<const AudioStreamBasicDescription*>(inData);

                if (desc->mFormatID != kAudioFormatLinearPCM)
                    return kAudioUnitErr_FormatNotSupported;
                if (desc->mBitsPerChannel != 32)
                    return kAudioUnitErr_FormatNotSupported;
                if (desc->mBytesPerFrame != sizeof(float))
                    return kAudioUnitErr_FormatNotSupported;
                if (desc->mBytesPerPacket != sizeof(float))
                    return kAudioUnitErr_FormatNotSupported;
                if (desc->mFramesPerPacket != 1)
                    return kAudioUnitErr_FormatNotSupported;

               #if 1
                // dont allow interleaved data
                if (desc->mFormatFlags != (kAudioFormatFlagsNativeFloatPacked|kAudioFormatFlagIsNonInterleaved))
               #else
                // allow interleaved data
                if ((desc->mFormatFlags & ~kAudioFormatFlagIsNonInterleaved) != kAudioFormatFlagsNativeFloatPacked)
               #endif
                    return kAudioUnitErr_FormatNotSupported;

                if (desc->mChannelsPerFrame != (inScope == kAudioUnitScope_Input ? DISTRHO_PLUGIN_NUM_INPUTS
                                                                                 : DISTRHO_PLUGIN_NUM_OUTPUTS))
                    return kAudioUnitErr_FormatNotSupported;

               #if DISTRHO_PLUGIN_NUM_INPUTS != 0
                if (inScope == kAudioUnitScope_Input)
                {
                    if (d_isNotEqual(fSampleRateForInput, desc->mSampleRate))
                    {
                        fSampleRateForInput = desc->mSampleRate;

                       #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
                        if (d_isEqual(fSampleRateForOutput, desc->mSampleRate))
                       #endif
                        {
                            fPlugin.setSampleRate(desc->mSampleRate, true);
                        }

                        notifyListeners(inProp, inScope, inElement);
                    }
                    return noErr;
                }
               #endif

               #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
                if (inScope == kAudioUnitScope_Output)
                {
                    if (d_isNotEqual(fSampleRateForOutput, desc->mSampleRate))
                    {
                        fSampleRateForOutput = desc->mSampleRate;

                       #if DISTRHO_PLUGIN_NUM_INPUTS != 0
                        if (d_isEqual(fSampleRateForInput, desc->mSampleRate))
                       #endif
                        {
                            fPlugin.setSampleRate(desc->mSampleRate, true);
                        }

                        notifyListeners(inProp, inScope, inElement);
                    }
                    return noErr;
                }
               #endif
            }
            return kAudioUnitErr_InvalidScope;
       #endif

        case kAudioUnitProperty_MaximumFramesPerSlice:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(UInt32), inDataSize, kAudioUnitErr_InvalidPropertyValue);

            if (fPlugin.setBufferSize(*static_cast<const UInt32*>(inData), true))
	            notifyListeners(inProp, inScope, inElement);

            return noErr;

        case kAudioUnitProperty_BypassEffect:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(UInt32), inDataSize, kAudioUnitErr_InvalidPropertyValue);
            DISTRHO_SAFE_ASSERT_RETURN(fBypassParameterIndex != UINT32_MAX, kAudioUnitErr_InvalidPropertyValue);
            {
                const bool bypass = *static_cast<const UInt32*>(inData) != 0;

                if ((fLastParameterValues[fBypassParameterIndex] > 0.5f) != bypass)
                {
                    const float value = bypass ? 1.f : 0.f;
                    fLastParameterValues[fBypassParameterIndex] = value;
                    fPlugin.setParameterValue(fBypassParameterIndex, value);
	                notifyListeners(inProp, inScope, inElement);
                }
            }
            return noErr;

        case kAudioUnitProperty_SetRenderCallback:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Input, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(AURenderCallbackStruct), inDataSize, kAudioUnitErr_InvalidPropertyValue);
            d_stdout("WIP SetProperty(%d:%s, %d:%s, %d, %p, %u)", inProp, AudioUnitPropertyID2Str(inProp), inScope, AudioUnitScope2Str(inScope), inElement, inData, inDataSize);
            // TODO
            return noErr;

        case kAudioUnitProperty_PresentPreset:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(AUPreset), inDataSize, kAudioUnitErr_InvalidPropertyValue);
            d_stdout("WIP SetProperty(%d:%s, %d:%s, %d, %p, %u)", inProp, AudioUnitPropertyID2Str(inProp), inScope, AudioUnitScope2Str(inScope), inElement, inData, inDataSize);
            // TODO
            return noErr;

        case 'DPFp':
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement < fParameterCount, inElement, kAudioUnitErr_InvalidElement);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(float), inDataSize, kAudioUnitErr_InvalidPropertyValue);
            {
                const float value = *static_cast<const float*>(inData);
                DISTRHO_SAFE_ASSERT_RETURN(std::isfinite(value), kAudioUnitErr_InvalidParameterValue);

                fLastParameterValues[inElement] = value;
                fPlugin.setParameterValue(inElement, value);

                AudioUnitEvent event;
                std::memset(&event, 0, sizeof(event));

                event.mEventType                        = kAudioUnitEvent_ParameterValueChange;
                event.mArgument.mParameter.mAudioUnit   = fComponent;
                event.mArgument.mParameter.mParameterID = inElement;
                event.mArgument.mParameter.mScope       = kAudioUnitScope_Global;
                AUEventListenerNotify(NULL, NULL, &event);
            }
            return noErr;

        case 'DPFt':
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement < fParameterCount, inElement, kAudioUnitErr_InvalidElement);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(bool), inDataSize, kAudioUnitErr_InvalidPropertyValue);
            {
                const bool started = *static_cast<const bool*>(inData);

                AudioUnitEvent event;
                std::memset(&event, 0, sizeof(event));

                event.mEventType                        = started ? kAudioUnitEvent_BeginParameterChangeGesture
                                                                  : kAudioUnitEvent_EndParameterChangeGesture;
                event.mArgument.mParameter.mAudioUnit   = fComponent;
                event.mArgument.mParameter.mParameterID = inElement;
                event.mArgument.mParameter.mScope       = kAudioUnitScope_Global;
                AUEventListenerNotify(NULL, NULL, &event);
            }
            return noErr;
        }

        d_stdout("TODO SetProperty(%d:%s, %d:%s, %d, %p, %u)", inProp, AudioUnitPropertyID2Str(inProp), inScope, AudioUnitScope2Str(inScope), inElement, inData, inDataSize);
        return kAudioUnitErr_InvalidProperty;
    }

    OSStatus auAddPropertyListener(const AudioUnitPropertyID prop,
                                   const AudioUnitPropertyListenerProc proc,
                                   void* const userData)
    {
        const PropertyListener pl = {
            prop, proc, userData
        };

        if (fPropertyListeners.empty())
            fPropertyListeners.reserve(32);

        fPropertyListeners.push_back(pl);
        return noErr;
    }

    OSStatus auRemovePropertyListener(const AudioUnitPropertyID prop, const AudioUnitPropertyListenerProc proc)
    {
        for (PropertyListeners::iterator it = fPropertyListeners.begin(); it != fPropertyListeners.end(); ++it)
        {
            const PropertyListener& pl(*it);

            if (pl.prop == prop && pl.proc == proc)
            {
                fPropertyListeners.erase(it);
                return auRemovePropertyListener(prop, proc);
            }
        }

        return noErr;
    }

    OSStatus auRemovePropertyListenerWithUserData(const AudioUnitPropertyID prop,
                                                  const AudioUnitPropertyListenerProc proc,
                                                  void* const userData)
    {
        for (PropertyListeners::iterator it = fPropertyListeners.begin(); it != fPropertyListeners.end(); ++it)
        {
            const PropertyListener& pl(*it);

            if (pl.prop == prop && pl.proc == proc && pl.userData == userData)
            {
                fPropertyListeners.erase(it);
                return auRemovePropertyListenerWithUserData(prop, proc, userData);
            }
        }

        return noErr;
    }

    OSStatus auAddRenderNotify(const AURenderCallback proc, void* const userData)
    {
        d_stdout("WIP AddRenderNotify(%p, %p)", proc, userData);
        // TODO
        return noErr;
    }

    OSStatus auRemoveRenderNotify(const AURenderCallback proc, void* const userData)
    {
        d_stdout("WIP RemoveRenderNotify(%p, %p)", proc, userData);
        // TODO
        return noErr;
    }

    OSStatus auGetParameter(const AudioUnitParameterID param,
                            const AudioUnitScope scope,
                            const AudioUnitElement elem,
                            AudioUnitParameterValue* const value)
    {
        DISTRHO_SAFE_ASSERT_UINT_RETURN(scope == kAudioUnitScope_Global, scope, kAudioUnitErr_InvalidScope);
        DISTRHO_SAFE_ASSERT_UINT_RETURN(param < fParameterCount, param, kAudioUnitErr_InvalidElement);
        DISTRHO_SAFE_ASSERT_UINT_RETURN(elem == 0, elem, kAudioUnitErr_InvalidElement);
        DISTRHO_SAFE_ASSERT_RETURN(value != nullptr, kAudio_ParamError);

        *value = fPlugin.getParameterValue(param);
        return noErr;
    }

    OSStatus auSetParameter(const AudioUnitParameterID param,
                            const AudioUnitScope scope,
                            const AudioUnitElement elem,
                            const AudioUnitParameterValue value,
                            const UInt32 bufferOffset)
    {
        DISTRHO_SAFE_ASSERT_UINT_RETURN(scope == kAudioUnitScope_Global, scope, kAudioUnitErr_InvalidScope);
        DISTRHO_SAFE_ASSERT_UINT_RETURN(param < fParameterCount, param, kAudioUnitErr_InvalidElement);
        DISTRHO_SAFE_ASSERT_UINT_RETURN(elem == 0, elem, kAudioUnitErr_InvalidElement);
        DISTRHO_SAFE_ASSERT_RETURN(std::isfinite(value), kAudioUnitErr_InvalidParameterValue);

        if (d_isNotEqual(fLastParameterValues[param], value))
        {
            fLastParameterValues[param] = value;
            fPlugin.setParameterValue(param, value);
            // TODO flag param only, notify listeners later on bg thread (sem_post etc)
            notifyListeners('DPFP', kAudioUnitScope_Global, param);
        }

        return noErr;
    }

    OSStatus auScheduleParameters(const AudioUnitParameterEvent* const events, const UInt32 numEvents)
    {
        for (UInt32 i=0; i<numEvents; ++i)
        {
            const AudioUnitParameterEvent& event(events[i]);

            if (event.eventType == kParameterEvent_Immediate)
            {
                auSetParameter(event.parameter,
                               event.scope,
                               event.element,
                               event.eventValues.immediate.value,
                               event.eventValues.immediate.bufferOffset);
            }
            else
            {
                // TODO?
                d_stdout("WIP ScheduleParameters(%p, %u)", events, numEvents);
            }
        }
        return noErr;
    }

    OSStatus auRender(AudioUnitRenderActionFlags& ioActionFlags,
                      const AudioTimeStamp& inTimeStamp,
                      const UInt32 inBusNumber,
                      const UInt32 inFramesToProcess,
                      AudioBufferList& ioData)
    {
        DISTRHO_SAFE_ASSERT_UINT_RETURN(inBusNumber == 0, inBusNumber, kAudioUnitErr_InvalidElement);
        DISTRHO_SAFE_ASSERT_UINT_RETURN(ioData.mNumberBuffers == std::max<uint>(DISTRHO_PLUGIN_NUM_INPUTS, DISTRHO_PLUGIN_NUM_OUTPUTS), ioData.mNumberBuffers, kAudio_ParamError);

        if (inFramesToProcess > fPlugin.getBufferSize())
        {
            setLastRenderError(kAudioUnitErr_TooManyFramesToProcess);
            return kAudioUnitErr_TooManyFramesToProcess;
        }

        for (uint i=0; i<ioData.mNumberBuffers; ++i)
        {
            AudioBuffer& buffer(ioData.mBuffers[i]);

            // TODO there must be something more to this...
            if (buffer.mData == nullptr)
                return noErr;

            DISTRHO_SAFE_ASSERT_UINT_RETURN(buffer.mDataByteSize == inFramesToProcess * sizeof(float), buffer.mDataByteSize, kAudio_ParamError);
        }

       #if DISTRHO_PLUGIN_NUM_INPUTS != 0
        const float* inputs[DISTRHO_PLUGIN_NUM_INPUTS];

        for (uint16_t i=0; i<DISTRHO_PLUGIN_NUM_INPUTS; ++i)
        {
            inputs[i] = static_cast<const float*>(ioData.mBuffers[i].mData);
        }
       #else
        constexpr const float** inputs = nullptr;
       #endif

       #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
        float* outputs[DISTRHO_PLUGIN_NUM_OUTPUTS];

        for (uint16_t i=0; i<DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
        {
            outputs[i] = static_cast<float*>(ioData.mBuffers[i].mData);
        }
       #else
        constexpr float** outputs = nullptr;
       #endif

       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        fPlugin.run(inputs, outputs, inFramesToProcess, nullptr, 0);
       #else
        fPlugin.run(inputs, outputs, inFramesToProcess);
       #endif

        float value;
        AudioUnitEvent event;
        std::memset(&event, 0, sizeof(event));
        event.mEventType                      = kAudioUnitEvent_ParameterValueChange;
        event.mArgument.mParameter.mAudioUnit = fComponent;
        event.mArgument.mParameter.mScope     = kAudioUnitScope_Global;

        for (uint32_t i=0; i<fParameterCount; ++i)
        {
            if (fPlugin.isParameterOutputOrTrigger(i))
            {
                value = fPlugin.getParameterValue(i);

                if (d_isEqual(fLastParameterValues[i], value))
                    continue;

                fLastParameterValues[i] = value;

                // TODO flag param only, notify listeners later on bg thread (sem_post etc)
                event.mArgument.mParameter.mParameterID = i;
                AUEventListenerNotify(NULL, NULL, &event);
                notifyListeners('DPFP', kAudioUnitScope_Global, i);
            }
        }

        return noErr;
    }

    OSStatus auReset(const AudioUnitScope scope, const AudioUnitElement elem)
    {
        DISTRHO_SAFE_ASSERT_UINT_RETURN(scope == kAudioUnitScope_Global || scope == kAudioUnitScope_Input || scope == kAudioUnitScope_Output, scope, kAudioUnitErr_InvalidScope);
        DISTRHO_SAFE_ASSERT_UINT_RETURN(elem == 0, elem, kAudioUnitErr_InvalidElement);

        if (fPlugin.isActive())
        {
            fPlugin.deactivate();
            fPlugin.activate();
        }

        return noErr;
    }

    // ----------------------------------------------------------------------------------------------------------------

private:
    PluginExporter fPlugin;

    // AU component
    const AudioUnit fComponent;

    // AUv2 related fields
    OSStatus fLastRenderError;
    PropertyListeners fPropertyListeners;
   #if DISTRHO_PLUGIN_NUM_INPUTS != 0
    Float64 fSampleRateForInput;
   #endif
   #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
    Float64 fSampleRateForOutput;
   #endif

    // Caching
    const uint32_t fParameterCount;
    float* fLastParameterValues;
    uint32_t fBypassParameterIndex;

    // ----------------------------------------------------------------------------------------------------------------

    void notifyListeners(const AudioUnitPropertyID prop, const AudioUnitScope scope, const AudioUnitElement elem)
    {
        for (PropertyListeners::iterator it = fPropertyListeners.begin(); it != fPropertyListeners.end(); ++it)
        {
            const PropertyListener& pl(*it);

            if (pl.prop == prop)
			    pl.proc(pl.userData, fComponent, prop, scope, elem);
        }
    }

    void setLastRenderError(const OSStatus err)
    {
        if (fLastRenderError != noErr)
            return;

        fLastRenderError = err;
        notifyListeners(kAudioUnitProperty_LastRenderError, kAudioUnitScope_Global, 0);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // DPF callbacks

   #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    bool writeMidi(const MidiEvent&)
    {
        return true;
    }

    static bool writeMidiCallback(void* const ptr, const MidiEvent& midiEvent)
    {
        return static_cast<PluginAU*>(ptr)->writeMidi(midiEvent);
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
    bool requestParameterValueChange(uint32_t, float)
    {
        return true;
    }

    static bool requestParameterValueChangeCallback(void* const ptr, const uint32_t index, const float value)
    {
        return static_cast<PluginAU*>(ptr)->requestParameterValueChange(index, value);
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_STATE
    bool updateState(const char*, const char*)
    {
        return true;
    }

    static bool updateStateValueCallback(void* const ptr, const char* const key, const char* const value)
    {
        return static_cast<PluginAU*>(ptr)->updateState(key, value);
    }
   #endif

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginAU)
};

// --------------------------------------------------------------------------------------------------------------------

struct AudioComponentPlugInInstance {
	AudioComponentPlugInInterface acpi;
    PluginAU* plugin;

    AudioComponentPlugInInstance() noexcept
        : acpi(),
          plugin(nullptr)
    {
        std::memset(&acpi, 0, sizeof(acpi));
        acpi.Open = Open;
		acpi.Close = Close;
		acpi.Lookup = Lookup;
		acpi.reserved = nullptr;
    }

    ~AudioComponentPlugInInstance()
    {
        delete plugin;
    }

	static OSStatus Open(void* const self, const AudioUnit component)
    {
        static_cast<AudioComponentPlugInInstance*>(self)->plugin = new PluginAU(component);
        return noErr;
    }

	static OSStatus Close(void* const self)
    {
        delete static_cast<AudioComponentPlugInInstance*>(self);
        return noErr;
    }

    static AudioComponentMethod Lookup(const SInt16 selector)
    {
        d_debug("AudioComponentPlugInInstance::Lookup(%3d:%s)", selector, AudioUnitSelector2Str(selector));

        switch (selector)
        {
        case kAudioUnitInitializeSelect:
            return reinterpret_cast<AudioComponentMethod>(Initialize);
        case kAudioUnitUninitializeSelect:
            return reinterpret_cast<AudioComponentMethod>(Uninitialize);
        case kAudioUnitGetPropertyInfoSelect:
            return reinterpret_cast<AudioComponentMethod>(GetPropertyInfo);
        case kAudioUnitGetPropertySelect:
            return reinterpret_cast<AudioComponentMethod>(GetProperty);
        case kAudioUnitSetPropertySelect:
            return reinterpret_cast<AudioComponentMethod>(SetProperty);
        case kAudioUnitAddPropertyListenerSelect:
            return reinterpret_cast<AudioComponentMethod>(AddPropertyListener);
        case kAudioUnitRemovePropertyListenerSelect:
            return reinterpret_cast<AudioComponentMethod>(RemovePropertyListener);
        case kAudioUnitRemovePropertyListenerWithUserDataSelect:
            return reinterpret_cast<AudioComponentMethod>(RemovePropertyListenerWithUserData);
        case kAudioUnitAddRenderNotifySelect:
            return reinterpret_cast<AudioComponentMethod>(AddRenderNotify);
        case kAudioUnitRemoveRenderNotifySelect:
            return reinterpret_cast<AudioComponentMethod>(RemoveRenderNotify);
        case kAudioUnitGetParameterSelect:
            return reinterpret_cast<AudioComponentMethod>(GetParameter);
        case kAudioUnitSetParameterSelect:
            return reinterpret_cast<AudioComponentMethod>(SetParameter);
        case kAudioUnitScheduleParametersSelect:
            return reinterpret_cast<AudioComponentMethod>(ScheduleParameters);
        case kAudioUnitRenderSelect:
            return reinterpret_cast<AudioComponentMethod>(Render);
        /*
        case kAudioUnitComplexRenderSelect:
            return reinterpret_cast<AudioComponentMethod>(ComplexRender);
        */
        case kAudioUnitResetSelect:
            return reinterpret_cast<AudioComponentMethod>(Reset);
        /*
        case kAudioUnitProcessSelect:
            return reinterpret_cast<AudioComponentMethod>(Process);
        case kAudioUnitProcessMultipleSelect:
            return reinterpret_cast<AudioComponentMethod>(ProcessMultiple);
        */
        }

        d_stdout("TODO Lookup(%3d:%s)", selector, AudioUnitSelector2Str(selector));
        return nullptr;
    }

    static OSStatus Initialize(AudioComponentPlugInInstance* const self)
    {
        d_debug("AudioComponentPlugInInstance::Initialize(%p)", self);
        return self->plugin->auInitialize();
    }

    static OSStatus Uninitialize(AudioComponentPlugInInstance* const self)
    {
        d_debug("AudioComponentPlugInInstance::Uninitialize(%p)", self);
        return self->plugin->auUninitialize();
    }

    static OSStatus GetPropertyInfo(AudioComponentPlugInInstance* const self,
                                    const AudioUnitPropertyID inProp,
                                    const AudioUnitScope inScope,
                                    const AudioUnitElement inElement,
                                    UInt32* const outDataSize,
                                    Boolean* const outWritable)
    {
        d_debug("AudioComponentPlugInInstance::GetPropertyInfo(%p, %2d:%s, %d:%s, %d, ...)",
                self, inProp, AudioUnitPropertyID2Str(inProp), inScope, AudioUnitScope2Str(inScope), inElement);

		UInt32 dataSize = 0;
		Boolean writable = false;
        const OSStatus res = self->plugin->auGetPropertyInfo(inProp, inScope, inElement, dataSize, writable);

		if (outDataSize != nullptr)
			*outDataSize = dataSize;

		if (outWritable != nullptr)
			*outWritable = writable;

        return res;
    }

    static OSStatus GetProperty(AudioComponentPlugInInstance* const self,
                                const AudioUnitPropertyID inProp,
                                const AudioUnitScope inScope,
                                const AudioUnitElement inElement,
                                void* const outData,
                                UInt32* const ioDataSize)
    {
        d_debug("AudioComponentPlugInInstance::GetProperty    (%p, %2d:%s, %d:%s, %d, ...)",
                self, inProp, AudioUnitPropertyID2Str(inProp), inScope, AudioUnitScope2Str(inScope), inElement);
        DISTRHO_SAFE_ASSERT_RETURN(ioDataSize != nullptr, kAudio_ParamError);

        Boolean writable;
        UInt32 outDataSize = 0;
        OSStatus res;

        if (outData == nullptr)
        {
            res = self->plugin->auGetPropertyInfo(inProp, inScope, inElement, outDataSize, writable);
            *ioDataSize = outDataSize;
            return res;
        }

        const UInt32 inDataSize = *ioDataSize;
        if (inDataSize == 0)
            return kAudio_ParamError;

        res = self->plugin->auGetPropertyInfo(inProp, inScope, inElement, outDataSize, writable);

        if (res != noErr)
            return res;

		void* outBuffer;
        uint8_t* tmpBuffer;
        if (inDataSize < outDataSize)
		{
			tmpBuffer = new uint8_t[outDataSize];
			outBuffer = tmpBuffer;
		}
        else
        {
			tmpBuffer = nullptr;
			outBuffer = outData;
		}

        res = self->plugin->auGetProperty(inProp, inScope, inElement, outBuffer);

		if (res != noErr)
        {
			*ioDataSize = 0;
            return res;
        }

        if (tmpBuffer != nullptr)
        {
            memcpy(outData, tmpBuffer, inDataSize);
            delete[] tmpBuffer;
        }
        else
        {
            *ioDataSize = outDataSize;
        }

        return noErr;
    }

    static OSStatus SetProperty(AudioComponentPlugInInstance* const self,
                                const AudioUnitPropertyID inProp,
                                const AudioUnitScope inScope,
                                const AudioUnitElement inElement,
                                const void* const inData,
                                const UInt32 inDataSize)
    {
        d_debug("AudioComponentPlugInInstance::SetProperty(%p, %d:%s, %d:%s, %d, %p, %u)",
                self, inProp, AudioUnitPropertyID2Str(inProp), inScope, AudioUnitScope2Str(inScope), inElement, inData, inDataSize);
        return self->plugin->auSetProperty(inProp, inScope, inElement, inData, inDataSize);
    }

    static OSStatus AddPropertyListener(AudioComponentPlugInInstance* const self,
                                        const AudioUnitPropertyID prop,
                                        const AudioUnitPropertyListenerProc proc,
                                        void* const userData)
    {
        d_debug("AudioComponentPlugInInstance::AddPropertyListener(%p, %d:%s, %p, %p)",
                self, prop, AudioUnitPropertyID2Str(prop), proc, userData);
        return self->plugin->auAddPropertyListener(prop, proc, userData);
    }

    static OSStatus RemovePropertyListener(AudioComponentPlugInInstance* const self,
                                           const AudioUnitPropertyID prop,
                                           const AudioUnitPropertyListenerProc proc)
    {
        d_debug("AudioComponentPlugInInstance::RemovePropertyListener(%p, %d:%s, %p)",
                self, prop, AudioUnitPropertyID2Str(prop), proc);
        return self->plugin->auRemovePropertyListener(prop, proc);
    }

    static OSStatus RemovePropertyListenerWithUserData(AudioComponentPlugInInstance* const self,
                                                       const AudioUnitPropertyID prop,
                                                       const AudioUnitPropertyListenerProc proc,
                                                       void* const userData)
    {
        d_debug("AudioComponentPlugInInstance::RemovePropertyListenerWithUserData(%p, %d:%s, %p, %p)",
                self, prop, AudioUnitPropertyID2Str(prop), proc, userData);
        return self->plugin->auRemovePropertyListenerWithUserData(prop, proc, userData);
    }

    static OSStatus AddRenderNotify(AudioComponentPlugInInstance* const self,
                                    const AURenderCallback proc,
                                    void* const userData)
    {
        d_debug("AudioComponentPlugInInstance::AddRenderNotify(%p, %p, %p)", self, proc, userData);
        return self->plugin->auAddRenderNotify(proc, userData);
    }

    static OSStatus RemoveRenderNotify(AudioComponentPlugInInstance* const self,
                                       const AURenderCallback proc,
                                       void* const userData)
    {
        d_debug("AudioComponentPlugInInstance::RemoveRenderNotify(%p, %p, %p)", self, proc, userData);
        return self->plugin->auRemoveRenderNotify(proc, userData);
    }

    static OSStatus GetParameter(AudioComponentPlugInInstance* const self,
                                 const AudioUnitParameterID param,
                                 const AudioUnitScope scope,
                                 const AudioUnitElement elem,
                                 AudioUnitParameterValue* const value)
    {
        d_debug("AudioComponentPlugInInstance::GetParameter(%p, %d, %d:%s, %d, %p)",
                self, param, scope, AudioUnitScope2Str(scope), elem, value);
        return self->plugin->auGetParameter(param, scope, elem, value);
    }

    static OSStatus SetParameter(AudioComponentPlugInInstance* const self,
                                 const AudioUnitParameterID param,
                                 const AudioUnitScope scope,
                                 const AudioUnitElement elem,
                                 const AudioUnitParameterValue value,
                                 const UInt32 bufferOffset)
    {
        d_debug("AudioComponentPlugInInstance::SetParameter(%p, %d %d:%s, %d, %f, %u)",
                self, param, scope, AudioUnitScope2Str(scope), elem, value, bufferOffset);
        return self->plugin->auSetParameter(param, scope, elem, value, bufferOffset);
    }

    static OSStatus ScheduleParameters(AudioComponentPlugInInstance* const self,
                                       const AudioUnitParameterEvent* const events,
                                       const UInt32 numEvents)
    {
        d_debug("AudioComponentPlugInInstance::ScheduleParameters(%p, %p, %u)", self, events, numEvents);
        return self->plugin->auScheduleParameters(events, numEvents);
    }

    static OSStatus Reset(AudioComponentPlugInInstance* const self,
                          const AudioUnitScope scope,
                          const AudioUnitElement elem)
    {
        d_debug("AudioComponentPlugInInstance::Reset(%p, %d:%s, %d)", self, scope, AudioUnitScope2Str(scope), elem);
        return self->plugin->auReset(scope, elem);
    }

    static OSStatus Render(AudioComponentPlugInInstance* const self,
                           AudioUnitRenderActionFlags* ioActionFlags,
                           const AudioTimeStamp* const inTimeStamp,
                           const UInt32 inOutputBusNumber,
                           const UInt32 inNumberFrames,
                           AudioBufferList* const ioData)
    {
        DISTRHO_SAFE_ASSERT_RETURN(inTimeStamp != nullptr, kAudio_ParamError);
        DISTRHO_SAFE_ASSERT_RETURN(ioData != nullptr, kAudio_ParamError);

        AudioUnitRenderActionFlags tmpFlags;

        if (ioActionFlags == nullptr)
        {
            tmpFlags = 0;
            ioActionFlags = &tmpFlags;
        }

        return self->plugin->auRender(*ioActionFlags, *inTimeStamp, inOutputBusNumber, inNumberFrames, *ioData);
    }

    DISTRHO_DECLARE_NON_COPYABLE(AudioComponentPlugInInstance)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

DISTRHO_PLUGIN_EXPORT
void* PluginAUFactory(const AudioComponentDescription* const desc)
{
    USE_NAMESPACE_DISTRHO

    DISTRHO_SAFE_ASSERT_UINT2_RETURN(desc->componentType == kType, desc->componentType, kType, nullptr);
    DISTRHO_SAFE_ASSERT_UINT2_RETURN(desc->componentSubType == kSubType, desc->componentSubType, kSubType, nullptr);
    DISTRHO_SAFE_ASSERT_UINT2_RETURN(desc->componentManufacturer == kManufacturer, desc->componentManufacturer, kManufacturer, nullptr);

    if (d_nextBufferSize == 0)
        d_nextBufferSize = 1156;

    if (d_isZero(d_nextSampleRate))
        d_nextSampleRate = 44100.0;

    if (d_nextBundlePath == nullptr)
    {
        static String bundlePath;

        String tmpPath(getBinaryFilename());
        tmpPath.truncate(tmpPath.rfind(DISTRHO_OS_SEP));
        tmpPath.truncate(tmpPath.rfind(DISTRHO_OS_SEP));

        if (tmpPath.endsWith(DISTRHO_OS_SEP_STR "Contents"))
        {
            tmpPath.truncate(tmpPath.rfind(DISTRHO_OS_SEP));
            bundlePath = tmpPath;
        }
        else
        {
            bundlePath = "error";
        }

        d_nextBundlePath = bundlePath.buffer();
    }

    d_nextCanRequestParameterValueChanges = true;

    return new AudioComponentPlugInInstance();
}

// --------------------------------------------------------------------------------------------------------------------
