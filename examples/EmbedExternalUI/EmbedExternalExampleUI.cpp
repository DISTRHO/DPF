/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2021 Filipe Coelho <falktx@falktx.com>
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

// needed for IDE
#include "DistrhoPluginInfo.h"

#include "DistrhoUI.hpp"

#if defined(DISTRHO_OS_MAC)
# import <Cocoa/Cocoa.h>
#elif defined(DISTRHO_OS_WINDOWS)
#else
# include <sys/types.h>
# include <X11/Xatom.h>
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# define X11Key_Escape 9
#endif

#if defined(DISTRHO_OS_MAC)
@interface NSExternalWindow : NSWindow
@end
@implementation NSExternalWindow
- (id)initWithContentRect:(NSRect)contentRect
                styleMask:(unsigned long)aStyle
                  backing:(NSBackingStoreType)bufferingType
                    defer:(BOOL)flag
{
    NSWindow* result = [super initWithContentRect:contentRect
                                        styleMask:aStyle
                                          backing:bufferingType
                                            defer:flag];
    [result setAcceptsMouseMovedEvents:YES];
    return (NSExternalWindow*)result;
}
- (BOOL)canBecomeKeyWindow
{
    return YES;
}
- (BOOL)canBecomeMainWindow
{
    return NO;
}
@end
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

class EmbedExternalExampleUI : public UI
{
#if defined(DISTRHO_OS_MAC)
    NSView* fView;
    NSExternalWindow* fWindow;
#elif defined(DISTRHO_OS_WINDOWS)
#else
    ::Display* fDisplay;
    ::Window fWindow;
#endif

public:
    EmbedExternalExampleUI()
        : UI(512, 256),
#if defined(DISTRHO_OS_MAC)
          fView(nullptr),
          fWindow(nullptr),
#elif defined(DISTRHO_OS_WINDOWS)
#else
          fDisplay(nullptr),
          fWindow(0),
#endif
          fValue(0.0f)
    {
#if defined(DISTRHO_OS_MAC)
        if (isStandalone())
        {
            [[NSApplication sharedApplication]new];
            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
            [NSApp activateIgnoringOtherApps:YES];
        }

        NSAutoreleasePool* const pool = [[NSAutoreleasePool alloc] init];
        [NSApplication sharedApplication];

        fView = [NSView new];
        DISTRHO_SAFE_ASSERT_RETURN(fView != nullptr,);

        [fView initWithFrame:NSMakeRect(0, 0, getWidth(), getHeight())];
        [fView setAutoresizesSubviews:YES];
        [fView setFrame:NSMakeRect(0, 0, getWidth(), getHeight())];
        [fView setHidden:NO];
        [fView setNeedsDisplay:YES];

        if (isEmbed())
        {
            [fView retain];
            [(NSView*)getParentWindowHandle() addSubview:fView];
        }
        else
        {
            fWindow = [[[NSExternalWindow alloc]
                         initWithContentRect:[fView frame]
                                   styleMask:(NSWindowStyleMaskClosable | NSWindowStyleMaskTitled | NSWindowStyleMaskResizable)
                                     backing:NSBackingStoreBuffered
                                       defer:NO]retain];
            DISTRHO_SAFE_ASSERT_RETURN(fWindow != nullptr,);

            // [fWindow setIsVisible:NO];

            if (NSString* const nsTitle = [[NSString alloc]
                                            initWithBytes:getTitle()
                                                   length:strlen(getTitle())
                                                 encoding:NSUTF8StringEncoding])
                [fWindow setTitle:nsTitle];

            [fWindow setContentView:fView];
            [fWindow setContentSize:NSMakeSize(getWidth(), getHeight())];
            [fWindow makeFirstResponder:fView];
            [fWindow makeKeyAndOrderFront:fWindow];
        }

        [pool release];
#elif defined(DISTRHO_OS_WINDOWS)
#else
        fDisplay = XOpenDisplay(nullptr);
        DISTRHO_SAFE_ASSERT_RETURN(fDisplay != nullptr,);

        const ::Window parent = isEmbed()
                              ? (::Window)getParentWindowHandle()
                              : RootWindow(fDisplay, DefaultScreen(fDisplay));

        fWindow = XCreateSimpleWindow(fDisplay, parent, 0, 0, getWidth(), getHeight(), 0, 0, 0);
        DISTRHO_SAFE_ASSERT_RETURN(fWindow != 0,);

        XSizeHints sizeHints = {};
        sizeHints.flags      = PMinSize;
        sizeHints.min_width  = getWidth();
        sizeHints.min_height = getHeight();
        XSetNormalHints(fDisplay, fWindow, &sizeHints);
        XStoreName(fDisplay, fWindow, getTitle());

        if (isEmbed())
        {
            // start with window mapped, so host can access it
            XMapWindow(fDisplay, fWindow);
        }
        else
        {
            // grab Esc key for auto-close
            XGrabKey(fDisplay, X11Key_Escape, AnyModifier, fWindow, 1, GrabModeAsync, GrabModeAsync);

            // destroy window on close
            Atom wmDelete = XInternAtom(fDisplay, "WM_DELETE_WINDOW", True);
            XSetWMProtocols(fDisplay, fWindow, &wmDelete, 1);

            // set pid WM hint
            const pid_t pid = getpid();
            const Atom _nwp = XInternAtom(fDisplay, "_NET_WM_PID", False);
            XChangeProperty(fDisplay, fWindow, _nwp, XA_CARDINAL, 32, PropModeReplace, (const uchar*)&pid, 1);

            // set the window to both dialog and normal to produce a decorated floating dialog
            // order is important: DIALOG needs to come before NORMAL
            const Atom _wt = XInternAtom(fDisplay, "_NET_WM_WINDOW_TYPE", False);
            const Atom _wts[2] = {
                XInternAtom(fDisplay, "_NET_WM_WINDOW_TYPE_DIALOG", False),
                XInternAtom(fDisplay, "_NET_WM_WINDOW_TYPE_NORMAL", False)
            };
            XChangeProperty(fDisplay, fWindow, _wt, XA_ATOM, 32, PropModeReplace, (const uchar*)&_wts, 2);
        }
#endif
        d_stdout("created external window with size %u %u", getWidth(), getHeight());
    }

