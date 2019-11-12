/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2019 Filipe Coelho <falktx@falktx.com>
 * Copyright (C) 2019 Jean Pierre Cimalando <jp-dev@inbox.ru>
 * Copyright (C) 2019 Robin Gareus <robin@gareus.org>
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

// we need this for now
//#define PUGL_GRAB_FOCUS 1

#include "../Base.hpp"

#ifdef DGL_CAIRO
# define PUGL_CAIRO
# include "../Cairo.hpp"
#endif
#ifdef DGL_OPENGL
# define PUGL_OPENGL
# include "../OpenGL.hpp"
#endif

#include "pugl/pugl.h"

#if defined(__GNUC__) && (__GNUC__ >= 7)
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#endif

#if defined(DISTRHO_OS_HAIKU)
# define DGL_DEBUG_EVENTS
# include "pugl/pugl_haiku.cpp"
#elif defined(DISTRHO_OS_MAC)
# define PuglWindow     DISTRHO_JOIN_MACRO(PuglWindow,     DGL_NAMESPACE)
# define PuglOpenGLView DISTRHO_JOIN_MACRO(PuglOpenGLView, DGL_NAMESPACE)
# include "pugl/pugl_osx.m"
#elif defined(DISTRHO_OS_WINDOWS)
# include "pugl/pugl_win.cpp"
# undef max
# undef min
#else
# include <sys/types.h>
# include <unistd.h>
extern "C" {
# include "pugl/pugl_x11.c"
}
#endif

#if defined(__GNUC__) && (__GNUC__ >= 7)
# pragma GCC diagnostic pop
#endif

#include "ApplicationPrivateData.hpp"
#include "WidgetPrivateData.hpp"
#include "../StandaloneWindow.hpp"
#include "../../distrho/extra/String.hpp"

#define FOR_EACH_WIDGET(it) \
  for (std::list<Widget*>::iterator it = fWidgets.begin(); it != fWidgets.end(); ++it)

#define FOR_EACH_WIDGET_INV(rit) \
  for (std::list<Widget*>::reverse_iterator rit = fWidgets.rbegin(); rit != fWidgets.rend(); ++rit)

#if defined(DEBUG) && defined(DGL_DEBUG_EVENTS)
# define DBG(msg)  std::fprintf(stderr, "%s", msg);
# define DBGp(...) std::fprintf(stderr, __VA_ARGS__);
# define DBGF      std::fflush(stderr);
#else
# define DBG(msg)
# define DBGp(...)
# define DBGF
#endif

START_NAMESPACE_DGL

// -----------------------------------------------------------------------
// Window Private

struct Window::PrivateData {
    PrivateData(Application& app, Window* const self)
        : fApp(app),
          fSelf(self),
          fView(puglInit()),
          fFirstInit(true),
          fVisible(false),
          fResizable(true),
          fUsingEmbed(false),
          fWidth(1),
          fHeight(1),
          fScaling(1.0),
          fAutoScaling(1.0),
          fTitle(nullptr),
          fWidgets(),
          fModal(),
#if defined(DISTRHO_OS_HAIKU)
          bApplication(nullptr),
          bView(nullptr),
          bWindow(nullptr)
#elif defined(DISTRHO_OS_MAC)
          fNeedsIdle(true),
          mView(nullptr),
          mWindow(nullptr),
          mParentWindow(nullptr)
# ifndef DGL_FILE_BROWSER_DISABLED
        , fOpenFilePanel(nullptr),
          fFilePanelDelegate(nullptr)
# endif
#elif defined(DISTRHO_OS_WINDOWS)
          hwnd(nullptr),
          hwndParent(nullptr)
#else
          xDisplay(nullptr),
          xWindow(0)
#endif
    {
        DBG("Creating window without parent..."); DBGF;
        init();
    }

    PrivateData(Application& app, Window* const self, Window& parent)
        : fApp(app),
          fSelf(self),
          fView(puglInit()),
          fFirstInit(true),
          fVisible(false),
          fResizable(true),
          fUsingEmbed(false),
          fWidth(1),
          fHeight(1),
          fScaling(1.0),
          fAutoScaling(1.0),
          fTitle(nullptr),
          fWidgets(),
          fModal(parent.pData),
#if defined(DISTRHO_OS_HAIKU)
          bApplication(nullptr),
          bView(nullptr),
          bWindow(nullptr)
#elif defined(DISTRHO_OS_MAC)
          fNeedsIdle(false),
          mView(nullptr),
          mWindow(nullptr),
          mParentWindow(nullptr)
# ifndef DGL_FILE_BROWSER_DISABLED
        , fOpenFilePanel(nullptr),
          fFilePanelDelegate(nullptr)
# endif
#elif defined(DISTRHO_OS_WINDOWS)
          hwnd(nullptr),
          hwndParent(nullptr)
#else
          xDisplay(nullptr),
          xWindow(0)
#endif
    {
        DBG("Creating window with parent..."); DBGF;
        init();

        const PuglInternals* const parentImpl(parent.pData->fView->impl);

        // NOTE: almost a 1:1 copy of setTransientWinId()
#if defined(DISTRHO_OS_HAIKU)
        // TODO
#elif defined(DISTRHO_OS_MAC)
        mParentWindow = parentImpl->window;
#elif defined(DISTRHO_OS_WINDOWS)
        hwndParent = parentImpl->hwnd;
        SetWindowLongPtr(hwnd, GWLP_HWNDPARENT, (LONG_PTR)hwndParent);
#else
        XSetTransientForHint(xDisplay, xWindow, parentImpl->win);
#endif
    }

