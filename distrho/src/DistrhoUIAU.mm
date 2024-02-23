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

#include "DistrhoUIInternal.hpp"

#define Point AudioUnitPoint
#define Size AudioUnitSize

#include <AudioUnit/AudioUnit.h>
#include <AudioUnit/AUCocoaUIView.h>

#undef Point
#undef Size

#ifndef DISTRHO_PLUGIN_AU_SUBTYPE
# error DISTRHO_PLUGIN_AU_SUBTYPE undefined!
#endif

#ifndef DISTRHO_PLUGIN_AU_MANUFACTURER
# error DISTRHO_PLUGIN_AU_MANUFACTURER undefined!
#endif

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

#if ! DISTRHO_PLUGIN_WANT_STATE
static constexpr const setStateFunc setStateCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_MIDI_INPUT
static constexpr const sendNoteFunc sendNoteCallback = nullptr;
#endif

// --------------------------------------------------------------------------------------------------------------------
// Static data, see DistrhoPlugin.cpp

extern double d_nextSampleRate;
extern const char* d_nextBundlePath;

// --------------------------------------------------------------------------------------------------------------------

class DPF_UI_AU
{
public:
    DPF_UI_AU(const AudioUnit component,
              const intptr_t winId,
              const double sampleRate,
              void* const instancePointer)
        : fComponent(component),
          fTimerRef(nullptr),
          fUI(this, winId, sampleRate,
              editParameterCallback,
              setParameterCallback,
              setStateCallback,
              sendNoteCallback,
              setSizeCallback,
              nullptr, // TODO file request
              d_nextBundlePath,
              instancePointer)
    {
        constexpr const CFTimeInterval interval = 60 * 0.0001;

        CFRunLoopTimerContext context = {};
        context.info = this;
        fTimerRef = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + interval, interval, 0, 0,
                                         _idleCallback, &context);
        DISTRHO_SAFE_ASSERT_RETURN(fTimerRef != nullptr,);

        CFRunLoopAddTimer(CFRunLoopGetCurrent(), fTimerRef, kCFRunLoopCommonModes);

        AudioUnitAddPropertyListener(fComponent, 'DPFP', auPropertyChangedCallback, this);
    }

    ~DPF_UI_AU()
    {
        AudioUnitRemovePropertyListenerWithUserData(fComponent, 'DPFP', auPropertyChangedCallback, this);

        if (fTimerRef != nullptr)
        {
            CFRunLoopRemoveTimer(CFRunLoopGetCurrent(), fTimerRef, kCFRunLoopCommonModes);
            CFRelease(fTimerRef);
        }
    }

    NSView* getNativeView() const
    {
        return reinterpret_cast<NSView*>(fUI.getNativeWindowHandle());
    }

