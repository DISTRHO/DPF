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
// - report latency changes

#include "DistrhoPluginInternal.hpp"
#include "../DistrhoPluginUtils.hpp"

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
# include "../extra/RingBuffer.hpp"
#endif

#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioUnitUtilities.h>

#include <map>
#include <vector>

#ifndef DISTRHO_PLUGIN_BRAND_ID
# error DISTRHO_PLUGIN_BRAND_ID undefined!
#endif

#ifndef DISTRHO_PLUGIN_UNIQUE_ID
# error DISTRHO_PLUGIN_UNIQUE_ID undefined!
#endif

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

#ifndef __MAC_12_3
enum {
    kAudioUnitProperty_MIDIOutputBufferSizeHint = 66,
};
#endif

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
    PROP(kMusicDeviceProperty_DualSchedulingMode)
    #undef PROP
    // DPF specific properties
    #define PROPX(s) (s[0] << 24) | (s[1] << 16) | (s[2] << 8) | (s[3] << 0)
    #define PROP(s) case PROPX(#s): return #s;
    PROP(DPFi)
    PROP(DPFe)
    PROP(DPFp)
    PROP(DPFn)
    PROP(DPFo)
    PROP(DPFl)
    PROP(DPFs)
    PROP(DPFa)
    #undef PROP
    #undef PROPX
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

#if 0
static OSStatus FastDispatchGetParameter(void*, AudioUnitParameterID, AudioUnitScope, AudioUnitElement, Float32*);
static OSStatus FastDispatchSetParameter(void*, AudioUnitParameterID, AudioUnitScope, AudioUnitElement, Float32, UInt32);
static OSStatus FastDispatchRender(void*, AudioUnitRenderActionFlags*, const AudioTimeStamp*, UInt32, UInt32, AudioBufferList*);
#endif

// --------------------------------------------------------------------------------------------------------------------

static constexpr const uint32_t kType = d_cconst(STRINGIFY(DISTRHO_PLUGIN_AU_TYPE));
static constexpr const uint32_t kSubType = d_cconst(STRINGIFY(DISTRHO_PLUGIN_UNIQUE_ID));
static constexpr const uint32_t kManufacturer = d_cconst(STRINGIFY(DISTRHO_PLUGIN_BRAND_ID));

static constexpr const uint32_t kWantedAudioFormat = kAudioFormatFlagsNativeFloatPacked
                                                   | kAudioFormatFlagIsNonInterleaved;


// --------------------------------------------------------------------------------------------------------------------
// clang `std::max` is not constexpr compatible, we need to define our own

template<typename T>
static inline constexpr T d_max(const T a, const T b) { return a > b ? a : b; }

// --------------------------------------------------------------------------------------------------------------------

static constexpr const AUChannelInfo kChannelInfo[] = {
    { DISTRHO_PLUGIN_NUM_INPUTS, DISTRHO_PLUGIN_NUM_OUTPUTS },
   #ifdef DISTRHO_PLUGIN_EXTRA_IO
    DISTRHO_PLUGIN_EXTRA_IO
   #endif
};

#ifdef DISTRHO_PLUGIN_EXTRA_IO
#if DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS == 0
#error DISTRHO_PLUGIN_EXTRA_IO defined but no IO available
#endif

static inline
bool isInputNumChannelsValid(const uint16_t numChannels)
{
    for (uint16_t i = 0; i < ARRAY_SIZE(kChannelInfo); ++i)
    {
        if (kChannelInfo[i].inChannels == numChannels)
            return true;
    }
    return false;
}

static inline
bool isOutputNumChannelsValid(const uint16_t numChannels)
{
    for (uint16_t i = 0; i < ARRAY_SIZE(kChannelInfo); ++i)
    {
        if (kChannelInfo[i].outChannels == numChannels)
            return true;
    }
    return false;
}

static inline
bool isNumChannelsComboValid(const uint16_t numInputs, const uint16_t numOutputs)
{
    for (uint16_t i = 0; i < ARRAY_SIZE(kChannelInfo); ++i)
    {
        if (kChannelInfo[i].inChannels == numInputs && kChannelInfo[i].outChannels == numOutputs)
            return true;
    }
    return false;
}
#endif

// --------------------------------------------------------------------------------------------------------------------

struct PropertyListener {
    AudioUnitPropertyID	prop;
    AudioUnitPropertyListenerProc proc;
    void* userData;
};

struct RenderListener {
    AURenderCallback proc;
    void* userData;
};

typedef std::vector<PropertyListener> PropertyListeners;
typedef std::vector<RenderListener> RenderListeners;

// --------------------------------------------------------------------------------------------------------------------

typedef struct {
    UInt32 numPackets;
    MIDIPacket packets[kMaxMidiEvents];
} d_MIDIPacketList;

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
#if DISTRHO_PLUGIN_WANT_TIMEPOS
static constexpr const double kDefaultTicksPerBeat = 1920.0;
#endif

typedef std::map<const String, String> StringMap;

// --------------------------------------------------------------------------------------------------------------------

class PluginAU
{
public:
    PluginAU(const AudioUnit component)
        : fPlugin(this, writeMidiCallback, requestParameterValueChangeCallback, updateStateValueCallback),
          fComponent(component),
          fLastRenderError(noErr),
          fPropertyListeners(),
          fRenderListeners(),
        #if DISTRHO_PLUGIN_NUM_INPUTS != 0
          fInputConnectionBus(0),
          fInputConnectionUnit(nullptr),
          fSampleRateForInput(d_nextSampleRate),
         #ifdef DISTRHO_PLUGIN_EXTRA_IO
          fNumInputs(DISTRHO_PLUGIN_NUM_INPUTS),
         #endif
        #endif
        #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
          fSampleRateForOutput(d_nextSampleRate),
         #ifdef DISTRHO_PLUGIN_EXTRA_IO
          fNumOutputs(DISTRHO_PLUGIN_NUM_OUTPUTS),
         #endif
        #endif
         #if DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS != 0
          fAudioBufferList(nullptr),
         #endif
          fUsingRenderListeners(false),
          fParameterCount(fPlugin.getParameterCount()),
          fLastParameterValues(nullptr),
          fBypassParameterIndex(UINT32_MAX)
       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        , fMidiEventCount(0)
       #endif
       #if DISTRHO_PLUGIN_WANT_PROGRAMS
        , fCurrentProgram(-1)
        , fLastFactoryProgram(0)
        , fProgramCount(fPlugin.getProgramCount())
        , fFactoryPresetsData(nullptr)
       #endif
       #if DISTRHO_PLUGIN_WANT_STATE
        , fStateCount(fPlugin.getStateCount())
       #endif
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

       #if DISTRHO_PLUGIN_NUM_INPUTS != 0
        std::memset(&fInputRenderCallback, 0, sizeof(fInputRenderCallback));
        fInputRenderCallback.inputProc = nullptr;
        fInputRenderCallback.inputProcRefCon = nullptr;
       #endif

       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        std::memset(&fMidiEvents, 0, sizeof(fMidiEvents));
       #endif

       #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        std::memset(&fMidiOutput, 0, sizeof(fMidiOutput));
        std::memset(&fMidiOutputPackets, 0, sizeof(fMidiOutputPackets));
       #endif

       #if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (fProgramCount != 0)
        {
            fFactoryPresetsData = new AUPreset[fProgramCount];
            std::memset(fFactoryPresetsData, 0, sizeof(AUPreset) * fProgramCount);

            for (uint32_t i=0; i<fProgramCount; ++i)
            {
                fFactoryPresetsData[i].presetNumber = i;

                if (const CFStringRef nameRef = CFStringCreateWithCString(nullptr,
                                                                          fPlugin.getProgramName(i),
                                                                          kCFStringEncodingUTF8))
                    fFactoryPresetsData[i].presetName = nameRef;
                else
                    fFactoryPresetsData[i].presetName = CFSTR("");
            }
        }
        else
        {
            fFactoryPresetsData = new AUPreset;
            std::memset(fFactoryPresetsData, 0, sizeof(AUPreset));

            fFactoryPresetsData->presetNumber = 0;
            fFactoryPresetsData->presetName = CFSTR("Default");
        }
       #endif

        fUserPresetData.presetNumber = -1;
        fUserPresetData.presetName = CFSTR("");

       #if DISTRHO_PLUGIN_WANT_STATE
        for (uint32_t i=0; i<fStateCount; ++i)
        {
            const String& dkey(fPlugin.getStateKey(i));
            fStateMap[dkey] = fPlugin.getStateDefaultValue(i);
        }
       #endif