    PrivateData(Application& app, Window* const self, const intptr_t parentId, const double scaling, const bool resizable)
        : fApp(app),
          fSelf(self),
          fView(puglInit()),
          fFirstInit(true),
          fVisible(parentId != 0),
          fResizable(resizable),
          fUsingEmbed(parentId != 0),
          fWidth(1),
          fHeight(1),
          fScaling(scaling),
          fAutoScaling(1.0),
          fTitle(nullptr),
          fWidgets(),
          fModal(),
#if defined(DISTRHO_OS_HAIKU)
          bApplication(nullptr),
          bView(nullptr),
          bWindow(nullptr)
#elif defined(DISTRHO_OS_MAC)
          fNeedsIdle(parentId == 0),
          mView(nullptr),
          mWindow(nullptr),
          mParentWindow(nullptr)
# ifndef DGL_FILE_BROWSER_DISABLED
        , fOpenFilePanel(nullptr),
          fFilePanelDelegate(nullptr)
# endif
#elif defined(DISTRHO_OS_WINDOWS)
          hwnd(nullptr),
          hwndParent(nullptr)
#else
          xDisplay(nullptr),
          xWindow(0)
#endif
    {
        if (fUsingEmbed)
        {
            DBG("Creating embedded window..."); DBGF;
            puglInitWindowParent(fView, parentId);
        }
        else
        {
            DBG("Creating window without parent..."); DBGF;
        }

        init();

        if (fUsingEmbed)
        {
            DBG("NOTE: Embed window is always visible and non-resizable\n");
            puglShowWindow(fView);
            fApp.pData->oneShown();
            fFirstInit = false;
        }
    }

    void init()
    {
        if (fSelf == nullptr || fView == nullptr)
        {
            DBG("Failed!\n");
            return;
        }

        puglInitUserResizable(fView, fResizable);
        puglInitWindowSize(fView, static_cast<int>(fWidth), static_cast<int>(fHeight));

        puglSetHandle(fView, this);
        puglSetDisplayFunc(fView, onDisplayCallback);
        puglSetKeyboardFunc(fView, onKeyboardCallback);
        puglSetMotionFunc(fView, onMotionCallback);
        puglSetMouseFunc(fView, onMouseCallback);
        puglSetScrollFunc(fView, onScrollCallback);
        puglSetSpecialFunc(fView, onSpecialCallback);
        puglSetReshapeFunc(fView, onReshapeCallback);
        puglSetCloseFunc(fView, onCloseCallback);
#ifndef DGL_FILE_BROWSER_DISABLED
        puglSetFileSelectedFunc(fView, fileBrowserSelectedCallback);
#endif

        puglCreateWindow(fView, nullptr);

        PuglInternals* impl = fView->impl;

#if defined(DISTRHO_OS_HAIKU)
        bApplication = impl->app;
        bView        = impl->view;
        bWindow      = impl->window;
#elif defined(DISTRHO_OS_MAC)
        mView   = impl->view;
        mWindow = impl->window;
        DISTRHO_SAFE_ASSERT(mView != nullptr);
        if (fUsingEmbed) {
            DISTRHO_SAFE_ASSERT(mWindow == nullptr);
        } else {
            DISTRHO_SAFE_ASSERT(mWindow != nullptr);
        }
#elif defined(DISTRHO_OS_WINDOWS)
        hwnd = impl->hwnd;
        DISTRHO_SAFE_ASSERT(hwnd != 0);
#else
        xDisplay = impl->display;
        xWindow  = impl->win;
        DISTRHO_SAFE_ASSERT(xWindow != 0);

        if (! fUsingEmbed)
        {
            const pid_t pid = getpid();
            const Atom _nwp = XInternAtom(xDisplay, "_NET_WM_PID", False);
            XChangeProperty(xDisplay, xWindow, _nwp, XA_CARDINAL, 32, PropModeReplace, (const uchar*)&pid, 1);

            const Atom _wt = XInternAtom(xDisplay, "_NET_WM_WINDOW_TYPE", False);

            // Setting the window to both dialog and normal will produce a decorated floating dialog
            // Order is important: DIALOG needs to come before NORMAL
            const Atom _wts[2] = {
                XInternAtom(xDisplay, "_NET_WM_WINDOW_TYPE_DIALOG", False),
                XInternAtom(xDisplay, "_NET_WM_WINDOW_TYPE_NORMAL", False)
            };
            XChangeProperty(xDisplay, xWindow, _wt, XA_ATOM, 32, PropModeReplace, (const uchar*)&_wts, 2);
        }
#endif
        puglEnterContext(fView);

        fApp.pData->windows.push_back(fSelf);

        DBG("Success!\n");
    }

    ~PrivateData()
    {
        DBG("Destroying window..."); DBGF;

        if (fModal.enabled)
        {
            exec_fini();
            close();
        }

        fWidgets.clear();

        if (fUsingEmbed)
        {
            puglHideWindow(fView);
            fApp.pData->oneHidden();
        }

        if (fSelf != nullptr)
        {
            fApp.pData->windows.remove(fSelf);
            fSelf = nullptr;
        }

        if (fView != nullptr)
        {
            puglDestroy(fView);
            fView = nullptr;
        }

        if (fTitle != nullptr)
        {
            std::free(fTitle);
            fTitle = nullptr;
        }

#if defined(DISTRHO_OS_HAIKU)
        bApplication = nullptr;
        bView        = nullptr;
        bWindow      = nullptr;
#elif defined(DISTRHO_OS_MAC)
        mView   = nullptr;
        mWindow = nullptr;
#elif defined(DISTRHO_OS_WINDOWS)
        hwnd = 0;
#else
        xDisplay = nullptr;
        xWindow  = 0;
#endif

#if defined(DISTRHO_OS_MAC) && !defined(DGL_FILE_BROWSER_DISABLED)
        if (fOpenFilePanel)
        {
            [fOpenFilePanel release];
            fOpenFilePanel = nullptr;
        }
        if (fFilePanelDelegate)
        {
            [fFilePanelDelegate release];
            fFilePanelDelegate = nullptr;
        }
#endif

        DBG("Success!\n");
    }

    // -------------------------------------------------------------------

    void close()
    {
        DBG("Window close\n");

        if (fUsingEmbed)
            return;

        setVisible(false);

        if (! fFirstInit)
        {
            fApp.pData->oneHidden();
            fFirstInit = true;
        }
    }

    void exec(const bool lockWait)
    {
        DBG("Window exec\n");
        exec_init();

        if (lockWait)
        {
            for (; fVisible && fModal.enabled;)
            {
                idle();
                d_msleep(10);
            }

            exec_fini();
        }
        else
        {
            idle();
        }
    }

