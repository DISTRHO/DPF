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
#include "../DistrhoPluginUtils.hpp"

// #define CA_BASIC_AU_FEATURES 1
#define CA_NO_AU_UI_FEATURES 1
#define CA_USE_AUDIO_PLUGIN_ONLY 1
#define TARGET_OS_MAC 1

#include <AudioUnit/AudioUnit.h>
#include <AudioUnit/AUCocoaUIView.h>

#define TRACE d_stderr("////////--------------------------------------------------------------- %s %d", __PRETTY_FUNCTION__, __LINE__);

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"

#include "au/MusicDeviceBase.h"

#pragma clang diagnostic pop

#ifndef TARGET_OS_MAC
#error TARGET_OS_MAC missing
#endif

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT || DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
typedef MusicDeviceBase PluginBase;
#define PluginBaseFactory AUMIDIEffectFactory
#else
typedef AUBase PluginBase;
#define PluginBaseFactory AUBaseFactory
#endif

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

class PluginHolder
{
public:
    PluginExporter fPlugin;

   #if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS != 0
    struct BusInfo {
        char name[32];
        uint32_t numChannels;
        bool hasPair;
        bool isCV;
        bool isMain;
        uint32_t groupId;
    };
    std::vector<BusInfo> fAudioInputBuses, fAudioOutputBuses;
    bool fUsingCV;
   #endif

    PluginHolder()
        : fPlugin(this, writeMidiCallback, requestParameterValueChangeCallback, updateStateValueCallback)
       #if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS != 0
        , fAudioInputBuses()
        , fAudioOutputBuses()
        , fUsingCV(false)
       #endif
    {
       #if DISTRHO_PLUGIN_NUM_INPUTS != 0
        fillInBusInfoDetails<true>();
       #endif
       #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
        fillInBusInfoDetails<false>();
       #endif
       #if DISTRHO_PLUGIN_NUM_INPUTS != 0 && DISTRHO_PLUGIN_NUM_OUTPUTS != 0
        fillInBusInfoPairs();
       #endif
    }

    // virtual ~PluginHolder() {}

protected:
    uint32_t getPluginBusCount(const bool isInput) noexcept
    {
       #if DISTRHO_PLUGIN_NUM_INPUTS != 0
        if (isInput)
            return fAudioInputBuses.size();
       #endif
       #if DISTRHO_PLUGIN_NUM_OUTPUTS != 0
        if (!isInput)
            return fAudioOutputBuses.size();
       #endif
        return 0;
    }

private:
   #if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS != 0
    template<bool isInput>
    void fillInBusInfoDetails()
    {
        constexpr const uint32_t numPorts = isInput ? DISTRHO_PLUGIN_NUM_INPUTS : DISTRHO_PLUGIN_NUM_OUTPUTS;
        std::vector<BusInfo>& busInfos(isInput ? fAudioInputBuses : fAudioOutputBuses);

        enum {
            kPortTypeNull,
            kPortTypeAudio,
            kPortTypeSidechain,
            kPortTypeCV,
            kPortTypeGroup
        } lastSeenPortType = kPortTypeNull;
        uint32_t lastSeenGroupId = kPortGroupNone;
        uint32_t nonGroupAudioId = 0;
        uint32_t nonGroupSidechainId = 0x20000000;

        for (uint32_t i=0; i<numPorts; ++i)
        {
            const AudioPortWithBusId& port(fPlugin.getAudioPort(isInput, i));

            if (port.groupId != kPortGroupNone)
            {
                if (lastSeenPortType != kPortTypeGroup || lastSeenGroupId != port.groupId)
                {
                    lastSeenPortType = kPortTypeGroup;
                    lastSeenGroupId = port.groupId;

                    BusInfo busInfo = {
                        {}, 1, false, false,
                        // if this is the first port, set it as main
                        busInfos.empty(),
                        // user given group id with extra safety offset
                        port.groupId + 0x80000000
                    };

                    const PortGroupWithId& group(fPlugin.getPortGroupById(port.groupId));

                    switch (port.groupId)
                    {
                    case kPortGroupStereo:
                    case kPortGroupMono:
                        if (busInfo.isMain)
                        {
                            d_strncpy(busInfo.name, isInput ? "Audio Input" : "Audio Output", 32);
                            break;
                        }
                    // fall-through
                    default:
                        if (group.name.isNotEmpty())
                            d_strncpy(busInfo.name, group.name, 32);
                        else
                            d_strncpy(busInfo.name, port.name, 32);
                        break;
                    }

                    busInfos.push_back(busInfo);
                }
                else
                {
                    ++busInfos.back().numChannels;
                }
            }
            else if (port.hints & kAudioPortIsCV)
            {
                // TODO
                lastSeenPortType = kPortTypeCV;
                lastSeenGroupId = kPortGroupNone;
                fUsingCV = true;
            }
            else if (port.hints & kAudioPortIsSidechain)
            {
                if (lastSeenPortType != kPortTypeSidechain)
                {
                    lastSeenPortType = kPortTypeSidechain;
                    lastSeenGroupId = kPortGroupNone;

                    BusInfo busInfo = {
                        {}, 1, false, false,
                        // not main
                        false,
                        // give unique id
                        nonGroupSidechainId++
                    };

                    d_strncpy(busInfo.name, port.name, 32);

                    busInfos.push_back(busInfo);
                }
                else
                {
                    ++busInfos.back().numChannels;
                }
            }
            else
            {
                if (lastSeenPortType != kPortTypeAudio)
                {
                    lastSeenPortType = kPortTypeAudio;
                    lastSeenGroupId = kPortGroupNone;

                    BusInfo busInfo = {
                        {}, 1, false, false,
                        // if this is the first port, set it as main
                        busInfos.empty(),
                        // give unique id
                        nonGroupAudioId++
                    };

                    if (busInfo.isMain)
                    {
                        d_strncpy(busInfo.name, isInput ? "Audio Input" : "Audio Output", 32);
                    }
                    else
                    {
                        d_strncpy(busInfo.name, port.name, 32);
                    }

                    busInfos.push_back(busInfo);
                }
                else
                {
                    ++busInfos.back().numChannels;
                }
            }
        }
    }
   #endif

