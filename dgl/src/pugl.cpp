/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2025 Filipe Coelho <falktx@falktx.com>
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
#include "pugl/pugl.h"

// --------------------------------------------------------------------------------------------------------------------
// include base headers

#ifdef DGL_CAIRO
# include <cairo.h>
#endif
#ifdef DGL_OPENGL
# include "../OpenGL-include.hpp"
#endif
#ifdef DGL_VULKAN
# include <vulkan/vulkan_core.h>
#endif

/* we will include all header files used in pugl in their C++ friendly form, then pugl stuff in custom namespace */
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>

#if defined(DISTRHO_OS_HAIKU)
# include <Application.h>
# include <Window.h>
# ifdef DGL_OPENGL
#  include <GL/gl.h>
#  include <opengl/GLView.h>
# endif
#elif defined(DISTRHO_OS_MAC)
# import <Cocoa/Cocoa.h>
# include <dlfcn.h>
# include <mach/mach_time.h>
# ifdef DGL_CAIRO
#  include <cairo-quartz.h>
# endif
# ifdef DGL_VULKAN
#  import <QuartzCore/CAMetalLayer.h>
#  include <vulkan/vulkan_macos.h>
# endif
#elif defined(DISTRHO_OS_WASM)
# include <emscripten/emscripten.h>
# include <emscripten/html5.h>
# ifdef DGL_OPENGL
#  include <EGL/egl.h>
# endif
#elif defined(DISTRHO_OS_WINDOWS)
# include <wctype.h>
# include <winsock2.h>
# include <windows.h>
# include <windowsx.h>
# ifdef DGL_CAIRO
#  include <cairo-win32.h>
# endif
# ifdef DGL_OPENGL
#  include <GL/gl.h>
# endif
# ifdef DGL_VULKAN
#  include <vulkan/vulkan.h>
#  include <vulkan/vulkan_win32.h>
# endif
#elif defined(HAVE_X11) || defined(HAVE_WAYLAND)
# include <dlfcn.h>
# include <limits.h>
# include <unistd.h>
# include <sys/mman.h>
# include <sys/select.h>
// # include <sys/time.h>
# ifdef HAVE_X11
#  include <X11/X.h>
#  include <X11/Xatom.h>
#  include <X11/Xlib.h>
#  include <X11/Xresource.h>
#  include <X11/Xutil.h>
#  include <X11/keysym.h>
#  ifdef HAVE_XCURSOR
#   include <X11/Xcursor/Xcursor.h>
// #  include <X11/cursorfont.h>
#  endif
#  ifdef HAVE_XRANDR
#   include <X11/extensions/Xrandr.h>
#  endif
#  ifdef HAVE_XSYNC
#   include <X11/extensions/sync.h>
#   include <X11/extensions/syncconst.h>
#  endif
#  ifdef DGL_CAIRO
#   include <cairo-xlib.h>
#  endif
#  ifdef DGL_OPENGL
#   include <GL/glx.h>
#  endif
#  ifdef DGL_VULKAN
#   include <vulkan/vulkan_xlib.h>
#  endif
# endif
# ifdef HAVE_WAYLAND
#  include <xkbcommon/xkbcommon-keysyms.h>
#  include <xkbcommon/xkbcommon.h>
#  include <wayland-client.h>
#  include <wayland-util.h>
#  include <wayland-client-core.h>
#  include <wayland-client-protocol.h>
#  include <wayland-cursor.h>
#  include "pugl-upstream/src/xdg-decoration.h"
#  include "pugl-upstream/src/xdg-shell.h"
#  ifdef DGL_OPENGL
#   include <wayland-egl-core.h>
#   include <EGL/egl.h>
#   include <EGL/eglplatform.h>
#  endif
#  ifdef DGL_VULKAN
#   include <vulkan/vulkan_wayland.h>
#  endif
# endif
#endif

#ifdef DGL_USE_FILE_BROWSER
# define DGL_FILE_BROWSER_DIALOG_HPP_INCLUDED
# define FILE_BROWSER_DIALOG_DGL_NAMESPACE
# define FILE_BROWSER_DIALOG_NAMESPACE DGL_NAMESPACE
START_NAMESPACE_DGL
# include "../../distrho/extra/FileBrowserDialogImpl.hpp"
END_NAMESPACE_DGL
# include "../../distrho/extra/FileBrowserDialogImpl.cpp"
#endif

#ifdef DGL_USE_WEB_VIEW
# define DGL_WEB_VIEW_HPP_INCLUDED
# define WEB_VIEW_NAMESPACE DGL_NAMESPACE
# define WEB_VIEW_DGL_NAMESPACE
START_NAMESPACE_DGL
# include "../../distrho/extra/WebViewImpl.hpp"
END_NAMESPACE_DGL
# include "../../distrho/extra/WebViewImpl.cpp"
#endif

#if defined(DGL_USING_X11) && defined(DGL_X11_WINDOW_ICON_NAME)
extern const ulong* DGL_X11_WINDOW_ICON_NAME;
#endif

#ifndef DISTRHO_OS_MAC
START_NAMESPACE_DGL
#endif

// --------------------------------------------------------------------------------------------------------------------