    // -------------------------------------------------------------------

    void exec_init()
    {
        DBG("Window modal loop starting..."); DBGF;
        DISTRHO_SAFE_ASSERT_RETURN(fModal.parent != nullptr, setVisible(true));

        fModal.enabled = true;
        fModal.parent->fModal.childFocus = this;

        fModal.parent->setVisible(true);
        setVisible(true);

        DBG("Ok\n");
    }

    void exec_fini()
    {
        DBG("Window modal loop stopping..."); DBGF;
        fModal.enabled = false;

        if (fModal.parent != nullptr)
        {
            fModal.parent->fModal.childFocus = nullptr;

            // the mouse position probably changed since the modal appeared,
            // so send a mouse motion event to the modal's parent window
#if defined(DISTRHO_OS_HAIKU)
            // TODO
#elif defined(DISTRHO_OS_MAC)
            // TODO
#elif defined(DISTRHO_OS_WINDOWS)
            // TODO
#else
            int i, wx, wy;
            uint u;
            ::Window w;
            if (XQueryPointer(fModal.parent->xDisplay, fModal.parent->xWindow, &w, &w, &i, &i, &wx, &wy, &u) == True)
                fModal.parent->onPuglMotion(wx, wy);
#endif
        }

        DBG("Ok\n");
    }

    // -------------------------------------------------------------------

    void focus()
    {
        DBG("Window focus\n");
#if defined(DISTRHO_OS_HAIKU)
        if (bWindow != nullptr)
        {
            if (bWindow->LockLooper())
            {
                bWindow->Activate(true);
                bWindow->UnlockLooper();
            }
        }
        else
        {
            bView->MakeFocus(true);
        }
#elif defined(DISTRHO_OS_MAC)
        if (mWindow != nullptr)
            [mWindow makeKeyWindow];
#elif defined(DISTRHO_OS_WINDOWS)
        SetForegroundWindow(hwnd);
        SetActiveWindow(hwnd);
        SetFocus(hwnd);
#else
        XRaiseWindow(xDisplay, xWindow);
        XSetInputFocus(xDisplay, xWindow, RevertToPointerRoot, CurrentTime);
        XFlush(xDisplay);
#endif
    }

    // -------------------------------------------------------------------

    void setVisible(const bool yesNo)
    {
        if (fVisible == yesNo)
        {
            DBG("Window setVisible matches current state, ignoring request\n");
            return;
        }
        if (fUsingEmbed)
        {
            DBG("Window setVisible cannot be called when embedded\n");
            return;
        }

        DBG("Window setVisible called\n");

        fVisible = yesNo;

        if (yesNo && fFirstInit)
            setSize(fWidth, fHeight, true);

#if defined(DISTRHO_OS_HAIKU)
        if (bWindow != nullptr)
        {
            if (bWindow->LockLooper())
            {
                if (yesNo)
                    bWindow->Show();
                else
                    bWindow->Hide();

                // TODO use flush?
                bWindow->Sync();
                bWindow->UnlockLooper();
            }
        }
        else
        {
            if (yesNo)
                bView->Show();
            else
                bView->Hide();
        }
#elif defined(DISTRHO_OS_MAC)
        if (yesNo)
        {
            if (mWindow != nullptr)
            {
                if (mParentWindow != nullptr)
                    [mParentWindow addChildWindow:mWindow
                                          ordered:NSWindowAbove];

                [mWindow setIsVisible:YES];
            }
            else
            {
                [mView setHidden:NO];
            }
        }
        else
        {
            if (mWindow != nullptr)
            {
                if (mParentWindow != nullptr)
                    [mParentWindow removeChildWindow:mWindow];

                [mWindow setIsVisible:NO];
            }
            else
            {
                [mView setHidden:YES];
            }
        }
#elif defined(DISTRHO_OS_WINDOWS)
        if (yesNo)
        {
            if (fFirstInit)
            {
                RECT rectChild, rectParent;

                if (hwndParent != nullptr &&
                    GetWindowRect(hwnd, &rectChild) &&
                    GetWindowRect(hwndParent, &rectParent))
                {
                    SetWindowPos(hwnd, hwndParent,
                                 rectParent.left + (rectChild.right-rectChild.left)/2,
                                 rectParent.top + (rectChild.bottom-rectChild.top)/2,
                                 0, 0, SWP_SHOWWINDOW|SWP_NOSIZE);
                }
                else
                {
                    ShowWindow(hwnd, SW_SHOWNORMAL);
                }
            }
            else
            {
                ShowWindow(hwnd, SW_RESTORE);
            }
        }
        else
        {
            ShowWindow(hwnd, SW_HIDE);
        }

        UpdateWindow(hwnd);
#else
        if (yesNo)
            XMapRaised(xDisplay, xWindow);
        else
            XUnmapWindow(xDisplay, xWindow);

        XFlush(xDisplay);
#endif

        if (yesNo)
        {
            if (fFirstInit)
            {
                fApp.pData->oneShown();
                fFirstInit = false;
            }
        }
        else if (fModal.enabled)
            exec_fini();
    }

    // -------------------------------------------------------------------

    void setResizable(const bool yesNo)
    {
        if (fResizable == yesNo)
        {
            DBG("Window setResizable matches current state, ignoring request\n");
            return;
        }
        if (fUsingEmbed)
        {
            DBG("Window setResizable cannot be called when embedded\n");
            return;
        }

        DBG("Window setResizable called\n");

        fResizable = yesNo;
        fView->user_resizable = yesNo;

#if defined(DISTRHO_OS_HAIKU)
        // TODO
        // B_NO_BORDER
        // B_TITLED_WINDOW_LOOK
        // bWindow->SetFlags();
#elif defined(DISTRHO_OS_MAC)
        const uint flags = yesNo ? (NSViewWidthSizable|NSViewHeightSizable) : 0x0;
        [mView setAutoresizingMask:flags];
#elif defined(DISTRHO_OS_WINDOWS)
        const int winFlags = fResizable ? GetWindowLong(hwnd, GWL_STYLE) |  WS_SIZEBOX
                                        : GetWindowLong(hwnd, GWL_STYLE) & ~WS_SIZEBOX;
        SetWindowLong(hwnd, GWL_STYLE, winFlags);
#endif

        setSize(fWidth, fHeight, true);
    }