   #if DISTRHO_PLUGIN_NUM_INPUTS != 0 && DISTRHO_PLUGIN_NUM_OUTPUTS != 0
    void fillInBusInfoPairs()
    {
        const size_t numSharedBuses = std::min(fAudioInputBuses.size(), fAudioOutputBuses.size());

        for (size_t i=0; i<numSharedBuses; ++i)
        {
            if (fAudioInputBuses[i].groupId != fAudioOutputBuses[i].groupId)
                break;
            if (fAudioInputBuses[i].numChannels != fAudioOutputBuses[i].numChannels)
                break;
            if (fAudioInputBuses[i].isMain != fAudioOutputBuses[i].isMain)
                break;

            fAudioInputBuses[i].hasPair = fAudioOutputBuses[i].hasPair = true;
        }
    }
   #endif

    // ----------------------------------------------------------------------------------------------------------------
    // DPF callbacks

   #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT && 0
    bool writeMidi(const MidiEvent&)
    {
        return true;
    }

    static bool writeMidiCallback(void* const ptr, const MidiEvent& midiEvent)
    {
        return static_cast<PluginHolder*>(ptr)->writeMidi(midiEvent);
    }
   #endif


   #if DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST && 0
    bool requestParameterValueChange(uint32_t, float)
    {
        return true;
    }

