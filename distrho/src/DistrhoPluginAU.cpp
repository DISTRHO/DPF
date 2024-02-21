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

#define TRACE d_stderr("////////--------------------------------------------------------------- %s %d", __PRETTY_FUNCTION__, __LINE__);

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
    PROP(kAudioUnitProperty_FastDispatch)
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
          fParameterCount(fPlugin.getParameterCount()),
          fCachedParameterValues(nullptr)
    {
        TRACE

#if 0
	    if (fParameterCount != 0)
        {
            CreateElements();

            fCachedParameterValues = new float[fParameterCount];
            std::memset(fCachedParameterValues, 0, sizeof(float) * fParameterCount);

            AUElement* const globals = GlobalScope().GetElement(0);
            DISTRHO_SAFE_ASSERT_RETURN(globals != nullptr,);

            globals->UseIndexedParameters(fParameterCount);

            for (uint32_t i=0; i<fParameterCount; ++i)
            {
                fCachedParameterValues[i] = fPlugin.getParameterValue(i);
                globals->SetParameter(i, fCachedParameterValues[i]);
            }
        }

        // static
        SupportedNumChannels(nullptr);
            #endif
    }

    ~PluginAU()
    {
        TRACE
        delete[] fCachedParameterValues;
    }

    OSStatus auInitialize()
    {
        return noErr;
    }

    OSStatus auUninitialize()
    {
        return noErr;
    }

    OSStatus auGetPropertyInfo(const AudioUnitPropertyID inID,
                               const AudioUnitScope inScope,
                               const AudioUnitElement inElement,
                               UInt32& outDataSize,
                               Boolean& outWritable)
    {
        switch (inID)
        {
        case kAudioUnitProperty_ClassInfo:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            /* TODO used for storing plugin state
            outDataSize = sizeof(CFPropertyListRef);
            outWritable = true;
            */
            break;
        case kAudioUnitProperty_ElementCount:
            outDataSize = sizeof(UInt32);
            outWritable = false;
            return noErr;
       #if DISTRHO_PLUGIN_WANT_LATENCY
        case kAudioUnitProperty_Latency:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            outDataSize = sizeof(Float64);
            outWritable = false;
            return noErr;
       #endif
        case kAudioUnitProperty_SupportedNumChannels:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            outDataSize = sizeof(AUChannelInfo);
            outWritable = false;
            return noErr;
        case kAudioUnitProperty_MaximumFramesPerSlice:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            outDataSize = sizeof(UInt32);
            outWritable = true;
            return noErr;
       #if DISTRHO_PLUGIN_HAS_UI
        case kAudioUnitProperty_CocoaUI:
            outDataSize = sizeof(AudioUnitCocoaViewInfo);
            outWritable = false;
            return noErr;
       #endif
        }

        return kAudioUnitErr_InvalidProperty;
    }

    OSStatus auGetProperty(const AudioUnitPropertyID inID,
                           const AudioUnitScope inScope,
                           const AudioUnitElement inElement,
                           void* const outData)
    {
        switch (inID)
        {
        case kAudioUnitProperty_ClassInfo:
            /* TODO used for storing plugin state
            *static_cast<CFPropertyListRef*>(outData) = nullptr;
            */
            break;
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
       #if DISTRHO_PLUGIN_HAS_UI
        case kAudioUnitProperty_CocoaUI:
            {
                AudioUnitCocoaViewInfo* const info = static_cast<AudioUnitCocoaViewInfo*>(outData);

                NSString* const bundlePathString = [[NSString alloc]
                    initWithBytes:d_nextBundlePath
                           length:strlen(d_nextBundlePath)
                         encoding:NSUTF8StringEncoding];

                info->mCocoaAUViewBundleLocation = static_cast<CFURLRef>([[NSURL fileURLWithPath: bundlePathString] retain]);
                info->mCocoaAUViewClass[0] = CFSTR("DPF_UI_ViewFactory");

                [bundlePathString release];
            }
            return noErr;
       #endif
        }

        return kAudioUnitErr_InvalidProperty;
    }

    OSStatus auSetProperty(const AudioUnitPropertyID inID,
                           const AudioUnitScope inScope,
                           const AudioUnitElement inElement,
                           const void* const inData,
                           const UInt32 inDataSize)
    {
        switch (inID)
        {
        case kAudioUnitProperty_ClassInfo:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(CFPropertyListRef*), inDataSize, kAudioUnitErr_InvalidPropertyValue);
            /* TODO used for restoring plugin state
            *static_cast<CFPropertyListRef*>(inData);
            */
            break;
        case kAudioUnitProperty_MaximumFramesPerSlice:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(UInt32), inDataSize, kAudioUnitErr_InvalidPropertyValue);
            fPlugin.setBufferSize(*static_cast<const UInt32*>(inData));
            return noErr;
        }

        return kAudioUnitErr_InvalidProperty;
    }