    // -------------------------------------------------------------------

    void setGeometryConstraints(uint width, uint height, bool aspect)
    {
        // Did you forget to set DISTRHO_UI_USER_RESIZABLE ?
        DISTRHO_SAFE_ASSERT_RETURN(fResizable,);

        fView->min_width  = width;
        fView->min_height = height;
        puglUpdateGeometryConstraints(fView, width, height, aspect);
    }

    // -------------------------------------------------------------------

    void setSize(uint width, uint height, const bool forced = false)
    {
        if (width <= 1 || height <= 1)
        {
            DBGp("Window setSize called with invalid value(s) %i %i, ignoring request\n", width, height);
            return;
        }

        if (fWidth == width && fHeight == height && ! forced)
        {
            DBGp("Window setSize matches current size, ignoring request (%i %i)\n", width, height);
            return;
        }

        fWidth  = width;
        fHeight = height;

        DBGp("Window setSize called %s, size %i %i, resizable %s\n", forced ? "(forced)" : "(not forced)", width, height, fResizable?"true":"false");

#if defined(DISTRHO_OS_HAIKU)
        bView->ResizeTo(width, height);

        if (bWindow != nullptr && bWindow->LockLooper())
        {
            bWindow->MoveTo(50, 50);
            bWindow->ResizeTo(width, height);

            if (! forced)
                bWindow->Flush();

            bWindow->UnlockLooper();
        }
        // TODO resizable
#elif defined(DISTRHO_OS_MAC)
        [mView setFrame:NSMakeRect(0, 0, width, height)];

        if (mWindow != nullptr)
        {
            const NSSize size = NSMakeSize(width, height);
            [mWindow setContentSize:size];

            if (fResizable)
            {
                [mWindow setContentMinSize:NSMakeSize(1, 1)];
                [mWindow setContentMaxSize:NSMakeSize(99999, 99999)];
                [[mWindow standardWindowButton:NSWindowZoomButton] setHidden:NO];
            }
            else
            {
                [mWindow setContentMinSize:size];
                [mWindow setContentMaxSize:size];
                [[mWindow standardWindowButton:NSWindowZoomButton] setHidden:YES];
            }
        }
#elif defined(DISTRHO_OS_WINDOWS)
        const int winFlags = WS_POPUPWINDOW | WS_CAPTION | (fResizable ? WS_SIZEBOX : 0x0);
        RECT wr = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
        AdjustWindowRectEx(&wr, fUsingEmbed ? WS_CHILD : winFlags, FALSE, WS_EX_TOPMOST);

        SetWindowPos(hwnd, 0, 0, 0, wr.right-wr.left, wr.bottom-wr.top,
                     SWP_NOACTIVATE|SWP_NOMOVE|SWP_NOOWNERZORDER|SWP_NOZORDER);

        if (! forced)
            UpdateWindow(hwnd);
#else

        if (! fResizable)
        {
            XSizeHints sizeHints;
            memset(&sizeHints, 0, sizeof(sizeHints));

            sizeHints.flags      = PSize|PMinSize|PMaxSize;
            sizeHints.width      = static_cast<int>(width);
            sizeHints.height     = static_cast<int>(height);
            sizeHints.min_width  = static_cast<int>(width);
            sizeHints.min_height = static_cast<int>(height);
            sizeHints.max_width  = static_cast<int>(width);
            sizeHints.max_height = static_cast<int>(height);

            XSetWMNormalHints(xDisplay, xWindow, &sizeHints);
        }

        XResizeWindow(xDisplay, xWindow, width, height);

        if (! forced)
            XFlush(xDisplay);
#endif

        puglPostRedisplay(fView);
    }

    // -------------------------------------------------------------------

    const char* getTitle() const noexcept
    {
        static const char* const kFallback = "";

        return fTitle != nullptr ? fTitle : kFallback;
    }

    void setTitle(const char* const title)
    {
        DBGp("Window setTitle \"%s\"\n", title);

        if (fTitle != nullptr)
            std::free(fTitle);

        fTitle = strdup(title);

#if defined(DISTRHO_OS_HAIKU)
        if (bWindow != nullptr&& bWindow->LockLooper())
        {
            bWindow->SetTitle(title);
            bWindow->UnlockLooper();
        }
#elif defined(DISTRHO_OS_MAC)
        if (mWindow != nullptr)
        {
            NSString* titleString = [[NSString alloc]
                                      initWithBytes:title
                                             length:strlen(title)
                                          encoding:NSUTF8StringEncoding];

            [mWindow setTitle:titleString];
        }
#elif defined(DISTRHO_OS_WINDOWS)
        SetWindowTextA(hwnd, title);
#else
        XStoreName(xDisplay, xWindow, title);
        Atom netWmName = XInternAtom(xDisplay, "_NET_WM_NAME", False);
        Atom utf8String = XInternAtom(xDisplay, "UTF8_STRING", False);
        XChangeProperty(xDisplay, xWindow, netWmName, utf8String, 8, PropModeReplace, (unsigned char *)title, strlen(title));
#endif
    }

    void setTransientWinId(const uintptr_t winId)
    {
        DISTRHO_SAFE_ASSERT_RETURN(winId != 0,);

#if defined(DISTRHO_OS_HAIKU)
        // TODO
#elif defined(DISTRHO_OS_MAC)
        NSWindow* const parentWindow = [NSApp windowWithWindowNumber:winId];
        DISTRHO_SAFE_ASSERT_RETURN(parentWindow != nullptr,);

        [parentWindow addChildWindow:mWindow
                             ordered:NSWindowAbove];
#elif defined(DISTRHO_OS_WINDOWS)
        hwndParent = (HWND)winId;
        SetWindowLongPtr(hwnd, GWLP_HWNDPARENT, (LONG_PTR)winId);
#else
        XSetTransientForHint(xDisplay, xWindow, static_cast< ::Window>(winId));
#endif
    }

