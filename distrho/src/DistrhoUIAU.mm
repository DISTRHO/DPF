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
#include <Cocoa/Cocoa.h>

#undef Point
#undef Size

#ifndef DISTRHO_PLUGIN_BRAND_ID
# error DISTRHO_PLUGIN_BRAND_ID undefined!
#endif

#ifndef DISTRHO_PLUGIN_UNIQUE_ID
# error DISTRHO_PLUGIN_UNIQUE_ID undefined!
#endif

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

#if ! DISTRHO_PLUGIN_WANT_STATE
static constexpr const setStateFunc setStateCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_MIDI_INPUT
static constexpr const sendNoteFunc sendNoteCallback = nullptr;
#endif

// unsupported in AU
static constexpr const fileRequestFunc fileRequestCallback = nullptr;

// --------------------------------------------------------------------------------------------------------------------
// Static data, see DistrhoPlugin.cpp

extern double d_nextSampleRate;
extern const char* d_nextBundlePath;

// --------------------------------------------------------------------------------------------------------------------

class DPF_UI_AU
{
public:
    DPF_UI_AU(const AudioUnit component,
              NSView* const view,
              const double sampleRate,
              void* const instancePointer)
        : fComponent(component),
          fParentView(view),
          fTimerRef(nullptr),
          fUI(this,
              reinterpret_cast<uintptr_t>(view),
              sampleRate,
              editParameterCallback,
              setParameterCallback,
              setStateCallback,
              sendNoteCallback,
              setSizeCallback,
              fileRequestCallback,
              d_nextBundlePath,
              instancePointer)
    {
       #if DISTRHO_PLUGIN_WANT_STATE
        // create state keys
        {
            CFArrayRef keysRef = nullptr;
            UInt32 dataSize = sizeof(CFArrayRef);
            if (AudioUnitGetProperty(fComponent, 'DPFl', kAudioUnitScope_Global, 0, &keysRef, &dataSize) == noErr
                && dataSize == sizeof(CFArrayRef))
            {
                const CFIndex numStates = CFArrayGetCount(keysRef);
                char* key = nullptr;
                CFIndex keyLen = -1;

                fStateKeys.resize(numStates);

                for (CFIndex i=0; i<numStates; ++i)
                {
                    const CFStringRef keyRef = static_cast<CFStringRef>(CFArrayGetValueAtIndex(keysRef, i));
                    DISTRHO_SAFE_ASSERT_BREAK(CFGetTypeID(keyRef) == CFStringGetTypeID());

                    const CFIndex keyRefLen = CFStringGetLength(keyRef);
                    if (keyLen < keyRefLen)
                    {
                        keyLen = keyRefLen;
                        key = static_cast<char*>(std::realloc(key, keyLen + 1));
                    }
                    DISTRHO_SAFE_ASSERT_BREAK(CFStringGetCString(keyRef, key, keyLen + 1, kCFStringEncodingASCII));

                    fStateKeys[i] = key;
                }

                CFRelease(keysRef);
                std::free(key);
            }
        }
       #endif

        // setup idle timer
        constexpr const CFTimeInterval interval = 60 * 0.0001;

        CFRunLoopTimerContext context = {};
        context.info = this;
        fTimerRef = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + interval, interval, 0, 0,
                                         _idleCallback, &context);
        DISTRHO_SAFE_ASSERT_RETURN(fTimerRef != nullptr,);

        CFRunLoopAddTimer(CFRunLoopGetCurrent(), fTimerRef, kCFRunLoopCommonModes);

