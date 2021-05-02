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

#include "WindowPrivateData.hpp"
#include "../Widget.hpp"

#include "pugl.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

Window::PrivateData::PrivateData(Application::PrivateData* const a, Window* const s)
    : appData(a),
      self(s),
      view(puglNewView(appData->world))
{
    init(true);
}

Window::PrivateData::PrivateData(Application::PrivateData* const a, Window* const s, Window& transientWindow)
    : appData(a),
      self(s),
      view(puglNewView(appData->world))
{
    puglSetTransientFor(view, transientWindow.getNativeWindowHandle());
    init(true);
}

Window::PrivateData::PrivateData(Application::PrivateData* const a, Window* const s,
                                 const uintptr_t parentWindowHandle, const double scaling, const bool resizable)
    : appData(a),
      self(s),
      view(puglNewView(appData->world))
{
    // TODO view parent
    init(resizable);
}

Window::PrivateData::~PrivateData()
{
    appData->idleCallbacks.remove(this);
    appData->windows.remove(self);

    if (view != nullptr)
        puglFreeView(view);
}

// -----------------------------------------------------------------------

void Window::PrivateData::init(const bool resizable)
{
    appData->windows.push_back(self);
    appData->idleCallbacks.push_back(this);

    if (view == nullptr)
    {
        /*
        DGL_DBG("Failed!\n");
        */
        return;
    }

#ifdef DGL_CAIRO
    puglSetBackend(view, puglCairoBackend());
#endif
#ifdef DGL_OPENGL
    puglSetBackend(view, puglGlBackend());
#endif
#ifdef DGL_Vulkan
    puglSetBackend(view, puglVulkanBackend());
#endif

    puglSetHandle(view, this);
    puglSetViewHint(view, PUGL_RESIZABLE, resizable ? PUGL_TRUE : PUGL_FALSE);
    puglSetViewHint(view, PUGL_IGNORE_KEY_REPEAT, PUGL_FALSE);
    puglSetEventFunc(view, puglEventCallback);
// #ifndef DGL_FILE_BROWSER_DISABLED
//     puglSetFileSelectedFunc(fView, fileBrowserSelectedCallback);
// #endif

    // DGL_DBG("Success!\n");
}

void Window::PrivateData::close()
{
    /*
    DGL_DBG("Window close\n");

    if (fUsingEmbed)
        return;

    setVisible(false);

    if (! fFirstInit)
    {
        fAppData->oneWindowHidden();
        fFirstInit = true;
    }
    */
}

// -----------------------------------------------------------------------

void Window::PrivateData::idleCallback()
{
}

// -----------------------------------------------------------------------

