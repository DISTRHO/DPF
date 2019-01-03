/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2018 Filipe Coelho <falktx@falktx.com>
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

    void display(const uint width, const uint height, const double scaling, const bool renderingSubWidget)
    {
        if ((skipDisplay && ! renderingSubWidget) || size.isInvalid() || ! visible)
            return;

#ifdef HAVE_DGL
        bool needsDisableScissor = false;

        // reset color
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        if (needsFullViewport || (absolutePos.isZero() && size == Size<uint>(width, height)))
        {
            // full viewport size
            glViewport(0,
                       -(height * scaling - height),
                       width * scaling,
                       height * scaling);
        }
        else if (needsScaling)
        {
            // limit viewport to widget bounds
            glViewport(absolutePos.getX(),
                       height - self->getHeight() - absolutePos.getY(),
                       self->getWidth(),
                       self->getHeight());
        }
        else
        {
            // only set viewport pos
            glViewport(absolutePos.getX() * scaling,
                       -std::round((height * scaling - height) + (absolutePos.getY() * scaling)),
                       std::round(width * scaling),
                       std::round(height * scaling));

            // then cut the outer bounds
            glScissor(absolutePos.getX() * scaling,
                      height - std::round((self->getHeight() + absolutePos.getY()) * scaling),
                      std::round(self->getWidth() * scaling),
                      std::round(self->getHeight() * scaling));

            glEnable(GL_SCISSOR_TEST);
            needsDisableScissor = true;
        }
#endif

        // display widget
        self->onDisplay();

#ifdef HAVE_DGL
        if (needsDisableScissor)
        {
            glDisable(GL_SCISSOR_TEST);
            needsDisableScissor = false;
        }
#endif

        displaySubWidgets(width, height, scaling);
    }

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
