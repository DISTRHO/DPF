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
#elif defined(DISTRHO_OS_WINDOWS)
#else
# include <sys/types.h>
# include <X11/Xatom.h>
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# define X11Key_Escape 9
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

class EmbedExternalExampleUI : public UI
{
#if defined(DISTRHO_OS_MAC)
#elif defined(DISTRHO_OS_WINDOWS)
#else
    ::Display* fDisplay;
    ::Window fWindow;
#endif

public:
    EmbedExternalExampleUI()
        : UI(512, 256),
          fDisplay(nullptr),
          fWindow(0),
          fValue(0.0f)
    {
#if defined(DISTRHO_OS_MAC)
#elif defined(DISTRHO_OS_WINDOWS)
#else
        fDisplay = XOpenDisplay(nullptr);
        DISTRHO_SAFE_ASSERT_RETURN(fDisplay != nullptr,);

        const int screen = DefaultScreen(fDisplay);
        const ::Window root   = RootWindow(fDisplay, screen);
        const ::Window parent = isEmbed() ? (::Window)getParentWindowHandle() : root;

        XSetWindowAttributes attr = {};
        attr.event_mask = KeyPressMask|KeyReleaseMask;

        fWindow = XCreateWindow(fDisplay, parent,
                                0, 0, getWidth(), getHeight(),
                                0, DefaultDepth(fDisplay, screen), InputOutput, DefaultVisual(fDisplay, screen),
                                CWColormap | CWEventMask, &attr);
        DISTRHO_SAFE_ASSERT_RETURN(fWindow != 0,);

        XSizeHints sizeHints = {};
        sizeHints.flags      = PMinSize;
        sizeHints.min_width  = getWidth();
        sizeHints.min_height = getHeight();
        XSetNormalHints(fDisplay, fWindow, &sizeHints);

        XStoreName(fDisplay, fWindow, getTitle());

        if (parent == root)
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
    }

    ~EmbedExternalExampleUI()
    {
        if (fDisplay == nullptr)
            return;

        if (fWindow != 0)
            XDestroyWindow(fDisplay, fWindow);

        XCloseDisplay(fDisplay);
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

    void titleChanged(const char* const title) override
    {
        d_stdout("visibilityChanged %s", title);
#if defined(DISTRHO_OS_MAC)
#elif defined(DISTRHO_OS_WINDOWS)
#else
        DISTRHO_SAFE_ASSERT_RETURN(fWindow != 0,);
        XStoreName(fDisplay, fWindow, title);
#endif
    }

    void transientWindowChanged(const uintptr_t winId) override
    {
        d_stdout("transientWindowChanged %lu", winId);
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
#elif defined(DISTRHO_OS_WINDOWS)
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