#if defined(DISTRHO_OS_HAIKU)
# include "pugl-extra/haiku.cpp"
# include "pugl-extra/haiku_stub.cpp"
# ifdef DGL_OPENGL
#  include "pugl-extra/haiku_gl.cpp"
# endif
#elif defined(DISTRHO_OS_MAC)
# ifndef DISTRHO_MACOS_NAMESPACE_MACRO
#  ifndef DISTRHO_MACOS_NAMESPACE_TIME
#   define DISTRHO_MACOS_NAMESPACE_TIME __apple_build_version__
#  endif
#  define DISTRHO_MACOS_NAMESPACE_MACRO_HELPER(NS, SEP, TIME, INTERFACE) NS ## SEP ## TIME ## SEP ## INTERFACE
#  define DISTRHO_MACOS_NAMESPACE_MACRO(NS, TIME, INTERFACE) DISTRHO_MACOS_NAMESPACE_MACRO_HELPER(NS, _, TIME, INTERFACE)
#  define PuglCairoView      DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, DISTRHO_MACOS_NAMESPACE_TIME, PuglCairoView)
#  define PuglOpenGLView     DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, DISTRHO_MACOS_NAMESPACE_TIME, PuglOpenGLView)
#  define PuglStubView       DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, DISTRHO_MACOS_NAMESPACE_TIME, PuglStubView)
#  define PuglVulkanView     DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, DISTRHO_MACOS_NAMESPACE_TIME, PuglVulkanView)
#  define PuglWindow         DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, DISTRHO_MACOS_NAMESPACE_TIME, PuglWindow)
#  define PuglWindowDelegate DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, DISTRHO_MACOS_NAMESPACE_TIME, PuglWindowDelegate)
#  define PuglWrapperView    DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, DISTRHO_MACOS_NAMESPACE_TIME, PuglWrapperView)
# endif
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wdeprecated-declarations"
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
# pragma clang diagnostic pop
#elif defined(DISTRHO_OS_WASM)
# include "pugl-extra/wasm.c"
# include "pugl-extra/wasm_stub.c"
# ifdef DGL_OPENGL
#  include "pugl-extra/wasm_gl.c"
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
#elif defined(DGL_USING_X11_OR_WAYLAND)
# include "pugl-upstream/src/types.h"
# ifdef HAVE_X11
#  undef PUGL_PUGL_H
#  undef PUGL_CAIRO_H
#  undef PUGL_STUB_H
#  undef PUGL_GL_H
#  undef PUGL_VULKAN_H
#  undef PUGL_SRC_TYPES_H
namespace x11 {
struct PuglBackendImpl;
struct PuglInternalsImpl;
struct PuglViewImpl;
struct PuglWorldImpl;
struct PuglWorldInternalsImpl;
#  include "pugl-upstream/src/common.c"
#  include "pugl-upstream/src/internal.c"
#  include "pugl-upstream/src/x11.c"
#  include "pugl-upstream/src/x11_stub.c"
#  ifdef DGL_CAIRO
#   include "pugl-upstream/src/x11_cairo.c"
#  endif
#  ifdef DGL_OPENGL
#   include "pugl-upstream/src/x11_gl.c"
#  endif
#  ifdef DGL_VULKAN
#   include "pugl-upstream/src/x11_vulkan.c"
#  endif
}
# endif
# ifdef HAVE_WAYLAND
#  undef PUGL_PUGL_H
#  undef PUGL_CAIRO_H
#  undef PUGL_STUB_H
#  undef PUGL_GL_H
#  undef PUGL_VULKAN_H
#  undef PUGL_INTERNAL_H
#  undef PUGL_PLATFORM_H
#  undef PUGL_SRC_STUB_H
#  undef PUGL_SRC_TYPES_H
END_NAMESPACE_DGL
extern "C" {
#  include "pugl-upstream/src/xdg-decoration.c"
#  include "pugl-upstream/src/xdg-shell.c"
}
START_NAMESPACE_DGL
static const char kUsingWaylandCheck[] = "wl";
namespace wl {
struct PuglBackendImpl;
struct PuglInternalsImpl;
struct PuglViewImpl;
struct PuglWorldImpl;
struct PuglWorldInternalsImpl;
#  define wl_callback ::wl_callback
#  define wl_seat ::wl_seat
#  define wl_surface ::wl_surface
#  include "pugl-upstream/src/common.c"
#  include "pugl-upstream/src/internal.c"
#  include "pugl-upstream/src/wayland.c"
#  include "pugl-upstream/src/wayland_stub.c"
#  ifdef DGL_CAIRO
#   include "pugl-upstream/src/wayland_cairo.c"
#  endif
#  ifdef DGL_OPENGL
#   include "pugl-upstream/src/wayland_gl.c"
#  endif
#  ifdef DGL_VULKAN
#   include "pugl-upstream/src/wayland_vulkan.c"
#  endif
}
# endif
#endif

#ifndef DGL_USING_X11_OR_WAYLAND
# include "pugl-upstream/src/common.c"
# include "pugl-upstream/src/internal.c"
#endif

// --------------------------------------------------------------------------------------------------------------------
// DGL specific, expose backend enter

bool puglBackendEnter(PuglView* const view)
{
    return view->backend->enter(view, nullptr) == PUGL_SUCCESS;
}

// --------------------------------------------------------------------------------------------------------------------
// DGL specific, expose backend leave

bool puglBackendLeave(PuglView* const view)
{
    return view->backend->leave(view, nullptr) == PUGL_SUCCESS;
}

// --------------------------------------------------------------------------------------------------------------------
// DGL specific, assigns backend that matches current DGL build

