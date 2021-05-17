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
# import <Cocoa/Cocoa.h>
# include <dlfcn.h>
# include <mach/mach_time.h>
# ifdef DGL_CAIRO
#  include <cairo.h>
#  include <cairo-quartz.h>
# endif
# ifdef DGL_OPENGL
#  include <OpenGL/gl.h>
# endif
# ifdef DGL_VULKAN
#  import <QuartzCore/CAMetalLayer.h>
#  include <vulkan/vulkan_core.h>
#  include <vulkan/vulkan_macos.h>
# endif
#elif defined(DISTRHO_OS_WINDOWS)
# include <wctype.h>
# include <windows.h>
# include <windowsx.h>
# ifdef DGL_CAIRO
#  include <cairo.h>
#  include <cairo-win32.h>
# endif
# ifdef DGL_OPENGL
#  include <GL/gl.h>
# endif
# ifdef DGL_VULKAN
#  include <vulkan/vulkan.h>
#  include <vulkan/vulkan_win32.h>
# endif
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
#  include <GL/gl.h>
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
/*
# define PuglWindow     DISTRHO_JOIN_MACRO(PuglWindow,     DGL_NAMESPACE)
# define PuglOpenGLView DISTRHO_JOIN_MACRO(PuglOpenGLView, DGL_NAMESPACE)
*/
# import "pugl-upstream/src/mac.m"
# import "pugl-upstream/src/mac_stub.m"
# ifdef DGL_CAIRO
#  import "pugl-upstream/src/mac_cairo.m"
# endif
# ifdef DGL_OPENGL
#  import "pugl-upstream/src/mac_gl.m"
# endif
# ifdef DGL_VULKAN
#  import "pugl-upstream/src/mac_vulkan.m"
# endif
#elif defined(DISTRHO_OS_WINDOWS)
# include "pugl-upstream/src/win.c"
# include "pugl-upstream/src/win_stub.c"
# ifdef DGL_CAIRO
#  include "pugl-upstream/src/win_cairo.c"
# endif
# ifdef DGL_OPENGL
#  include "pugl-upstream/src/win_gl.c"
# endif
# ifdef DGL_VULKAN
#  include "pugl-upstream/src/win_vulkan.c"
# endif
#else
# include "pugl-upstream/src/x11.c"
# include "pugl-upstream/src/x11_stub.c"
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
// expose backend enter

void puglBackendEnter(PuglView* view)
{
    view->backend->enter(view, NULL);
}

// --------------------------------------------------------------------------------------------------------------------
// missing in pugl, directly returns title char* pointer

const char* puglGetWindowTitle(const PuglView* view)
{
    return view->title;
}

// --------------------------------------------------------------------------------------------------------------------
// bring view window into the foreground, aka "raise" window

void puglRaiseWindow(PuglView* view)
{
#if defined(DISTRHO_OS_HAIKU) || defined(DISTRHO_OS_MAC)
    // nothing here yet
#elif defined(DISTRHO_OS_WINDOWS)
    SetForegroundWindow(view->impl->hwnd);
    SetActiveWindow(view->impl->hwnd);
#else
    XRaiseWindow(view->impl->display, view->impl->win);
#endif
}

// --------------------------------------------------------------------------------------------------------------------
// set backend that matches current build

void puglSetMatchingBackendForCurrentBuild(PuglView* const view)
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
// Combine puglSetMinSize and puglSetAspectRatio

PuglStatus puglSetGeometryConstraints(PuglView* const view, const uint width, const uint height, const bool aspect)
{
    view->minWidth  = width;
    view->minHeight = height;

    if (aspect) {
        view->minAspectX = width;
        view->minAspectY = height;
        view->maxAspectX = width;
        view->maxAspectY = height;
    }

#if defined(DISTRHO_OS_HAIKU)
    // nothing?
#elif defined(DISTRHO_OS_MAC)
    if (view->impl->window)
    {
        [view->impl->window setContentMinSize:sizePoints(view, view->minWidth, view->minHeight)];

        if (aspect)
            [view->impl->window setContentAspectRatio:sizePoints(view, view->minAspectX, view->minAspectY)];
    }
#elif defined(DISTRHO_OS_WINDOWS)
    // nothing
#else
    return updateSizeHints(view);
#endif

    return PUGL_SUCCESS;
}

// --------------------------------------------------------------------------------------------------------------------
// set window size without changing frame x/y position

PuglStatus puglSetWindowSize(PuglView* const view, const uint width, const uint height)
{
#if defined(DISTRHO_OS_HAIKU) || defined(DISTRHO_OS_MAC)
    // keep upstream behaviour
    const PuglRect frame = { view->frame.x, view->frame.y, (double)width, (double)height };
    return puglSetFrame(view, frame);
#elif defined(DISTRHO_OS_WINDOWS)
    // matches upstream pugl, except we add SWP_NOMOVE flag
    if (view->impl->hwnd)
    {
        const PuglRect frame = view->frame;

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
    }
#endif

    view->frame.width = width;
    view->frame.height = height;
    return PUGL_SUCCESS;
}

// --------------------------------------------------------------------------------------------------------------------
// DGL specific, build-specific drawing prepare

void puglOnDisplayPrepare(PuglView*)
{
#ifdef DGL_OPENGL
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
#endif
}

// --------------------------------------------------------------------------------------------------------------------
// DGL specific, build-specific fallback resize

void puglFallbackOnResize(PuglView* const view)
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

#ifdef DISTRHO_OS_WINDOWS
// --------------------------------------------------------------------------------------------------------------------
// win32 specific, call ShowWindow with SW_RESTORE

void puglWin32RestoreWindow(PuglView* const view)
{
    PuglInternals* impl = view->impl;
    DISTRHO_SAFE_ASSERT_RETURN(impl->hwnd != nullptr,);

    ShowWindow(impl->hwnd, SW_RESTORE);
    SetFocus(impl->hwnd);
}

// --------------------------------------------------------------------------------------------------------------------
// win32 specific, center view based on parent coordinates (if there is one)

void puglWin32ShowWindowCentered(PuglView* const view)
{
    PuglInternals* impl = view->impl;
    DISTRHO_SAFE_ASSERT_RETURN(impl->hwnd != nullptr,);

    RECT rectChild, rectParent;

    if (view->transientParent != 0 &&
        GetWindowRect(impl->hwnd, &rectChild) &&
        GetWindowRect((HWND)view->transientParent, &rectParent))
    {
        SetWindowPos(impl->hwnd, (HWND)view->transientParent,
                     rectParent.left + (rectChild.right-rectChild.left)/2,
                     rectParent.top + (rectChild.bottom-rectChild.top)/2,
                     0, 0, SWP_SHOWWINDOW|SWP_NOSIZE);
    }
    else
    {
        ShowWindow(impl->hwnd, SW_SHOWNORMAL);
    }

    SetFocus(impl->hwnd);
}

// --------------------------------------------------------------------------------------------------------------------
// win32 specific, set or unset WS_SIZEBOX style flag

void puglWin32SetWindowResizable(PuglView* const view, const bool resizable)
{
    PuglInternals* impl = view->impl;
    DISTRHO_SAFE_ASSERT_RETURN(impl->hwnd != nullptr,);

    const int winFlags = resizable ? GetWindowLong(impl->hwnd, GWL_STYLE) |  WS_SIZEBOX
                                   : GetWindowLong(impl->hwnd, GWL_STYLE) & ~WS_SIZEBOX;
    SetWindowLong(impl->hwnd, GWL_STYLE, winFlags);
}
#endif

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