        AudioUnitAddPropertyListener(fComponent, kAudioUnitProperty_SampleRate, auPropertyChangedCallback, this);
        AudioUnitAddPropertyListener(fComponent, 'DPFp', auPropertyChangedCallback, this);
       #if DISTRHO_PLUGIN_WANT_PROGRAMS
        AudioUnitAddPropertyListener(fComponent, 'DPFo', auPropertyChangedCallback, this);
       #endif
       #if DISTRHO_PLUGIN_WANT_STATE
        AudioUnitAddPropertyListener(fComponent, 'DPFs', auPropertyChangedCallback, this);
       #endif
    }

    ~DPF_UI_AU()
    {
        AudioUnitRemovePropertyListenerWithUserData(fComponent, kAudioUnitProperty_SampleRate, auPropertyChangedCallback, this);
        AudioUnitRemovePropertyListenerWithUserData(fComponent, 'DPFp', auPropertyChangedCallback, this);
       #if DISTRHO_PLUGIN_WANT_PROGRAMS
        AudioUnitRemovePropertyListenerWithUserData(fComponent, 'DPFo', auPropertyChangedCallback, this);
       #endif
       #if DISTRHO_PLUGIN_WANT_STATE
        AudioUnitRemovePropertyListenerWithUserData(fComponent, 'DPFs', auPropertyChangedCallback, this);
       #endif

        if (fTimerRef != nullptr)
        {
            CFRunLoopRemoveTimer(CFRunLoopGetCurrent(), fTimerRef, kCFRunLoopCommonModes);
            CFRelease(fTimerRef);
        }
    }

    void postSetup()
    {
        const double scaleFactor = fUI.getScaleFactor();
        const NSSize size = NSMakeSize(fUI.getWidth() / scaleFactor, fUI.getHeight() / scaleFactor);

        [fParentView setFrameSize:size];
        [fParentView setHidden:NO];
    }