void puglSetMatchingBackendForCurrentBuild(PuglView* const view)
{
  #ifdef DGL_USING_X11_OR_WAYLAND
    if (view->world->handle == kUsingWaylandCheck)
    {
      #ifdef HAVE_WAYLAND
        using namespace wl;
        wl::PuglView* const wlview = reinterpret_cast<wl::PuglView*>(view);
       #ifdef DGL_CAIRO
        puglSetBackend(wlview, puglCairoBackend());
       #endif
       #ifdef DGL_OPENGL
        puglSetBackend(wlview, puglGlBackend());
       #endif
       #ifdef DGL_VULKAN
        puglSetBackend(wlview, puglVulkanBackend());
       #endif
      #endif
    }
    else
    {
      #ifdef HAVE_X11
        using namespace x11;
        x11::PuglView* const x11view = reinterpret_cast<x11::PuglView*>(view);
       #ifdef DGL_CAIRO
        puglSetBackend(x11view, puglCairoBackend());
       #endif
       #ifdef DGL_OPENGL
        puglSetBackend(x11view, puglGlBackend());
       #endif
       #ifdef DGL_VULKAN
        puglSetBackend(x11view, puglVulkanBackend());
       #endif
      #endif
    }
  #else
   #ifdef DGL_CAIRO
    puglSetBackend(view, puglCairoBackend());
   #endif
   #ifdef DGL_OPENGL
    puglSetBackend(view, puglGlBackend());
   #endif
   #ifdef DGL_VULKAN
    puglSetBackend(view, puglVulkanBackend());
   #endif
  #endif

    if (view->backend != nullptr)
    {
       #if defined(DGL_USE_GLES2)
        puglSetViewHint(view, PUGL_CONTEXT_API, PUGL_OPENGL_ES_API);
        puglSetViewHint(view, PUGL_CONTEXT_PROFILE, PUGL_OPENGL_CORE_PROFILE);
        puglSetViewHint(view, PUGL_CONTEXT_VERSION_MAJOR, 2);
       #elif defined(DGL_USE_GLES3)
        puglSetViewHint(view, PUGL_CONTEXT_API, PUGL_OPENGL_ES_API);
        puglSetViewHint(view, PUGL_CONTEXT_PROFILE, PUGL_OPENGL_CORE_PROFILE);
        puglSetViewHint(view, PUGL_CONTEXT_VERSION_MAJOR, 3);
       #elif defined(DGL_USE_OPENGL3)
        puglSetViewHint(view, PUGL_CONTEXT_API, PUGL_OPENGL_API);
        puglSetViewHint(view, PUGL_CONTEXT_PROFILE, PUGL_OPENGL_CORE_PROFILE);
        puglSetViewHint(view, PUGL_CONTEXT_VERSION_MAJOR, 3);
       #elif defined(DGL_OPENGL)
        puglSetViewHint(view, PUGL_CONTEXT_API, PUGL_OPENGL_API);
        puglSetViewHint(view, PUGL_CONTEXT_PROFILE, PUGL_OPENGL_COMPATIBILITY_PROFILE);
        puglSetViewHint(view, PUGL_CONTEXT_VERSION_MAJOR, 2);
       #endif
    }
    else
    {
       #ifdef DGL_USING_X11_OR_WAYLAND
        if (view->world->handle == kUsingWaylandCheck)
        {
           #ifdef HAVE_WAYLAND
            wl::puglSetBackend(reinterpret_cast<wl::PuglView*>(view), wl::puglStubBackend());
           #endif
        }
        else
        {
           #ifdef HAVE_X11
            x11::puglSetBackend(reinterpret_cast<x11::PuglView*>(view), x11::puglStubBackend());
           #endif
        }
       #else
        puglSetBackend(view, puglStubBackend());
       #endif
    }
}

// --------------------------------------------------------------------------------------------------------------------
// bring view window into the foreground, aka "raise" window

void puglRaiseWindow(PuglView* const view)
{
    // this does the same as puglShow(view, PUGL_SHOW_FORCE_RAISE) + puglShow(view, PUGL_SHOW_RAISE)
   #if defined(DISTRHO_OS_HAIKU)
   #elif defined(DISTRHO_OS_MAC)
    NSWindow* const window = [view->impl->wrapperView window];
    [window orderFrontRegardless];
    [window orderFront:view->impl->wrapperView];
   #elif defined(DISTRHO_OS_WASM)
    // nothing
   #elif defined(DISTRHO_OS_WINDOWS)
    SetForegroundWindow(view->impl->hwnd);
    SetActiveWindow(view->impl->hwnd);
   #else
    if (view->world->handle == kUsingWaylandCheck)
    {
    }
    else
    {
       #ifdef HAVE_X11
        x11::PuglView* const x11view = reinterpret_cast<x11::PuglView*>(view);
        XRaiseWindow(x11view->world->impl->display, x11view->impl->win);
       #endif
    }
   #endif
}

// --------------------------------------------------------------------------------------------------------------------
// Combined puglSetSizeHint using PUGL_MIN_SIZE and PUGL_FIXED_ASPECT