    // -------------------------------------------------------------------

    double getScaling() const noexcept
    {
        return fScaling;
    }

    void setAutoScaling(const double scaling) noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(scaling > 0.0,);

        fAutoScaling = scaling;
    }

    // -------------------------------------------------------------------

    bool getIgnoringKeyRepeat() const noexcept
    {
        return fView->ignoreKeyRepeat;
    }

    void setIgnoringKeyRepeat(bool ignore) noexcept
    {
        puglIgnoreKeyRepeat(fView, ignore);
    }

    // -------------------------------------------------------------------

    void addWidget(Widget* const widget)
    {
        fWidgets.push_back(widget);
    }

    void removeWidget(Widget* const widget)
    {
        fWidgets.remove(widget);
    }

    void idle()
    {
        puglProcessEvents(fView);

#ifdef DISTRHO_OS_HAIKU
        if (bApplication != nullptr)
        {
            // bApplication->Lock();
            // bApplication->Loop();
            // bApplication->Unlock();
        }
#endif

#ifdef DISTRHO_OS_MAC
        if (fNeedsIdle)
        {
            NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
            NSEvent* event;

            for (;;)
            {
                event = [NSApp
                         nextEventMatchingMask:NSAnyEventMask
                                     untilDate:[NSDate distantPast]
                                        inMode:NSDefaultRunLoopMode
                                       dequeue:YES];

                if (event == nil)
                    break;

                [NSApp sendEvent: event];
            }

            [pool release];
        }
#endif

#if defined(DISTRHO_OS_WINDOWS) && !defined(DGL_FILE_BROWSER_DISABLED)
        if (fSelectedFile.isNotEmpty())
        {
            char* const buffer = fSelectedFile.getAndReleaseBuffer();
            fView->fileSelectedFunc(fView, buffer);
            std::free(buffer);
        }
#endif

        if (fModal.enabled && fModal.parent != nullptr)
            fModal.parent->idle();
    }

    // -------------------------------------------------------------------

    void onPuglDisplay()
    {
        fSelf->onDisplayBefore();

        FOR_EACH_WIDGET(it)
        {
            Widget* const widget(*it);
            widget->pData->display(fWidth, fHeight, fAutoScaling, false);
        }

        fSelf->onDisplayAfter();
    }

    int onPuglKeyboard(const bool press, const uint key)
    {
        DBGp("PUGL: onKeyboard : %i %i\n", press, key);

        if (fModal.childFocus != nullptr)
        {
            fModal.childFocus->focus();
            return 0;
        }

        Widget::KeyboardEvent ev;
        ev.press = press;
        ev.key  = key;
        ev.mod  = static_cast<Modifier>(puglGetModifiers(fView));
        ev.time = puglGetEventTimestamp(fView);

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);

            if (widget->isVisible() && widget->onKeyboard(ev))
                return 0;
        }

        return 1;
    }

    int onPuglSpecial(const bool press, const Key key)
    {
        DBGp("PUGL: onSpecial : %i %i\n", press, key);

        if (fModal.childFocus != nullptr)
        {
            fModal.childFocus->focus();
            return 0;
        }

        Widget::SpecialEvent ev;
        ev.press = press;
        ev.key   = key;
        ev.mod   = static_cast<Modifier>(puglGetModifiers(fView));
        ev.time  = puglGetEventTimestamp(fView);

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);

            if (widget->isVisible() && widget->onSpecial(ev))
                return 0;
        }

        return 1;
    }

    void onPuglMouse(const int button, const bool press, int x, int y)
    {
        DBGp("PUGL: onMouse : %i %i %i %i\n", button, press, x, y);

        // FIXME - pugl sends 2 of these for each window on init, don't ask me why. we'll ignore it
        if (press && button == 0 && x == 0 && y == 0) return;

        if (fModal.childFocus != nullptr)
            return fModal.childFocus->focus();

        x /= fAutoScaling;
        y /= fAutoScaling;

        Widget::MouseEvent ev;
        ev.button = button;
        ev.press  = press;
        ev.mod    = static_cast<Modifier>(puglGetModifiers(fView));
        ev.time   = puglGetEventTimestamp(fView);

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);

            ev.pos = Point<int>(x-widget->getAbsoluteX(), y-widget->getAbsoluteY());

            if (widget->isVisible() && widget->onMouse(ev))
                break;
        }
    }

    void onPuglMotion(int x, int y)
    {
        // DBGp("PUGL: onMotion : %i %i\n", x, y);

        if (fModal.childFocus != nullptr)
            return;

        x /= fAutoScaling;
        y /= fAutoScaling;

        Widget::MotionEvent ev;
        ev.mod  = static_cast<Modifier>(puglGetModifiers(fView));
        ev.time = puglGetEventTimestamp(fView);

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);

            ev.pos = Point<int>(x-widget->getAbsoluteX(), y-widget->getAbsoluteY());

            if (widget->isVisible() && widget->onMotion(ev))
                break;
        }
    }

    void onPuglScroll(int x, int y, float dx, float dy)
    {
        DBGp("PUGL: onScroll : %i %i %f %f\n", x, y, dx, dy);

        if (fModal.childFocus != nullptr)
            return;

        x /= fAutoScaling;
        y /= fAutoScaling;
        dx /= fAutoScaling;
        dy /= fAutoScaling;

        Widget::ScrollEvent ev;
        ev.delta = Point<float>(dx, dy);
        ev.mod   = static_cast<Modifier>(puglGetModifiers(fView));
        ev.time  = puglGetEventTimestamp(fView);

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);

            ev.pos = Point<int>(x-widget->getAbsoluteX(), y-widget->getAbsoluteY());

            if (widget->isVisible() && widget->onScroll(ev))
                break;
        }
    }

    void onPuglReshape(const int width, const int height)
    {
        DBGp("PUGL: onReshape : %i %i\n", width, height);

        if (width <= 1 && height <= 1)
            return;

        fWidth  = static_cast<uint>(width);
        fHeight = static_cast<uint>(height);

        fSelf->onReshape(fWidth, fHeight);

        FOR_EACH_WIDGET(it)
        {
            Widget* const widget(*it);

            if (widget->pData->needsFullViewport)
                widget->setSize(fWidth, fHeight);
        }
    }

    void onPuglClose()
    {
        DBG("PUGL: onClose\n");

        if (fModal.enabled)
            exec_fini();

        fSelf->onClose();

        if (fModal.childFocus != nullptr)
            fModal.childFocus->fSelf->onClose();

        close();
    }

    // -------------------------------------------------------------------

    bool handlePluginKeyboard(const bool press, const uint key)
    {
        DBGp("PUGL: handlePluginKeyboard : %i %i\n", press, key);

        if (fModal.childFocus != nullptr)
        {
            fModal.childFocus->focus();
            return true;
        }

        Widget::KeyboardEvent ev;
        ev.press = press;
        ev.key   = key;
        ev.mod   = static_cast<Modifier>(fView->mods);
        ev.time  = 0;

        if ((ev.mod & kModifierShift) != 0 && ev.key >= 'a' && ev.key <= 'z')
            ev.key -= 'a' - 'A'; // a-z -> A-Z

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);

            if (widget->isVisible() && widget->onKeyboard(ev))
                return true;
        }

        return false;
    }

    bool handlePluginSpecial(const bool press, const Key key)
    {
        DBGp("PUGL: handlePluginSpecial : %i %i\n", press, key);

        if (fModal.childFocus != nullptr)
        {
            fModal.childFocus->focus();
            return true;
        }

        int mods = 0x0;

        switch (key)
        {
        case kKeyShift:
            mods |= kModifierShift;
            break;
        case kKeyControl:
            mods |= kModifierControl;
            break;
        case kKeyAlt:
            mods |= kModifierAlt;
            break;
        default:
            break;
        }

        if (mods != 0x0)
        {
            if (press)
                fView->mods |= mods;
            else
                fView->mods &= ~(mods);
        }

        Widget::SpecialEvent ev;
        ev.press = press;
        ev.key   = key;
        ev.mod   = static_cast<Modifier>(fView->mods);
        ev.time  = 0;

        FOR_EACH_WIDGET_INV(rit)
        {
            Widget* const widget(*rit);

            if (widget->isVisible() && widget->onSpecial(ev))
                return true;
        }

        return false;
    }

