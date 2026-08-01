/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2026 Filipe Coelho <falktx@falktx.com>
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

#ifndef DGL_LAYOUT_HPP_INCLUDED
#define DGL_LAYOUT_HPP_INCLUDED

#include "Geometry.hpp"

#include <climits>
#include <list>

START_NAMESPACE_DGL

class SubWidget;

// --------------------------------------------------------------------------------------------------------------------

// NOTE: under development, API to be finalized and documented soon

enum SizeHint {
    Expanding,
    Fixed
};

struct SubWidgetWithSizeHint {
    SubWidget* widget;
    SizeHint sizeHint;
};

struct HorizontalLayout
{
    std::list<SubWidgetWithSizeHint> widgets;
    void align(int x, int y, uint width, uint height, uint padding, uint margin);
    uint setAbsolutePos(int x, int y, uint padding, uint margin = UINT_MAX);
    void setWidth(uint width, uint padding, uint margin = UINT_MAX);

    DISTRHO_DEPRECATED
    inline void setSize(const uint size, const uint padding)
    {
        setWidth(size, padding);
    }
};

struct VerticalLayout
{
    std::list<SubWidgetWithSizeHint> widgets;
    void align(int x, int y, uint width, uint height, uint padding, uint margin);
    uint setAbsolutePos(int x, int y, uint padding, uint margin = UINT_MAX);
    void setHeight(uint height, uint padding, uint margin = UINT_MAX);

    DISTRHO_DEPRECATED
    inline void setSize(const uint size, const uint padding)
    {
        setHeight(size, padding);
    }
};

struct HorizontallyStackedVerticalLayout
{
    std::list<VerticalLayout*> items;
    Size<uint> adjustSize(uint padding); // TODO
    void setAbsolutePos(int x, int y, uint padding);
};

struct VerticallyStackedHorizontalLayout
{
    std::list<HorizontalLayout*> items;
    Size<uint> adjustSize(uint padding);
    void setAbsolutePos(int x, int y, uint padding);
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_LAYOUT_HPP_INCLUDED