PuglStatus Window::PrivateData::puglEventCallback(PuglView* const view, const PuglEvent* const event)
{
    Window::PrivateData* const pData = (Window::PrivateData*)puglGetHandle(view);
    return PUGL_SUCCESS;
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#if 0
#ifdef DGL_CAIRO
# define PUGL_CAIRO
# include "../Cairo.hpp"
#endif
#ifdef DGL_OPENGL
# define PUGL_OPENGL
# include "../OpenGL.hpp"
#endif

#ifndef DPF_TEST_WINDOW_CPP
#include "WidgetPrivateData.hpp"
#include "pugl-upstream/include/pugl/pugl.h"
#include "pugl-extra/extras.h"
#endif

extern "C" {
#include "pugl-upstream/src/implementation.c"
#include "pugl-extra/extras.c"
}

#if defined(DISTRHO_OS_HAIKU)
# define DGL_DEBUG_EVENTS
# include "pugl-upstream/src/haiku.cpp"
#elif defined(DISTRHO_OS_MAC)
# define PuglWindow     DISTRHO_JOIN_MACRO(PuglWindow,     DGL_NAMESPACE)
# define PuglOpenGLView DISTRHO_JOIN_MACRO(PuglOpenGLView, DGL_NAMESPACE)
# include "pugl-upstream/src/mac.m"
#elif defined(DISTRHO_OS_WINDOWS)
# include "ppugl-upstream/src/win.c"
# undef max
# undef min
#else
# define DGL_PUGL_USING_X11
extern "C" {
# include "pugl-upstream/src/x11.c"
// # ifdef DGL_CAIRO
// #  include "pugl-upstream/src/x11_cairo.c"
// # endif
# ifdef DGL_OPENGL
#  include "pugl-upstream/src/x11_gl.c"
# endif
# define PUGL_DETAIL_X11_H_INCLUDED
# include "pugl-extra/x11.c"
}
#endif

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define FOR_EACH_WIDGET(it) \
  for (std::list<Widget*>::iterator it = fWidgets.begin(); it != fWidgets.end(); ++it)

#define FOR_EACH_WIDGET_INV(rit) \
  for (std::list<Widget*>::reverse_iterator rit = fWidgets.rbegin(); rit != fWidgets.rend(); ++rit)

#define DGL_DEBUG_EVENTS

#if defined(DEBUG) && defined(DGL_DEBUG_EVENTS)
# define DGL_DBG(msg)  std::fprintf(stderr, "%s", msg);
# define DGL_DBGp(...) std::fprintf(stderr, __VA_ARGS__);
# define DGL_DBGF      std::fflush(stderr);
#else
# define DGL_DBG(msg)
# define DGL_DBGp(...)
# define DGL_DBGF
#endif

START_NAMESPACE_DGL

// Fallback build-specific Window functions
struct Fallback {
    static void onDisplayBefore();
    static void onDisplayAfter();
    static void onReshape(uint width, uint height);
};

// -----------------------------------------------------------------------

Window::PrivateData::PrivateData(Application::PrivateData* const appData, Window* const self)
    : fAppData(appData),
      fSelf(self),
      fView(puglNewView(appData->world)),
      fFirstInit(true),
      fVisible(false),
      fUsingEmbed(false),
      fScaling(1.0),
      fAutoScaling(1.0),
      fWidgets(),
      fModal()
{
    DGL_DBG("Creating window without parent..."); DGL_DBGF;
    init();
}

#ifndef DPF_TEST_WINDOW_CPP
Window::PrivateData::PrivateData(Application::PrivateData* const appData, Window* const self, Window& transientWindow)
    : fAppData(appData),
      fSelf(self),
      fView(puglNewView(appData->world)),
      fFirstInit(true),
      fVisible(false),
      fUsingEmbed(false),
      fScaling(1.0),
      fAutoScaling(1.0),
      fWidgets(),
      fModal(transientWindow.pData)
{
    DGL_DBG("Creating window with parent..."); DGL_DBGF;
    init();

    puglSetTransientFor(fView, transientWindow.getNativeWindowHandle());
}
#endif

Window::PrivateData::PrivateData(Application::PrivateData* const appData, Window* const self,
            const uintptr_t parentWindowHandle,
            const double scaling,
            const bool resizable)
    : fAppData(appData),
      fSelf(self),
      fView(puglNewView(appData->world)),
      fFirstInit(true),
      fVisible(parentWindowHandle != 0),
      fUsingEmbed(parentWindowHandle != 0),
      fScaling(scaling),
      fAutoScaling(1.0),
      fWidgets(),
      fModal()
{
    if (fUsingEmbed)
    {
        DGL_DBG("Creating embedded window..."); DGL_DBGF;
        puglSetParentWindow(fView, parentWindowHandle);
    }
    else
    {
        DGL_DBG("Creating window without parent..."); DGL_DBGF;
    }

    init(resizable);

    if (fUsingEmbed)
    {
        DGL_DBG("NOTE: Embed window is always visible and non-resizable\n");
//             puglShowWindow(fView);
//             fAppData->oneWindowShown();
//             fFirstInit = false;
    }
}

Window::PrivateData::~PrivateData()
{
    DGL_DBG("Destroying window..."); DGL_DBGF;

#if 0
    if (fModal.enabled)
    {
        exec_fini();
        close();
    }
#endif

    fWidgets.clear();

    if (fUsingEmbed)
    {
//             puglHideWindow(fView);
//             fAppData->oneWindowHidden();
    }

    if (fSelf != nullptr)
        fAppData->windows.remove(fSelf);

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

    if (fView != nullptr)
        puglFreeView(fView);

    DGL_DBG("Success!\n");
}

// -----------------------------------------------------------------------

void Window::PrivateData::setVisible(const bool visible)
{
    if (fVisible == visible)
    {
        DGL_DBG("Window setVisible matches current state, ignoring request\n");
        return;
    }
    if (fUsingEmbed)
    {
        DGL_DBG("Window setVisible cannot be called when embedded\n");
        return;
    }

    DGL_DBG("Window setVisible called\n");

    fVisible = visible;

    if (visible)
    {
#if 0 && defined(DISTRHO_OS_MAC)
        if (mWindow != nullptr)
        {
            if (mParentWindow != nullptr)
                [mParentWindow addChildWindow:mWindow
                                      ordered:NSWindowAbove];
        }
#endif

        if (fFirstInit)
        {
            puglRealize(fView);
#ifdef DISTRHO_OS_WINDOWS
            puglShowWindowCentered(fView);
#else
            puglShow(fView);
#endif
            fAppData->oneWindowShown();
            fFirstInit = false;
        }
        else
        {
#ifdef DISTRHO_OS_WINDOWS
            puglWin32RestoreWindow(fView);
#else
            puglShow(fView);
#endif
        }
    }
    else
    {
#if 0 && defined(DISTRHO_OS_MAC)
        if (mWindow != nullptr)
        {
            if (mParentWindow != nullptr)
                [mParentWindow removeChildWindow:mWindow];
        }
#endif

        puglHide(fView);

//             if (fModal.enabled)
//                 exec_fini();
    }
}

// -----------------------------------------------------------------------

void Window::PrivateData::addWidget(Widget* const widget)
{
    fWidgets.push_back(widget);
}

void Window::PrivateData::removeWidget(Widget* const widget)
{
    fWidgets.remove(widget);
}

// -----------------------------------------------------------------------

void Window::PrivateData::onPuglClose()
{
    DGL_DBG("PUGL: onClose\n");

//         if (fModal.enabled)
//             exec_fini();

    fSelf->onClose();

    if (fModal.childFocus != nullptr)
        fModal.childFocus->fSelf->onClose();

    close();
}

void Window::PrivateData::onPuglDisplay()
{
    fSelf->onDisplayBefore();

    if (fWidgets.size() != 0)
    {
        const PuglRect rect = puglGetFrame(fView);
        const int width  = rect.width;
        const int height = rect.height;

        FOR_EACH_WIDGET(it)
        {
            Widget* const widget(*it);
            widget->pData->display(width, height, fAutoScaling, false);
        }
    }

    fSelf->onDisplayAfter();
}

void Window::PrivateData::onPuglReshape(const int width, const int height)
{
    DISTRHO_SAFE_ASSERT_INT2_RETURN(width > 1 && height > 1, width, height,);

    DGL_DBGp("PUGL: onReshape : %i %i\n", width, height);

    fSelf->onReshape(width, height);

    FOR_EACH_WIDGET(it)
    {
        Widget* const widget(*it);

        if (widget->pData->needsFullViewport)
            widget->setSize(width, height);
    }
}

void Window::PrivateData::onPuglMouse(const Widget::MouseEvent& ev)
{
    DGL_DBGp("PUGL: onMouse : %i %i %i %i\n", ev.button, ev.press, ev.pos.getX(), ev.pos.getY());

//         if (fModal.childFocus != nullptr)
//             return fModal.childFocus->focus();

    Widget::MouseEvent rev = ev;
    double x = ev.pos.getX() / fAutoScaling;
    double y = ev.pos.getY() / fAutoScaling;

    FOR_EACH_WIDGET_INV(rit)
    {
        Widget* const widget(*rit);

        rev.pos = Point<double>(x - widget->getAbsoluteX(),
                                y - widget->getAbsoluteY());

        if (widget->isVisible() && widget->onMouse(rev))
            break;
    }
}

// -----------------------------------------------------------------------

void Window::PrivateData::windowSpecificIdle()
{
#if defined(DISTRHO_OS_WINDOWS) && !defined(DGL_FILE_BROWSER_DISABLED)
    if (fSelectedFile.isNotEmpty())
    {
        char* const buffer = fSelectedFile.getAndReleaseBuffer();
        fView->fileSelectedFunc(fView, buffer);
        std::free(buffer);
    }
#endif

    if (fModal.enabled && fModal.parent != nullptr)
        fModal.parent->windowSpecificIdle();
}

// -----------------------------------------------------------------------

static inline int
printModifiers(const uint32_t mods)
{
	return fprintf(stderr, "Modifiers:%s%s%s%s\n",
	               (mods & PUGL_MOD_SHIFT) ? " Shift"   : "",
	               (mods & PUGL_MOD_CTRL)  ? " Ctrl"    : "",
	               (mods & PUGL_MOD_ALT)   ? " Alt"     : "",
	               (mods & PUGL_MOD_SUPER) ? " Super" : "");
}

static inline int
printEvent(const PuglEvent* event, const char* prefix, const bool verbose)
{
#define FFMT            "%6.1f"
#define PFMT            FFMT " " FFMT
#define PRINT(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)

	switch (event->type) {
	case PUGL_NOTHING:
		return 0;
	case PUGL_KEY_PRESS:
		return PRINT("%sKey press   code %3u key  U+%04X\n",
		             prefix,
		             event->key.keycode,
		             event->key.key);
	case PUGL_KEY_RELEASE:
		return PRINT("%sKey release code %3u key  U+%04X\n",
		             prefix,
		             event->key.keycode,
		             event->key.key);
	case PUGL_TEXT:
		return PRINT("%sText entry  code %3u char U+%04X (%s)\n",
		             prefix,
		             event->text.keycode,
		             event->text.character,
		             event->text.string);
	case PUGL_BUTTON_PRESS:
	case PUGL_BUTTON_RELEASE:
		return (PRINT("%sMouse %u %s at " PFMT " ",
		              prefix,
		              event->button.button,
		              (event->type == PUGL_BUTTON_PRESS) ? "down" : "up  ",
		              event->button.x,
		              event->button.y) +
		        printModifiers(event->scroll.state));
	case PUGL_SCROLL:
		return (PRINT("%sScroll %5.1f %5.1f at " PFMT " ",
		              prefix,
		              event->scroll.dx,
		              event->scroll.dy,
		              event->scroll.x,
		              event->scroll.y) +
		        printModifiers(event->scroll.state));
	case PUGL_POINTER_IN:
		return PRINT("%sMouse enter  at " PFMT "\n",
		             prefix,
		             event->crossing.x,
		             event->crossing.y);
	case PUGL_POINTER_OUT:
		return PRINT("%sMouse leave  at " PFMT "\n",
		             prefix,
		             event->crossing.x,
		             event->crossing.y);
	case PUGL_FOCUS_IN:
		return PRINT("%sFocus in %i\n",
		             prefix,
		             event->focus.mode);
	case PUGL_FOCUS_OUT:
		return PRINT("%sFocus out %i\n",
		             prefix,
		             event->focus.mode);
	case PUGL_CLIENT:
		return PRINT("%sClient %" PRIXPTR " %" PRIXPTR "\n",
		             prefix,
		             event->client.data1,
		             event->client.data2);
	case PUGL_TIMER:
		return PRINT("%sTimer %" PRIuPTR "\n", prefix, event->timer.id);
	default:
		break;
	}

	if (verbose) {
		switch (event->type) {
		case PUGL_CREATE:
			return fprintf(stderr, "%sCreate\n", prefix);
		case PUGL_DESTROY:
			return fprintf(stderr, "%sDestroy\n", prefix);
		case PUGL_MAP:
			return fprintf(stderr, "%sMap\n", prefix);
		case PUGL_UNMAP:
			return fprintf(stderr, "%sUnmap\n", prefix);
		case PUGL_UPDATE:
			return 0; // fprintf(stderr, "%sUpdate\n", prefix);
		case PUGL_CONFIGURE:
			return PRINT("%sConfigure " PFMT " " PFMT "\n",
			             prefix,
			             event->configure.x,
			             event->configure.y,
			             event->configure.width,
			             event->configure.height);
		case PUGL_EXPOSE:
			return PRINT("%sExpose    " PFMT " " PFMT "\n",
			             prefix,
			             event->expose.x,
			             event->expose.y,
			             event->expose.width,
			             event->expose.height);
		case PUGL_CLOSE:
			return PRINT("%sClose\n", prefix);
		case PUGL_MOTION:
			return PRINT("%sMouse motion at " PFMT "\n",
			             prefix,
			             event->motion.x,
			             event->motion.y);
		default:
			return PRINT("%sUnknown event type %d\n", prefix, (int)event->type);
		}
	}

#undef PRINT
#undef PFMT
#undef FFMT

	return 0;
}

PuglStatus Window::PrivateData::puglEventCallback(PuglView* const view, const PuglEvent* const event)
{
    printEvent(event, "", true);
    Window::PrivateData* const pData = (Window::PrivateData*)puglGetHandle(view);

    switch (event->type)
    {
    ///< No event
    case PUGL_NOTHING:
        break;

    ///< View created, a #PuglEventCreate
    case PUGL_CREATE:
#ifdef DGL_PUGL_USING_X11
        if (! pData->fUsingEmbed)
            puglExtraSetWindowTypeAndPID(view);
#endif
        break;

    ///< View destroyed, a #PuglEventDestroy
    case PUGL_DESTROY:
        break;

    ///< View moved/resized, a #PuglEventConfigure
    case PUGL_CONFIGURE:
        pData->onPuglReshape(event->configure.width, event->configure.height);
        break;

    case PUGL_MAP:            ///< View made visible, a #PuglEventMap
    case PUGL_UNMAP:          ///< View made invisible, a #PuglEventUnmap
        break;

    ///< View ready to draw, a #PuglEventUpdate
    case PUGL_UPDATE:
        break;

    ///< View must be drawn, a #PuglEventExpose
    case PUGL_EXPOSE:
        pData->onPuglDisplay();
        break;

    ///< View will be closed, a #PuglEventClose
    case PUGL_CLOSE:
        pData->onPuglClose();
        break;

    case PUGL_BUTTON_PRESS:   ///< Mouse button pressed, a #PuglEventButton
    case PUGL_BUTTON_RELEASE: ///< Mouse button released, a #PuglEventButton
    {
        Widget::MouseEvent ev;
        ev.mod    = event->button.state;
        ev.flags  = event->button.flags;
        ev.time   = static_cast<uint>(event->button.time * 1000.0 + 0.5);
        ev.button = event->button.button;
        ev.press  = event->type == PUGL_BUTTON_PRESS;
        ev.pos    = Point<double>(event->button.x, event->button.y);
        pData->onPuglMouse(ev);
        break;
    }

    case PUGL_FOCUS_IN:       ///< Keyboard focus entered view, a #PuglEventFocus
    case PUGL_FOCUS_OUT:      ///< Keyboard focus left view, a #PuglEventFocus
    case PUGL_KEY_PRESS:      ///< Key pressed, a #PuglEventKey
    case PUGL_KEY_RELEASE:    ///< Key released, a #PuglEventKey
    case PUGL_TEXT:           ///< Character entered, a #PuglEventText
    case PUGL_POINTER_IN:     ///< Pointer entered view, a #PuglEventCrossing
    case PUGL_POINTER_OUT:    ///< Pointer left view, a #PuglEventCrossing
    case PUGL_MOTION:         ///< Pointer moved, a #PuglEventMotion
    case PUGL_SCROLL:         ///< Scrolled, a #PuglEventScroll
    case PUGL_CLIENT:         ///< Custom client message, a #PuglEventClient
    case PUGL_TIMER:          ///< Timer triggered, a #PuglEventTimer
    case PUGL_LOOP_ENTER:     ///< Recursive loop entered, a #PuglEventLoopEnter
    case PUGL_LOOP_LEAVE:     ///< Recursive loop left, a #PuglEventLoopLeave
        break;
    }

    return PUGL_SUCCESS;
}

// -----------------------------------------------------------------------

void Window::PrivateData::Fallback::onDisplayBefore()
{
#ifdef DGL_OPENGL
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
#endif
}

void Window::PrivateData::Fallback::onDisplayAfter()
{
}

void Window::PrivateData::Fallback::onReshape(const uint width, const uint height)
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
#else
    // unused
    (void)width;
    (void)height;
#endif
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
#endif
