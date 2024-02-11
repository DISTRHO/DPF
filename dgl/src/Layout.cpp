/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2024 Filipe Coelho <falktx@falktx.com>
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

typedef std::list<SubWidgetWithSizeHint>::iterator SubWidgetWithSizeHintIterator;
typedef std::list<HorizontalLayout*>::iterator HorizontalLayoutIterator;
typedef std::list<VerticalLayout*>::iterator VerticalLayoutIterator;

// --------------------------------------------------------------------------------------------------------------------

template<> // horizontal
uint Layout<true>::setAbsolutePos(int x, const int y, const uint padding)
{
    uint maxHeight = 0;

    for (SubWidgetWithSizeHintIterator it=widgets.begin(), end=widgets.end(); it != end; ++it)
    {
        SubWidgetWithSizeHint& s(*it);
        maxHeight = std::max(maxHeight, s.widget->getHeight());
        s.widget->setAbsolutePos(x, y);
        x += static_cast<int>(s.widget->getWidth());
        x += static_cast<int>(padding);
    }

    return maxHeight;
}

template<> // vertical
uint Layout<false>::setAbsolutePos(const int x, int y, const uint padding)
{
    uint maxWidth = 0;

    for (SubWidgetWithSizeHintIterator it=widgets.begin(), end=widgets.end(); it != end; ++it)
    {
        SubWidgetWithSizeHint& s(*it);
        maxWidth = std::max(maxWidth, s.widget->getWidth());
        s.widget->setAbsolutePos(x, y);
        y += static_cast<int>(s.widget->getHeight());
        y += static_cast<int>(padding);
    }

    return maxWidth;
}

template<> // horizontal
void Layout<true>::setSize(const uint width, const uint padding)
{
    uint maxHeight = 0;
    uint nonFixedWidth = width;
    uint numDynamiclySizedWidgets = 0;

    for (SubWidgetWithSizeHintIterator it=widgets.begin(), end=widgets.end(); it != end; ++it)
    {
        SubWidgetWithSizeHint& s(*it);
        maxHeight = std::max(maxHeight, s.widget->getHeight());

        if (s.sizeHint == Fixed)
            nonFixedWidth -= s.widget->getWidth();
        else
             ++numDynamiclySizedWidgets;
    }

    if (const size_t numWidgets = widgets.size())
        nonFixedWidth -= padding * static_cast<uint>(numWidgets - 1);

    const uint widthPerWidget = numDynamiclySizedWidgets != 0 ? nonFixedWidth / numDynamiclySizedWidgets : 0;

    for (SubWidgetWithSizeHintIterator it=widgets.begin(), end=widgets.end(); it != end; ++it)
    {
        SubWidgetWithSizeHint& s(*it);
        if (s.sizeHint != Fixed)
            s.widget->setSize(widthPerWidget, maxHeight);
        else
            s.widget->setHeight(maxHeight);
    }
}

template<> // vertical
void Layout<false>::setSize(const uint height, const uint padding)
{
    uint biggestWidth = 0;
    uint nonFixedHeight = height;
    uint numDynamiclySizedWidgets = 0;

    for (SubWidgetWithSizeHintIterator it=widgets.begin(), end=widgets.end(); it != end; ++it)
    {
        SubWidgetWithSizeHint& s(*it);
        biggestWidth = std::max(biggestWidth, s.widget->getWidth());

        if (s.sizeHint == Fixed)
            nonFixedHeight -= s.widget->getHeight();
        else
             ++numDynamiclySizedWidgets;
    }

    if (const size_t numWidgets = widgets.size())
        nonFixedHeight -= padding * static_cast<uint>(numWidgets - 1);

    const uint heightPerWidget = numDynamiclySizedWidgets != 0 ? nonFixedHeight / numDynamiclySizedWidgets : 0;

    for (SubWidgetWithSizeHintIterator it=widgets.begin(), end=widgets.end(); it != end; ++it)
    {
        SubWidgetWithSizeHint& s(*it);
        if (s.sizeHint != Fixed)
            s.widget->setSize(biggestWidth, heightPerWidget);
        else
            s.widget->setWidth(biggestWidth);
    }
}

// --------------------------------------------------------------------------------------------------------------------

/* TODO
void HorizontallyStackedVerticalLayout::adjustSize(const uint padding)
{
}
*/

void HorizontallyStackedVerticalLayout::setAbsolutePos(int x, const int y, const uint padding)
{
    for (VerticalLayoutIterator it=items.begin(), end=items.end(); it != end; ++it)
    {
        VerticalLayout* l(*it);
        x += static_cast<int>(l->setAbsolutePos(x, y, padding));
        x += static_cast<int>(padding);
    }
}

// --------------------------------------------------------------------------------------------------------------------

Size<uint> VerticallyStackedHorizontalLayout::adjustSize(const uint padding)
{
    uint biggestWidth = 0;
    uint totalHeight = 0;

    // iterate all widgets to find which one is the biggest (horizontally)
    for (HorizontalLayoutIterator it=items.begin(), end=items.end(); it != end; ++it)
    {
        HorizontalLayout* const l(*it);
        uint width = 0;
        uint height = 0;

        for (SubWidgetWithSizeHintIterator it2=l->widgets.begin(), end2=l->widgets.end(); it2 != end2; ++it2)
        {
            SubWidgetWithSizeHint& s(*it2);

            if (width != 0)
                width += padding;

            width += s.widget->getWidth();
            height = std::max(height, s.widget->getHeight());
        }

        biggestWidth = std::max(biggestWidth, width);

        if (totalHeight != 0)
            totalHeight += padding;

        totalHeight += height;
    }

    // now make all horizontal lines the same width
    for (HorizontalLayoutIterator it=items.begin(), end=items.end(); it != end; ++it)
    {
        HorizontalLayout* const l(*it);
        l->setSize(biggestWidth, padding);
    }

    return Size<uint>(biggestWidth, totalHeight);
}

void VerticallyStackedHorizontalLayout::setAbsolutePos(const int x, int y, const uint padding)
{
    for (HorizontalLayoutIterator it=items.begin(), end=items.end(); it != end; ++it)
    {
        HorizontalLayout* l(*it);
        y += static_cast<int>(l->setAbsolutePos(x, y, padding));
        y += static_cast<int>(padding);
    }
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
