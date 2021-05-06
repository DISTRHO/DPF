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

#include "pugl.hpp"

/* we will include all header files used in pugl in their C++ friendly form, then pugl stuff in custom namespace */
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <ctime>

#if defined(DISTRHO_OS_HAIKU)
#elif defined(DISTRHO_OS_MAC)
#elif defined(DISTRHO_OS_WINDOWS)
#else
# include <dlfcn.h>
# include <sys/select.h>
# include <sys/time.h>
# include <X11/X.h>
# include <X11/Xatom.h>
# include <X11/Xlib.h>
# include <X11/Xutil.h>
# include <X11/keysym.h>
# ifdef HAVE_XCURSOR
#  include <X11/Xcursor/Xcursor.h>
#  include <X11/cursorfont.h>
# endif
# ifdef HAVE_XRANDR
#  include <X11/extensions/Xrandr.h>
# endif
# ifdef HAVE_XSYNC
#  include <X11/extensions/sync.h>
#  include <X11/extensions/syncconst.h>
# endif
# ifdef DGL_CAIRO
#  include <cairo.h>
#  include <cairo-xlib.h>
# endif
# ifdef DGL_OPENGL
#  include <GL/glx.h>
# endif
# ifdef DGL_VULKAN
#  include <vulkan/vulkan_core.h>
#  include <vulkan/vulkan_xlib.h>
# endif
#endif

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

#define PUGL_DISABLE_DEPRECATED

#if defined(DISTRHO_OS_HAIKU)
#elif defined(DISTRHO_OS_MAC)
# define PuglWindow     DISTRHO_JOIN_MACRO(PuglWindow,     DGL_NAMESPACE)
# define PuglOpenGLView DISTRHO_JOIN_MACRO(PuglOpenGLView, DGL_NAMESPACE)
#elif defined(DISTRHO_OS_WINDOWS)
#else
# include "pugl-upstream/src/x11.c"
# ifdef DGL_CAIRO
#  include "pugl-upstream/src/x11_cairo.c"
# endif
# ifdef DGL_OPENGL
#  include "pugl-upstream/src/x11_gl.c"
# endif
# ifdef DGL_VULKAN
#  include "pugl-upstream/src/x11_vulkan.c"
# endif
#endif

#include "pugl-upstream/src/implementation.c"

// --------------------------------------------------------------------------------------------------------------------
// missing in pugl, directly returns title char* pointer

const char* puglGetWindowTitle(const PuglView* view)
{
    return view->title;
}

// --------------------------------------------------------------------------------------------------------------------
// set window size without changing frame x/y position

PuglStatus puglSetWindowSize(PuglView* view, unsigned int width, unsigned int height)
{
#if defined(DISTRHO_OS_HAIKU) || defined(DISTRHO_OS_MAC)
    // TODO
    const PuglRect frame = { 0.0, 0.0, (double)width, (double)height };
    return puglSetFrame(view, frame);
#elif defined(DISTRHO_OS_WINDOWS)
    // matches upstream pugl, except we add SWP_NOMOVE flag
    if (view->impl->hwnd)
    {
        RECT rect = { (long)frame.x,
                      (long)frame.y,
                      (long)frame.x + (long)frame.width,
                      (long)frame.y + (long)frame.height };

        AdjustWindowRectEx(&rect, puglWinGetWindowFlags(view), FALSE, puglWinGetWindowExFlags(view));

        if (! SetWindowPos(view->impl->hwnd,
                           HWND_TOP,
                           rect.left,
                           rect.top,
                           rect.right - rect.left,
                           rect.bottom - rect.top,
                           SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER))
            return PUGL_UNKNOWN_ERROR;
    }
#else
    // matches upstream pugl, except we use XResizeWindow instead of XMoveResizeWindow
    if (view->impl->win)
    {
        if (! XResizeWindow(view->world->impl->display, view->impl->win, width, height))
            return PUGL_UNKNOWN_ERROR;
#if 0
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
#endif
    }
#endif

    view->frame.width = width;
    view->frame.height = height;
    return PUGL_SUCCESS;
}

// --------------------------------------------------------------------------------------------------------------------
// set backend that matches current build

void puglSetMatchingBackendForCurrentBuild(PuglView* view)
{
#ifdef DGL_CAIRO
    puglSetBackend(view, puglCairoBackend());
#endif
#ifdef DGL_OPENGL
    puglSetBackend(view, puglGlBackend());
#endif
#ifdef DGL_Vulkan
    puglSetBackend(view, puglVulkanBackend());
#endif
}

// --------------------------------------------------------------------------------------------------------------------
// DGL specific, build-specific fallback drawing

void puglFallbackOnDisplay(PuglView*)
{
#ifdef DGL_OPENGL
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
#endif
}

// --------------------------------------------------------------------------------------------------------------------
// DGL specific, build-specific fallback resize

void puglFallbackOnResize(PuglView* view)
{
#ifdef DGL_OPENGL
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, static_cast<GLdouble>(view->frame.width), static_cast<GLdouble>(view->frame.height), 0.0, 0.0, 1.0);
    glViewport(0, 0, static_cast<GLsizei>(view->frame.width), static_cast<GLsizei>(view->frame.height));
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
#endif
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
