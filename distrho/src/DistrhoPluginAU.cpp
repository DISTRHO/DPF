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

// #define CA_NO_AU_UI_FEATURES

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

    PluginHolder()
        : fPlugin(this, writeMidiCallback, requestParameterValueChangeCallback, updateStateValueCallback) {}

    virtual ~PluginHolder() {}

protected:
    uint32_t getPluginBusCount(const bool isInput) noexcept
    {
        return 0;
    }

private:
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
    ComponentResult Initialize() override
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
        PluginBase::Cleanup();
        fPlugin.deactivate();
    }

    ComponentResult Reset(AudioUnitScope inScope, AudioUnitElement inElement) override
    {
        TRACE
        fPlugin.deactivateIfNeeded();
        setBufferSizeAndSampleRate();

        const ComponentResult res = PluginBase::Reset(inScope, inElement);

        if (res == noErr)
            fPlugin.activate();

        return res;
    }

    ComponentResult GetPropertyInfo(AudioUnitPropertyID inID,
                                    AudioUnitScope inScope,
                                    AudioUnitElement inElement,
                                    UInt32& outDataSize,
                                    Boolean& outWritable) override
    {
        TRACE
        return PluginBase::GetPropertyInfo(inID, inScope, inElement, outDataSize, outWritable);
    }

    ComponentResult GetProperty(AudioUnitPropertyID inID,
                                AudioUnitScope inScope,
                                AudioUnitElement inElement,
                                void* outData) override
    {
        TRACE
        return PluginBase::GetProperty(inID, inScope, inElement, outData);
    }

    ComponentResult SetProperty(AudioUnitPropertyID inID,
                                AudioUnitScope inScope,
                                AudioUnitElement inElement,
                                const void* inData,
                                UInt32 inDataSize) override
    {
        TRACE
        return PluginBase::SetProperty(inID, inScope, inElement, inData, inDataSize);
    }

    ComponentResult GetParameter(AudioUnitParameterID inID,
                                 AudioUnitScope inScope,
                                 AudioUnitElement inElement,
                                 Float32& outValue) override
    {
        TRACE
        return kAudioUnitErr_InvalidProperty;
        // return PluginBase::GetParameter(inID, inScope, inElement, outValue);
    }

    ComponentResult SetParameter(AudioUnitParameterID inID,
                                 AudioUnitScope inScope,
                                 AudioUnitElement inElement,
                                 Float32 inValue,
                                 UInt32 inBufferOffsetInFrames) override
    {
        TRACE
        return kAudioUnitErr_InvalidProperty;
        // return PluginBase::SetParameter(inID, inScope, inElement, inValue, inBufferOffsetInFrames);
    }

    bool CanScheduleParameters() const override
    {
        TRACE
        return false;
    }

    ComponentResult Render(AudioUnitRenderActionFlags& ioActionFlags,
                           const AudioTimeStamp& inTimeStamp,
                           const UInt32 nFrames) override
    {
        TRACE
        return noErr;
    }

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

    ComponentResult GetParameterInfo(AudioUnitScope inScope,
                                     AudioUnitParameterID inParameterID,
                                     AudioUnitParameterInfo& outParameterInfo) override
    {
        TRACE
        return kAudioUnitErr_InvalidProperty;
    }

    ComponentResult SaveState(CFPropertyListRef* outData) override
    {
        TRACE
        return PluginBase::SaveState(outData);
    }

    ComponentResult RestoreState(CFPropertyListRef inData) override
    {
        TRACE
        return noErr;
    }

    ComponentResult GetParameterValueStrings(AudioUnitScope inScope,
                                             AudioUnitParameterID inParameterID,
                                             CFArrayRef *outStrings) override
    {
        TRACE
        return kAudioUnitErr_InvalidProperty;
    }

    ComponentResult GetPresets(CFArrayRef* outData) const override
    {
        TRACE
        return noErr;
    }

    OSStatus NewFactoryPresetSet(const AUPreset& inNewFactoryPreset) override
    {
        TRACE
        return noErr;
    }

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

    UInt32 SupportedNumChannels(const AUChannelInfo** outInfo) override
    {
        TRACE
        return 0;
    }

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

    ComponentResult StartNote(MusicDeviceInstrumentID, MusicDeviceGroupID, NoteInstanceID*, UInt32, const MusicDeviceNoteParams&) override
    {
        TRACE
        return noErr;
    }

    ComponentResult StopNote(MusicDeviceGroupID, NoteInstanceID, UInt32) override
    {
        TRACE
        return noErr;    
    }

    UInt32 GetChannelLayoutTags(AudioUnitScope scope, AudioUnitElement element, AudioChannelLayoutTag* outLayoutTags) override
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
#include "au/AUDispatch.cpp"
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
        d_nextBufferSize = 512;
    if (d_nextSampleRate == 0)
        d_nextSampleRate = 44100;

    return PluginBaseFactory<PluginAU>::Factory(inDesc);
}

// --------------------------------------------------------------------------------------------------------------------