    static bool requestParameterValueChangeCallback(void* const ptr, const uint32_t index, const float value)
    {
        return static_cast<PluginHolder*>(ptr)->requestParameterValueChange(index, value);
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_STATE && 0
    bool updateState(const char*, const char*)
    {
        return true;
    }

    static bool updateStateValueCallback(void* const ptr, const char* const key, const char* const value)
    {
        return static_cast<PluginHolder*>(ptr)->updateState(key, value);
    }
   #endif
};

class PluginAU : public PluginHolder,
                 public PluginBase
{
public:
    PluginAU(AudioUnit component)
        : PluginHolder(),
          PluginBase(component, getPluginBusCount(true), getPluginBusCount(false))
    {
        TRACE
    }

    ~PluginAU() override
    {
        TRACE
    }

protected:
#if 0
    OSStatus Initialize() override
    {
        TRACE
        ComponentResult res;

        res = PluginBase::Initialize();
        DISTRHO_SAFE_ASSERT_INT_RETURN(res == noErr, res, res);

        setBufferSizeAndSampleRate();
        return noErr;
    }

    void Cleanup() override
    {
        TRACE
        fPlugin.deactivate();
        PluginBase::Cleanup();
    }
#endif

    OSStatus Reset(AudioUnitScope inScope, AudioUnitElement inElement) override
    {
        TRACE
        fPlugin.deactivateIfNeeded();
        setBufferSizeAndSampleRate();

        const OSStatus res = PluginBase::Reset(inScope, inElement);

        if (res == noErr)
            fPlugin.activate();

        return res;
    }

    OSStatus GetPropertyInfo(const AudioUnitPropertyID inID,
                             const AudioUnitScope inScope,
                             const AudioUnitElement inElement,
                             UInt32& outDataSize,
                             Boolean& outWritable) override
    {
        TRACE
        if (inScope == kAudioUnitScope_Global)
        {
            switch (inID)
            {
           #if DISTRHO_PLUGIN_HAS_UI
            case kAudioUnitProperty_CocoaUI:
                outDataSize = sizeof(AudioUnitCocoaViewInfo);
                outWritable = true;
                return noErr;
           #endif
            default:
                break;
            }
        }

        return PluginBase::GetPropertyInfo(inID, inScope, inElement, outDataSize, outWritable);
    }

    OSStatus GetProperty(const AudioUnitPropertyID inID,
                         const AudioUnitScope inScope,
                         const AudioUnitElement inElement,
                         void* const outData) override
    {
        TRACE
        if (inScope == kAudioUnitScope_Global)
        {
            switch (inID)
            {
           #if DISTRHO_PLUGIN_HAS_UI
            case kAudioUnitProperty_CocoaUI:
                if (AudioUnitCocoaViewInfo* const info = static_cast<AudioUnitCocoaViewInfo*>(outData))
                {
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
            default:
                break;
            }
        }

        return PluginBase::GetProperty(inID, inScope, inElement, outData);
    }

#if 0
    OSStatus SetProperty(AudioUnitPropertyID inID,
                                AudioUnitScope inScope,
                                AudioUnitElement inElement,
                                const void* inData,
                                UInt32 inDataSize) override
    {
        TRACE
        return PluginBase::SetProperty(inID, inScope, inElement, inData, inDataSize);
    }

    OSStatus GetParameter(AudioUnitParameterID inID,
                                 AudioUnitScope inScope,
                                 AudioUnitElement inElement,
                                 Float32& outValue) override
    {
        TRACE
        return kAudioUnitErr_InvalidProperty;
        // return PluginBase::GetParameter(inID, inScope, inElement, outValue);
    }

    OSStatus SetParameter(AudioUnitParameterID inID,
                                 AudioUnitScope inScope,
                                 AudioUnitElement inElement,
                                 Float32 inValue,
                                 UInt32 inBufferOffsetInFrames) override
    {
        TRACE
        return kAudioUnitErr_InvalidProperty;
        // return PluginBase::SetParameter(inID, inScope, inElement, inValue, inBufferOffsetInFrames);
    }
#endif

    bool CanScheduleParameters() const override
    {
        TRACE
        return false;
    }

    OSStatus Render(AudioUnitRenderActionFlags& ioActionFlags,
                    const AudioTimeStamp& inTimeStamp,
                    const UInt32 nFrames) override
    {
        TRACE
        return noErr;
    }

#if 0
    bool BusCountWritable(AudioUnitScope scope) override
    {
        TRACE
        return false;
    }

    OSStatus SetBusCount(AudioUnitScope scope, UInt32 count) override
    {
        TRACE
        return noErr;
    }

    OSStatus GetParameterInfo(AudioUnitScope inScope,
                                     AudioUnitParameterID inParameterID,
                                     AudioUnitParameterInfo& outParameterInfo) override
    {
        TRACE
        return kAudioUnitErr_InvalidProperty;
    }

    OSStatus SaveState(CFPropertyListRef* outData) override
    {
        TRACE
        return PluginBase::SaveState(outData);
    }

    OSStatus RestoreState(CFPropertyListRef inData) override
    {
        TRACE
        return noErr;
    }

    OSStatus GetParameterValueStrings(AudioUnitScope inScope,
                                             AudioUnitParameterID inParameterID,
                                             CFArrayRef *outStrings) override
    {
        TRACE
        return kAudioUnitErr_InvalidProperty;
    }

    OSStatus GetPresets(CFArrayRef* outData) const override
    {
        TRACE
        return noErr;
    }

    OSStatus NewFactoryPresetSet(const AUPreset& inNewFactoryPreset) override
    {
        TRACE
        return noErr;
    }
#endif

   #if DISTRHO_PLUGIN_WANT_LATENCY
    Float64 GetLatency() override
    {
        TRACE
        return static_cast<double>(fPlugin.getLatency()) / fPlugin.getSampleRate();
    }
   #endif

    bool StreamFormatWritable(AudioUnitScope, AudioUnitElement) override
    {
        TRACE
        return false;
    }

    UInt32 SupportedNumChannels(const AUChannelInfo** const outInfo) override
    {
        TRACE
        static const AUChannelInfo sChannels[1] = {{ DISTRHO_PLUGIN_NUM_INPUTS, DISTRHO_PLUGIN_NUM_OUTPUTS }};

        if (outInfo != nullptr)
            *outInfo = sChannels;

        return 1;
    }

#if 0
    bool ValidFormat(AudioUnitScope scope, AudioUnitElement element, const CAStreamBasicDescription& format) override
    {
        TRACE
        return false;
    }

    OSStatus ChangeStreamFormat(AudioUnitScope scope, AudioUnitElement element, const CAStreamBasicDescription& old, const CAStreamBasicDescription& format) override
    {
        TRACE
        return noErr;
    }

    OSStatus StartNote(MusicDeviceInstrumentID, MusicDeviceGroupID, NoteInstanceID*, UInt32, const MusicDeviceNoteParams&) override
    {
        TRACE
        return noErr;
    }

    OSStatus StopNote(MusicDeviceGroupID, NoteInstanceID, UInt32) override
    {
        TRACE
        return noErr;    
    }

	void SetMaxFramesPerSlice(const UInt32 nFrames) override
    {
        PluginBase::SetMaxFramesPerSlice(nFrames);

        DISTRHO_SAFE_ASSERT_RETURN(!fPlugin.isActive(),);
        fPlugin.setBufferSize(nFrames, true);
    }

    UInt32 GetChannelLayoutTags(const AudioUnitScope scope,
                                const AudioUnitElement element,
                                AudioChannelLayoutTag* const outLayoutTags) override
    {
        TRACE
        return 0;
    }

    UInt32 GetAudioChannelLayout(AudioUnitScope scope, AudioUnitElement element,
                                 AudioChannelLayout* outLayoutPtr, Boolean& outWritable) override
    {
        TRACE
        return 0;
    }

    OSStatus SetAudioChannelLayout(AudioUnitScope scope, AudioUnitElement element, const AudioChannelLayout* inLayout) override
    {
        TRACE
        return noErr;
    }
#endif

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
#if !CA_USE_AUDIO_PLUGIN_ONLY
#include "au/AUDispatch.cpp"
#endif
#include "au/AUInputElement.cpp"
#include "au/AUMIDIBase.cpp"
#include "au/AUOutputBase.cpp"
#include "au/AUOutputElement.cpp"
#include "au/AUPlugInDispatch.cpp"
#include "au/AUScopeElement.cpp"
#include "au/CAAUParameter.cpp"
#include "au/CAAudioChannelLayout.cpp"
#include "au/CAMutex.cpp"
#include "au/CAStreamBasicDescription.cpp"
#include "au/CAVectorUnit.cpp"
#include "au/ComponentBase.cpp"
#include "au/MusicDeviceBase.cpp"

#pragma clang diagnostic pop

#undef verify
#undef verify_noerr

// --------------------------------------------------------------------------------------------------------------------

/*
DISTRHO_PLUGIN_EXPORT
OSStatus PluginAUEntry(ComponentParameters*, PluginAU*);

DISTRHO_PLUGIN_EXPORT
void* PluginAUFactory(const AudioComponentDescription*);

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT || DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
AUDIOCOMPONENT_ENTRY(AUMIDIEffectFactory, PluginAU)
#else
AUDIOCOMPONENT_ENTRY(AUBaseFactory, PluginAU)
#endif

DISTRHO_PLUGIN_EXPORT
OSStatus PluginAUEntry(ComponentParameters* const params, PluginAU* const obj)
{
    TRACE
    return ComponentEntryPoint<PluginAU>::Dispatch(params, obj);
}
*/

DISTRHO_PLUGIN_EXPORT
void* PluginAUFactory(const AudioComponentDescription* const inDesc)
{
    TRACE
    if (d_nextBufferSize == 0)
        d_nextBufferSize = kAUDefaultMaxFramesPerSlice;

    if (d_isZero(d_nextSampleRate))
        d_nextSampleRate = kAUDefaultSampleRate;

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

    // d_nextCanRequestParameterValueChanges = true;

    return PluginBaseFactory<PluginAU>::Factory(inDesc);
}

// --------------------------------------------------------------------------------------------------------------------