    ~EmbedExternalExampleUI()
    {
#if defined(DISTRHO_OS_MAC)
        if (fView == nullptr)
            return;

        if (fWindow != nil)
            [fWindow close];

        [fView release];

        if (fWindow != nil)
            [fWindow release];
#elif defined(DISTRHO_OS_WINDOWS)
#else
        if (fDisplay == nullptr)
            return;

        if (fWindow != 0)
            XDestroyWindow(fDisplay, fWindow);

        XCloseDisplay(fDisplay);
#endif
    }

protected:
   /* --------------------------------------------------------------------------------------------------------
    * DSP/Plugin Callbacks */

   /**
      A parameter has changed on the plugin side.
      This is called by the host to inform the UI about parameter changes.
    */
    void parameterChanged(uint32_t index, float value) override
    {
        if (index != 0)
            return;

        fValue = value;
    }

   /* --------------------------------------------------------------------------------------------------------
    * External Window overrides */

    void focus() override
    {
        d_stdout("focus");
#if defined(DISTRHO_OS_MAC)
        DISTRHO_SAFE_ASSERT_RETURN(fWindow != nil,);
        [fWindow orderFrontRegardless];
#elif defined(DISTRHO_OS_WINDOWS)
#else
        DISTRHO_SAFE_ASSERT_RETURN(fWindow != 0,);
        XRaiseWindow(fDisplay, fWindow);
#endif
    }

    uintptr_t getNativeWindowHandle() const noexcept override
    {
#if defined(DISTRHO_OS_MAC)
        return (uintptr_t)fView;
#elif defined(DISTRHO_OS_WINDOWS)
#else
        return (uintptr_t)fWindow;
#endif
        return 0;
    }

