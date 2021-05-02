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

#ifndef DGL_WINDOW_PRIVATE_DATA_HPP_INCLUDED
#define DGL_WINDOW_PRIVATE_DATA_HPP_INCLUDED

#include "../Window.hpp"
// #include "ApplicationPrivateData.hpp"

#include "pugl.hpp"

START_NAMESPACE_DGL

class Widget;

// -----------------------------------------------------------------------

struct Window::PrivateData : IdleCallback {
    /** Handy typedef for ... */
    typedef Application::PrivateData AppData;

    /** Direct access to DGL Application private data where we registers ourselves in. */
    AppData* const appData;

    /** Pointer to the DGL Window class that this private data belongs to. */
    Window* const self;

    /** Pugl view instance. */
    PuglView* const view;

    /** Constructor for a regular, standalone window. */
    PrivateData(AppData* appData, Window* self);

    /** Constructor for a regular, standalone window with a transient parent. */
    PrivateData(AppData* appData, Window* self, Window& transientWindow);

    /** Constructor for an embed Window, with a few extra hints from the host side. */
    PrivateData(AppData* appData, Window* self, uintptr_t parentWindowHandle, double scaling, bool resizable);

    /** Destructor. */
    ~PrivateData() override;

    /** Helper initialization function called at the end of all this class constructors. */
    void init(bool resizable);

    /** Hide window and notify application of a window close event.
      * Does nothing if window is embed (that is, not standalone).
      * The application event-loop will stop if all windows have been closed.
      *
      * @note It is possible to hide the window while not stopping event-loop.
      *       A closed window is always hidden, but the reverse is not always true.
      */
    void close();

    void idleCallback() override;

    static PuglStatus puglEventCallback(PuglView* view, const PuglEvent* event);

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrivateData)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#if 0
    // this one depends on build type
    // GraphicsContext fContext;

    bool fFirstInit;
    bool fVisible;
    bool fUsingEmbed;
    double fScaling;
    double fAutoScaling;
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

// #if defined(DISTRHO_OS_HAIKU)
//     BApplication* bApplication;
//     BView*        bView;
//     BWindow*      bWindow;
#if defined(DISTRHO_OS_MAC)
//     NSView<PuglGenericView>* mView;
//     id              mWindow;
//     id              mParentWindow;
# ifndef DGL_FILE_BROWSER_DISABLED
    NSOpenPanel*    fOpenFilePanel;
    id              fFilePanelDelegate;
# endif
#elif defined(DISTRHO_OS_WINDOWS)
//     HWND hwnd;
//     HWND hwndParent;
# ifndef DGL_FILE_BROWSER_DISABLED
    String fSelectedFile;
# endif
#endif
#endif

#if 0 // ndef DPF_TEST_WINDOW_CPP
    // -------------------------------------------------------------------
    // stuff that uses pugl internals or build-specific things

    void init(const bool resizable = false);
    void setVisible(const bool visible);
    void windowSpecificIdle();

    // -------------------------------------------------------------------

    // -------------------------------------------------------------------

    void addWidget(Widget* const widget);
    void removeWidget(Widget* const widget);

    // -------------------------------------------------------------------

    void onPuglClose();
    void onPuglDisplay();
    void onPuglReshape(const int width, const int height);
    void onPuglMouse(const Widget::MouseEvent& ev);

    // -------------------------------------------------------------------
#endif

// #ifdef DISTRHO_DEFINES_H_INCLUDED
//     friend class DISTRHO_NAMESPACE::UI;
// #endif

#if 0
// -----------------------------------------------------------------------
// Window Private

struct Window::PrivateData {
    // -------------------------------------------------------------------

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

    // -------------------------------------------------------------------

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

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrivateData)
};
#endif

// #undef DGL_DBG
// #undef DGL_DBGF

#endif // DGL_WINDOW_PRIVATE_DATA_HPP_INCLUDED
