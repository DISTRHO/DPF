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
#include "TopLevelWidgetPrivateData.hpp"

#include "pugl.hpp"

#include <cinttypes>

START_NAMESPACE_DGL

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

#define DEFAULT_WIDTH 640
#define DEFAULT_HEIGHT 480

// -----------------------------------------------------------------------

Window::PrivateData::PrivateData(Application& a, Window* const s)
    : app(a),
      appData(a.pData),
      self(s),
      view(puglNewView(appData->world)),
      topLevelWidget(nullptr),
      isClosed(true),
      isVisible(false),
      isEmbed(false),
      scaling(1.0),
      autoScaling(1.0),
      pendingVisibility(kPendingVisibilityNone)
{
    init(DEFAULT_WIDTH, DEFAULT_HEIGHT, false);
}

Window::PrivateData::PrivateData(Application& a, Window* const s, Window& transientWindow)
    : app(a),
      appData(a.pData),
      self(s),
      view(puglNewView(appData->world)),
      topLevelWidget(nullptr),
      isClosed(true),
      isVisible(false),
      isEmbed(false),
      scaling(1.0),
      autoScaling(1.0),
      pendingVisibility(kPendingVisibilityNone)
{
    init(DEFAULT_WIDTH, DEFAULT_HEIGHT, false);

    puglSetTransientFor(view, transientWindow.getNativeWindowHandle());
}

Window::PrivateData::PrivateData(Application& a, Window* const s,
                                 const uintptr_t parentWindowHandle,
                                 const double scale, const bool resizable)
    : app(a),
      appData(a.pData),
      self(s),
      view(puglNewView(appData->world)),
      topLevelWidget(nullptr),
      isClosed(parentWindowHandle == 0),
      isVisible(parentWindowHandle != 0),
      isEmbed(parentWindowHandle != 0),
      scaling(scale),
      autoScaling(1.0),
      pendingVisibility(kPendingVisibilityNone)
{
    if (isEmbed)
    {
        // puglSetDefaultSize(DEFAULT_WIDTH, DEFAULT_HEIGHT, height);
        puglSetParentWindow(view, parentWindowHandle);
    }

    init(DEFAULT_WIDTH, DEFAULT_HEIGHT, resizable);

    if (isEmbed)
    {
        appData->oneWindowShown();
        puglShow(view);
    }
}

Window::PrivateData::PrivateData(Application& a, Window* const s,
                                 const uintptr_t parentWindowHandle,
                                 const uint width, const uint height,
                                 const double scale, const bool resizable)
    : app(a),
      appData(a.pData),
      self(s),
      view(puglNewView(appData->world)),
      topLevelWidget(nullptr),
      isClosed(parentWindowHandle == 0),
      isVisible(parentWindowHandle != 0),
      isEmbed(parentWindowHandle != 0),
      scaling(scale),
      autoScaling(1.0),
      pendingVisibility(kPendingVisibilityNone)
{
    if (isEmbed)
    {
        puglSetDefaultSize(view, width, height);
        puglSetParentWindow(view, parentWindowHandle);
    }

    init(width, height, resizable);

    if (isEmbed)
    {
        appData->oneWindowShown();
        puglShow(view);
    }
}

Window::PrivateData::~PrivateData()
{
    if (isEmbed)
    {
        puglHide(view);
        appData->oneWindowClosed();
        isClosed = true;
        isVisible = false;
    }

    appData->idleCallbacks.remove(this);
    appData->windows.remove(self);

    if (view != nullptr)
        puglFreeView(view);
}

// -----------------------------------------------------------------------