PuglStatus puglSetGeometryConstraints(PuglView* const view, const uint width, const uint height, const bool aspect)
{
    view->sizeHints[PUGL_MIN_SIZE].width = static_cast<PuglSpan>(width);
    view->sizeHints[PUGL_MIN_SIZE].height = static_cast<PuglSpan>(height);

    if (aspect)
    {
        view->sizeHints[PUGL_FIXED_ASPECT].width = static_cast<PuglSpan>(width);
        view->sizeHints[PUGL_FIXED_ASPECT].height = static_cast<PuglSpan>(height);
    }

   #if defined(DISTRHO_OS_HAIKU)
   #elif defined(DISTRHO_OS_MAC)
    if (view->impl->window)
    {
        if (const PuglStatus status = updateSizeHint(view, PUGL_MIN_SIZE))
            return status;

        if (const PuglStatus status = updateSizeHint(view, PUGL_FIXED_ASPECT))
            return status;
    }
   #elif defined(DISTRHO_OS_WASM)
    const char* const className = view->world->strings[PUGL_CLASS_NAME];
    EM_ASM({
      var canvasWrapper = document.getElementById(UTF8ToString($0)).parentElement;
      canvasWrapper.style.setProperty("min-width", parseInt($1 / window.devicePixelRatio) + 'px');
      canvasWrapper.style.setProperty("min-height", parseInt($2 / window.devicePixelRatio) + 'px');
    }, className, width, height);
   #elif defined(DISTRHO_OS_WINDOWS)
    // nothing
   #else
    if (view->world->handle == kUsingWaylandCheck)
    {
    }
    else
    {
       #ifdef HAVE_X11
        x11::PuglView* const x11view = reinterpret_cast<x11::PuglView*>(view);
        if (x11view->impl->win)
        {
            if (const x11::PuglStatus status = x11::updateSizeHints(x11view))
                return static_cast<PuglStatus>(status);

            XFlush(x11view->world->impl->display);
        }
       #endif
    }
   #endif

    return PUGL_SUCCESS;
}

// --------------------------------------------------------------------------------------------------------------------
// set view as resizable (or not) during runtime

void puglSetResizable(PuglView* const view, const bool resizable)
{
    puglSetViewHint(view, PUGL_RESIZABLE, resizable ? PUGL_TRUE : PUGL_FALSE);

   #if defined(DISTRHO_OS_HAIKU)
   #elif defined(DISTRHO_OS_MAC)
    if (PuglWindow* const window = view->impl->window)
    {
        const uint style = (NSClosableWindowMask | NSTitledWindowMask | NSMiniaturizableWindowMask)
                         | (resizable ? NSResizableWindowMask : 0);
        [window setStyleMask:style];
    }
    // FIXME use [view setAutoresizingMask:NSViewNotSizable] ?
   #elif defined(DISTRHO_OS_WASM)
    // nothing
   #elif defined(DISTRHO_OS_WINDOWS)
    if (const HWND hwnd = view->impl->hwnd)
    {
        const uint winFlags = resizable ? GetWindowLong(hwnd, GWL_STYLE) |  (WS_SIZEBOX | WS_MAXIMIZEBOX)
                                        : GetWindowLong(hwnd, GWL_STYLE) & ~(WS_SIZEBOX | WS_MAXIMIZEBOX);
        SetWindowLong(hwnd, GWL_STYLE, winFlags);
    }
   #else
    if (view->world->handle == kUsingWaylandCheck)
    {
    }
    else
    {
       #ifdef HAVE_X11
        x11::updateSizeHints(reinterpret_cast<x11::PuglView*>(view));
       #endif
    }
   #endif
}

// --------------------------------------------------------------------------------------------------------------------
// set window size while also changing default

PuglStatus puglSetSizeAndDefault(PuglView* const view, const uint width, const uint height)
{
    // set default size first
    view->sizeHints[PUGL_DEFAULT_SIZE].width = view->sizeHints[PUGL_CURRENT_SIZE].width = width;
    view->sizeHints[PUGL_DEFAULT_SIZE].height = view->sizeHints[PUGL_CURRENT_SIZE].height = height;

   #if defined(DISTRHO_OS_HAIKU)
   #elif defined(DISTRHO_OS_MAC)
    // matches upstream pugl
    if (view->impl->wrapperView)
    {
        // nothing to do for PUGL_DEFAULT_SIZE hint

        if (const PuglStatus status = puglSetWindowSize(view, width, height))
            return status;
    }
   #elif defined(DISTRHO_OS_WASM)
    emscripten_set_canvas_element_size(view->world->strings[PUGL_CLASS_NAME], width, height);
   #elif defined(DISTRHO_OS_WINDOWS)
    // matches upstream pugl, except we re-enter context after resize
    if (view->impl->hwnd)
    {
        // nothing to do for PUGL_DEFAULT_SIZE hint

        if (const PuglStatus status = puglSetWindowSize(view, width, height))
            return status;

        // make sure to return context back to ourselves
        puglBackendEnter(view);
    }
   #else
    if (view->world->handle == kUsingWaylandCheck)
    {
    }
    else
    {
       #ifdef HAVE_X11
        // matches upstream pugl, adds flush at the end
        x11::PuglView* const x11view = reinterpret_cast<x11::PuglView*>(view);
        if (x11view->impl->win)
        {
            if (const x11::PuglStatus status = updateSizeHints(x11view))
                return static_cast<PuglStatus>(status);

            if (const x11::PuglStatus status = puglSetWindowSize(x11view, width, height))
                return static_cast<PuglStatus>(status);

            // flush size changes
            XFlush(x11view->world->impl->display);
        }
       #endif
    }
   #endif

    return PUGL_SUCCESS;
}

// --------------------------------------------------------------------------------------------------------------------
// DGL specific, build-specific drawing prepare

void puglOnDisplayPrepare(PuglView*)
{
  #ifdef DGL_OPENGL
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   #ifndef DGL_USE_OPENGL3
    glLoadIdentity();
   #endif
  #endif
}

// --------------------------------------------------------------------------------------------------------------------
// DGL specific, build-specific fallback resize