#if defined(DISTRHO_OS_MAC) && !defined(DGL_FILE_BROWSER_DISABLED)
    static void openPanelDidEnd(NSOpenPanel* panel, int returnCode, void *userData)
    {
        PrivateData* pData = (PrivateData*)userData;

        if (returnCode == NSOKButton)
        {
            NSArray* urls = [panel URLs];
            NSURL* fileUrl = nullptr;

            for (NSUInteger i = 0, n = [urls count]; i < n && !fileUrl; ++i)
            {
                NSURL* url = (NSURL*)[urls objectAtIndex:i];
                if ([url isFileURL])
                    fileUrl = url;
            }

            if (fileUrl)
            {
                PuglView* view = pData->fView;
                if (view->fileSelectedFunc)
                {
                    const char* fileName = [fileUrl.path UTF8String];
                    view->fileSelectedFunc(view, fileName);
                }
            }
        }

        [pData->fOpenFilePanel release];
        pData->fOpenFilePanel = nullptr;
    }
#endif

    // -------------------------------------------------------------------

    Application&    fApp;
    Window*         fSelf;
    GraphicsContext fContext;
    PuglView*       fView;

    bool fFirstInit;
    bool fVisible;
    bool fResizable;
    bool fUsingEmbed;
    uint fWidth;
    uint fHeight;
    double fScaling;
    double fAutoScaling;
    char* fTitle;
    std::list<Widget*> fWidgets;

    struct Modal {
        bool enabled;
        PrivateData* parent;
        PrivateData* childFocus;

        Modal()
            : enabled(false),
              parent(nullptr),
              childFocus(nullptr) {}

        Modal(PrivateData* const p)
            : enabled(false),
              parent(p),
              childFocus(nullptr) {}

        ~Modal()
        {
            DISTRHO_SAFE_ASSERT(! enabled);
            DISTRHO_SAFE_ASSERT(childFocus == nullptr);
        }

        DISTRHO_DECLARE_NON_COPY_STRUCT(Modal)
    } fModal;

#if defined(DISTRHO_OS_HAIKU)
    BApplication* bApplication;
    BView*        bView;
    BWindow*      bWindow;
#elif defined(DISTRHO_OS_MAC)
    bool            fNeedsIdle;
    NSView<PuglGenericView>* mView;
    id              mWindow;
    id              mParentWindow;
# ifndef DGL_FILE_BROWSER_DISABLED
    NSOpenPanel*    fOpenFilePanel;
    id              fFilePanelDelegate;
# endif
#elif defined(DISTRHO_OS_WINDOWS)
    HWND hwnd;
    HWND hwndParent;
# ifndef DGL_FILE_BROWSER_DISABLED
    String fSelectedFile;
# endif
#else
    Display* xDisplay;
    ::Window xWindow;
#endif

    // -------------------------------------------------------------------
    // Callbacks

    #define handlePtr ((PrivateData*)puglGetHandle(view))

    static void onDisplayCallback(PuglView* view)
    {
        handlePtr->onPuglDisplay();
    }

    static int onKeyboardCallback(PuglView* view, bool press, uint32_t key)
    {
        return handlePtr->onPuglKeyboard(press, key);
    }

    static int onSpecialCallback(PuglView* view, bool press, PuglKey key)
    {
        return handlePtr->onPuglSpecial(press, static_cast<Key>(key));
    }

    static void onMouseCallback(PuglView* view, int button, bool press, int x, int y)
    {
        handlePtr->onPuglMouse(button, press, x, y);
    }

    static void onMotionCallback(PuglView* view, int x, int y)
    {
        handlePtr->onPuglMotion(x, y);
    }

    static void onScrollCallback(PuglView* view, int x, int y, float dx, float dy)
    {
        handlePtr->onPuglScroll(x, y, dx, dy);
    }

    static void onReshapeCallback(PuglView* view, int width, int height)
    {
        handlePtr->onPuglReshape(width, height);
    }

    static void onCloseCallback(PuglView* view)
    {
        handlePtr->onPuglClose();
    }