    void titleChanged(const char* const title) override
    {
        d_stdout("titleChanged %s", title);
#if defined(DISTRHO_OS_MAC)
        if (fWindow != nil)
        {
            if (NSString* const nsTitle = [[NSString alloc]
                                           initWithBytes:title
                                                  length:strlen(title)
                                                encoding:NSUTF8StringEncoding])
            {
                [fWindow setTitle:nsTitle];
                [nsTitle release];
            }
        }
#elif defined(DISTRHO_OS_WINDOWS)
#else
        DISTRHO_SAFE_ASSERT_RETURN(fWindow != 0,);
        XStoreName(fDisplay, fWindow, title);
#endif
    }

    void transientParentWindowChanged(const uintptr_t winId) override
    {
        d_stdout("transientParentWindowChanged %lu", winId);
#if defined(DISTRHO_OS_MAC)
#elif defined(DISTRHO_OS_WINDOWS)
#else
        DISTRHO_SAFE_ASSERT_RETURN(fWindow != 0,);
        XSetTransientForHint(fDisplay, fWindow, (::Window)winId);
#endif
    }

    void visibilityChanged(const bool visible) override
    {
        d_stdout("visibilityChanged %d", visible);
#if defined(DISTRHO_OS_MAC)
        DISTRHO_SAFE_ASSERT_RETURN(fView != nullptr,);
        if (fWindow != nil)
            [fWindow setIsVisible:(visible ? YES : NO)];
        else
            [fView setHidden:(visible ? NO : YES)];
#elif defined(DISTRHO_OS_WINDOWS)
#else
        DISTRHO_SAFE_ASSERT_RETURN(fWindow != 0,);
        if (visible)
            XMapRaised(fDisplay, fWindow);
        else
            XUnmapWindow(fDisplay, fWindow);
#endif
    }

    void uiIdle() override
    {
        // d_stdout("uiIdle");
#if defined(DISTRHO_OS_MAC)
        NSAutoreleasePool* const pool = [[NSAutoreleasePool alloc] init];
        NSDate* const date = [NSDate distantPast];

        for (NSEvent* event;;)
        {
            event = [NSApp
                     nextEventMatchingMask:NSAnyEventMask
                                 untilDate:date
                                    inMode:NSDefaultRunLoopMode
                                   dequeue:YES];

            if (event == nil)
                break;
            d_stdout("uiIdle with event %p", event);

            [NSApp sendEvent:event];
        }

        [pool release];
#elif defined(DISTRHO_OS_WINDOWS)
        MSG msg;
        if (! ::PeekMessage(&msg, nullptr, 0, 0, PM_NOREMOVE))
            return true;

        if (::GetMessage(&msg, nullptr, 0, 0) >= 0)
        {
            if (msg.message == WM_QUIT)
                return false;

            //TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
#else
        if (fDisplay == nullptr)
            return;

        for (XEvent event; XPending(fDisplay) > 0;)
        {
            XNextEvent(fDisplay, &event);

            if (! isVisible())
                continue;

            switch (event.type)
            {
            case ClientMessage:
                if (char* const type = XGetAtomName(fDisplay, event.xclient.message_type))
                {
                    if (std::strcmp(type, "WM_PROTOCOLS") == 0)
                        hide();
                }
                break;

            case KeyRelease:
                if (event.xkey.keycode == X11Key_Escape)
                    hide();
                break;
            }
        }
#endif
    }

    // -------------------------------------------------------------------------------------------------------

private:
    // Current value, cached for when UI becomes visible
    float fValue;

   /**
      Set our UI class as non-copyable and add a leak detector just in case.
    */
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EmbedExternalExampleUI)
};

/* ------------------------------------------------------------------------------------------------------------
 * UI entry point, called by DPF to create a new UI instance. */

UI* createUI()
{
    return new EmbedExternalExampleUI();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