void Window::PrivateData::init(const uint width, const uint height, const bool resizable)
{
    appData->windows.push_back(self);
    appData->idleCallbacks.push_back(this);
    memset(graphicsContext, 0, sizeof(graphicsContext));

    if (view == nullptr)
    {
        DGL_DBG("Failed to create Pugl view, everything will fail!\n");
        return;
    }

    puglSetMatchingBackendForCurrentBuild(view);

    puglSetHandle(view, this);
    puglSetViewHint(view, PUGL_RESIZABLE, resizable ? PUGL_TRUE : PUGL_FALSE);
    puglSetViewHint(view, PUGL_IGNORE_KEY_REPEAT, PUGL_FALSE);
    puglSetViewHint(view, PUGL_DEPTH_BITS, 16);
    puglSetViewHint(view, PUGL_STENCIL_BITS, 8);
    // PUGL_SAMPLES ??
    puglSetEventFunc(view, puglEventCallback);
// #ifndef DGL_FILE_BROWSER_DISABLED
//     puglSetFileSelectedFunc(fView, fileBrowserSelectedCallback);
// #endif

    PuglRect rect = puglGetFrame(view);
    rect.width = width;
    rect.height = height;
    puglSetFrame(view, rect);

    // FIXME this is bad
    puglRealize(view);
    puglBackendEnter(view);
}

// -----------------------------------------------------------------------

void Window::PrivateData::show()
{
    if (isVisible)
    {
        DGL_DBG("Window show matches current visible state, ignoring request\n");
        return;
    }
    if (isEmbed)
    {
        DGL_DBG("Window show cannot be called when embedded\n");
        return;
    }

    DGL_DBG("Window show called\n");

#if 0 && defined(DISTRHO_OS_MAC)
//     if (mWindow != nullptr)
//     {
//         if (mParentWindow != nullptr)
//             [mParentWindow addChildWindow:mWindow
//                                   ordered:NSWindowAbove];
//     }
#endif

    if (isClosed)
    {
        isClosed = false;
        appData->oneWindowShown();

#ifdef DISTRHO_OS_WINDOWS
        puglWin32ShowWindowCentered(view);
#else
        puglShow(view);
#endif
        /*
        pendingVisibility = kPendingVisibilityShow;
        const PuglStatus status = puglRealize(view);
        DISTRHO_SAFE_ASSERT_INT_RETURN(status == PUGL_SUCCESS, status, close());
        */
    }
    else
    {
#ifdef DISTRHO_OS_WINDOWS
        puglWin32RestoreWindow(view);
#else
        puglShow(view);
#endif
    }

    isVisible = true;
}

void Window::PrivateData::hide()
{
    if (isEmbed)
    {
        DGL_DBG("Window hide cannot be called when embedded\n");
        return;
    }

    pendingVisibility = kPendingVisibilityHide;

    if (! isVisible)
    {
        DGL_DBG("Window hide matches current visible state, ignoring request\n");
        return;
    }

    DGL_DBG("Window hide called\n");

#if 0 && defined(DISTRHO_OS_MAC)
//     if (mWindow != nullptr)
//     {
//         if (mParentWindow != nullptr)
//             [mParentWindow removeChildWindow:mWindow];
//     }
#endif

    puglHide(view);

//     if (fModal.enabled)
//         exec_fini();

    isVisible = false;
}

// -----------------------------------------------------------------------

void Window::PrivateData::close()
{
    DGL_DBG("Window close\n");

    if (isEmbed || isClosed)
        return;

    isClosed = true;
    hide();
    appData->oneWindowClosed();
}

// -----------------------------------------------------------------------

void Window::PrivateData::idleCallback()
{
// #if defined(DISTRHO_OS_WINDOWS) && !defined(DGL_FILE_BROWSER_DISABLED)
//     if (fSelectedFile.isNotEmpty())
//     {
//         char* const buffer = fSelectedFile.getAndReleaseBuffer();
//         fView->fileSelectedFunc(fView, buffer);
//         std::free(buffer);
//     }
// #endif
//
//     if (fModal.enabled && fModal.parent != nullptr)
//         fModal.parent->windowSpecificIdle();
//     self->repaint();
}

// -----------------------------------------------------------------------

void Window::PrivateData::onPuglDisplay()
{
    DGL_DBGp("PUGL: onPuglDisplay : %p\n", topLevelWidget);

    puglOnDisplayPrepare(view);

#ifndef DPF_TEST_WINDOW_CPP
    if (topLevelWidget != nullptr)
        topLevelWidget->pData->display();
#endif
}

