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

#if defined(DISTRHO_OS_HAIKU)
#elif defined(DISTRHO_OS_MAC)
# import <Cocoa/Cocoa.h>
#elif defined(DISTRHO_OS_WINDOWS)
# define WIN32_CLASS_NAME "DPF-EmbedExternalExampleUI"
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#else
# include <sys/types.h>
# include <X11/Xatom.h>
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# define X11Key_Escape 9
#endif

#if defined(DISTRHO_OS_MAC)
# ifndef __MAC_10_12
#  define NSEventMaskAny NSAnyEventMask
#  define NSWindowStyleMaskClosable NSClosableWindowMask
#  define NSWindowStyleMaskMiniaturizable NSMiniaturizableWindowMask
#  define NSWindowStyleMaskResizable NSResizableWindowMask
#  define NSWindowStyleMaskTitled NSTitledWindowMask
# endif
@interface NSExternalWindow : NSWindow
@end
@implementation NSExternalWindow {
@public
    bool closed;
    bool standalone;
}
- (BOOL)canBecomeKeyWindow { return YES; }
- (BOOL)canBecomeMainWindow { return standalone ? YES : NO; }
- (BOOL)windowShouldClose:(id)_ { closed = true; return YES; }
@end
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

class EmbedExternalExampleUI : public UI
{
#if defined(DISTRHO_OS_HAIKU)
    char d;
#elif defined(DISTRHO_OS_MAC)
    NSView* fView;
    NSExternalWindow* fWindow;
#elif defined(DISTRHO_OS_WINDOWS)
    ::HWND fWindow;
#else
    ::Display* fDisplay;
    ::Window fWindow;
#endif

public:
    EmbedExternalExampleUI()
        : UI(512, 256),
#if defined(DISTRHO_OS_HAIKU)
          d(0)
#elif defined(DISTRHO_OS_MAC)
          fView(nullptr),
          fWindow(nullptr)
#elif defined(DISTRHO_OS_WINDOWS)
          fWindow(nullptr)
#else
          fDisplay(nullptr),
          fWindow(0)
#endif
    {
        const bool standalone = isStandalone();
        d_stdout("isStandalone %d", (int)standalone);

#if defined(DISTRHO_OS_HAIKU)
#elif defined(DISTRHO_OS_MAC)
        NSAutoreleasePool* const pool = [[NSAutoreleasePool alloc]init];
        [NSApplication sharedApplication];

        if (standalone)
        {
            [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
            [NSApp activateIgnoringOtherApps:YES];
        }

        fView = [NSView new];
        DISTRHO_SAFE_ASSERT_RETURN(fView != nullptr,);

        [fView setFrame:NSMakeRect(0, 0, getWidth(), getHeight())];
        [fView setAutoresizesSubviews:YES];
        [fView setWantsLayer:YES];
        [[fView layer] setBackgroundColor:[[NSColor blueColor] CGColor]];

        if (isEmbed())
        {
            [fView retain];
            [(NSView*)getParentWindowHandle() addSubview:fView];
        }
        else
        {
            const ulong styleMask = NSWindowStyleMaskClosable
                                  | NSWindowStyleMaskMiniaturizable
                                  | NSWindowStyleMaskResizable
                                  | NSWindowStyleMaskTitled;

            fWindow = [[[NSExternalWindow alloc]
                         initWithContentRect:[fView frame]
                                   styleMask:styleMask
                                     backing:NSBackingStoreBuffered
                                       defer:NO]retain];
            DISTRHO_SAFE_ASSERT_RETURN(fWindow != nullptr,);

            fWindow->closed = false; // is this needed?
            fWindow->standalone = standalone;
            [fWindow setIsVisible:NO];

            if (NSString* const nsTitle = [[NSString alloc]
                                            initWithBytes:getTitle()
                                                   length:strlen(getTitle())
                                                 encoding:NSUTF8StringEncoding])
                [fWindow setTitle:nsTitle];

            [fWindow setContentView:fView];
            [fWindow setContentSize:NSMakeSize(getWidth(), getHeight())];
            [fWindow makeFirstResponder:fView];
        }

        [pool release];
#elif defined(DISTRHO_OS_WINDOWS)
        WNDCLASS windowClass = {};
        windowClass.style         = CS_OWNDC;
        windowClass.lpfnWndProc   = DefWindowProc;
        windowClass.hInstance     = nullptr;
        windowClass.hIcon         = LoadIcon(nullptr, IDI_APPLICATION);
        windowClass.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        windowClass.lpszClassName = WIN32_CLASS_NAME;
        DISTRHO_SAFE_ASSERT_RETURN(RegisterClass(&windowClass),);

        const int winFlags = isEmbed() ? (WS_CHILD | WS_VISIBLE) : (WS_POPUPWINDOW | WS_CAPTION | WS_SIZEBOX);

        RECT rect = { 0, 0, static_cast<LONG>(getWidth()), static_cast<LONG>(getHeight()) };
        AdjustWindowRectEx(&rect, winFlags, FALSE, WS_EX_TOPMOST);

        fWindow = CreateWindowEx(WS_EX_TOPMOST,
                                 WIN32_CLASS_NAME,
                                 getTitle(),
                                 winFlags,
                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                 rect.right - rect.left,
                                 rect.bottom - rect.top,
                                 (HWND)getParentWindowHandle(),
                                 nullptr, nullptr, nullptr);
        DISTRHO_SAFE_ASSERT_RETURN(fWindow != nullptr,);

        SetWindowLongPtr(fWindow, GWLP_USERDATA, (LONG_PTR)this);
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
#if defined(DISTRHO_OS_HAIKU)
#elif defined(DISTRHO_OS_MAC)
        if (fView == nullptr)
            return;

        if (fWindow != nil)
            [fWindow close];

        [fView release];

        if (fWindow != nil)
            [fWindow release];
#elif defined(DISTRHO_OS_WINDOWS)
        if (fWindow != nullptr)
            DestroyWindow(fWindow);

        UnregisterClass(WIN32_CLASS_NAME, nullptr);
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
        d_stdout("parameterChanged %u %f", index, value);

        switch (index)
        {
        case kParameterWidth:
            setWidth(static_cast<int>(value + 0.5f));
            break;
        case kParameterHeight:
            setHeight(static_cast<int>(value + 0.5f));
            break;
        }
    }

   /* --------------------------------------------------------------------------------------------------------
    * External Window overrides */

    void focus() override
    {
        d_stdout("focus");
#if defined(DISTRHO_OS_HAIKU)
#elif defined(DISTRHO_OS_MAC)
        DISTRHO_SAFE_ASSERT_RETURN(fWindow != nullptr,);

        [fWindow orderFrontRegardless];
        [fWindow makeKeyWindow];
        [fWindow makeFirstResponder:fView];
#elif defined(DISTRHO_OS_WINDOWS)
        DISTRHO_SAFE_ASSERT_RETURN(fWindow != nullptr,);

        SetForegroundWindow(fWindow);
        SetActiveWindow(fWindow);
        SetFocus(fWindow);
#else
        DISTRHO_SAFE_ASSERT_RETURN(fWindow != 0,);

        XRaiseWindow(fDisplay, fWindow);
#endif
    }

    uintptr_t getNativeWindowHandle() const noexcept override
    {
#if defined(DISTRHO_OS_HAIKU)
#elif defined(DISTRHO_OS_MAC)
        return (uintptr_t)fView;
#elif defined(DISTRHO_OS_WINDOWS)
        return (uintptr_t)fWindow;
#else
        return (uintptr_t)fWindow;
#endif
    }

    void sizeChanged(uint width, uint height) override
    {
        d_stdout("sizeChanged %u %u", width, height);
        UI::sizeChanged(width, height);

#if defined(DISTRHO_OS_HAIKU)
#elif defined(DISTRHO_OS_MAC)
        NSRect rect = [fView frame];
        rect.size = CGSizeMake((CGFloat)width, (CGFloat)height);
        [fView setFrame:rect];
#elif defined(DISTRHO_OS_WINDOWS)
        if (fWindow != nullptr)
            SetWindowPos(fWindow,
                         HWND_TOP,
                         0, 0,
                         width, height,
                         SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
#else
        if (fWindow != 0)
            XResizeWindow(fDisplay, fWindow, width, height);
#endif
    }

    void titleChanged(const char* const title) override
    {
        d_stdout("titleChanged %s", title);
#if defined(DISTRHO_OS_HAIKU)
#elif defined(DISTRHO_OS_MAC)
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
#if defined(DISTRHO_OS_HAIKU)
#elif defined(DISTRHO_OS_MAC)
#elif defined(DISTRHO_OS_WINDOWS)
#else
        DISTRHO_SAFE_ASSERT_RETURN(fWindow != 0,);
        XSetTransientForHint(fDisplay, fWindow, (::Window)winId);
#endif
    }

    void visibilityChanged(const bool visible) override
    {
        d_stdout("visibilityChanged %d", visible);
#if defined(DISTRHO_OS_HAIKU)
#elif defined(DISTRHO_OS_MAC)
        DISTRHO_SAFE_ASSERT_RETURN(fView != nullptr,);
        if (fWindow != nil)
        {
            [fWindow setIsVisible:(visible ? YES : NO)];

            if (isStandalone())
                [fWindow makeMainWindow];

            [fWindow makeKeyAndOrderFront:fWindow];
        }
        else
        {
            [fView setHidden:(visible ? NO : YES)];
        }
#elif defined(DISTRHO_OS_WINDOWS)
        DISTRHO_SAFE_ASSERT_RETURN(fWindow != nullptr,);

        ShowWindow(fWindow, visible ? SW_SHOWNORMAL : SW_HIDE);
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
#if defined(DISTRHO_OS_HAIKU)
#elif defined(DISTRHO_OS_MAC)
        if (isEmbed()) {
            return;
        }

        NSAutoreleasePool* const pool = [[NSAutoreleasePool alloc] init];
        NSDate* const date = [NSDate distantPast];

        for (NSEvent* event;;)
        {
            event = [NSApp
                     nextEventMatchingMask:NSEventMaskAny
                                 untilDate:date
                                    inMode:NSDefaultRunLoopMode
                                   dequeue:YES];

            if (event == nil)
                break;

            [NSApp sendEvent:event];
        }

        if (fWindow->closed)
        {
            fWindow->closed = false;
            close();
        }

        [pool release];
#elif defined(DISTRHO_OS_WINDOWS)
        if (fWindow == nullptr)
            return;

        /*
        MSG msg;
        if (! ::PeekMessage(&msg, fWindow, 0, 0, PM_NOREMOVE))
            return true;

        if (::GetMessage(&msg, nullptr, 0, 0) >= 0)
        {
            if (msg.message == WM_QUIT)
                return false;

            //TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        */
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