       #if DISTRHO_PLUGIN_WANT_TIMEPOS
        std::memset(&fHostCallbackInfo, 0, sizeof(fHostCallbackInfo));
        fTimePosition.bbt.ticksPerBeat = kDefaultTicksPerBeat;
       #endif
    }

    ~PluginAU()
    {
        delete[] fLastParameterValues;
        CFRelease(fUserPresetData.presetName);

       #if DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS != 0
        reallocAudioBufferList(false);
       #endif

       #if DISTRHO_PLUGIN_WANT_PROGRAMS
        for (uint32_t i=0; i<fProgramCount; ++i)
            CFRelease(fFactoryPresetsData[i].presetName);
        delete[] fFactoryPresetsData;
       #endif
    }

    OSStatus auInitialize()
    {
       #if defined(DISTRHO_PLUGIN_EXTRA_IO) && DISTRHO_PLUGIN_NUM_INPUTS != 0 && DISTRHO_PLUGIN_NUM_OUTPUTS != 0
        if (! isNumChannelsComboValid(fNumInputs, fNumOutputs))
            return kAudioUnitErr_FormatNotSupported;
       #endif

       #if DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS != 0
        if (! reallocAudioBufferList(true))
            return kAudio_ParamError;
       #endif

       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        fMidiEventCount = 0;
       #endif
       #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        fMidiOutputPackets.numPackets = 0;
       #endif
       #if DISTRHO_PLUGIN_WANT_TIMEPOS
        fTimePosition.clear();
        fTimePosition.bbt.ticksPerBeat = kDefaultTicksPerBeat;
       #endif

        fPlugin.activate();
        return noErr;
    }

    OSStatus auUninitialize()
    {
        fPlugin.deactivateIfNeeded();

       #if DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS != 0
        reallocAudioBufferList(false);
       #endif
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
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Input, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
           #if DISTRHO_PLUGIN_NUM_INPUTS != 0
            outDataSize = sizeof(AudioUnitConnection);
            outWritable = true;
            return noErr;
           #else
            return kAudioUnitErr_InvalidProperty;
           #endif

        case kAudioUnitProperty_SampleRate:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
           #if DISTRHO_PLUGIN_NUM_INPUTS != 0
            if (inScope == kAudioUnitScope_Input)
            {
                outDataSize = sizeof(Float64);
                outWritable = true;
                return noErr;
            }
           #endif
           #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
            if (inScope == kAudioUnitScope_Output)
            {
                outDataSize = sizeof(Float64);
                outWritable = true;
                return noErr;
            }
           #endif
            return kAudioUnitErr_InvalidScope;

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

       #if 0
        case kAudioUnitProperty_FastDispatch:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            outDataSize = sizeof(void*);
            outWritable = false;
            return noErr;
       #endif

        case kAudioUnitProperty_StreamFormat:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
           #if DISTRHO_PLUGIN_NUM_INPUTS != 0
            if (inScope == kAudioUnitScope_Input)
            {
                outDataSize = sizeof(AudioStreamBasicDescription);
                outWritable = true;
                return noErr;
            }
           #endif
           #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
            if (inScope == kAudioUnitScope_Output)
            {
                outDataSize = sizeof(AudioStreamBasicDescription);
                outWritable = true;
                return noErr;
            }
           #endif
            return kAudioUnitErr_InvalidScope;

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
            outDataSize = sizeof(kChannelInfo);
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
                outWritable = true;
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
           #if DISTRHO_PLUGIN_NUM_INPUTS != 0
            outDataSize = sizeof(AURenderCallbackStruct);
            outWritable = true;
            return noErr;
           #else
            return kAudioUnitErr_InvalidProperty;
           #endif

        case kAudioUnitProperty_FactoryPresets:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
           #if DISTRHO_PLUGIN_WANT_PROGRAMS
            outDataSize = sizeof(CFArrayRef);
            outWritable = false;
            return noErr;
           #else
            return kAudioUnitErr_InvalidProperty;
           #endif

        case kAudioUnitProperty_HostCallbacks:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
           #if DISTRHO_PLUGIN_WANT_TIMEPOS
            outDataSize = sizeof(HostCallbackInfo);
            outWritable = false;
            return noErr;
           #else
            return kAudioUnitErr_InvalidProperty;
           #endif

        case kAudioUnitProperty_InPlaceProcessing:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
           #if DISTRHO_PLUGIN_NUM_INPUTS != 0 && DISTRHO_PLUGIN_NUM_OUTPUTS != 0
            outDataSize = sizeof(UInt32);
            outWritable = false;
            return noErr;
           #else
            return kAudioUnitErr_InvalidProperty;
           #endif

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

        case kAudioUnitProperty_MIDIOutputCallbackInfo:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
           #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
            outDataSize = sizeof(CFArrayRef);
            outWritable = false;
            return noErr;
           #else
            return kAudioUnitErr_InvalidProperty;
           #endif

        case kAudioUnitProperty_MIDIOutputCallback:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
           #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
            outDataSize = sizeof(AUMIDIOutputCallbackStruct);
            outWritable = true;
            return noErr;
           #else
            return kAudioUnitErr_InvalidProperty;
           #endif

        case kAudioUnitProperty_AudioUnitMIDIProtocol:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            // FIXME implement the event list stuff
           #if 0 && (DISTRHO_PLUGIN_WANT_MIDI_INPUT || DISTRHO_PLUGIN_WANT_MIDI_OUTPUT)
            outDataSize = sizeof(SInt32);
            outWritable = false;
            return noErr;
           #else
            return kAudioUnitErr_InvalidProperty;
           #endif

        case 'DPFi':
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            outDataSize = sizeof(uint16_t);
            outWritable = false;
            return noErr;

        case 'DPFe':
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement < fParameterCount, inElement, kAudioUnitErr_InvalidElement);
            outDataSize = sizeof(uint8_t);
            outWritable = true;
            return noErr;

        case 'DPFp':
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement < fParameterCount, inElement, kAudioUnitErr_InvalidElement);
            outDataSize = sizeof(float);
            outWritable = true;
            return noErr;

       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        case 'DPFn':
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            outDataSize = sizeof(uint8_t) * 3;
            outWritable = true;
            return noErr;
       #endif

       #if DISTRHO_PLUGIN_WANT_PROGRAMS
        case 'DPFo':
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            outDataSize = sizeof(uint32_t);
            outWritable = false;
            return noErr;
       #endif

       #if DISTRHO_PLUGIN_WANT_STATE
        case 'DPFl':
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            outDataSize = sizeof(CFArrayRef);
            outWritable = false;
            return noErr;

        case 'DPFs':
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement < fStateCount, inElement, kAudioUnitErr_InvalidElement);
            outDataSize = sizeof(CFStringRef);
            outWritable = true;
            return noErr;
       #endif

       #if DISTRHO_PLUGIN_HAS_UI && DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
        case 'DPFa':
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            outDataSize = sizeof(void*);
            outWritable = false;
            return noErr;
       #endif

        // unwanted properties
        case kAudioUnitProperty_CPULoad:
        case kAudioUnitProperty_RenderContextObserver:
        case kAudioUnitProperty_AudioChannelLayout:
        case kAudioUnitProperty_TailTime:
        case kAudioUnitProperty_SupportedChannelLayoutTags:
        case kMusicDeviceProperty_DualSchedulingMode:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            return kAudioUnitErr_InvalidProperty;
        }

        d_stdout("TODO GetPropertyInfo(%d:%x:%s, %d:%s, %d, ...)",
                 inProp, inProp, AudioUnitPropertyID2Str(inProp), inScope, AudioUnitScope2Str(inScope), inElement);
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
            *static_cast<CFPropertyListRef*>(outData) = retrieveClassInfo();
            return noErr;

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
                    info->unit = kAudioUnitParameterUnit_Boolean;

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

       #if 0
        case kAudioUnitProperty_FastDispatch:
            switch (inElement)
            {
			case kAudioUnitGetParameterSelect:
                *static_cast<AudioUnitGetParameterProc*>(outData) = FastDispatchGetParameter;
                return noErr;
			case kAudioUnitSetParameterSelect:
                *static_cast<AudioUnitSetParameterProc*>(outData) = FastDispatchSetParameter;
                return noErr;
			case kAudioUnitRenderSelect:
                *static_cast<AudioUnitRenderProc*>(outData) = FastDispatchRender;
                return noErr;
            }
            d_stdout("WIP FastDispatch(%d:%x:%s)", inElement, inElement, AudioUnitPropertyID2Str(inElement));
            return kAudioUnitErr_InvalidElement;
       #endif

        case kAudioUnitProperty_StreamFormat:
            {
                AudioStreamBasicDescription* const desc = static_cast<AudioStreamBasicDescription*>(outData);
                std::memset(desc, 0, sizeof(*desc));

                if (inElement != 0)
                    return kAudioUnitErr_InvalidElement;

               #if DISTRHO_PLUGIN_NUM_INPUTS != 0
                if (inScope == kAudioUnitScope_Input)
                {
                   #ifdef DISTRHO_PLUGIN_EXTRA_IO
                    desc->mChannelsPerFrame = fNumInputs;
                   #else
                    desc->mChannelsPerFrame = DISTRHO_PLUGIN_NUM_INPUTS;
                   #endif
                }
                else
               #endif
               #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
                if (inScope == kAudioUnitScope_Output)
                {
                   #ifdef DISTRHO_PLUGIN_EXTRA_IO
                    desc->mChannelsPerFrame = fNumOutputs;
                   #else
                    desc->mChannelsPerFrame = DISTRHO_PLUGIN_NUM_OUTPUTS;
                   #endif
                }
                else
               #endif
                {
                    return kAudioUnitErr_InvalidScope;
                }

                desc->mFormatID         = kAudioFormatLinearPCM;
                desc->mFormatFlags      = kWantedAudioFormat;
                desc->mSampleRate       = fPlugin.getSampleRate();
                desc->mBitsPerChannel   = 32;
                desc->mBytesPerFrame    = sizeof(float);
                desc->mBytesPerPacket   = sizeof(float);
                desc->mFramesPerPacket  = 1;
            }
            return noErr;

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
            std::memcpy(outData, kChannelInfo, sizeof(kChannelInfo));
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

       #if DISTRHO_PLUGIN_NUM_INPUTS != 0
        case kAudioUnitProperty_SetRenderCallback:
            std::memcpy(outData, &fInputRenderCallback, sizeof(AURenderCallbackStruct));
            return noErr;
       #endif

       #if DISTRHO_PLUGIN_WANT_PROGRAMS
        case kAudioUnitProperty_FactoryPresets:
            if (const CFMutableArrayRef presetsRef = CFArrayCreateMutable(nullptr, fProgramCount, nullptr))
            {
                for (uint32_t i=0; i<fProgramCount; ++i)
                    CFArrayAppendValue(presetsRef, &fFactoryPresetsData[i]);

                *static_cast<CFArrayRef*>(outData) = presetsRef;
                return noErr;
            }
            return kAudio_ParamError;
       #endif

       #if DISTRHO_PLUGIN_WANT_TIMEPOS
        case kAudioUnitProperty_HostCallbacks:
            std::memcpy(outData, &fHostCallbackInfo, sizeof(HostCallbackInfo));
            return noErr;
       #endif

       #if DISTRHO_PLUGIN_NUM_INPUTS != 0 && DISTRHO_PLUGIN_NUM_OUTPUTS != 0
        case kAudioUnitProperty_InPlaceProcessing:
            *static_cast<UInt32*>(outData) = 1;
            return noErr;
       #endif

        case kAudioUnitProperty_PresentPreset:
           #if DISTRHO_PLUGIN_WANT_PROGRAMS
            if (fCurrentProgram >= 0)
            {
                std::memcpy(outData, &fFactoryPresetsData[fCurrentProgram], sizeof(AUPreset));
            }
            else
           #endif
            {
                std::memcpy(outData, &fUserPresetData, sizeof(AUPreset));
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
                                                                            DISTRHO_PLUGIN_UNIQUE_ID,
                                                                            DISTRHO_PLUGIN_BRAND_ID));

                #undef MACRO_STR
                #undef MACRO_STR2
                #undef MACRO_STR3

                [bundlePathString release];
            }
            return noErr;
       #endif

       #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        case kAudioUnitProperty_MIDIOutputCallbackInfo:
            {
                CFStringRef refs[1] = { CFSTR("MIDI Output") };
                *static_cast<CFArrayRef*>(outData) = CFArrayCreate(nullptr,
                                                                   reinterpret_cast<const void**>(refs),
                                                                   1,
                                                                   &kCFTypeArrayCallBacks);
            }
            return noErr;
       #endif

       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT || DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        /* FIXME implement the event list stuff
        case kAudioUnitProperty_AudioUnitMIDIProtocol:
            *static_cast<SInt32*>(outData) = kMIDIProtocol_1_0;
            return noErr;
        */
       #endif

        case 'DPFp':
            *static_cast<float*>(outData) = fPlugin.getParameterValue(inElement);
            return noErr;

       #if DISTRHO_PLUGIN_WANT_PROGRAMS
        case 'DPFo':
            *static_cast<uint32_t*>(outData) = fLastFactoryProgram;
            return noErr;
       #endif

       #if DISTRHO_PLUGIN_WANT_STATE
        case 'DPFl':
            if (const CFMutableArrayRef keysRef = CFArrayCreateMutable(nullptr,
                                                                       fStateCount,
                                                                       &kCFTypeArrayCallBacks))
            {
                for (uint32_t i=0; i<fStateCount; ++i)
                {
                    const CFStringRef keyRef = CFStringCreateWithCString(nullptr,
                                                                         fPlugin.getStateKey(i),
                                                                         kCFStringEncodingASCII);
                    CFArrayAppendValue(keysRef, keyRef);
                    CFRelease(keyRef);
                }

                *static_cast<CFArrayRef*>(outData) = keysRef;
                return noErr;
            }
            return kAudio_ParamError;

        case 'DPFs':
            {
                const String& key(fPlugin.getStateKey(inElement));
               #if DISTRHO_PLUGIN_WANT_FULL_STATE
                fStateMap[key] = fPlugin.getStateValue(key);
               #endif

                *static_cast<CFStringRef*>(outData) = CFStringCreateWithCString(nullptr,
                                                                                fStateMap[key],
                                                                                kCFStringEncodingUTF8);
            }
            return noErr;
       #endif

       #if DISTRHO_PLUGIN_HAS_UI && DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
        case 'DPFa':
            *static_cast<void**>(outData) = fPlugin.getInstancePointer();
            return noErr;
       #endif
        }

        d_stdout("TODO GetProperty(%d:%x:%s, %d:%s, %d, ...)",
                 inProp, inProp, AudioUnitPropertyID2Str(inProp), inScope, AudioUnitScope2Str(inScope), inElement);
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
            {
                const CFPropertyListRef propList = *static_cast<const CFPropertyListRef*>(inData);
                DISTRHO_SAFE_ASSERT_RETURN(CFGetTypeID(propList) == CFDictionaryGetTypeID(), kAudioUnitErr_InvalidPropertyValue);

                restoreClassInfo(static_cast<CFDictionaryRef>(propList));
            }
            return noErr;

        case kAudioUnitProperty_MakeConnection:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Input, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(AudioUnitConnection), inDataSize, kAudioUnitErr_InvalidPropertyValue);
           #if DISTRHO_PLUGIN_NUM_INPUTS != 0
            {
                const AudioUnitConnection conn = *static_cast<const AudioUnitConnection*>(inData);

                if (conn.sourceAudioUnit == nullptr)
                {
                    fInputConnectionBus = 0;
                    fInputConnectionUnit = nullptr;
                    return noErr;
                }

                AudioStreamBasicDescription desc;
                std::memset(&desc, 0, sizeof(desc));
                UInt32 dataSize = sizeof(AudioStreamBasicDescription);
                if (AudioUnitGetProperty(conn.sourceAudioUnit,
                                         kAudioUnitProperty_StreamFormat,
                                         kAudioUnitScope_Output,
                                         conn.sourceOutputNumber, &desc, &dataSize) != noErr)
                    return kAudioUnitErr_InvalidPropertyValue;

                DISTRHO_SAFE_ASSERT_INT_RETURN(desc.mFormatID == kAudioFormatLinearPCM,
                                               desc.mFormatID, kAudioUnitErr_FormatNotSupported);
                DISTRHO_SAFE_ASSERT_INT_RETURN(desc.mBitsPerChannel == 32,
                                               desc.mBitsPerChannel, kAudioUnitErr_FormatNotSupported);
                DISTRHO_SAFE_ASSERT_INT_RETURN(desc.mBytesPerFrame == sizeof(float),
                                               desc.mBytesPerFrame, kAudioUnitErr_FormatNotSupported);
                DISTRHO_SAFE_ASSERT_INT_RETURN(desc.mBytesPerPacket == sizeof(float),
                                               desc.mBytesPerPacket, kAudioUnitErr_FormatNotSupported);
                DISTRHO_SAFE_ASSERT_INT_RETURN(desc.mFramesPerPacket == 1,
                                               desc.mFramesPerPacket, kAudioUnitErr_FormatNotSupported);
                DISTRHO_SAFE_ASSERT_INT_RETURN(desc.mFormatFlags == kWantedAudioFormat,
                                               desc.mFormatFlags, kAudioUnitErr_FormatNotSupported);
               #ifdef DISTRHO_PLUGIN_EXTRA_IO
                DISTRHO_SAFE_ASSERT_UINT_RETURN(desc.mChannelsPerFrame == fNumInputs,
                                                desc.mChannelsPerFrame, kAudioUnitErr_FormatNotSupported);
               #else
                DISTRHO_SAFE_ASSERT_UINT_RETURN(desc.mChannelsPerFrame == DISTRHO_PLUGIN_NUM_INPUTS,
                                                desc.mChannelsPerFrame, kAudioUnitErr_FormatNotSupported);
               #endif

                fInputConnectionBus = conn.sourceOutputNumber;
                fInputConnectionUnit = conn.sourceAudioUnit;
            }
            return noErr;
           #else
            return kAudioUnitErr_PropertyNotInUse;
           #endif

        case kAudioUnitProperty_SampleRate:
           #if DISTRHO_PLUGIN_NUM_INPUTS != 0 && DISTRHO_PLUGIN_NUM_OUTPUTS != 0
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Input || inScope == kAudioUnitScope_Output, inScope, kAudioUnitErr_InvalidScope);
           #elif DISTRHO_PLUGIN_NUM_OUTPUTS != 0
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Output, inScope, kAudioUnitErr_InvalidScope);
           #else
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Input, inScope, kAudioUnitErr_InvalidScope);
           #endif
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(Float64), inDataSize, kAudioUnitErr_InvalidPropertyValue);
            {
               #if DISTRHO_PLUGIN_NUM_INPUTS != 0 || DISTRHO_PLUGIN_NUM_OUTPUTS != 0
                const Float64 sampleRate = *static_cast<const Float64*>(inData);
               #endif

               #if DISTRHO_PLUGIN_NUM_INPUTS != 0
                if (inScope == kAudioUnitScope_Input)
                {
                    if (d_isNotEqual(fSampleRateForInput, sampleRate))
                    {
                        fSampleRateForInput = sampleRate;
                        d_nextSampleRate = sampleRate;

                       #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
                        if (d_isEqual(fSampleRateForOutput, sampleRate))
                       #endif
                        {
                            fPlugin.setSampleRate(sampleRate, true);
                        }

                        notifyPropertyListeners(inProp, inScope, inElement);
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
                        d_nextSampleRate = sampleRate;

                       #if DISTRHO_PLUGIN_NUM_INPUTS != 0
                        if (d_isEqual(fSampleRateForInput, sampleRate))
                       #endif
                        {
                            fPlugin.setSampleRate(sampleRate, true);
                        }

                        notifyPropertyListeners(inProp, inScope, inElement);
                    }
                    return noErr;
                }
               #endif
            }
            return kAudioUnitErr_PropertyNotInUse;

        case kAudioUnitProperty_StreamFormat:
           #if DISTRHO_PLUGIN_NUM_INPUTS != 0 && DISTRHO_PLUGIN_NUM_OUTPUTS != 0
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Input || inScope == kAudioUnitScope_Output, inScope, kAudioUnitErr_InvalidScope);
           #elif DISTRHO_PLUGIN_NUM_OUTPUTS != 0
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Output, inScope, kAudioUnitErr_InvalidScope);
           #else
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Input, inScope, kAudioUnitErr_InvalidScope);
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
                if (desc->mFormatFlags != kWantedAudioFormat)
                    return kAudioUnitErr_FormatNotSupported;

               #ifndef DISTRHO_PLUGIN_EXTRA_IO
                if (desc->mChannelsPerFrame != (inScope == kAudioUnitScope_Input ? DISTRHO_PLUGIN_NUM_INPUTS
                                                                                 : DISTRHO_PLUGIN_NUM_OUTPUTS))
                    return kAudioUnitErr_FormatNotSupported;
               #endif

               #if DISTRHO_PLUGIN_NUM_INPUTS != 0
                if (inScope == kAudioUnitScope_Input)
                {
                    bool changed = false;

                   #ifdef DISTRHO_PLUGIN_EXTRA_IO
                    if (! isInputNumChannelsValid(desc->mChannelsPerFrame))
                        return kAudioUnitErr_FormatNotSupported;

                    if (fNumInputs != desc->mChannelsPerFrame)
                    {
                        changed = true;
                        fNumInputs = desc->mChannelsPerFrame;

                       #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
                        if (isNumChannelsComboValid(fNumInputs, fNumOutputs))
                       #endif
                        {
                            fPlugin.setAudioPortIO(fNumInputs, fNumOutputs);
                        }
                    }
                   #endif

                    if (d_isNotEqual(fSampleRateForInput, desc->mSampleRate))
                    {
                        changed = true;
                        fSampleRateForInput = desc->mSampleRate;

                       #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
                        if (d_isEqual(fSampleRateForOutput, desc->mSampleRate))
                       #endif
                        {
                            fPlugin.setSampleRate(desc->mSampleRate, true);
                        }

                        notifyPropertyListeners(kAudioUnitProperty_SampleRate, inScope, inElement);
                    }

                    if (changed)
                        notifyPropertyListeners(inProp, inScope, inElement);

                    return noErr;
                }
               #endif

               #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
                if (inScope == kAudioUnitScope_Output)
                {
                    bool changed = false;

                   #ifdef DISTRHO_PLUGIN_EXTRA_IO
                    if (! isOutputNumChannelsValid(desc->mChannelsPerFrame))
                        return kAudioUnitErr_FormatNotSupported;

                    if (fNumOutputs != desc->mChannelsPerFrame)
                    {
                        changed = true;
                        fNumOutputs = desc->mChannelsPerFrame;

                       #if DISTRHO_PLUGIN_NUM_INPUTS != 0
                        if (isNumChannelsComboValid(fNumInputs, fNumOutputs))
                       #endif
                        {
                            fPlugin.setAudioPortIO(fNumInputs, fNumOutputs);
                        }
                    }
                   #endif

                    if (d_isNotEqual(fSampleRateForOutput, desc->mSampleRate))
                    {
                        changed = true;
                        fSampleRateForOutput = desc->mSampleRate;

                       #if DISTRHO_PLUGIN_NUM_INPUTS != 0
                        if (d_isEqual(fSampleRateForInput, desc->mSampleRate))
                       #endif
                        {
                            fPlugin.setSampleRate(desc->mSampleRate, true);
                        }

                        notifyPropertyListeners(kAudioUnitProperty_SampleRate, inScope, inElement);
                    }

                    if (changed)
                        notifyPropertyListeners(inProp, inScope, inElement);

                    return noErr;
                }
               #endif
            }
            return kAudioUnitErr_PropertyNotInUse;

        case kAudioUnitProperty_MaximumFramesPerSlice:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(UInt32), inDataSize, kAudioUnitErr_InvalidPropertyValue);
            {
                const UInt32 bufferSize = *static_cast<const UInt32*>(inData);

                if (fPlugin.setBufferSize(bufferSize, true))
                    notifyPropertyListeners(inProp, inScope, inElement);
            }
            return noErr;

        case kAudioUnitProperty_BypassEffect:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(UInt32), inDataSize, kAudioUnitErr_InvalidPropertyValue);
            DISTRHO_SAFE_ASSERT_RETURN(fBypassParameterIndex != UINT32_MAX, kAudioUnitErr_PropertyNotInUse);
            {
                const bool bypass = *static_cast<const UInt32*>(inData) != 0;

                if ((fLastParameterValues[fBypassParameterIndex] > 0.5f) != bypass)
                {
                    const float value = bypass ? 1.f : 0.f;
                    fLastParameterValues[fBypassParameterIndex] = value;
                    fPlugin.setParameterValue(fBypassParameterIndex, value);
	                notifyPropertyListeners(inProp, inScope, inElement);
                }
            }
            return noErr;

        case kAudioUnitProperty_SetRenderCallback:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Input, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(AURenderCallbackStruct), inDataSize, kAudioUnitErr_InvalidPropertyValue);
           #if DISTRHO_PLUGIN_NUM_INPUTS != 0
            std::memcpy(&fInputRenderCallback, inData, sizeof(AURenderCallbackStruct));
            return noErr;
           #else
            return kAudioUnitErr_PropertyNotInUse;
           #endif

        case kAudioUnitProperty_HostCallbacks:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
           #if DISTRHO_PLUGIN_WANT_TIMEPOS
            {
                const UInt32 usableDataSize = std::min(inDataSize, static_cast<UInt32>(sizeof(HostCallbackInfo)));
		        const bool changed = std::memcmp(&fHostCallbackInfo, inData, usableDataSize) != 0;

		        std::memcpy(&fHostCallbackInfo, inData, usableDataSize);

                if (sizeof(HostCallbackInfo) > usableDataSize)
		            std::memset(&fHostCallbackInfo + usableDataSize, 0, sizeof(HostCallbackInfo) - usableDataSize);

                if (changed)
                    notifyPropertyListeners(inProp, inScope, inElement);
            }
            return noErr;
           #else
            return kAudioUnitErr_PropertyNotInUse;
           #endif

        case kAudioUnitProperty_InPlaceProcessing:
            // nothing to do
            return noErr;

        case kAudioUnitProperty_PresentPreset:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(AUPreset), inDataSize, kAudioUnitErr_InvalidPropertyValue);
            {
                const int32_t presetNumber = static_cast<const AUPreset*>(inData)->presetNumber;

               #if DISTRHO_PLUGIN_WANT_PROGRAMS
                if (presetNumber >= 0)
                {
                    if (fCurrentProgram != presetNumber)
                    {
                        fCurrentProgram = presetNumber;
                        fLastFactoryProgram = presetNumber;
                        fPlugin.loadProgram(fLastFactoryProgram);
                        notifyPropertyListeners('DPFo', kAudioUnitScope_Global, 0);
                    }
                }
                else
                {
                    fCurrentProgram = presetNumber;
                    CFRelease(fUserPresetData.presetName);
                    std::memcpy(&fUserPresetData, inData, sizeof(AUPreset));
                }
               #else
                DISTRHO_SAFE_ASSERT_INT_RETURN(presetNumber < 0, presetNumber, kAudioUnitErr_InvalidPropertyValue);
                CFRelease(fUserPresetData.presetName);
                std::memcpy(&fUserPresetData, inData, sizeof(AUPreset));
               #endif
            }
            return noErr;

        case kAudioUnitProperty_MIDIOutputCallback:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(AUMIDIOutputCallbackStruct), inDataSize, kAudioUnitErr_InvalidPropertyValue);
           #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
            std::memcpy(&fMidiOutput, inData, sizeof(AUMIDIOutputCallbackStruct));
            return noErr;
           #else
            return kAudioUnitErr_PropertyNotInUse;
           #endif

        case 'DPFi':
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(uint16_t), inDataSize, kAudioUnitErr_InvalidPropertyValue);
            {
                const uint16_t magic = *static_cast<const uint16_t*>(inData);

                if (magic != 1337)
                    return noErr;

               #if DISTRHO_PLUGIN_WANT_PROGRAMS
                notifyPropertyListeners('DPFo', kAudioUnitScope_Global, 0);
               #endif

               #if DISTRHO_PLUGIN_WANT_STATE
                for (uint32_t i=0; i<fStateCount; ++i)
                    notifyPropertyListeners('DPFs', kAudioUnitScope_Global, i);
               #endif

                for (uint32_t i=0; i<fParameterCount; ++i)
                    notifyPropertyListeners('DPFp', kAudioUnitScope_Global, i);
            }
            return noErr;

        case 'DPFe':
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement < fParameterCount, inElement, kAudioUnitErr_InvalidElement);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(uint8_t), inDataSize, kAudioUnitErr_InvalidPropertyValue);
            {
                const uint8_t flag = *static_cast<const uint8_t*>(inData);

                if (flag != 1 && flag != 2)
                    return noErr;

                AudioUnitEvent event;
                std::memset(&event, 0, sizeof(event));

                event.mEventType                        = flag == 1 ? kAudioUnitEvent_BeginParameterChangeGesture
                                                                    : kAudioUnitEvent_EndParameterChangeGesture;
                event.mArgument.mParameter.mAudioUnit   = fComponent;
                event.mArgument.mParameter.mParameterID = inElement;
                event.mArgument.mParameter.mScope       = kAudioUnitScope_Global;
                AUEventListenerNotify(NULL, NULL, &event);
            }
            return noErr;

        case 'DPFp':
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement < fParameterCount, inElement, kAudioUnitErr_InvalidElement);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(float), inDataSize, kAudioUnitErr_InvalidPropertyValue);
            {
                const float value = *static_cast<const float*>(inData);
                DISTRHO_SAFE_ASSERT_RETURN(std::isfinite(value), kAudioUnitErr_InvalidParameterValue);

                if (d_isEqual(fLastParameterValues[inElement], value))
                    return noErr;

                fLastParameterValues[inElement] = value;
                fPlugin.setParameterValue(inElement, value);

                AudioUnitEvent event;
                std::memset(&event, 0, sizeof(event));

                event.mEventType                        = kAudioUnitEvent_ParameterValueChange;
                event.mArgument.mParameter.mAudioUnit   = fComponent;
                event.mArgument.mParameter.mParameterID = inElement;
                event.mArgument.mParameter.mScope       = kAudioUnitScope_Global;
                AUEventListenerNotify(NULL, NULL, &event);

                if (fBypassParameterIndex == inElement)
	                notifyPropertyListeners(kAudioUnitProperty_BypassEffect, kAudioUnitScope_Global, 0);
            }
            return noErr;

        case 'DPFn':
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement == 0, inElement, kAudioUnitErr_InvalidElement);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(uint8_t) * 3, inDataSize, kAudioUnitErr_InvalidPropertyValue);
           #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
            {
                const uint8_t* const midiData = static_cast<const uint8_t*>(inData);

                if (midiData[0] == 0)
                    return noErr;

                fNotesRingBuffer.writeCustomData(midiData, 3);
                fNotesRingBuffer.commitWrite();
            }
            return noErr;
           #else
            return kAudioUnitErr_PropertyNotInUse;
           #endif

        case 'DPFs':
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inScope == kAudioUnitScope_Global, inScope, kAudioUnitErr_InvalidScope);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inDataSize == sizeof(CFStringRef), inDataSize, kAudioUnitErr_InvalidPropertyValue);
           #if DISTRHO_PLUGIN_WANT_STATE
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inElement < fStateCount, inElement, kAudioUnitErr_InvalidElement);
            {
                const CFStringRef valueRef = *static_cast<const CFStringRef*>(inData);
                DISTRHO_SAFE_ASSERT_RETURN(valueRef != nullptr && CFGetTypeID(valueRef) == CFStringGetTypeID(),
                                           kAudioUnitErr_InvalidPropertyValue);

                const CFIndex valueLen = CFStringGetLength(valueRef);
                char* const value = static_cast<char*>(std::malloc(valueLen + 1));
                DISTRHO_SAFE_ASSERT_RETURN(value != nullptr, kAudio_ParamError);
                DISTRHO_SAFE_ASSERT_RETURN(CFStringGetCString(valueRef, value, valueLen + 1, kCFStringEncodingUTF8),
                                           kAudioUnitErr_InvalidPropertyValue);

                const String& key(fPlugin.getStateKey(inElement));

                // save this key as needed
                if (fPlugin.wantStateKey(key))
                    fStateMap[key] = value;

                fPlugin.setState(key, value);

                std::free(value);
            }
            return noErr;
           #else
            return kAudioUnitErr_PropertyNotInUse;
           #endif

        // unwanted properties
        case kMusicDeviceProperty_DualSchedulingMode:
            return kAudioUnitErr_PropertyNotInUse;
        }

        d_stdout("TODO SetProperty(%d:%x:%s, %d:%s, %d, %p, %u)",
                 inProp, inProp, AudioUnitPropertyID2Str(inProp), inScope, AudioUnitScope2Str(inScope), inElement, inData, inDataSize);
        return kAudioUnitErr_InvalidProperty;
    }

    OSStatus auAddPropertyListener(const AudioUnitPropertyID prop,
                                   const AudioUnitPropertyListenerProc proc,
                                   void* const userData)
    {
        const PropertyListener pl = {
            prop, proc, userData
        };

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
        fUsingRenderListeners = true;

        const RenderListener rl = {
            proc, userData
        };

        fRenderListeners.push_back(rl);
        return noErr;
    }

    OSStatus auRemoveRenderNotify(const AURenderCallback proc, void* const userData)
    {
        for (RenderListeners::iterator it = fRenderListeners.begin(); it != fRenderListeners.end(); ++it)
        {
            const RenderListener& rl(*it);

            if (rl.proc == proc && rl.userData == userData)
            {
                fRenderListeners.erase(it);
                return auRemoveRenderNotify(proc, userData);
            }
        }

        if (fRenderListeners.empty())
            fUsingRenderListeners = false;

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
                            UInt32 /* bufferOffset */)
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
            notifyPropertyListeners('DPFp', kAudioUnitScope_Global, param);

            if (fBypassParameterIndex == elem)
                notifyPropertyListeners(kAudioUnitProperty_BypassEffect, kAudioUnitScope_Global, 0);
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

    OSStatus auReset(const AudioUnitScope scope, const AudioUnitElement elem)
    {
        DISTRHO_SAFE_ASSERT_UINT_RETURN(scope == kAudioUnitScope_Global || scope == kAudioUnitScope_Input || scope == kAudioUnitScope_Output, scope, kAudioUnitErr_InvalidScope);
        DISTRHO_SAFE_ASSERT_UINT_RETURN(elem == 0, elem, kAudioUnitErr_InvalidElement);

        if (fPlugin.isActive())
        {
            fPlugin.deactivate();
            fPlugin.activate();
        }

       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        fMidiEventCount = 0;
       #endif
       #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        fMidiOutputPackets.numPackets = 0;
       #endif
       #if DISTRHO_PLUGIN_WANT_TIMEPOS
        fTimePosition.clear();
        fTimePosition.bbt.ticksPerBeat = kDefaultTicksPerBeat;
       #endif
        return noErr;
    }

    OSStatus auRender(const AudioUnitRenderActionFlags actionFlags,
                      const AudioTimeStamp* const inTimeStamp,
                      const UInt32 inBusNumber,
                      const UInt32 inFramesToProcess,
                      AudioBufferList* const ioData)
    {
        if ((actionFlags & kAudioUnitRenderAction_DoNotCheckRenderArgs) == 0x0)
        {
            DISTRHO_SAFE_ASSERT_RETURN(fPlugin.isActive(), kAudio_ParamError);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(inBusNumber == 0, inBusNumber, kAudioUnitErr_InvalidElement);
           #if DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS != 0
            DISTRHO_SAFE_ASSERT_UINT_RETURN(ioData->mNumberBuffers == fAudioBufferList->mNumberBuffers,
                                            ioData->mNumberBuffers, kAudio_ParamError);
           #else
            DISTRHO_SAFE_ASSERT_UINT_RETURN(ioData->mNumberBuffers == 0, ioData->mNumberBuffers, kAudio_ParamError);
           #endif

            if (inFramesToProcess > fPlugin.getBufferSize())
            {
                setLastRenderError(kAudioUnitErr_TooManyFramesToProcess);
                return kAudioUnitErr_TooManyFramesToProcess;
            }

           #if DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS != 0
            for (uint16_t i = 0; i < ioData->mNumberBuffers; ++i)
            {
                if (ioData->mBuffers[i].mDataByteSize != sizeof(float) * inFramesToProcess)
                {
                    setLastRenderError(kAudio_ParamError);
                    return kAudio_ParamError;
                }
            }
           #endif
        }

      #if DISTRHO_PLUGIN_NUM_INPUTS != 0
       #ifdef DISTRHO_PLUGIN_EXTRA_IO
        const uint32_t numInputs = fNumInputs;
       #else
        constexpr const uint32_t numInputs = DISTRHO_PLUGIN_NUM_INPUTS;
       #endif
        const float* inputs[numInputs];
      #else
        constexpr const float** inputs = nullptr;
      #endif

      #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
       #ifdef DISTRHO_PLUGIN_EXTRA_IO
        const uint32_t numOutputs = fNumOutputs;
       #else
        constexpr const uint32_t numOutputs = DISTRHO_PLUGIN_NUM_OUTPUTS;
       #endif
        float* outputs[numOutputs];
      #else
        constexpr float** outputs = nullptr;
      #endif

       #if DISTRHO_PLUGIN_NUM_INPUTS != 0
        if (fInputConnectionUnit != nullptr)
        {
            AudioUnitRenderActionFlags ioActionFlags = 0;
            const OSStatus err = AudioUnitRender(fInputConnectionUnit,
                                                 &ioActionFlags,
                                                 inTimeStamp,
                                                 fInputConnectionBus,
                                                 inFramesToProcess,
                                                 fAudioBufferList);

            if (err != noErr)
            {
                setLastRenderError(err);
                return err;
            }

            for (uint16_t i = 0; i < numInputs; ++i)
                inputs[i] = static_cast<const float*>(fAudioBufferList->mBuffers[i].mData);

           #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
            for (uint16_t i = 0; i < numOutputs; ++i)
            {
                if (ioData->mBuffers[i].mData == nullptr)
                    ioData->mBuffers[i].mData = fAudioBufferList->mBuffers[i].mData;

                outputs[i] = static_cast<float*>(ioData->mBuffers[i].mData);
            }
           #endif
        }
        else if (fInputRenderCallback.inputProc != nullptr)
        {
            bool adjustDataByteSize, usingHostBuffer = true;
            UInt32 prevDataByteSize;

            for (uint16_t i = 0; i < ioData->mNumberBuffers; ++i)
            {
                if (ioData->mBuffers[i].mData == nullptr)
                {
                    usingHostBuffer = false;
                    ioData->mBuffers[i].mData = fAudioBufferList->mBuffers[i].mData;
                }
            }

            if (! usingHostBuffer)
            {
                prevDataByteSize = fAudioBufferList->mBuffers[0].mDataByteSize;
                adjustDataByteSize = prevDataByteSize != sizeof(float) * inFramesToProcess;

                if (adjustDataByteSize)
                {
                    for (uint16_t i = 0; i < ioData->mNumberBuffers; ++i)
                        fAudioBufferList->mBuffers[i].mDataByteSize = sizeof(float) * inFramesToProcess;
                }
            }
            else
            {
                adjustDataByteSize = false;
            }

            AudioUnitRenderActionFlags rActionFlags = 0;
            AudioBufferList* const rData = usingHostBuffer ? ioData : fAudioBufferList;
            const OSStatus err = fInputRenderCallback.inputProc(fInputRenderCallback.inputProcRefCon,
                                                                &rActionFlags,
                                                                inTimeStamp,
                                                                inBusNumber,
                                                                inFramesToProcess,
                                                                rData);

            if (err != noErr)
            {
                if (adjustDataByteSize)
                {
                    for (uint16_t i = 0; i < ioData->mNumberBuffers; ++i)
                        fAudioBufferList->mBuffers[i].mDataByteSize = prevDataByteSize;
                }

                setLastRenderError(err);
                return err;
            }

            if (usingHostBuffer)
            {
                for (uint16_t i = 0; i < numInputs; ++i)
                    inputs[i] = static_cast<const float*>(ioData->mBuffers[i].mData);

               #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
                for (uint16_t i = 0; i < numOutputs; ++i)
                    outputs[i] = static_cast<float*>(ioData->mBuffers[i].mData);
               #endif

            }
            else
            {
                for (uint16_t i = 0; i < numInputs; ++i)
                    inputs[i] = static_cast<const float*>(fAudioBufferList->mBuffers[i].mData);

               #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
                for (uint16_t i = 0; i < numOutputs; ++i)
                    outputs[i] = static_cast<float*>(ioData->mBuffers[i].mData);
               #endif
            }
        }
        else
       #endif // DISTRHO_PLUGIN_NUM_INPUTS != 0
        {
           #if DISTRHO_PLUGIN_NUM_INPUTS != 0
            for (uint16_t i = 0; i < numInputs; ++i)
            {
                if (ioData->mBuffers[i].mData == nullptr)
                {
                    ioData->mBuffers[i].mData = fAudioBufferList->mBuffers[i].mData;
                    std::memset(ioData->mBuffers[i].mData, 0, sizeof(float) * inFramesToProcess);
                }

                inputs[i] = static_cast<const float*>(ioData->mBuffers[i].mData);
            }
           #endif

           #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
            for (uint16_t i = 0; i < numOutputs; ++i)
            {
                if (ioData->mBuffers[i].mData == nullptr)
                    ioData->mBuffers[i].mData = fAudioBufferList->mBuffers[i].mData;

                outputs[i] = static_cast<float*>(ioData->mBuffers[i].mData);
            }
           #endif
        }

        if (fUsingRenderListeners)
        {
            AudioUnitRenderActionFlags ioActionFlags = actionFlags | kAudioUnitRenderAction_PreRender;
            notifyRenderListeners(&ioActionFlags, inTimeStamp, inBusNumber, inFramesToProcess, ioData);
        }

        run(inputs, outputs, inFramesToProcess, inTimeStamp);

        if (fUsingRenderListeners)
        {
            AudioUnitRenderActionFlags ioActionFlags = actionFlags | kAudioUnitRenderAction_PostRender;
            notifyRenderListeners(&ioActionFlags, inTimeStamp, inBusNumber, inFramesToProcess, ioData);
        }

        return noErr;
    }

   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    OSStatus auMIDIEvent(const UInt32 inStatus,
                         const UInt32 inData1,
                         const UInt32 inData2,
                         const UInt32 inOffsetSampleFrame)
    {
        if (fMidiEventCount >= kMaxMidiEvents)
            return noErr;

        MidiEvent& midiEvent(fMidiEvents[fMidiEventCount++]);
        midiEvent.frame   = inOffsetSampleFrame;
        midiEvent.data[0] = inStatus;
        midiEvent.data[1] = inData1;
        midiEvent.data[2] = inData2;

        switch (inStatus)
        {
        case 0x80:
        case 0x90:
        case 0xA0:
        case 0xB0:
        case 0xE0:
            midiEvent.size = 3;
            break;
        case 0xC0:
        case 0xD0:
            midiEvent.size = 2;
            break;
        case 0xF0:
            switch (inStatus & 0x0F)
            {
            case 0x0:
            case 0x4:
            case 0x5:
            case 0x7:
            case 0x9:
            case 0xD:
                // unsupported
                kAudioUnitErr_InvalidPropertyValue;
            case 0x1:
            case 0x2:
            case 0x3:
            case 0xE:
                midiEvent.size = 3;
                break;
            case 0x6:
            case 0x8:
            case 0xA:
            case 0xB:
            case 0xC:
            case 0xF:
                midiEvent.size = 1;
                break;
            }
            break;
        default:
            // invalid
            return kAudioUnitErr_InvalidPropertyValue;
        }

       #if DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS == 0
        // handle case of plugin having no working audio, simulate audio-side processing
        run(nullptr, nullptr, std::max(1u, inOffsetSampleFrame), nullptr);
       #endif

        return noErr;
    }

    OSStatus auSysEx(const UInt8* const inData, const UInt32 inLength)
    {
        if (fMidiEventCount >= kMaxMidiEvents)
            return noErr;

        MidiEvent& midiEvent(fMidiEvents[fMidiEventCount++]);
        midiEvent.frame = fMidiEventCount != 1 ? fMidiEvents[fMidiEventCount - 1].frame : 0;
        midiEvent.size  = inLength;

        // FIXME who owns inData ??
        if (inLength > MidiEvent::kDataSize)
        {
            std::memset(midiEvent.data, 0, MidiEvent::kDataSize);
            midiEvent.dataExt = inData;
        }
        else
        {
            std::memcpy(midiEvent.data, inData, inLength);
        }

       #if DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS == 0
        // handle case of plugin having no working audio, simulate audio-side processing
        run(nullptr, nullptr, 1, nullptr);
       #endif

        return noErr;
    }
   #endif

    // ----------------------------------------------------------------------------------------------------------------

