/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2020 Filipe Coelho <falktx@falktx.com>
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

#ifdef DGL_CAIRO
# define PUGL_CAIRO
# include "../Cairo.hpp"
#endif
#ifdef DGL_OPENGL
# define PUGL_OPENGL
# include "../OpenGL.hpp"
#endif

extern "C" {
#include "pugl-upstream/pugl/detail/implementation.c"
#include "pugl-extra/extras.c"
}

#if defined(DISTRHO_OS_HAIKU)
# define DGL_DEBUG_EVENTS
# include "pugl-upstream/pugl/detail/haiku.cpp"
#elif defined(DISTRHO_OS_MAC)
# define PuglWindow     DISTRHO_JOIN_MACRO(PuglWindow,     DGL_NAMESPACE)
# define PuglOpenGLView DISTRHO_JOIN_MACRO(PuglOpenGLView, DGL_NAMESPACE)
# include "pugl-upstream/pugl/detail/mac.m"
#elif defined(DISTRHO_OS_WINDOWS)
# include "ppugl-upstream/pugl/detail/win.c"
# undef max
# undef min
#else
# define DGL_PUGL_USING_X11
extern "C" {
# include "pugl-upstream/pugl/detail/x11.c"
// # ifdef DGL_CAIRO
// #  include "pugl-upstream/pugl/detail/x11_cairo.c"
// # endif
# ifdef DGL_OPENGL
#  include "pugl-upstream/pugl/detail/x11_gl.c"
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

void Window::PrivateData::init(const bool resizable)
{
    if (fSelf == nullptr || fView == nullptr)
    {
        DGL_DBG("Failed!\n");
        return;
    }

// #ifdef DGL_CAIRO
//     puglSetBackend(fView, puglCairoBackend());
// #endif
#ifdef DGL_OPENGL
    puglSetBackend(fView, puglGlBackend());
#endif

    puglSetHandle(fView, this);
    puglSetViewHint(fView, PUGL_RESIZABLE, resizable ? PUGL_TRUE : PUGL_FALSE);
    puglSetViewHint(fView, PUGL_IGNORE_KEY_REPEAT, PUGL_FALSE);
    puglSetEventFunc(fView, puglEventCallback);
// #ifndef DGL_FILE_BROWSER_DISABLED
//     puglSetFileSelectedFunc(fView, fileBrowserSelectedCallback);
// #endif

    fAppData->windows.push_back(fSelf);

    DGL_DBG("Success!\n");
}

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
            puglShowWindow(fView);
#endif
            fAppData->oneWindowShown();
            fFirstInit = false;
        }
        else
        {
#ifdef DISTRHO_OS_WINDOWS
            puglWin32RestoreWindow(fView);
#else
            puglShowWindow(fView);
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

        puglHideWindow(fView);

//             if (fModal.enabled)
//                 exec_fini();
    }
}

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
		return PRINT("%sFocus in%s\n",
		             prefix,
		             event->focus.grab ? " (grab)" : "");
	case PUGL_FOCUS_OUT:
		return PRINT("%sFocus out%s\n",
		             prefix,
		             event->focus.grab ? " (ungrab)" : "");
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
			return fprintf(stderr, "%sUpdate\n", prefix);
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

    case PUGL_FOCUS_IN:       ///< Keyboard focus entered view, a #PuglEventFocus
    case PUGL_FOCUS_OUT:      ///< Keyboard focus left view, a #PuglEventFocus
    case PUGL_KEY_PRESS:      ///< Key pressed, a #PuglEventKey
    case PUGL_KEY_RELEASE:    ///< Key released, a #PuglEventKey
    case PUGL_TEXT:           ///< Character entered, a #PuglEventText
    case PUGL_POINTER_IN:     ///< Pointer entered view, a #PuglEventCrossing
    case PUGL_POINTER_OUT:    ///< Pointer left view, a #PuglEventCrossing
    case PUGL_BUTTON_PRESS:   ///< Mouse button pressed, a #PuglEventButton
    case PUGL_BUTTON_RELEASE: ///< Mouse button released, a #PuglEventButton
    case PUGL_MOTION:         ///< Pointer moved, a #PuglEventMotion
    case PUGL_SCROLL:         ///< Scrolled, a #PuglEventScroll
    case PUGL_CLIENT:         ///< Custom client message, a #PuglEventClient
    case PUGL_TIMER:          ///< Timer triggered, a #PuglEventTimer
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
#endif

    // may be unused
    return;
    (void)width; (void)height;
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
