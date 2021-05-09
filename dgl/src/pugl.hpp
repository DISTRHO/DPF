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

#ifndef DGL_PUGL_HPP_INCLUDED
#define DGL_PUGL_HPP_INCLUDED

#include "../Base.hpp"

/* we will include all header files used in pugl in their C++ friendly form, then pugl stuff in custom namespace */
#include <cstdbool>
#include <cstddef>
#include <cstdint>

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

#define PUGL_DISABLE_DEPRECATED
#include "pugl-upstream/include/pugl/pugl.h"

PUGL_BEGIN_DECLS

// missing in pugl, directly returns title char* pointer
PUGL_API const char*
puglGetWindowTitle(const PuglView* view);

// expose backend enter
PUGL_API void
puglBackendEnter(PuglView* view);

// set window size without changing frame x/y position
PUGL_API PuglStatus
puglSetWindowSize(PuglView* view, unsigned int width, unsigned int height);

// DGL specific, assigns backend that matches current DGL build
PUGL_API void
puglSetMatchingBackendForCurrentBuild(PuglView* view);

// DGL specific, build-specific drawing prepare
PUGL_API void
puglOnDisplayPrepare(PuglView* view);

// DGL specific, build-specific fallback resize
PUGL_API void
puglFallbackOnResize(PuglView* view);

PUGL_END_DECLS

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_PUGL_HPP_INCLUDED