private:
    PluginExporter fPlugin;

    // AU component
    const AudioUnit fComponent;

    // AUv2 related fields
    OSStatus fLastRenderError;
    PropertyListeners fPropertyListeners;
    RenderListeners fRenderListeners;
  #if DISTRHO_PLUGIN_NUM_INPUTS != 0
    UInt32 fInputConnectionBus;
    AudioUnit fInputConnectionUnit;
    AURenderCallbackStruct fInputRenderCallback;
    Float64 fSampleRateForInput;
   #ifdef DISTRHO_PLUGIN_EXTRA_IO
    uint32_t fNumInputs;
   #endif
  #endif
  #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
    Float64 fSampleRateForOutput;
   #ifdef DISTRHO_PLUGIN_EXTRA_IO
    uint32_t fNumOutputs;
   #endif
  #endif
   #if DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS != 0
    AudioBufferList* fAudioBufferList;
   #endif
    bool fUsingRenderListeners;

    // Caching
    const uint32_t fParameterCount;
    float* fLastParameterValues;
    uint32_t fBypassParameterIndex;

   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    uint32_t fMidiEventCount;
    MidiEvent fMidiEvents[kMaxMidiEvents];
    SmallStackRingBuffer fNotesRingBuffer;
   #endif

   #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    AUMIDIOutputCallbackStruct fMidiOutput;
    d_MIDIPacketList fMidiOutputPackets;
   #endif

   #if DISTRHO_PLUGIN_WANT_PROGRAMS
    int32_t fCurrentProgram;
    uint32_t fLastFactoryProgram;
    uint32_t fProgramCount;
    AUPreset* fFactoryPresetsData;
   #endif
    AUPreset fUserPresetData;

   #if DISTRHO_PLUGIN_WANT_STATE
    const uint32_t fStateCount;
    StringMap fStateMap;
   #endif

   #if DISTRHO_PLUGIN_WANT_TIMEPOS
    HostCallbackInfo fHostCallbackInfo;
    TimePosition fTimePosition;
   #endif

    // ----------------------------------------------------------------------------------------------------------------

    void notifyPropertyListeners(const AudioUnitPropertyID prop, const AudioUnitScope scope, const AudioUnitElement elem)
    {
        for (PropertyListeners::iterator it = fPropertyListeners.begin(); it != fPropertyListeners.end(); ++it)
        {
            const PropertyListener& pl(*it);

            if (pl.prop == prop)
			    pl.proc(pl.userData, fComponent, prop, scope, elem);
        }
    }

    void notifyRenderListeners(AudioUnitRenderActionFlags* const ioActionFlags,
                               const AudioTimeStamp* const inTimeStamp,
                               const UInt32 inBusNumber,
                               const UInt32 inNumberFrames,
                               AudioBufferList* const ioData)
    {
        for (RenderListeners::iterator it = fRenderListeners.begin(); it != fRenderListeners.end(); ++it)
        {
            const RenderListener& rl(*it);

            rl.proc(rl.userData, ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, ioData);
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

    void run(const float** inputs, float** outputs, const uint32_t frames, const AudioTimeStamp* const inTimeStamp)
    {
       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if (fMidiEventCount != kMaxMidiEvents && fNotesRingBuffer.isDataAvailableForReading())
        {
            uint8_t midiData[3];
            const uint32_t frame = fMidiEventCount != 0 ? fMidiEvents[fMidiEventCount - 1].frame : 0;

            while (fNotesRingBuffer.isDataAvailableForReading())
            {
                if (! fNotesRingBuffer.readCustomData(midiData, 3))
                    break;

                MidiEvent& midiEvent(fMidiEvents[fMidiEventCount++]);
                midiEvent.frame = frame;
                midiEvent.size  = 3;
                std::memcpy(midiEvent.data, midiData, 3);

                if (fMidiEventCount == kMaxMidiEvents)
                    break;
            }
        }
       #endif

       #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        fMidiOutputPackets.numPackets = 0;
       #endif

       #if DISTRHO_PLUGIN_WANT_TIMEPOS
        if (fHostCallbackInfo.beatAndTempoProc != nullptr ||
            fHostCallbackInfo.musicalTimeLocationProc != nullptr ||
            fHostCallbackInfo.transportStateProc != nullptr)
        {
            // reused values so we can check null and return value together
            Boolean b1 = false;
            Float32 f1 = 4.f; // initial value for beats per bar
            Float64 g1 = 0.0;
            Float64 g2 = 0.0;
            UInt32 u1 = 0;

            if (fHostCallbackInfo.musicalTimeLocationProc != nullptr
                && fHostCallbackInfo.musicalTimeLocationProc(fHostCallbackInfo.hostUserData,
                                                             nullptr, &f1, &u1, nullptr) == noErr)
            {
                fTimePosition.bbt.beatsPerBar = f1;
                fTimePosition.bbt.beatType    = u1;
            }
            else
            {
                fTimePosition.bbt.beatsPerBar = 4.f;
                fTimePosition.bbt.beatType    = 4.f;
            }

            if (fHostCallbackInfo.beatAndTempoProc != nullptr
                && fHostCallbackInfo.beatAndTempoProc(fHostCallbackInfo.hostUserData, &g1, &g2) == noErr)
            {
                const double beat = static_cast<int32_t>(g1);

                fTimePosition.bbt.valid = true;
                fTimePosition.bbt.bar   = static_cast<int32_t>(beat / f1) + 1;
                fTimePosition.bbt.beat  = static_cast<int32_t>(std::fmod(beat, f1)) + 1;
                fTimePosition.bbt.tick  = std::fmod(g1, 1.0) * 1920.0;
                fTimePosition.bbt.beatsPerMinute = g2;
            }
            else
            {
                fTimePosition.bbt.valid = false;
                fTimePosition.bbt.bar   = 1;
                fTimePosition.bbt.beat  = 1;
                fTimePosition.bbt.tick  = 0.0;
                fTimePosition.bbt.beatsPerMinute = 120.0;
            }

            if (fHostCallbackInfo.transportStateProc != nullptr
                && fHostCallbackInfo.transportStateProc(fHostCallbackInfo.hostUserData,
                                                        &b1, nullptr, &g1, nullptr, nullptr, nullptr) == noErr)
            {
                fTimePosition.playing = b1;
                fTimePosition.frame = static_cast<int64_t>(g1);
            }
            else
            {
                fTimePosition.playing = false;
                fTimePosition.frame = 0;
            }

            fTimePosition.bbt.barStartTick = kDefaultTicksPerBeat *
                                             fTimePosition.bbt.beatsPerBar *
                                             (fTimePosition.bbt.bar - 1);

            fPlugin.setTimePosition(fTimePosition);
        }
       #endif

       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        fPlugin.run(inputs, outputs, frames, fMidiEvents, fMidiEventCount);
        fMidiEventCount = 0;
       #else
        fPlugin.run(inputs, outputs, frames);
       #endif

       #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        if (fMidiOutputPackets.numPackets != 0 && fMidiOutput.midiOutputCallback != nullptr)
        {
            fMidiOutput.midiOutputCallback(fMidiOutput.userData,
                                           inTimeStamp,
                                           0,
                                           reinterpret_cast<const ::MIDIPacketList*>(&fMidiOutputPackets));
        }
       #else
        // unused
        (void)inTimeStamp;
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
                notifyPropertyListeners('DPFp', kAudioUnitScope_Global, i);
            }
        }
    }

    void setLastRenderError(const OSStatus err)
    {
        if (fLastRenderError != noErr)
            return;

        fLastRenderError = err;
        notifyPropertyListeners(kAudioUnitProperty_LastRenderError, kAudioUnitScope_Global, 0);
    }

    // ----------------------------------------------------------------------------------------------------------------

   #if DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS != 0
    bool reallocAudioBufferList(const bool alloc)
    {
        if (fAudioBufferList != nullptr)
        {
            for (uint16_t i = 0; i < fAudioBufferList->mNumberBuffers; ++i)
                delete[] static_cast<float*>(fAudioBufferList->mBuffers[i].mData);
        }

      #ifdef DISTRHO_PLUGIN_EXTRA_IO
       #if DISTRHO_PLUGIN_NUM_INPUTS != 0 && DISTRHO_PLUGIN_NUM_OUTPUTS != 0
        const uint16_t numBuffers = std::max(fNumInputs, fNumOutputs);
       #elif DISTRHO_PLUGIN_NUM_INPUTS != 0
        const uint16_t numBuffers = fNumInputs;
       #else
        const uint16_t numBuffers = fNumOutputs;
       #endif
      #else
        constexpr const uint16_t numBuffers = d_max(DISTRHO_PLUGIN_NUM_INPUTS, DISTRHO_PLUGIN_NUM_OUTPUTS);
      #endif
        const uint32_t bufferSize = fPlugin.getBufferSize();

        if (! alloc)
        {
            std::free(fAudioBufferList);
            fAudioBufferList = nullptr;
            return true;
        }

        if (AudioBufferList* const abl = static_cast<AudioBufferList*>(
            std::realloc(fAudioBufferList, sizeof(uint32_t) + sizeof(AudioBuffer) * numBuffers)))
        {
            abl->mNumberBuffers = numBuffers;

            for (uint16_t i = 0; i < numBuffers; ++i)
            {
                abl->mBuffers[i].mNumberChannels = 1;
                abl->mBuffers[i].mData = new float[bufferSize];
                abl->mBuffers[i].mDataByteSize = sizeof(float) * bufferSize;
            }

            fAudioBufferList = abl;
            return true;
        }

        std::free(fAudioBufferList);
        fAudioBufferList = nullptr;
        return false;
    }
   #endif

    // ----------------------------------------------------------------------------------------------------------------

    CFMutableDictionaryRef retrieveClassInfo()
    {
        CFMutableDictionaryRef clsInfo = CFDictionaryCreateMutable(nullptr,
                                                                   0,
                                                                   &kCFTypeDictionaryKeyCallBacks,
                                                                   &kCFTypeDictionaryValueCallBacks);
        SInt32 value;

        value = 0;
        if (const CFNumberRef num = CFNumberCreate(nullptr, kCFNumberSInt32Type, &value))
        {
            CFDictionarySetValue(clsInfo, CFSTR(kAUPresetVersionKey), num);
            CFRelease(num);
        }

        value = kType;
        if (const CFNumberRef num = CFNumberCreate(nullptr, kCFNumberSInt32Type, &value))
        {
            CFDictionarySetValue(clsInfo, CFSTR(kAUPresetTypeKey), num);
            CFRelease(num);
        }

        value = kSubType;
        if (const CFNumberRef num = CFNumberCreate(nullptr, kCFNumberSInt32Type, &value))
        {
            CFDictionarySetValue(clsInfo, CFSTR(kAUPresetSubtypeKey), num);
            CFRelease(num);
        }

        value = kManufacturer;
        if (const CFNumberRef num = CFNumberCreate(nullptr, kCFNumberSInt32Type, &value))
        {
            CFDictionarySetValue(clsInfo, CFSTR(kAUPresetManufacturerKey), num);
            CFRelease(num);
        }

       #if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (fCurrentProgram >= 0)
        {
            CFDictionarySetValue(clsInfo, CFSTR(kAUPresetNameKey), fFactoryPresetsData[fCurrentProgram].presetName);
        }
        else
       #endif
        {
            CFDictionarySetValue(clsInfo, CFSTR(kAUPresetNameKey), fUserPresetData.presetName);
        }

        if (const CFMutableDictionaryRef data = CFDictionaryCreateMutable(nullptr,
                                                                          0,
                                                                          &kCFTypeDictionaryKeyCallBacks,
                                                                          &kCFTypeDictionaryValueCallBacks))
        {
           #if DISTRHO_PLUGIN_WANT_PROGRAMS
            const SInt32 program = fCurrentProgram;
            if (const CFNumberRef programRef = CFNumberCreate(nullptr, kCFNumberSInt32Type, &program))
            {
               CFDictionarySetValue(data, CFSTR("program"), programRef);
               CFRelease(programRef);
            }
           #endif

           #if DISTRHO_PLUGIN_WANT_FULL_STATE
            // Update current state
            for (StringMap::const_iterator cit=fStateMap.begin(), cite=fStateMap.end(); cit != cite; ++cit)
            {
                const String& key(cit->first);
                fStateMap[key] = fPlugin.getStateValue(key);
            }
           #endif

           #if DISTRHO_PLUGIN_WANT_STATE
            if (const CFMutableArrayRef statesRef = CFArrayCreateMutable(nullptr,
                                                                         fStateCount,
                                                                         &kCFTypeArrayCallBacks))
            {
                for (StringMap::const_iterator cit=fStateMap.begin(), cite=fStateMap.end(); cit != cite; ++cit)
                {
                    const String& key(cit->first);
                    const String& value(cit->second);

                   #if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS && ! DISTRHO_PLUGIN_HAS_UI
                    bool wantStateKey = true;

                    for (uint32_t i=0; i<fStateCount; ++i)
                    {
                        if (fPlugin.getStateKey(i) == key)
                        {
                            if (fPlugin.getStateHints(i) & kStateIsOnlyForUI)
                                wantStateKey = false;

                            break;
                        }
                    }

                    if (! wantStateKey)
                        continue;
                   #endif

                    CFStringRef keyRef = CFStringCreateWithCString(nullptr, key, kCFStringEncodingASCII);
                    CFStringRef valueRef = CFStringCreateWithCString(nullptr, value, kCFStringEncodingUTF8);

                    if (CFDictionaryRef dictRef = CFDictionaryCreate(nullptr,
                                                                     reinterpret_cast<const void**>(&keyRef),
                                                                     reinterpret_cast<const void**>(&valueRef),
                                                                     1,
                                                                     &kCFTypeDictionaryKeyCallBacks,
                                                                     &kCFTypeDictionaryValueCallBacks))
                    {
                        CFArrayAppendValue(statesRef, dictRef);
                        CFRelease(dictRef);
                    }

                    CFRelease(keyRef);
                    CFRelease(valueRef);
                }

                CFDictionarySetValue(data, CFSTR("states"), statesRef);
                CFRelease(statesRef);
            }
           #endif

            if (const CFMutableArrayRef paramsRef = CFArrayCreateMutable(nullptr,
                                                                         fParameterCount,
                                                                         &kCFTypeArrayCallBacks))
            {
                for (uint32_t i=0; i<fParameterCount; ++i)
                {
                    if (fPlugin.isParameterOutputOrTrigger(i))
                        continue;

                    const float value = fPlugin.getParameterValue(i);

                    CFStringRef keyRef = CFStringCreateWithCString(nullptr,
                                                                   fPlugin.getParameterSymbol(i),
                                                                   kCFStringEncodingASCII);
                    CFNumberRef valueRef = CFNumberCreate(nullptr, kCFNumberFloat32Type, &value);

                    if (CFDictionaryRef dictRef = CFDictionaryCreate(nullptr,
                                                                     reinterpret_cast<const void**>(&keyRef),
                                                                     reinterpret_cast<const void**>(&valueRef),
                                                                     1,
                                                                     &kCFTypeDictionaryKeyCallBacks,
                                                                     &kCFTypeDictionaryValueCallBacks))
                    {
                        CFArrayAppendValue(paramsRef, dictRef);
                        CFRelease(dictRef);
                    }

                    CFRelease(keyRef);
                    CFRelease(valueRef);
                }

                CFDictionarySetValue(data, CFSTR("params"), paramsRef);
                CFRelease(paramsRef);
            }

            CFDictionarySetValue(clsInfo, CFSTR(kAUPresetDataKey), data);
            CFRelease(data);
        }

        return clsInfo;
    }

    void restoreClassInfo(const CFDictionaryRef clsInfo)
    {
        CFDictionaryRef data = nullptr;
        DISTRHO_SAFE_ASSERT_RETURN(CFDictionaryGetValueIfPresent(clsInfo,
                                                                 CFSTR(kAUPresetDataKey),
                                                                 reinterpret_cast<const void**>(&data)),);
        DISTRHO_SAFE_ASSERT_RETURN(CFGetTypeID(data) == CFDictionaryGetTypeID(),);

       #if DISTRHO_PLUGIN_WANT_PROGRAMS
        CFNumberRef programRef = nullptr;
        if (CFDictionaryGetValueIfPresent(data, CFSTR("program"), reinterpret_cast<const void**>(&programRef))
            && CFGetTypeID(programRef) == CFNumberGetTypeID())
        {
            SInt32 program = -1;
            if (CFNumberGetValue(programRef, kCFNumberSInt32Type, &program))
            {
                fCurrentProgram = program;

                if (program >= 0)
                {
                    fLastFactoryProgram = program;
                    fPlugin.loadProgram(fLastFactoryProgram);
                    notifyPropertyListeners('DPFo', kAudioUnitScope_Global, 0);
                }

                notifyPropertyListeners(kAudioUnitProperty_PresentPreset, kAudioUnitScope_Global, 0);
            }
        }
       #endif

       #if DISTRHO_PLUGIN_WANT_STATE
        CFArrayRef statesRef = nullptr;
        if (CFDictionaryGetValueIfPresent(data, CFSTR("states"), reinterpret_cast<const void**>(&statesRef))
            && CFGetTypeID(statesRef) == CFArrayGetTypeID())
        {
            const CFIndex numStates = CFArrayGetCount(statesRef);
            char* key = nullptr;
            char* value = nullptr;
            CFIndex keyLen = -1;
            CFIndex valueLen = -1;

            for (CFIndex i=0; i<numStates; ++i)
            {
                const CFDictionaryRef state = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(statesRef, i));
                DISTRHO_SAFE_ASSERT_BREAK(CFGetTypeID(state) == CFDictionaryGetTypeID());
                DISTRHO_SAFE_ASSERT_BREAK(CFDictionaryGetCount(state) == 1);

                CFStringRef keyRef = nullptr;
                CFStringRef valueRef = nullptr;
                CFDictionaryGetKeysAndValues(state,
                                             reinterpret_cast<const void**>(&keyRef),
                                             reinterpret_cast<const void**>(&valueRef));
                DISTRHO_SAFE_ASSERT_BREAK(keyRef != nullptr && CFGetTypeID(keyRef) == CFStringGetTypeID());
                DISTRHO_SAFE_ASSERT_BREAK(valueRef != nullptr && CFGetTypeID(valueRef) == CFStringGetTypeID());

                const CFIndex keyRefLen = CFStringGetLength(keyRef);
                if (keyLen < keyRefLen)
                {
                    keyLen = keyRefLen;
                    key = static_cast<char*>(std::realloc(key, keyLen + 1));
                }
                DISTRHO_SAFE_ASSERT_BREAK(CFStringGetCString(keyRef, key, keyLen + 1, kCFStringEncodingASCII));

                if (! fPlugin.wantStateKey(key))
                    continue;

                const CFIndex valueRefLen = CFStringGetLength(valueRef);
                if (valueLen < valueRefLen)
                {
                    valueLen = valueRefLen;
                    value = static_cast<char*>(std::realloc(value, valueLen + 1));
                }
                DISTRHO_SAFE_ASSERT_BREAK(CFStringGetCString(valueRef, value, valueLen + 1, kCFStringEncodingUTF8));

                const String dkey(key);
                fStateMap[dkey] = value;
                fPlugin.setState(key, value);

                for (uint32_t j=0; j<fStateCount; ++j)
                {
                    if (fPlugin.getStateKey(j) == key)
                    {
                        if ((fPlugin.getStateHints(i) & kStateIsOnlyForDSP) == 0x0)
                            notifyPropertyListeners('DPFs', kAudioUnitScope_Global, j);

                        break;
                    }
                }
            }

            std::free(key);
            std::free(value);
        }
       #endif

        CFArrayRef paramsRef = nullptr;
        if (CFDictionaryGetValueIfPresent(data, CFSTR("params"), reinterpret_cast<const void**>(&paramsRef))
            && CFGetTypeID(paramsRef) == CFArrayGetTypeID())
        {
            const CFIndex numParams = CFArrayGetCount(paramsRef);
            char* symbol = nullptr;
            CFIndex symbolLen = -1;

            for (CFIndex i=0; i<numParams; ++i)
            {
                const CFDictionaryRef param = static_cast<CFDictionaryRef>(CFArrayGetValueAtIndex(paramsRef, i));
                DISTRHO_SAFE_ASSERT_BREAK(CFGetTypeID(param) == CFDictionaryGetTypeID());
                DISTRHO_SAFE_ASSERT_BREAK(CFDictionaryGetCount(param) == 1);

                CFStringRef keyRef = nullptr;
                CFNumberRef valueRef = nullptr;
                CFDictionaryGetKeysAndValues(param,
                                             reinterpret_cast<const void**>(&keyRef),
                                             reinterpret_cast<const void**>(&valueRef));
                DISTRHO_SAFE_ASSERT_BREAK(keyRef != nullptr && CFGetTypeID(keyRef) == CFStringGetTypeID());
                DISTRHO_SAFE_ASSERT_BREAK(valueRef != nullptr && CFGetTypeID(valueRef) == CFNumberGetTypeID());

                float value = 0.f;
                DISTRHO_SAFE_ASSERT_BREAK(CFNumberGetValue(valueRef, kCFNumberFloat32Type, &value));

                const CFIndex keyRefLen = CFStringGetLength(keyRef);
                if (symbolLen < keyRefLen)
                {
                    symbolLen = keyRefLen;
                    symbol = static_cast<char*>(std::realloc(symbol, symbolLen + 1));
                }
                DISTRHO_SAFE_ASSERT_BREAK(CFStringGetCString(keyRef, symbol, symbolLen + 1, kCFStringEncodingASCII));

                for (uint32_t j=0; j<fParameterCount; ++j)
                {
                    if (fPlugin.isParameterOutputOrTrigger(j))
                        continue;
                    if (fPlugin.getParameterSymbol(j) != symbol)
                        continue;

                    fLastParameterValues[j] = value;
                    fPlugin.setParameterValue(j, value);
                    notifyPropertyListeners('DPFp', kAudioUnitScope_Global, j);

                    if (fBypassParameterIndex == j)
                        notifyPropertyListeners(kAudioUnitProperty_BypassEffect, kAudioUnitScope_Global, 0);

                    break;
                }
            }

            std::free(symbol);
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // DPF callbacks

   #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    bool writeMidi(const MidiEvent& midiEvent)
    {
        DISTRHO_CUSTOM_SAFE_ASSERT_ONCE_RETURN("MIDI output unsupported", fMidiOutput.midiOutputCallback != nullptr, false);

        if (midiEvent.size > sizeof(MIDIPacket::data))
            return true;
        if (fMidiOutputPackets.numPackets == kMaxMidiEvents)
            return false;

        const uint8_t* const midiData = midiEvent.size > MidiEvent::kDataSize ? midiEvent.dataExt : midiEvent.data;
        MIDIPacket& packet(fMidiOutputPackets.packets[fMidiOutputPackets.numPackets++]);
        packet.timeStamp = midiEvent.frame;
        packet.length = midiEvent.size;
        std::memcpy(packet.data, midiData, midiEvent.size);
        return true;
    }

    static bool writeMidiCallback(void* const ptr, const MidiEvent& midiEvent)
    {
        return static_cast<PluginAU*>(ptr)->writeMidi(midiEvent);
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
    bool requestParameterValueChange(const uint32_t index, const float value)
    {
        AudioUnitEvent event;
        std::memset(&event, 0, sizeof(event));
        event.mEventType                        = kAudioUnitEvent_ParameterValueChange;
        event.mArgument.mParameter.mAudioUnit   = fComponent;
        event.mArgument.mParameter.mParameterID = index;
        event.mArgument.mParameter.mScope       = kAudioUnitScope_Global;

        fLastParameterValues[index] = value;
        AUEventListenerNotify(NULL, NULL, &event);
        notifyPropertyListeners('DPFp', kAudioUnitScope_Global, index);
        return true;
    }

    static bool requestParameterValueChangeCallback(void* const ptr, const uint32_t index, const float value)
    {
        return static_cast<PluginAU*>(ptr)->requestParameterValueChange(index, value);
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_STATE
    bool updateState(const char* const key, const char* const newValue)
    {
        fPlugin.setState(key, newValue);

        for (uint32_t i=0; i<fStateCount; ++i)
        {
            if (fPlugin.getStateKey(i) == key)
            {
                const String dkey(key);
                fStateMap[dkey] = newValue;

                if ((fPlugin.getStateHints(i) & kStateIsOnlyForDSP) == 0x0)
                    notifyPropertyListeners('DPFs', kAudioUnitScope_Global, i);

                return true;
            }
        }

        d_stderr("Failed to find plugin state with key \"%s\"", key);
        return false;
    }

    static bool updateStateValueCallback(void* const ptr, const char* const key, const char* const newValue)
    {
        return static_cast<PluginAU*>(ptr)->updateState(key, newValue);
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
       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        case kMusicDeviceMIDIEventSelect:
            return reinterpret_cast<AudioComponentMethod>(MIDIEvent);
        case kMusicDeviceSysExSelect:
            return reinterpret_cast<AudioComponentMethod>(SysEx);
       #else
        case kMusicDeviceMIDIEventSelect:
        case kMusicDeviceSysExSelect:
            return nullptr;
       #endif
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
        d_debug("AudioComponentPlugInInstance::GetPropertyInfo(%p, %d:%x:%s, %d:%s, %d, ...)",
                self, inProp, inProp, AudioUnitPropertyID2Str(inProp), inScope, AudioUnitScope2Str(inScope), inElement);

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
        d_debug("AudioComponentPlugInInstance::GetProperty(%p, %d:%x:%s, %d:%s, %d, ...)",
                self, inProp, inProp, AudioUnitPropertyID2Str(inProp), inScope, AudioUnitScope2Str(inScope), inElement);
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
        d_debug("AudioComponentPlugInInstance::SetProperty(%p, %d:%x:%s, %d:%s, %d, %p, %u)",
                self, inProp, inProp, AudioUnitPropertyID2Str(inProp), inScope, AudioUnitScope2Str(inScope), inElement, inData, inDataSize);
        return self->plugin->auSetProperty(inProp, inScope, inElement, inData, inDataSize);
    }

    static OSStatus AddPropertyListener(AudioComponentPlugInInstance* const self,
                                        const AudioUnitPropertyID prop,
                                        const AudioUnitPropertyListenerProc proc,
                                        void* const userData)
    {
        d_debug("AudioComponentPlugInInstance::AddPropertyListener(%p, %d:%x:%s, %p, %p)",
                self, prop, prop, AudioUnitPropertyID2Str(prop), proc, userData);
        return self->plugin->auAddPropertyListener(prop, proc, userData);
    }

    static OSStatus RemovePropertyListener(AudioComponentPlugInInstance* const self,
                                           const AudioUnitPropertyID prop,
                                           const AudioUnitPropertyListenerProc proc)
    {
        d_debug("AudioComponentPlugInInstance::RemovePropertyListener(%p, %d:%x:%s, %p)",
                self, prop, prop, AudioUnitPropertyID2Str(prop), proc);
        return self->plugin->auRemovePropertyListener(prop, proc);
    }

    static OSStatus RemovePropertyListenerWithUserData(AudioComponentPlugInInstance* const self,
                                                       const AudioUnitPropertyID prop,
                                                       const AudioUnitPropertyListenerProc proc,
                                                       void* const userData)
    {
        d_debug("AudioComponentPlugInInstance::RemovePropertyListenerWithUserData(%p, %d:%x:%s, %p, %p)",
                self, prop, prop, AudioUnitPropertyID2Str(prop), proc, userData);
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
        const AudioUnitRenderActionFlags actionFlags = ioActionFlags != nullptr ? *ioActionFlags : 0;

        if ((actionFlags & kAudioUnitRenderAction_DoNotCheckRenderArgs) == 0x0)
        {
            DISTRHO_SAFE_ASSERT_RETURN(inTimeStamp != nullptr, kAudio_ParamError);
            DISTRHO_SAFE_ASSERT_RETURN(ioData != nullptr, kAudio_ParamError);
        }

        return self->plugin->auRender(actionFlags, inTimeStamp, inOutputBusNumber, inNumberFrames, ioData);
    }

   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    static OSStatus MIDIEvent(AudioComponentPlugInInstance* const self,
                              const UInt32 inStatus,
                              const UInt32 inData1,
                              const UInt32 inData2,
                              const UInt32 inOffsetSampleFrame)
    {
        return self->plugin->auMIDIEvent(inStatus, inData1, inData2, inOffsetSampleFrame);
    }

    static OSStatus SysEx(AudioComponentPlugInInstance* const self, const UInt8* const inData, const UInt32 inLength)
    {
        return self->plugin->auSysEx(inData, inLength);
    }
   #endif

    DISTRHO_DECLARE_NON_COPYABLE(AudioComponentPlugInInstance)
};

#if 0
static OSStatus FastDispatchGetParameter(void* const self,
                                         const AudioUnitParameterID param,
                                         const AudioUnitScope scope,
                                         const AudioUnitElement elem,
                                         Float32* const value)
{
    d_debug("FastDispatchGetParameter(%p, %d, %d:%s, %d, %p)",
            self, param, scope, AudioUnitScope2Str(scope), elem, value);
    return static_cast<AudioComponentPlugInInstance*>(self)->plugin->auGetParameter(param, scope, elem, value);
}

static OSStatus FastDispatchSetParameter(void* const self,
                                         const AudioUnitParameterID param,
                                         const AudioUnitScope scope,
                                         const AudioUnitElement elem,
                                         const Float32 value,
                                         const UInt32 bufferOffset)
{
    d_debug("FastDispatchSetParameter(%p, %d %d:%s, %d, %f, %u)",
            self, param, scope, AudioUnitScope2Str(scope), elem, value, bufferOffset);
    return static_cast<AudioComponentPlugInInstance*>(self)->plugin->auSetParameter(param, scope, elem, value, bufferOffset);
}

static OSStatus FastDispatchRender(void* const self,
                                   AudioUnitRenderActionFlags* const ioActionFlags,
                                   const AudioTimeStamp* const inTimeStamp,
                                   const UInt32 inBusNumber,
                                   const UInt32 inNumberFrames,
                                   AudioBufferList* const ioData)
{
    DISTRHO_SAFE_ASSERT_RETURN(inTimeStamp != nullptr, kAudio_ParamError);
    DISTRHO_SAFE_ASSERT_RETURN(ioData != nullptr, kAudio_ParamError);

    return static_cast<AudioComponentPlugInInstance*>(self)->plugin->auRender(ioActionFlags, inTimeStamp, inBusNumber, inNumberFrames, *ioData);
}
#endif

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
