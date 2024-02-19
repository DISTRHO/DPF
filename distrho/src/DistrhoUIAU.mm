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

// #include <AudioUnit/AudioUnit.h>
#include <AudioUnit/AUCocoaUIView.h>

#undef Point
#undef Size

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
    DPF_UI_AU(const AudioUnit inAU,
              const intptr_t winId,
              const double sampleRate,
              void* const instancePointer)
        : fAU(inAU),
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
    }

    ~DPF_UI_AU()
    {
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
    const AudioUnit fAU;
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
    // DPF callbacks

    void editParameter(uint32_t, bool) const
    {
    }

    static void editParameterCallback(void* const ptr, const uint32_t rindex, const bool started)
    {
        static_cast<DPF_UI_AU*>(ptr)->editParameter(rindex, started);
    }

    void setParameterValue(uint32_t, float)
    {
    }

    static void setParameterCallback(void* const ptr, const uint32_t rindex, const float value)
    {
        static_cast<DPF_UI_AU*>(ptr)->setParameterValue(rindex, value);
    }

   #if DISTRHO_PLUGIN_WANT_STATE
    void setState(const char*, const char*)
    {
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

@interface DPF_UI_ViewFactory : NSObject<AUCocoaUIBase>
{
    DPF_UI_AU* ui;
}
@end

@implementation DPF_UI_ViewFactory

- (NSString*) description
{
    return @DISTRHO_PLUGIN_NAME;
}

- (unsigned) interfaceVersion
{
    return 0;
}

- (NSView*) uiViewForAudioUnit:(AudioUnit)inAU withSize:(NSSize)inPreferredSize
{
    const double sampleRate = d_nextSampleRate;
    const intptr_t winId = 0;
    void* const instancePointer = nullptr;

    ui = new DPF_UI_AU(inAU, winId, sampleRate, instancePointer);

    return ui->getNativeView();
}

@end

// --------------------------------------------------------------------------------------------------------------------
