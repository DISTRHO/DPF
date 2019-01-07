/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2019 Filipe Coelho <falktx@falktx.com>
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

#ifndef DGL_WIDGET_PRIVATE_DATA_HPP_INCLUDED
#define DGL_WIDGET_PRIVATE_DATA_HPP_INCLUDED

#include "../Widget.hpp"
#include "../Window.hpp"

#include <vector>

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

struct Widget::PrivateData {
    Widget* const self;
    Window& parent;
    Point<int> absolutePos;
    Size<uint> size;
    std::vector<Widget*> subWidgets;

    uint id;
    bool needsFullViewport;
    bool needsScaling;
    bool skipDisplay;
    bool visible;

    PrivateData(Widget* const s, Window& p, Widget* groupWidget, bool addToSubWidgets)
        : self(s),
          parent(p),
          absolutePos(0, 0),
          size(0, 0),
          subWidgets(),
          id(0),
          needsFullViewport(false),
          needsScaling(false),
          skipDisplay(false),
          visible(true)
    {
        if (addToSubWidgets && groupWidget != nullptr)
        {
            skipDisplay = true;
            groupWidget->pData->subWidgets.push_back(self);
        }
    }

    ~PrivateData()
    {
        subWidgets.clear();
    }

    // display function is different depending on build type
    void display(const uint width, const uint height, const double scaling, const bool renderingSubWidget);

    void displaySubWidgets(const uint width, const uint height, const double scaling)
    {
        for (std::vector<Widget*>::iterator it = subWidgets.begin(); it != subWidgets.end(); ++it)
        {
            Widget* const widget(*it);
            DISTRHO_SAFE_ASSERT_CONTINUE(widget->pData != this);

            widget->pData->display(width, height, scaling, true);
        }
    }

    DISTRHO_DECLARE_NON_COPY_STRUCT(PrivateData)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_WIDGET_PRIVATE_DATA_HPP_INCLUDED