#if 0
protected:
    // ----------------------------------------------------------------------------------------------------------------
    // ComponentBase AU dispatch

	OSStatus GetParameter(const AudioUnitParameterID inParameterID,
                          const AudioUnitScope inScope,
                          const AudioUnitElement inElement,
                          AudioUnitParameterValue& outValue) override
    {
	    DISTRHO_SAFE_ASSERT_INT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
	    DISTRHO_SAFE_ASSERT_UINT_RETURN(inParameterID < fParameterCount, inParameterID, kAudioUnitErr_InvalidParameter);

        return fPlugin.getParameterValue(inParameterID);
    }

	OSStatus SetParameter(AudioUnitParameterID inParameterID,
                          const AudioUnitScope inScope,
                          const AudioUnitElement inElement,
                          const AudioUnitParameterValue inValue,
                          const UInt32 inBufferOffsetInFrames) override
    {
	    DISTRHO_SAFE_ASSERT_INT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
	    DISTRHO_SAFE_ASSERT_UINT_RETURN(inParameterID < fParameterCount, inParameterID, kAudioUnitErr_InvalidParameter);

        PluginBase::SetParameter(inParameterID, inScope, inElement, inValue, inBufferOffsetInFrames);
        fPlugin.setParameterValue(inParameterID, inValue);
        return noErr;
    }

    OSStatus Render(AudioUnitRenderActionFlags& ioActionFlags,
                    const AudioTimeStamp& inTimeStamp,
                    const UInt32 nFrames) override
    {
        float value;
        for (uint32_t i=0; i<fParameterCount; ++i)
        {
            if (fPlugin.isParameterOutputOrTrigger(i))
            {
                value = fPlugin.getParameterValue(i);

                if (d_isEqual(fCachedParameterValues[i], value))
                    continue;

                fCachedParameterValues[i] = value;

                if (AUElement* const elem = GlobalScope().GetElement(0))
                    elem->SetParameter(i, value);
            }
        }

        return noErr;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // ComponentBase Property Dispatch

    OSStatus GetParameterInfo(const AudioUnitScope inScope,
                              const AudioUnitParameterID inParameterID,
                              AudioUnitParameterInfo& outParameterInfo) override
    {
        TRACE
	    DISTRHO_SAFE_ASSERT_INT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
	    DISTRHO_SAFE_ASSERT_UINT_RETURN(inParameterID < fParameterCount, inParameterID, kAudioUnitErr_InvalidParameter);

        const uint32_t index = inParameterID;
        const ParameterRanges& ranges(fPlugin.getParameterRanges(index));

        outParameterInfo.flags = kAudioUnitParameterFlag_IsHighResolution
                               | kAudioUnitParameterFlag_IsReadable
                               | kAudioUnitParameterFlag_HasCFNameString;

        if (fPlugin.getParameterDesignation(index) == kParameterDesignationBypass)
        {
            outParameterInfo.flags |= kAudioUnitParameterFlag_IsWritable|kAudioUnitParameterFlag_NonRealTime;
            outParameterInfo.unit = kAudioUnitParameterUnit_Generic;

            d_strncpy(outParameterInfo.name, "Bypass", sizeof(outParameterInfo.name));
            outParameterInfo.cfNameString = CFSTR("Bypass");
        }
        else
        {
            const uint32_t hints = fPlugin.getParameterHints(index);

            outParameterInfo.flags |= kAudioUnitParameterFlag_CFNameRelease;

            if (hints & kParameterIsOutput)
            {
                outParameterInfo.flags |= kAudioUnitParameterFlag_MeterReadOnly;
            }
            else
            {
                outParameterInfo.flags |= kAudioUnitParameterFlag_IsWritable;

                if ((hints & kParameterIsAutomatable) == 0x0)
                    outParameterInfo.flags |= kAudioUnitParameterFlag_NonRealTime;
            }

            if (hints & kParameterIsBoolean)
                outParameterInfo.unit = kAudioUnitParameterUnit_Boolean;
            else if (hints & kParameterIsInteger)
                outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
            else
                outParameterInfo.unit = kAudioUnitParameterUnit_Generic;

            // | kAudioUnitParameterFlag_ValuesHaveStrings;

            const String& name(fPlugin.getParameterName(index));
            d_strncpy(outParameterInfo.name, name, sizeof(outParameterInfo.name));
            outParameterInfo.cfNameString = static_cast<CFStringRef>([[NSString stringWithUTF8String:name] retain]);
        }

        outParameterInfo.minValue = ranges.min;
        outParameterInfo.maxValue = ranges.max;
        outParameterInfo.defaultValue = ranges.def;

        return noErr;
    }

private:
    double getSampleRate()
    {
       #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0 && 0
        if (AUOutputElement* const output = GetOutput(0))
        {
            const double sampleRate = d_nextSampleRate = output->GetStreamFormat().mSampleRate;
            return sampleRate;
        }
       #endif

       #if DISTRHO_PLUGIN_NUM_INPUTS != 0 && 0
        if (AUInputElement* const input = GetInput(0))
        {
            const double sampleRate = d_nextSampleRate = input->GetStreamFormat().mSampleRate;
            return sampleRate;
        }
       #endif

        return d_nextSampleRate;
    }

    void setBufferSizeAndSampleRate()
    {
        fPlugin.setSampleRate(getSampleRate(), true);
        fPlugin.setBufferSize(GetMaxFramesPerSlice(), true);
    }
#endif

private:
    PluginExporter fPlugin;

    const uint32_t fParameterCount;
    float* fCachedParameterValues;

    // ----------------------------------------------------------------------------------------------------------------
    // DPF callbacks

   #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    bool writeMidi(const MidiEvent&)
    {
        return true;
    }

    static bool writeMidiCallback(void* const ptr, const MidiEvent& midiEvent)
    {
        return static_cast<PluginHolder*>(ptr)->writeMidi(midiEvent);
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
    bool requestParameterValueChange(uint32_t, float)
    {
        return true;
    }

    static bool requestParameterValueChangeCallback(void* const ptr, const uint32_t index, const float value)
    {
        return static_cast<PluginHolder*>(ptr)->requestParameterValueChange(index, value);
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_STATE
    bool updateState(const char*, const char*)
    {
        return true;
    }

    static bool updateStateValueCallback(void* const ptr, const char* const key, const char* const value)
    {
        return static_cast<PluginHolder*>(ptr)->updateState(key, value);
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
		acpi.reserved = NULL;
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
        d_stdout("AudioComponentPlugInInstance::Lookup(%3d:%s)", selector, AudioUnitSelector2Str(selector));

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
        #if 0
        case kAudioUnitAddPropertyListenerSelect:
            return reinterpret_cast<AudioComponentMethod>(Uninitialize);
        case kAudioUnitRemovePropertyListenerSelect:
            return reinterpret_cast<AudioComponentMethod>(Uninitialize);
        case kAudioUnitRemovePropertyListenerWithUserDataSelect:
            return reinterpret_cast<AudioComponentMethod>(Uninitialize);
        case kAudioUnitAddRenderNotifySelect:
            return reinterpret_cast<AudioComponentMethod>(Uninitialize);
        case kAudioUnitRemoveRenderNotifySelect:
            return reinterpret_cast<AudioComponentMethod>(Uninitialize);
        case kAudioUnitGetParameterSelect:
            return reinterpret_cast<AudioComponentMethod>(Uninitialize);
        case kAudioUnitSetParameterSelect:
            return reinterpret_cast<AudioComponentMethod>(Uninitialize);
        case kAudioUnitScheduleParametersSelect:
            return reinterpret_cast<AudioComponentMethod>(Uninitialize);
        case kAudioUnitRenderSelect:
            return reinterpret_cast<AudioComponentMethod>(Uninitialize);
        case kAudioUnitResetSelect:
            return reinterpret_cast<AudioComponentMethod>(Uninitialize);
        case kAudioUnitComplexRenderSelect:
            return reinterpret_cast<AudioComponentMethod>(Uninitialize);
        case kAudioUnitProcessSelect:
            return reinterpret_cast<AudioComponentMethod>(Uninitialize);
        case kAudioUnitProcessMultipleSelect:
            return reinterpret_cast<AudioComponentMethod>(Uninitialize);
        #endif
        }

        return nullptr;
    }

    static OSStatus Initialize(AudioComponentPlugInInstance* const self)
    {
        d_stdout("AudioComponentPlugInInstance::Initialize(%p)", self);
        return self->plugin->auInitialize();
    }

    static OSStatus Uninitialize(AudioComponentPlugInInstance* const self)
    {
        d_stdout("AudioComponentPlugInInstance::Uninitialize(%p)", self);
        return self->plugin->auUninitialize();
    }

    static OSStatus GetPropertyInfo(AudioComponentPlugInInstance* const self,
                                    const AudioUnitPropertyID inID,
                                    const AudioUnitScope inScope,
                                    const AudioUnitElement inElement,
                                    UInt32* const outDataSize,
                                    Boolean* const outWritable)
    {
        d_stdout("AudioComponentPlugInInstance::GetPropertyInfo(%p, %2d:%s, %d:%s, %d, %p, ...)",
                 self, inID, AudioUnitPropertyID2Str(inID), inScope, AudioUnitScope2Str(inScope), inElement);

		UInt32 dataSize = 0;
		Boolean writable = false;
        const OSStatus res = self->plugin->auGetPropertyInfo(inID, inScope, inElement, dataSize, writable);

		if (outDataSize != nullptr)
			*outDataSize = dataSize;

		if (outWritable != nullptr)
			*outWritable = writable;

        return res;
    }

    static OSStatus GetProperty(AudioComponentPlugInInstance* const self,
                                const AudioUnitPropertyID inID,
                                const AudioUnitScope inScope,
                                const AudioUnitElement inElement,
                                void* const outData,
                                UInt32* const ioDataSize)
    {
        d_stdout("AudioComponentPlugInInstance::GetProperty    (%p, %2d:%s, %d:%s, %d, %p, ...)",
                 self, inID, AudioUnitPropertyID2Str(inID), inScope, AudioUnitScope2Str(inScope), inElement);
        DISTRHO_SAFE_ASSERT_RETURN(ioDataSize != nullptr, kAudio_ParamError);

        Boolean writable;
        UInt32 outDataSize = 0;
        OSStatus res;

        if (outData == nullptr)
        {
            res = self->plugin->auGetPropertyInfo(inID, inScope, inElement, outDataSize, writable);
            *ioDataSize = outDataSize;
            return res;
        }

        const UInt32 inDataSize = *ioDataSize;
        DISTRHO_SAFE_ASSERT_RETURN(inDataSize != 0, kAudio_ParamError);

        res = self->plugin->auGetPropertyInfo(inID, inScope, inElement, outDataSize, writable);

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

        res = self->plugin->auGetProperty(inID, inScope, inElement, outBuffer);

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
                                const AudioUnitPropertyID inID,
                                const AudioUnitScope inScope,
                                const AudioUnitElement inElement,
                                const void* const inData,
                                const UInt32 inDataSize)
    {
        d_stdout("AudioComponentPlugInInstance::SetProperty(%p, %d:%s, %d:%s, %d, %d, %p, %u)",
                 self, inID, AudioUnitPropertyID2Str(inID), inScope, AudioUnitScope2Str(inScope), inElement, inData, inDataSize);
        return self->plugin->auSetProperty(inID, inScope, inElement, inData, inDataSize);
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

DISTRHO_PLUGIN_EXPORT
void* PluginAUFactory(const AudioComponentDescription*)
{
    USE_NAMESPACE_DISTRHO
    TRACE

    if (d_nextBufferSize == 0)
        d_nextBufferSize = 1156;

    if (d_isZero(d_nextSampleRate))
        d_nextSampleRate = 48000.0;

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
