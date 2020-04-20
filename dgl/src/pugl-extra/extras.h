/*
  Copyright (C) 2012-2020 Filipe Coelho <falktx@falktx.com>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/**
   @file pugl.h pugl extra API for DPF.
*/

#ifndef PUGL_EXTRAS_PUGL_H
#define PUGL_EXTRAS_PUGL_H

#include "../pugl-upstream/include/pugl/pugl.h"

PUGL_BEGIN_DECLS

PUGL_API const char*
puglGetWindowTitle(const PuglView* view);

PUGL_API int
puglGetViewHint(const PuglView* view, PuglViewHint hint);

PUGL_API void
puglRaiseWindow(PuglView* view);

PUGL_API void
puglSetWindowSize(PuglView* view, unsigned int width, unsigned int height);

PUGL_API void
puglUpdateGeometryConstraints(PuglView* view, unsigned int width, unsigned int height, bool aspect);

#ifdef DISTRHO_OS_WINDOWS
PUGL_API void
puglWin32SetWindowResizable(PuglView* view, bool resizable);
#endif

PUGL_END_DECLS

#endif // PUGL_EXTRAS_PUGL_H