void puglFallbackOnResize(PuglView* const view, const uint width, const uint height)
{
  #ifdef DGL_OPENGL
   #if defined(DGL_USE_OPENGL3)
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
   #else
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, static_cast<GLdouble>(width), static_cast<GLdouble>(height), 0.0, 0.0, 1.0);
    glViewport(0, 0, static_cast<GLsizei>(width), static_cast<GLsizei>(height));
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
   #endif
  #else
    // unused
    (void)view;
    (void)width;
    (void)height;
  #endif
}

// --------------------------------------------------------------------------------------------------------------------

#if defined(DISTRHO_OS_HAIKU)

// --------------------------------------------------------------------------------------------------------------------

#elif defined(DISTRHO_OS_MAC)

// --------------------------------------------------------------------------------------------------------------------
// macOS specific, add another view's window as child

PuglStatus
puglMacOSAddChildWindow(PuglView* const view, PuglView* const child)
{
    if (NSWindow* const viewWindow = view->impl->window ? view->impl->window
                                                        : [view->impl->wrapperView window])
    {
        if (NSWindow* const childWindow = child->impl->window ? child->impl->window
                                                              : [child->impl->wrapperView window])
        {
            [viewWindow addChildWindow:childWindow ordered:NSWindowAbove];
            return PUGL_SUCCESS;
        }
    }

    return PUGL_FAILURE;
}

// --------------------------------------------------------------------------------------------------------------------
// macOS specific, remove another view's window as child

PuglStatus
puglMacOSRemoveChildWindow(PuglView* const view, PuglView* const child)
{
    if (NSWindow* const viewWindow = view->impl->window ? view->impl->window
                                                        : [view->impl->wrapperView window])
    {
        if (NSWindow* const childWindow = child->impl->window ? child->impl->window
                                                              : [child->impl->wrapperView window])
        {
            [viewWindow removeChildWindow:childWindow];
            return PUGL_SUCCESS;
        }
    }

    return PUGL_FAILURE;
}

// --------------------------------------------------------------------------------------------------------------------
// macOS specific, center view based on parent coordinates (if there is one)

void puglMacOSShowCentered(PuglView* const view)
{
    if (puglShow(view, PUGL_SHOW_RAISE) != PUGL_SUCCESS)
        return;

    if (view->transientParent != 0)
    {
        NSWindow* const transientWindow = [(NSView*)view->transientParent window];
        DISTRHO_SAFE_ASSERT_RETURN(transientWindow != nullptr,);

        const NSRect ourFrame       = [view->impl->window frame];
        const NSRect transientFrame = [transientWindow frame];

        const int x = transientFrame.origin.x + (transientFrame.size.width - ourFrame.size.width) / 2;
        const int y = transientFrame.origin.y + (transientFrame.size.height - ourFrame.size.height) / 2;

        [view->impl->window setFrameTopLeftPoint:NSMakePoint(x, y)];
    }
    else
    {
        [view->impl->window center];
    }
}

// --------------------------------------------------------------------------------------------------------------------

#elif defined(DISTRHO_OS_WINDOWS)

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

void puglWin32ShowCentered(PuglView* const view)
{
    PuglInternals* impl = view->impl;
    DISTRHO_SAFE_ASSERT_RETURN(impl->hwnd != nullptr,);

    RECT rectChild, rectParent;

    if (view->transientParent != 0 &&
        GetWindowRect(impl->hwnd, &rectChild) &&
        GetWindowRect((HWND)view->transientParent, &rectParent))
    {
        SetWindowPos(impl->hwnd, HWND_TOP,
                     rectParent.left + (rectParent.right-rectParent.left)/2 - (rectChild.right-rectChild.left)/2,
                     rectParent.top + (rectParent.bottom-rectParent.top)/2 - (rectChild.bottom-rectChild.top)/2,
                     0, 0, SWP_SHOWWINDOW|SWP_NOSIZE);
    }
    else
    {
        MONITORINFO mInfo;
        std::memset(&mInfo, 0, sizeof(mInfo));
        mInfo.cbSize = sizeof(mInfo);

        if (GetMonitorInfo(MonitorFromWindow(impl->hwnd, MONITOR_DEFAULTTOPRIMARY), &mInfo))
            SetWindowPos(impl->hwnd, HWND_TOP,
                         mInfo.rcWork.left + (mInfo.rcWork.right - mInfo.rcWork.left - view->lastConfigure.width) / 2,
                         mInfo.rcWork.top + (mInfo.rcWork.bottom - mInfo.rcWork.top - view->lastConfigure.height) / 2,
                         0, 0, SWP_SHOWWINDOW|SWP_NOSIZE);
        else
            ShowWindow(impl->hwnd, SW_NORMAL);
    }

    SetFocus(impl->hwnd);
}

// --------------------------------------------------------------------------------------------------------------------

#elif defined(DISTRHO_OS_WASM)

// nothing here yet

// --------------------------------------------------------------------------------------------------------------------

#elif defined(DGL_USING_X11_OR_WAYLAND)

// --------------------------------------------------------------------------------------------------------------------
// X11 vs Wayland redirect

PuglStatus puglAcceptOffer(PuglView* const view, const PuglDataOfferEvent* const offer, const uint32_t typeIndex)
{
   #ifdef HAVE_WAYLAND
    if (view->world->handle == kUsingWaylandCheck)
        return static_cast<PuglStatus>(wl::puglAcceptOffer(reinterpret_cast<wl::PuglView*>(view),
                                                           reinterpret_cast<const wl::PuglDataOfferEvent*>(offer),
                                                           typeIndex));
   #endif
    return static_cast<PuglStatus>(x11::puglAcceptOffer(reinterpret_cast<x11::PuglView*>(view),
                                                        reinterpret_cast<const x11::PuglDataOfferEvent*>(offer),
                                                        typeIndex));
}