#ifndef DGL_FILE_BROWSER_DISABLED
    static void fileBrowserSelectedCallback(PuglView* view, const char* filename)
    {
        handlePtr->fSelf->fileBrowserSelected(filename);
    }
#endif

    #undef handlePtr

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrivateData)
};

// -----------------------------------------------------------------------
// Window

Window::Window(Application& app)
    : pData(new PrivateData(app, this)) {}

Window::Window(Application& app, Window& parent)
    : pData(new PrivateData(app, this, parent)) {}

Window::Window(Application& app, intptr_t parentId, double scaling, bool resizable)
    : pData(new PrivateData(app, this, parentId, scaling, resizable)) {}

Window::~Window()
{
    delete pData;
}

void Window::show()
{
    pData->setVisible(true);
}

void Window::hide()
{
    pData->setVisible(false);
}

void Window::close()
{
    pData->close();
}

void Window::exec(bool lockWait)
{
    pData->exec(lockWait);
}

void Window::focus()
{
    pData->focus();
}

void Window::repaint() noexcept
{
    puglPostRedisplay(pData->fView);
}

// static int fib_filter_filename_filter(const char* const name)
// {
//     return 1;
//     (void)name;
// }

#ifndef DGL_FILE_BROWSER_DISABLED

#ifdef DISTRHO_OS_MAC
END_NAMESPACE_DGL
@interface FilePanelDelegate : NSObject
{
    void (*fCallback)(NSOpenPanel*, int, void*);
    void* fUserData;
}
-(id)initWithCallback:(void(*)(NSOpenPanel*, int, void*))callback userData:(void*)userData;
-(void)openPanelDidEnd:(NSOpenPanel *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo;
@end
START_NAMESPACE_DGL
#endif

bool Window::openFileBrowser(const FileBrowserOptions& options)
{
# ifdef SOFD_HAVE_X11
    using DISTRHO_NAMESPACE::String;

    // --------------------------------------------------------------------------
    // configure start dir

    // TODO: get abspath if needed
    // TODO: cross-platform

    String startDir(options.startDir);

#  ifdef DISTRHO_OS_LINUX
    if (startDir.isEmpty())
    {
        if (char* const dir_name = get_current_dir_name())
        {
            startDir = dir_name;
            std::free(dir_name);
        }
    }
#  endif

    DISTRHO_SAFE_ASSERT_RETURN(startDir.isNotEmpty(), false);

    if (! startDir.endsWith('/'))
        startDir += "/";

    DISTRHO_SAFE_ASSERT_RETURN(x_fib_configure(0, startDir) == 0, false);

    // --------------------------------------------------------------------------
    // configure title

    String title(options.title);

    if (title.isEmpty())
    {
        title = pData->getTitle();

        if (title.isEmpty())
            title = "FileBrowser";
    }

    DISTRHO_SAFE_ASSERT_RETURN(x_fib_configure(1, title) == 0, false);

    // --------------------------------------------------------------------------
    // configure filters

    x_fib_cfg_filter_callback(nullptr); //fib_filter_filename_filter);

    // --------------------------------------------------------------------------
    // configure buttons

    x_fib_cfg_buttons(3, options.buttons.listAllFiles-1);
    x_fib_cfg_buttons(1, options.buttons.showHidden-1);
    x_fib_cfg_buttons(2, options.buttons.showPlaces-1);

    // --------------------------------------------------------------------------
    // show

    return (x_fib_show(pData->xDisplay, pData->xWindow, /*options.width*/0, /*options.height*/0) == 0);
# elif defined(DISTRHO_OS_WINDOWS)
    // the old and compatible dialog API
    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = pData->hwnd;

    // set initial directory in UTF-16 coding
    std::vector<WCHAR> startDirW;
    if (options.startDir)
    {
        startDirW.resize(strlen(options.startDir) + 1);
        if (MultiByteToWideChar(CP_UTF8, 0, options.startDir, -1, startDirW.data(), startDirW.size()))
            ofn.lpstrInitialDir = startDirW.data();
    }

    // set title in UTF-16 coding
    std::vector<WCHAR> titleW;
    if (options.title)
    {
        titleW.resize(strlen(options.title) + 1);
        if (MultiByteToWideChar(CP_UTF8, 0, options.title, -1, titleW.data(), titleW.size()))
            ofn.lpstrTitle = titleW.data();
    }

    // prepare a buffer to receive the result
    std::vector<WCHAR> fileNameW(32768); // the Unicode maximum
    ofn.lpstrFile = fileNameW.data();
    ofn.nMaxFile = (DWORD)fileNameW.size();

    // TODO synchronous only, can't do better with WinAPI native dialogs.
    // threading might work, if someone is motivated to risk it.
    if (GetOpenFileNameW(&ofn))
    {
        // back to UTF-8
        std::vector<char> fileNameA(4 * 32768);
        if (WideCharToMultiByte(CP_UTF8, 0, fileNameW.data(), -1, fileNameA.data(), (int)fileNameA.size(), nullptr, nullptr))
        {
            // handle it during the next idle cycle (fake async)
            pData->fSelectedFile = fileNameA.data();
        }
    }

    return true;
# elif defined(DISTRHO_OS_MAC)
    if (pData->fOpenFilePanel) // permit one dialog at most
    {
        [pData->fOpenFilePanel makeKeyAndOrderFront:nil];
        return false;
    }

    NSOpenPanel* panel = [NSOpenPanel openPanel];
    pData->fOpenFilePanel = [panel retain];

    [panel setCanChooseFiles:YES];
    [panel setCanChooseDirectories:NO];
    [panel setAllowsMultipleSelection:NO];

    if (options.startDir)
        [panel setDirectory:[NSString stringWithUTF8String:options.startDir]];

    if (options.title)
    {
        NSString* titleString = [[NSString alloc]
            initWithBytes:options.title
                   length:strlen(options.title)
                 encoding:NSUTF8StringEncoding];
        [panel setTitle:titleString];
    }

    id delegate = pData->fFilePanelDelegate;
    if (!delegate)
    {
        delegate = [[FilePanelDelegate alloc] initWithCallback:&PrivateData::openPanelDidEnd
                                                      userData:pData];
        pData->fFilePanelDelegate = [delegate retain];
    }

    [panel beginSheetForDirectory:nullptr
                             file:nullptr
                   modalForWindow:nullptr
                    modalDelegate:delegate
                   didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:)
                      contextInfo:nullptr];

    return true;
# else
    // not implemented
    return false;

    // unused
    (void)options;
# endif
}

