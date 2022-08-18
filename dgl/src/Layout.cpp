/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2022 Filipe Coelho <falktx@falktx.com>
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

#include "../Layout.hpp"
#include "../SubWidget.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

template<> // horizontal
uint Layout<true>::setAbsolutePos(int x, const int y, const uint padding)
{
    uint maxHeight = 0;

    for (SubWidgetWithSizeHint& s : widgets)
    {
        maxHeight = std::max(maxHeight, s.widget->getHeight());
        s.widget->setAbsolutePos(x, y);
        x += s.widget->getWidth();
        x += padding;
    }

    return maxHeight;
}

template<> // horizontal
void Layout<true>::setSize(const uint width, const uint padding)
{
    uint maxHeight = 0;
    uint nonFixedWidth = width;
    uint numDynamiclySizedWidgets = 0;

    for (SubWidgetWithSizeHint& s : widgets)
    {
        maxHeight = std::max(maxHeight, s.widget->getHeight());

        if (s.sizeHint == Fixed)
            nonFixedWidth -= s.widget->getWidth();
        else
             ++numDynamiclySizedWidgets;
    }

    const uint widthPerWidget = numDynamiclySizedWidgets != 0
                              ? (nonFixedWidth - padding * numDynamiclySizedWidgets) / numDynamiclySizedWidgets
                              : 0;

    for (SubWidgetWithSizeHint& s : widgets)
    {
        if (s.sizeHint != Fixed)
            s.widget->setSize(widthPerWidget, maxHeight);
        else
            s.widget->setHeight(maxHeight);
    }
}

// --------------------------------------------------------------------------------------------------------------------

void VerticallyStackedHorizontalLayout::setAbsolutePos(const int x, int y, const uint padding)
{
    for (HorizontalLayout* l : items)
    {
        y += l->setAbsolutePos(x, y, padding);
        y += padding;
    }
}

void VerticallyStackedHorizontalLayout::setWidth(const uint width, const uint padding)
{
    for (HorizontalLayout* l : items)
        l->setSize(width, padding);
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