void puglFreeView(PuglView* const view)
{
    if (view->world->handle == kUsingWaylandCheck)
        return wl::puglFreeView(reinterpret_cast<wl::PuglView*>(view));
    x11::puglFreeView(reinterpret_cast<x11::PuglView*>(view));
}

void puglFreeViewInternals(PuglView* const view)
{
    if (view->world->handle == kUsingWaylandCheck)
        return x11::puglFreeViewInternals(reinterpret_cast<x11::PuglView*>(view));
    x11::puglFreeViewInternals(reinterpret_cast<x11::PuglView*>(view));
}

void puglFreeWorld(PuglWorld* const world)
{
    if (world->handle == kUsingWaylandCheck)
        return wl::puglFreeWorld(reinterpret_cast<wl::PuglWorld*>(world));
    x11::puglFreeWorld(reinterpret_cast<x11::PuglWorld*>(world));
}

const char* puglGetClipboardType(const PuglView* const view, const uint32_t typeIndex)
{
    if (view->world->handle == kUsingWaylandCheck)
        return wl::puglGetClipboardType(reinterpret_cast<const wl::PuglView*>(view), typeIndex);
    return x11::puglGetClipboardType(reinterpret_cast<const x11::PuglView*>(view), typeIndex);
}

PuglHandle puglGetHandle(PuglView* const view)
{
    if (view->world->handle == kUsingWaylandCheck)
        return wl::puglGetHandle(reinterpret_cast<wl::PuglView*>(view));
    return x11::puglGetHandle(reinterpret_cast<x11::PuglView*>(view));
}

uint32_t puglGetNumClipboardTypes(const PuglView* const view)
{
    if (view->world->handle == kUsingWaylandCheck)
        return wl::puglGetNumClipboardTypes(reinterpret_cast<const wl::PuglView*>(view));
    return x11::puglGetNumClipboardTypes(reinterpret_cast<const x11::PuglView*>(view));
}

double puglGetScaleFactor(const PuglView* const view)
{
    if (view->world->handle == kUsingWaylandCheck)
        return wl::puglGetScaleFactor(reinterpret_cast<const wl::PuglView*>(view));
    return x11::puglGetScaleFactor(reinterpret_cast<const x11::PuglView*>(view));
}

PuglArea puglGetSizeHint(const PuglView* const view, const PuglSizeHint hint)
{
    // FIXME
    if (view->world->handle == kUsingWaylandCheck)
    {
        wl::PuglArea area = wl::puglGetSizeHint(reinterpret_cast<const wl::PuglView*>(view),
                                                static_cast<wl::PuglSizeHint>(hint));
        return CPP_AGGREGATE_INIT(PuglArea){ area.width, area.height };
    }
    x11::PuglArea area = x11::puglGetSizeHint(reinterpret_cast<const x11::PuglView*>(view),
                                              static_cast<x11::PuglSizeHint>(hint));
    return CPP_AGGREGATE_INIT(PuglArea){ area.width, area.height };
}

PuglNativeView puglGetTransientParent(const PuglView* const view)
{
    if (view->world->handle == kUsingWaylandCheck)
        return wl::puglGetTransientParent(reinterpret_cast<const wl::PuglView*>(view));
    return x11::puglGetTransientParent(reinterpret_cast<const x11::PuglView*>(view));
}

const char* puglGetWorldString(const PuglWorld* const world, const PuglStringHint key)
{
    if (world->handle == kUsingWaylandCheck)
        return wl::puglGetWorldString(reinterpret_cast<const wl::PuglWorld*>(world),
                                      static_cast<wl::PuglStringHint>(key));
    return x11::puglGetWorldString(reinterpret_cast<const x11::PuglWorld*>(world),
                                   static_cast<x11::PuglStringHint>(key));
}

PuglStatus puglGrabFocus(PuglView* const view)
{
    if (view->world->handle == kUsingWaylandCheck)
        return static_cast<PuglStatus>(wl::puglGrabFocus(reinterpret_cast<wl::PuglView*>(view)));
    return static_cast<PuglStatus>(x11::puglGrabFocus(reinterpret_cast<x11::PuglView*>(view)));
}

PuglStatus puglHide(PuglView* const view)
{
    if (view->world->handle == kUsingWaylandCheck)
        return static_cast<PuglStatus>(wl::puglHide(reinterpret_cast<wl::PuglView*>(view)));
    return static_cast<PuglStatus>(x11::puglHide(reinterpret_cast<x11::PuglView*>(view)));
}

PuglStatus puglObscureView(PuglView* const view)
{
    if (view->world->handle == kUsingWaylandCheck)
        return static_cast<PuglStatus>(wl::puglObscureView(reinterpret_cast<wl::PuglView*>(view)));
    return static_cast<PuglStatus>(x11::puglObscureView(reinterpret_cast<x11::PuglView*>(view)));
}

PuglView* puglNewView(PuglWorld* const world)
{
    if (world->handle == kUsingWaylandCheck)
        return reinterpret_cast<PuglView*>(wl::puglNewView(reinterpret_cast<wl::PuglWorld*>(world)));
    return reinterpret_cast<PuglView*>(x11::puglNewView(reinterpret_cast<x11::PuglWorld*>(world)));
}

