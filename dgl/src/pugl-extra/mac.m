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
   @file mac.m MacOS extra implementation for DPF.
*/

#include "pugl/detail/mac.h"

void
puglRaiseWindow(PuglView* view)
{
}

void
puglSetWindowSize(PuglView* view, unsigned int width, unsigned int height)
{
    // NOTE: pugl mac code does nothing with x and y
    const PuglRect frame = { 0.0, 0.0, (double)width, (double)height };
    puglSetFrame(view, frame);
}

void
puglUpdateGeometryConstraints(PuglView* view, unsigned int width, unsigned int height, bool aspect)
{
    // NOTE this is a combination of puglSetMinSize and puglSetAspectRatio
    view->minWidth = width;
    view->minHeight = height;

    [view->impl->window setContentMinSize:sizePoints(view, width, height)];

    if (aspect) {
        [view->impl->window setContentAspectRatio:sizePoints(view, width, height)];
    }
}