void Window::PrivateData::onPuglReshape(const int width, const int height)
{
    DISTRHO_SAFE_ASSERT_INT2_RETURN(width > 1 && height > 1, width, height,);

    DGL_DBGp("PUGL: onReshape : %i %i\n", width, height);

    self->onReshape(width, height);

#ifndef DPF_TEST_WINDOW_CPP
    if (topLevelWidget != nullptr)
        topLevelWidget->setSize(width, height);
#endif
}

void Window::PrivateData::onPuglCreate()
{
    DGL_DBGp("PUGL: onPuglCreate %i\n", pendingVisibility);

    if (pendingVisibility != kPendingVisibilityShow)
        return;

    pendingVisibility = kPendingVisibilityNone;
#ifdef DISTRHO_OS_WINDOWS
    puglWin32ShowWindowCentered(view);
#else
    puglShow(view);
#endif
}

void Window::PrivateData::onPuglClose()
{
    DGL_DBG("PUGL: onClose\n");

//         if (fModal.enabled)
//             exec_fini();

    if (! self->onClose())
        return;

//     if (fModal.childFocus != nullptr)
//         fModal.childFocus->fSelf->onClose();

    close();
}

void Window::PrivateData::onPuglMouse(const Events::MouseEvent& ev)
{
    DGL_DBGp("PUGL: onMouse : %i %i %f %f\n", ev.button, ev.press, ev.pos.getX(), ev.pos.getY());

//         if (fModal.childFocus != nullptr)
//             return fModal.childFocus->focus();

#ifndef DPF_TEST_WINDOW_CPP
    if (topLevelWidget != nullptr)
        topLevelWidget->pData->mouseEvent(ev);
#endif
}

static int printEvent(const PuglEvent* event, const char* prefix, const bool verbose);

PuglStatus Window::PrivateData::puglEventCallback(PuglView* const view, const PuglEvent* const event)
{
    printEvent(event, "pugl event: ", true);
    Window::PrivateData* const pData = (Window::PrivateData*)puglGetHandle(view);

    switch (event->type)
    {
    ///< No event
    case PUGL_NOTHING:
        break;

    ///< View moved/resized, a #PuglEventConfigure
    case PUGL_CONFIGURE:
        pData->onPuglReshape(event->configure.width, event->configure.height);
        break;

    ///< View must be drawn, a #PuglEventExpose
    case PUGL_EXPOSE:
        pData->onPuglDisplay();
        break;

    ///< View will be closed, a #PuglEventClose
    case PUGL_CLOSE:
        pData->onPuglClose();
        break;

    case PUGL_CREATE:
        pData->onPuglCreate();
        break;

    case PUGL_BUTTON_PRESS:   ///< Mouse button pressed, a #PuglEventButton
    case PUGL_BUTTON_RELEASE: ///< Mouse button released, a #PuglEventButton
    {
        Events::MouseEvent ev;
        ev.mod    = event->button.state;
        ev.flags  = event->button.flags;
        ev.time   = static_cast<uint>(event->button.time * 1000.0 + 0.5);
        ev.button = event->button.button;
        ev.press  = event->type == PUGL_BUTTON_PRESS;
        ev.pos    = Point<double>(event->button.x, event->button.y);
        pData->onPuglMouse(ev);
        break;
    }

    // TODO
    default:
        break;
    }

    return PUGL_SUCCESS;
}

// -----------------------------------------------------------------------

static int printModifiers(const uint32_t mods)
{
	return fprintf(stderr, "Modifiers:%s%s%s%s\n",
	               (mods & PUGL_MOD_SHIFT) ? " Shift"   : "",
	               (mods & PUGL_MOD_CTRL)  ? " Ctrl"    : "",
	               (mods & PUGL_MOD_ALT)   ? " Alt"     : "",
	               (mods & PUGL_MOD_SUPER) ? " Super" : "");
}

static int printEvent(const PuglEvent* event, const char* prefix, const bool verbose)
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

START_NAMESPACE_DGL

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

PuglStatus Window::PrivateData::puglEventCallback(PuglView* const view, const PuglEvent* const event)
{
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

END_NAMESPACE_DGL
#endif