PuglWorld* puglNewWorld(const PuglWorldType type, PuglWorldFlags flags)
{
    bool supportsDecorations = true;
   #ifdef HAVE_WAYLAND
    const bool usingWayland = puglWaylandStatus(&supportsDecorations);
   #else
    constexpr bool usingWayland = false;
   #endif
    d_stdout("Using wayland: %d, compositor supports decorations: %d", usingWayland, supportsDecorations);

    if (usingWayland)
    {
        if (wl::PuglWorld* const world = wl::puglNewWorld(static_cast<wl::PuglWorldType>(type),
                                                          static_cast<wl::PuglWorldFlags>(flags)))
        {
            wl::puglSetWorldHandle(world, const_cast<char*>(kUsingWaylandCheck));
            return reinterpret_cast<PuglWorld*>(world);
        }
        return nullptr;
    }

    return reinterpret_cast<PuglWorld*>(x11::puglNewWorld(static_cast<x11::PuglWorldType>(type),
                                                          static_cast<x11::PuglWorldFlags>(flags)));
}

PuglStatus puglRealize(PuglView* const view)
{
   #ifdef HAVE_WAYLAND
    if (view->world->handle == kUsingWaylandCheck)
        return static_cast<PuglStatus>(wl::puglRealize(reinterpret_cast<wl::PuglView*>(view)));
   #endif
    return static_cast<PuglStatus>(x11::puglRealize(reinterpret_cast<x11::PuglView*>(view)));
}

PuglStatus puglSetEventFunc(PuglView* const view, const PuglEventFunc eventFunc)
{
    if (view->world->handle == kUsingWaylandCheck)
        return static_cast<PuglStatus>(wl::puglSetEventFunc(reinterpret_cast<wl::PuglView*>(view),
                                                            reinterpret_cast<wl::PuglEventFunc>(eventFunc)));
    return static_cast<PuglStatus>(x11::puglSetEventFunc(reinterpret_cast<x11::PuglView*>(view),
                                                         reinterpret_cast<x11::PuglEventFunc>(eventFunc)));
}

void puglSetHandle(PuglView* const view, const PuglHandle handle)
{
    if (view->world->handle == kUsingWaylandCheck)
        x11::puglSetHandle(reinterpret_cast<x11::PuglView*>(view), handle);
    x11::puglSetHandle(reinterpret_cast<x11::PuglView*>(view), handle);
}

PuglStatus puglSetParent(PuglView* const view, const PuglNativeView parent)
{
    if (view->world->handle == kUsingWaylandCheck)
        return static_cast<PuglStatus>(wl::puglSetParent(reinterpret_cast<wl::PuglView*>(view), parent));
    return static_cast<PuglStatus>(x11::puglSetParent(reinterpret_cast<x11::PuglView*>(view), parent));
}

PuglStatus puglSetPositionHint(PuglView* const view, const PuglPositionHint hint, const int x, const int y)
{
    if (view->world->handle == kUsingWaylandCheck)
        return static_cast<PuglStatus>(wl::puglSetPositionHint(reinterpret_cast<wl::PuglView*>(view),
                                                               static_cast<wl::PuglPositionHint>(hint),
                                                               x,
                                                               y));
    return static_cast<PuglStatus>(x11::puglSetPositionHint(reinterpret_cast<x11::PuglView*>(view),
                                                            static_cast<x11::PuglPositionHint>(hint),
                                                            x,
                                                            y));
}

PuglStatus puglSetViewHint(PuglView* const view, const PuglViewHint hint, const int value)
{
    if (view->world->handle == kUsingWaylandCheck)
        return static_cast<PuglStatus>(wl::puglSetViewHint(reinterpret_cast<wl::PuglView*>(view),
                                                           static_cast<wl::PuglViewHint>(hint),
                                                           value));
    return static_cast<PuglStatus>(x11::puglSetViewHint(reinterpret_cast<x11::PuglView*>(view),
                                                        static_cast<x11::PuglViewHint>(hint),
                                                        value));
}

PuglStatus puglSetViewString(PuglView* const view, const PuglStringHint key, const char* const value)
{
    if (view->world->handle == kUsingWaylandCheck)
        return static_cast<PuglStatus>(wl::puglSetViewString(reinterpret_cast<wl::PuglView*>(view),
                                                             static_cast<wl::PuglStringHint>(key),
                                                             value));
    return static_cast<PuglStatus>(x11::puglSetViewString(reinterpret_cast<x11::PuglView*>(view),
                                                          static_cast<x11::PuglStringHint>(key),
                                                          value));
}

void puglSetWorldHandle(PuglWorld* const world, const PuglWorldHandle handle)
{
    if (world->handle == kUsingWaylandCheck)
        return wl::puglSetWorldHandle(reinterpret_cast<wl::PuglWorld*>(world), handle);
    x11::puglSetWorldHandle(reinterpret_cast<x11::PuglWorld*>(world), handle);
}

PuglStatus puglSetWorldString(PuglWorld* const world, const PuglStringHint key, const char* const value)
{
    if (world->handle == kUsingWaylandCheck)
        return static_cast<PuglStatus>(wl::puglSetWorldString(reinterpret_cast<wl::PuglWorld*>(world),
                                                              static_cast<wl::PuglStringHint>(key),
                                                              value));
    return static_cast<PuglStatus>(x11::puglSetWorldString(reinterpret_cast<x11::PuglWorld*>(world),
                                                           static_cast<x11::PuglStringHint>(key),
                                                           value));
}

