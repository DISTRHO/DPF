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
# ifdef HAVE_XRANDR
#  include <X11/extensions/Xrandr.h>
# endif
# ifdef HAVE_XSYNC
#  include <X11/extensions/sync.h>
#  include <X11/extensions/syncconst.h>
# endif
# ifdef HAVE_XCURSOR
#  include <X11/Xcursor/Xcursor.h>
#  include <X11/cursorfont.h>
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

END_NAMESPACE_DGL