#ifdef DISTRHO_OS_MAC
END_NAMESPACE_DGL
@implementation FilePanelDelegate
-(id)initWithCallback:(void(*)(NSOpenPanel*, int, void*))callback userData:(void *)userData
{
    [super init];
    self->fCallback = callback;
    self->fUserData = userData;
    return self;
}

-(void)openPanelDidEnd:(NSOpenPanel *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
    self->fCallback(sheet, returnCode, self->fUserData);
    (void)contextInfo;
}
@end
START_NAMESPACE_DGL
#endif

#endif // !defined(DGL_FILE_BROWSER_DISABLED)

bool Window::isEmbed() const noexcept
{
    return pData->fUsingEmbed;
}

bool Window::isVisible() const noexcept
{
    return pData->fVisible;
}

void Window::setVisible(bool yesNo)
{
    pData->setVisible(yesNo);
}

bool Window::isResizable() const noexcept
{
    return pData->fResizable;
}

void Window::setResizable(bool yesNo)
{
    pData->setResizable(yesNo);
}

void Window::setGeometryConstraints(uint width, uint height, bool aspect)
{
    pData->setGeometryConstraints(width, height, aspect);
}

uint Window::getWidth() const noexcept
{
    return pData->fWidth;
}

uint Window::getHeight() const noexcept
{
    return pData->fHeight;
}

Size<uint> Window::getSize() const noexcept
{
    return Size<uint>(pData->fWidth, pData->fHeight);
}

void Window::setSize(uint width, uint height)
{
    pData->setSize(width, height);
}

void Window::setSize(Size<uint> size)
{
    pData->setSize(size.getWidth(), size.getHeight());
}

const char* Window::getTitle() const noexcept
{
    return pData->getTitle();
}

void Window::setTitle(const char* title)
{
    pData->setTitle(title);
}

void Window::setTransientWinId(uintptr_t winId)
{
    pData->setTransientWinId(winId);
}

double Window::getScaling() const noexcept
{
    return pData->getScaling();
}

bool Window::getIgnoringKeyRepeat() const noexcept
{
    return pData->getIgnoringKeyRepeat();
}

void Window::setIgnoringKeyRepeat(bool ignore) noexcept
{
    pData->setIgnoringKeyRepeat(ignore);
}

Application& Window::getApp() const noexcept
{
    return pData->fApp;
}

intptr_t Window::getWindowId() const noexcept
{
    return puglGetNativeWindow(pData->fView);
}

const GraphicsContext& Window::getGraphicsContext() const noexcept
{
    GraphicsContext& context = pData->fContext;
#ifdef DGL_CAIRO
    context.cairo = (cairo_t*)puglGetContext(pData->fView);
#endif
    return context;
}

void Window::_setAutoScaling(double scaling) noexcept
{
    pData->setAutoScaling(scaling);
}

void Window::_addWidget(Widget* const widget)
{
    pData->addWidget(widget);
}

void Window::_removeWidget(Widget* const widget)
{
    pData->removeWidget(widget);
}

void Window::_idle()
{
    pData->idle();
}

// -----------------------------------------------------------------------

void Window::addIdleCallback(IdleCallback* const callback)
{
    DISTRHO_SAFE_ASSERT_RETURN(callback != nullptr,)

    pData->fApp.pData->idleCallbacks.push_back(callback);
}

void Window::removeIdleCallback(IdleCallback* const callback)
{
    DISTRHO_SAFE_ASSERT_RETURN(callback != nullptr,)

    pData->fApp.pData->idleCallbacks.remove(callback);
}

// -----------------------------------------------------------------------

void Window::onDisplayBefore()
{
#ifdef DGL_OPENGL
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
#endif
}

void Window::onDisplayAfter()
{
}

void Window::onReshape(uint width, uint height)
{
#ifdef DGL_OPENGL
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, static_cast<GLdouble>(width), static_cast<GLdouble>(height), 0.0, 0.0, 1.0);
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
#endif
}

void Window::onClose()
{
}

#ifndef DGL_FILE_BROWSER_DISABLED
void Window::fileBrowserSelected(const char*)
{
}
#endif

bool Window::handlePluginKeyboard(const bool press, const uint key)
{
    return pData->handlePluginKeyboard(press, key);
}

bool Window::handlePluginSpecial(const bool press, const Key key)
{
    return pData->handlePluginSpecial(press, key);
}

// -----------------------------------------------------------------------

StandaloneWindow::StandaloneWindow()
    : Application(),
      Window((Application&)*this),
      fWidget(nullptr) {}

void StandaloneWindow::exec()
{
    Window::show();
    Application::exec();
}

void StandaloneWindow::onReshape(uint width, uint height)
{
    if (fWidget != nullptr)
        fWidget->setSize(width, height);
    Window::onReshape(width, height);
}

void StandaloneWindow::_addWidget(Widget* widget)
{
    if (fWidget == nullptr)
    {
        fWidget = widget;
        fWidget->pData->needsFullViewport = true;
    }
    Window::_addWidget(widget);
}

void StandaloneWindow::_removeWidget(Widget* widget)
{
    if (fWidget == widget)
    {
        fWidget->pData->needsFullViewport = false;
        fWidget = nullptr;
    }
    Window::_removeWidget(widget);
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#undef DBG
#undef DBGF