PuglStatus puglShow(PuglView* const view, const PuglShowCommand command)
{
    if (view->world->handle == kUsingWaylandCheck)
        return static_cast<PuglStatus>(wl::puglShow(reinterpret_cast<wl::PuglView*>(view),
                                                    static_cast<wl::PuglShowCommand>(command)));
    return static_cast<PuglStatus>(x11::puglShow(reinterpret_cast<x11::PuglView*>(view),
                                                 static_cast<x11::PuglShowCommand>(command)));
}

PuglStatus puglStopTimer(PuglView* const view, const uintptr_t id)
{
    if (view->world->handle == kUsingWaylandCheck)
        return static_cast<PuglStatus>(wl::puglStopTimer(reinterpret_cast<wl::PuglView*>(view), id));
    return static_cast<PuglStatus>(x11::puglStopTimer(reinterpret_cast<x11::PuglView*>(view), id));
}

PuglStatus puglUpdate(PuglWorld* const world, const double timeout)
{
    if (world->handle == kUsingWaylandCheck)
        return static_cast<PuglStatus>(wl::puglUpdate(reinterpret_cast<wl::PuglWorld*>(world), timeout));
    return static_cast<PuglStatus>(x11::puglUpdate(reinterpret_cast<x11::PuglWorld*>(world), timeout));
}

#ifdef HAVE_X11

// --------------------------------------------------------------------------------------------------------------------
// X11 specific, update world without triggering exposure events

PuglStatus puglX11UpdateWithoutExposures(PuglWorld* const world)
{
    if (world->handle == kUsingWaylandCheck)
        return PUGL_BACKEND_FAILED;

    x11::PuglWorld*          const x11world = reinterpret_cast<x11::PuglWorld*>(world);
    x11::PuglWorldInternals* const impl     = x11world->impl;

    const bool wasDispatchingEvents = impl->dispatchingEvents;
    impl->dispatchingEvents = true;
    x11::PuglStatus st = x11::PUGL_SUCCESS;

    const double startTime = puglGetTime(x11world);
    const double endTime   = startTime + 0.03;

    for (double t = startTime; !st && t < endTime; t = puglGetTime(x11world))
    {
        pollX11Socket(x11world, endTime - t);
        st = dispatchX11Events(x11world);
    }

    impl->dispatchingEvents = wasDispatchingEvents;
    return static_cast<PuglStatus>(st);
}

// --------------------------------------------------------------------------------------------------------------------
// X11 specific, set dialog window type

void puglX11SetWindowType(const PuglView* const view, const bool isStandalone)
{
    if (view->world->handle == kUsingWaylandCheck)
        return;

    const x11::PuglView*      const x11view = reinterpret_cast<const x11::PuglView*>(view);
    const x11::PuglInternals* const impl    = x11view->impl;
    Display*                  const display = x11view->world->impl->display;

   #if defined(DGL_X11_WINDOW_ICON_NAME) && defined(DGL_X11_WINDOW_ICON_SIZE)
    if (isStandalone)
    {
        const Atom NET_WM_ICON = XInternAtom(display, "_NET_WM_ICON", False);
        XChangeProperty(display,
                        impl->win,
                        NET_WM_ICON,
                        XA_CARDINAL,
                        32,
                        PropModeReplace,
                        reinterpret_cast<const uchar*>(DGL_X11_WINDOW_ICON_NAME),
                        DGL_X11_WINDOW_ICON_SIZE);
    }
   #endif

    const Atom NET_WM_WINDOW_TYPE = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);

    Atom windowTypes[2];
    int numWindowTypes = 0;

    if (! isStandalone)
        windowTypes[numWindowTypes++] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", False);

    windowTypes[numWindowTypes++] = XInternAtom(display, "_NET_WM_WINDOW_TYPE_NORMAL", False);

    XChangeProperty(display,
                    impl->win,
                    NET_WM_WINDOW_TYPE,
                    XA_ATOM,
                    32,
                    PropModeReplace,
                    reinterpret_cast<const uchar*>(&windowTypes),
                    numWindowTypes);
}

// --------------------------------------------------------------------------------------------------------------------

#endif // HAVE_X11

#ifdef HAVE_WAYLAND

// --------------------------------------------------------------------------------------------------------------------
// Wayland specific, check if running wayland and if compositor supports decorations

static void wayland_compositor_test(void* const data,
                                    struct wl_registry* const wl_registry,
                                    const uint32_t name,
                                    const char* const interface,
                                    const uint32_t version)
{
    if (std::strcmp(interface, zxdg_decoration_manager_v1_interface.name) == 0)
        *static_cast<bool*>(data) = wl_registry != NULL && name != 0 && version != 0;
}

bool puglWaylandStatus(bool* supportsDecorations)
{
    static constexpr const struct wl_registry_listener wl_registry_listener = {
        wayland_compositor_test,
        nullptr,
    };

    bool supportsWayland = false;
    *supportsDecorations = false;

    if (struct wl_display* const wl_display = wl_display_connect(nullptr))
    {
        if (struct wl_registry* const wl_registry = wl_display_get_registry(wl_display))
        {
            if (wl_registry_add_listener(wl_registry, &wl_registry_listener, supportsDecorations) == 0)
            {
                if (wl_display_roundtrip(wl_display) > 0)
                    // TODO also query required features
                    supportsWayland = true;
            }

            wl_registry_destroy(wl_registry);
        }

        wl_display_disconnect(wl_display);
    }

    return supportsWayland;
}

#endif // HAVE_WAYLAND

// --------------------------------------------------------------------------------------------------------------------

#endif

#ifndef DISTRHO_OS_MAC
END_NAMESPACE_DGL
#endif
