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

#include "SubWidgetPrivateData.hpp"
#include "../TopLevelWidget.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

SubWidget::SubWidget(Widget* const parentWidget)
    : Widget(parentWidget),
      pData(new PrivateData(this, parentWidget)) {}

SubWidget::~SubWidget()
{
    delete pData;
}

template<typename T>
bool SubWidget::contains(T x, T y) const noexcept
{
    return Rectangle<double>(0, 0, getWidth(), getHeight()).contains(x, y);
}

template<typename T>
bool SubWidget::contains(const Point<T>& pos) const noexcept
{
    return contains(pos.getX(), pos.getY());
}

int SubWidget::getAbsoluteX() const noexcept
{
    return pData->absolutePos.getX();
}

int SubWidget::getAbsoluteY() const noexcept
{
    return pData->absolutePos.getY();
}

Point<int> SubWidget::getAbsolutePos() const noexcept
{
    return pData->absolutePos;
}

Rectangle<int> SubWidget::getAbsoluteArea() const noexcept
{
    return Rectangle<int>(getAbsolutePos(), getSize().toInt());
}

Rectangle<uint> SubWidget::getConstrainedAbsoluteArea() const noexcept
{
    return Rectangle<uint>(static_cast<uint>(std::max(0, getAbsoluteX())),
                           static_cast<uint>(std::max(0, getAbsoluteY())),
                           getSize());
}

void SubWidget::setAbsoluteX(int x) noexcept
{
    setAbsolutePos(Point<int>(x, getAbsoluteY()));
}

void SubWidget::setAbsoluteY(int y) noexcept
{
    setAbsolutePos(Point<int>(getAbsoluteX(), y));
}

void SubWidget::setAbsolutePos(int x, int y) noexcept
{
    setAbsolutePos(Point<int>(x, y));
}

void SubWidget::setAbsolutePos(const Point<int>& pos) noexcept
{
    if (pData->absolutePos == pos)
        return;

    PositionChangedEvent ev;
    ev.oldPos = pData->absolutePos;
    ev.pos = pos;

    pData->absolutePos = pos;
    onPositionChanged(ev);

    repaint();
}

Widget* SubWidget::getParentWidget() const noexcept
{
    return pData->parentWidget;
}

void SubWidget::repaint() noexcept
{
    if (! isVisible())
        return;

    if (TopLevelWidget* const topw = getTopLevelWidget())
    {
        if (pData->needsFullViewportForDrawing)
            topw->repaint();
        else
            topw->repaint(getConstrainedAbsoluteArea());
    }
}

void SubWidget::setNeedsFullViewportDrawing(const bool needsFullViewportForDrawing)
{
    pData->needsFullViewportForDrawing = needsFullViewportForDrawing;
}

void SubWidget::onPositionChanged(const PositionChangedEvent&)
{
}

// --------------------------------------------------------------------------------------------------------------------
// Possible template data types

template<>
bool SubWidget::contains(const Point<double>& pos) const noexcept
{
    return contains(pos.getX(), pos.getY());
}

// float, int, uint, short, ushort

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
