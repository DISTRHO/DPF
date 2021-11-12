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

#ifndef PUGL_NAMESPACE
# error PUGL_NAMESPACE must be set when compiling this file
#endif

#include "src/DistrhoPluginChecks.h"
#include "src/DistrhoDefines.h"

#define DISTRHO_MACOS_NAMESPACE_MACRO_HELPER(DGL_NS, SEP, PUGL_NS, INTERFACE) DGL_NS ## SEP ## PUGL_NS ## SEP ## INTERFACE
#define DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NS, PUGL_NS, INTERFACE) DISTRHO_MACOS_NAMESPACE_MACRO_HELPER(DGL_NS, _, PUGL_NS, INTERFACE)

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
# import <Cocoa/Cocoa.h>
# include <algorithm>
# include <cmath>

START_NAMESPACE_DISTRHO
double getDesktopScaleFactor(const uintptr_t parentWindowHandle)
{
    // allow custom scale for testing
    if (const char* const scale = getenv("DPF_SCALE_FACTOR"))
        return std::max(1.0, std::atof(scale));

    if (NSView* const parentView = (NSView*)parentWindowHandle)
        if (NSWindow* const parentWindow = [parentView window])
           return [parentWindow screen].backingScaleFactor;

    return [NSScreen mainScreen].backingScaleFactor;
}
END_NAMESPACE_DISTRHO
#else // DISTRHO_PLUGIN_HAS_EXTERNAL_UI
# include "../dgl/Base.hpp"
# define PuglCairoView      DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, PUGL_NAMESPACE, CairoView)
# define PuglOpenGLView     DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, PUGL_NAMESPACE, OpenGLView)
# define PuglStubView       DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, PUGL_NAMESPACE, StubView)
# define PuglVulkanView     DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, PUGL_NAMESPACE, VulkanView)
# define PuglWindow         DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, PUGL_NAMESPACE, Window)
# define PuglWindowDelegate DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, PUGL_NAMESPACE, WindowDelegate)
# define PuglWrapperView    DISTRHO_MACOS_NAMESPACE_MACRO(DGL_NAMESPACE, PUGL_NAMESPACE, WrapperView)
# import "src/pugl.mm"
#endif // DISTRHO_PLUGIN_HAS_EXTERNAL_UI

#ifndef DGL_FILE_BROWSER_DISABLED
# define DISTRHO_FILE_BROWSER_DIALOG_EXTRA_NAMESPACE PUGL_NAMESPACE
# define x_fib_add_recent          DISTRHO_MACOS_NAMESPACE_MACRO(PUGL_NAMESPACE, x_fib_add_recent)
# define x_fib_cfg_buttons         DISTRHO_MACOS_NAMESPACE_MACRO(PUGL_NAMESPACE, x_fib_cfg_buttons)
# define x_fib_cfg_filter_callback DISTRHO_MACOS_NAMESPACE_MACRO(PUGL_NAMESPACE, x_fib_cfg_filter_callback)
# define x_fib_close               DISTRHO_MACOS_NAMESPACE_MACRO(PUGL_NAMESPACE, x_fib_close)
# define x_fib_configure           DISTRHO_MACOS_NAMESPACE_MACRO(PUGL_NAMESPACE, x_fib_configure)
# define x_fib_filename            DISTRHO_MACOS_NAMESPACE_MACRO(PUGL_NAMESPACE, x_fib_filename)
# define x_fib_free_recent         DISTRHO_MACOS_NAMESPACE_MACRO(PUGL_NAMESPACE, x_fib_free_recent)
# define x_fib_handle_events       DISTRHO_MACOS_NAMESPACE_MACRO(PUGL_NAMESPACE, x_fib_handle_events)
# define x_fib_load_recent         DISTRHO_MACOS_NAMESPACE_MACRO(PUGL_NAMESPACE, x_fib_load_recent)
# define x_fib_recent_at           DISTRHO_MACOS_NAMESPACE_MACRO(PUGL_NAMESPACE, x_fib_recent_at)
# define x_fib_recent_count        DISTRHO_MACOS_NAMESPACE_MACRO(PUGL_NAMESPACE, x_fib_recent_count)
# define x_fib_recent_file         DISTRHO_MACOS_NAMESPACE_MACRO(PUGL_NAMESPACE, x_fib_recent_file)
# define x_fib_save_recent         DISTRHO_MACOS_NAMESPACE_MACRO(PUGL_NAMESPACE, x_fib_save_recent)
# define x_fib_show                DISTRHO_MACOS_NAMESPACE_MACRO(PUGL_NAMESPACE, x_fib_show)
# define x_fib_status              DISTRHO_MACOS_NAMESPACE_MACRO(PUGL_NAMESPACE, x_fib_status)
# include "../extra/FileBrowserDialog.cpp"
#endif