private:
    const AudioUnit fComponent;
    NSView* const fParentView;
    CFRunLoopTimerRef fTimerRef;

    UIExporter fUI;

   #if DISTRHO_PLUGIN_WANT_STATE
    std::vector<String> fStateKeys;
   #endif

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

    void auSampleRateChanged(const AudioUnitScope scope)
    {
        Float64 sampleRate = 0;
        UInt32 dataSize = sizeof(Float64);
        if (AudioUnitGetProperty(fComponent, kAudioUnitProperty_SampleRate, scope, 0, &sampleRate, &dataSize) == noErr
            && dataSize == sizeof(Float64))
        {
            fUI.setSampleRate(sampleRate, true);
        }
    }

    void auParameterChanged(const AudioUnitElement elem)
    {
        float value = 0;
        UInt32 dataSize = sizeof(float);
        if (AudioUnitGetProperty(fComponent, 'DPFp', kAudioUnitScope_Global, elem, &value, &dataSize) == noErr
            && dataSize == sizeof(float))
        {
            fUI.parameterChanged(elem, value);
        }
    }

   #if DISTRHO_PLUGIN_WANT_PROGRAMS
    void auProgramChanged()
    {
        uint32_t program = 0;
        UInt32 dataSize = sizeof(uint32_t);
        if (AudioUnitGetProperty(fComponent, 'DPFo', kAudioUnitScope_Global, 0, &program, &dataSize) == noErr
            && dataSize == sizeof(uint32_t))
        {
            fUI.programLoaded(program);
        }
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_STATE
    void auStateChanged(const AudioUnitElement elem)
    {
        DISTRHO_SAFE_ASSERT_RETURN(elem < fStateKeys.size(),);

        CFStringRef valueRef = nullptr;
        UInt32 dataSize = sizeof(valueRef);
        if (AudioUnitGetProperty(fComponent, 'DPFs', kAudioUnitScope_Global, elem, &valueRef, &dataSize) == noErr
            && dataSize == sizeof(CFStringRef)
            && valueRef != nullptr
            && CFGetTypeID(valueRef) == CFStringGetTypeID())
        {
            const CFIndex valueLen = CFStringGetLength(valueRef);
            char* const value = static_cast<char*>(std::malloc(valueLen + 1));
            DISTRHO_SAFE_ASSERT_RETURN(value != nullptr,);
            DISTRHO_SAFE_ASSERT_RETURN(CFStringGetCString(valueRef, value, valueLen + 1, kCFStringEncodingUTF8),);

            fUI.stateChanged(fStateKeys[elem], value);

            CFRelease(valueRef);
            std::free(value);
        }
    }
   #endif

    static void auPropertyChangedCallback(void* const userData,
                                          const AudioUnit component,
                                          const AudioUnitPropertyID prop,
                                          const AudioUnitScope scope,
                                          const AudioUnitElement elem)
    {
        DPF_UI_AU* const self = static_cast<DPF_UI_AU*>(userData);

        DISTRHO_SAFE_ASSERT_RETURN(self != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(self->fComponent == component,);

        switch (prop)
        {
        case kAudioUnitProperty_SampleRate:
            DISTRHO_SAFE_ASSERT_UINT_RETURN(scope == kAudioUnitScope_Input || scope == kAudioUnitScope_Output, scope,);
            self->auSampleRateChanged(scope);
            break;
        case 'DPFp':
            DISTRHO_SAFE_ASSERT_UINT_RETURN(scope == kAudioUnitScope_Global, scope,);
            self->auParameterChanged(elem);
            break;
       #if DISTRHO_PLUGIN_WANT_PROGRAMS
        case 'DPFo':
            DISTRHO_SAFE_ASSERT_UINT_RETURN(scope == kAudioUnitScope_Global, scope,);
            self->auProgramChanged();
            break;
       #endif
       #if DISTRHO_PLUGIN_WANT_STATE
        case 'DPFs':
            DISTRHO_SAFE_ASSERT_UINT_RETURN(scope == kAudioUnitScope_Global, scope,);
            self->auStateChanged(elem);
            break;
       #endif
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // DPF callbacks

    void editParameter(const uint32_t rindex, const bool started) const
    {
        const uint8_t flag = started ? 1 : 2;
        AudioUnitSetProperty(fComponent, 'DPFe', kAudioUnitScope_Global, rindex, &flag, sizeof(uint8_t));

        if (! started)
        {
            const uint8_t cancel = 0;
            AudioUnitSetProperty(fComponent, 'DPFe', kAudioUnitScope_Global, rindex, &cancel, sizeof(uint8_t));
        }
    }

    static void editParameterCallback(void* const ptr, const uint32_t rindex, const bool started)
    {
        static_cast<DPF_UI_AU*>(ptr)->editParameter(rindex, started);
    }

    // ----------------------------------------------------------------------------------------------------------------

    void setParameter(const uint32_t rindex, const float value)
    {
        AudioUnitSetProperty(fComponent, 'DPFp', kAudioUnitScope_Global, rindex, &value, sizeof(float));
    }

    static void setParameterCallback(void* const ptr, const uint32_t rindex, const float value)
    {
        static_cast<DPF_UI_AU*>(ptr)->setParameter(rindex, value);
    }

    // ----------------------------------------------------------------------------------------------------------------

   #if DISTRHO_PLUGIN_WANT_STATE
    void setState(const char* const key, const char* const value)
    {
        const std::vector<String>::iterator it = std::find(fStateKeys.begin(), fStateKeys.end(), key);
        DISTRHO_SAFE_ASSERT_RETURN(it != fStateKeys.end(),);

        if (const CFStringRef valueRef = CFStringCreateWithCString(nullptr, value, kCFStringEncodingUTF8))
        {
            const uint32_t index = it - fStateKeys.begin();
            AudioUnitSetProperty(fComponent, 'DPFs', kAudioUnitScope_Global, index, &valueRef, sizeof(CFStringRef));
            CFRelease(valueRef);
        }
    }

    static void setStateCallback(void* const ptr, const char* const key, const char* const value)
    {
        static_cast<DPF_UI_AU*>(ptr)->setState(key, value);
    }
   #endif

    // ----------------------------------------------------------------------------------------------------------------

   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    void sendNote(const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        const uint8_t data[3] = { static_cast<uint8_t>((velocity != 0 ? 0x90 : 0x80) | channel), note, velocity };
        AudioUnitSetProperty(fComponent, 'DPFn', kAudioUnitScope_Global, 0, data, sizeof(data));

        const uint8_t cancel[3] = { 0, 0, 0 };
        AudioUnitSetProperty(fComponent, 'DPFn', kAudioUnitScope_Global, 0, cancel, sizeof(cancel));
    }

    static void sendNoteCallback(void* const ptr, const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        static_cast<DPF_UI_AU*>(ptr)->sendNote(channel, note, velocity);
    }
   #endif

    // ----------------------------------------------------------------------------------------------------------------

    void setSize(const uint width, const uint height)
    {
        const double scaleFactor = fUI.getScaleFactor();
        const NSSize size = NSMakeSize(width / scaleFactor, height / scaleFactor);

        [fParentView setFrameSize:size];
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

// --------------------------------------------------------------------------------------------------------------------

#define COCOA_VIEW_CLASS_NAME \
    MACRO_NAME(CocoaView_, DISTRHO_PLUGIN_AU_TYPE, _, DISTRHO_PLUGIN_UNIQUE_ID, _, DISTRHO_PLUGIN_BRAND_ID)

@interface COCOA_VIEW_CLASS_NAME : NSView
{
@public
    DPF_UI_AU* ui;
}
@end

@implementation COCOA_VIEW_CLASS_NAME

- (id) initWithPreferredSize:(NSSize)size
{
    ui = nullptr;
    self = [super initWithFrame: NSMakeRect(0, 0, size.width, size.height)];
    [self setHidden:YES];
    return self;
}

- (BOOL) acceptsFirstResponder
{
  return YES;
}

- (void) dealloc
{
    delete ui;
    ui = nullptr;

	[super dealloc];
}

- (BOOL) isFlipped
{
  return YES;
}

@end

// --------------------------------------------------------------------------------------------------------------------

#define COCOA_UI_CLASS_NAME \
    MACRO_NAME(CocoaAUView_, DISTRHO_PLUGIN_AU_TYPE, _, DISTRHO_PLUGIN_UNIQUE_ID, _, DISTRHO_PLUGIN_BRAND_ID)

@interface COCOA_UI_CLASS_NAME : NSObject<AUCocoaUIBase>
{
    COCOA_VIEW_CLASS_NAME* view;
}
@end

@implementation COCOA_UI_CLASS_NAME

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
    Float64 sampleRate = d_nextSampleRate;
    void* instancePointer = nullptr;
    AudioUnitScope scope;
    UInt32 dataSize;

    // fetch direct access pointer
   #if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
    dataSize = sizeof(void*);
    AudioUnitGetProperty(component, 'DPFa', kAudioUnitScope_Global, 0, &instancePointer, &dataSize);
   #endif

    // fetch current sample rate
   #if DISTRHO_PLUGIN_NUM_INPUTS != 0
    dataSize = sizeof(Float64);
    AudioUnitGetProperty(component, kAudioUnitProperty_SampleRate, kAudioUnitScope_Input, 0, &sampleRate, &dataSize);
   #elif DISTRHO_PLUGIN_NUM_OUTPUTS != 0
    dataSize = sizeof(Float64);
    AudioUnitGetProperty(component, kAudioUnitProperty_SampleRate, kAudioUnitScope_Output, 0, &sampleRate, &dataSize);
   #endif

   #if defined(DISTRHO_UI_DEFAULT_WIDTH) && defined(DISTRHO_UI_DEFAULT_HEIGHT)
    inPreferredSize = NSMakeSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT);
   #endif

    // create view
    view = [[[COCOA_VIEW_CLASS_NAME alloc] initWithPreferredSize:inPreferredSize] autorelease];
    view->ui = new DPF_UI_AU(component, view, sampleRate, instancePointer);
    view->ui->postSetup();

    // request data from DSP side
    {
        const uint16_t magic = 1337;
        AudioUnitSetProperty(component, 'DPFi', kAudioUnitScope_Global, 0, &magic, sizeof(uint16_t));
        const uint16_t cancel = 0;
        AudioUnitSetProperty(component, 'DPFi', kAudioUnitScope_Global, 0, &cancel, sizeof(uint16_t));
    }

    return view;

    // maybe unused
    (void)scope;
}

@end

// --------------------------------------------------------------------------------------------------------------------

#undef MACRO_NAME
#undef MACRO_NAME2

// --------------------------------------------------------------------------------------------------------------------