private:
    const AudioUnit fComponent;
    CFRunLoopTimerRef fTimerRef;

    UIExporter fUI;

    // ----------------------------------------------------------------------------------------------------------------
    // Idle setup

    void idleCallback()
    {
        fUI.idleFromNativeIdle();
    }

    static void _idleCallback(CFRunLoopTimerRef, void* const info)
    {
        static_cast<DPF_UI_AU*>(info)->idleCallback();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // AU callbacks

    void auPropertyChanged(const AudioUnitPropertyID prop, const AudioUnitElement elem)
    {
        switch (prop)
        {
        case 'DPFP':
            {
                AudioUnitParameterValue value;
                if (AudioUnitGetParameter(fComponent, elem, kAudioUnitScope_Global, 0, &value) == noErr)
                    fUI.parameterChanged(elem, value);
            }
            break;
        }
    }

    static void auPropertyChangedCallback(void* const userData,
                                          const AudioUnit component,
                                          const AudioUnitPropertyID prop,
                                          const AudioUnitScope scope,
                                          const AudioUnitElement elem)
    {
        DPF_UI_AU* const self = static_cast<DPF_UI_AU*>(userData);

        DISTRHO_SAFE_ASSERT_RETURN(self != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(self->fComponent == component,);
        DISTRHO_SAFE_ASSERT_UINT_RETURN(scope == kAudioUnitScope_Global, scope,);

        self->auPropertyChanged(prop, elem);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // DPF callbacks

    void editParameter(const uint32_t rindex, const bool started) const
    {
        AudioUnitSetProperty(fComponent, 'DPFe', kAudioUnitScope_Global, rindex, &started, sizeof(bool));
    }

    static void editParameterCallback(void* const ptr, const uint32_t rindex, const bool started)
    {
        static_cast<DPF_UI_AU*>(ptr)->editParameter(rindex, started);
    }

    void setParameterValue(const uint32_t rindex, const float value)
    {
        AudioUnitSetProperty(fComponent, 'DPFp', kAudioUnitScope_Global, rindex, &value, sizeof(float));
    }

    static void setParameterCallback(void* const ptr, const uint32_t rindex, const float value)
    {
        static_cast<DPF_UI_AU*>(ptr)->setParameterValue(rindex, value);
    }

   #if DISTRHO_PLUGIN_WANT_STATE
    void setState(const char* const key, const char* const value)
    {
        const size_t len_key = std::strlen(key);
        const size_t len_value = std::strlen(value);
        const size_t len_combined = len_key + len_value + 2;
        char* const data = new char[len_combined];
        std::memcpy(data, key, len_key);
        std::memcpy(data + len_key + 1, value, len_value);
        data[len_key] = data[len_key + len_value + 1] = '\0';

        AudioUnitSetProperty(fComponent, 'DPFs', kAudioUnitScope_Global, len_combined, data, len_combined);

        delete[] data;
    }

    static void setStateCallback(void* const ptr, const char* const key, const char* const value)
    {
        static_cast<DPF_UI_AU*>(ptr)->setState(key, value);
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    void sendNote(uint8_t, uint8_t, uint8_t)
    {
    }

    static void sendNoteCallback(void* const ptr, const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        static_cast<DPF_UI_AU*>(ptr)->sendNote(channel, note, velocity);
    }
   #endif

    void setSize(uint, uint)
    {
    }

    static void setSizeCallback(void* const ptr, const uint width, const uint height)
    {
        static_cast<DPF_UI_AU*>(ptr)->setSize(width, height);
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

#define MACRO_NAME2(a, b, c, d, e, f) a ## b ## c ## d ## e ## f
#define MACRO_NAME(a, b, c, d, e, f) MACRO_NAME2(a, b, c, d, e, f)

#define COCOA_VIEW_CLASS_NAME MACRO_NAME(CocoaAUView_, DISTRHO_PLUGIN_AU_TYPE, _, DISTRHO_PLUGIN_AU_SUBTYPE, _, DISTRHO_PLUGIN_AU_MANUFACTURER)

@interface COCOA_VIEW_CLASS_NAME : NSObject<AUCocoaUIBase>
{
    DPF_UI_AU* ui;
}
@end

@implementation COCOA_VIEW_CLASS_NAME

- (NSString*) description
{
    return @DISTRHO_PLUGIN_NAME;
}

- (unsigned) interfaceVersion
{
    return 0;
}

- (NSView*) uiViewForAudioUnit:(AudioUnit)component withSize:(NSSize)inPreferredSize
{
    const double sampleRate = d_nextSampleRate;
    const intptr_t winId = 0;
    void* instancePointer = nullptr;

   #if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
    UInt32 size = sizeof(void*);
    AudioUnitGetProperty(component, 'DPFa', kAudioUnitScope_Global, 0, &instancePointer, &size);
   #endif

    ui = new DPF_UI_AU(component, winId, sampleRate, instancePointer);

    return ui->getNativeView();
}

@end

#undef MACRO_NAME
#undef MACRO_NAME2

// --------------------------------------------------------------------------------------------------------------------
