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

#include "WidgetPrivateData.hpp"
#include "../TopLevelWidget.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------
// Widget

Widget::Widget(TopLevelWidget* const topLevelWidget)
    : pData(new PrivateData(this, topLevelWidget)) {}

Widget::Widget(Widget* const parentWidget)
    : pData(new PrivateData(this, parentWidget)) {}

Widget::~Widget()
{
    delete pData;
}

bool Widget::isVisible() const noexcept
{
    return pData->visible;
}

void Widget::setVisible(bool yesNo)
{
    if (pData->visible == yesNo)
        return;

    pData->visible = yesNo;
    repaint();
}

void Widget::show()
{
    setVisible(true);
}

void Widget::hide()
{
    setVisible(false);
}

uint Widget::getWidth() const noexcept
{
    return pData->size.getWidth();
}

uint Widget::getHeight() const noexcept
{
    return pData->size.getHeight();
}

const Size<uint> Widget::getSize() const noexcept
{
    return pData->size;
}

void Widget::setWidth(uint width) noexcept
{
    if (pData->size.getWidth() == width)
        return;

    ResizeEvent ev;
    ev.oldSize = pData->size;
    ev.size    = Size<uint>(width, pData->size.getHeight());

    pData->size.setWidth(width);
    onResize(ev);

    repaint();
}

void Widget::setHeight(uint height) noexcept
{
    if (pData->size.getHeight() == height)
        return;

    ResizeEvent ev;
    ev.oldSize = pData->size;
    ev.size    = Size<uint>(pData->size.getWidth(), height);

    pData->size.setHeight(height);
    onResize(ev);

    repaint();
}

void Widget::setSize(uint width, uint height) noexcept
{
    setSize(Size<uint>(width, height));
}

void Widget::setSize(const Size<uint>& size) noexcept
{
    if (pData->size == size)
        return;

    ResizeEvent ev;
    ev.oldSize = pData->size;
    ev.size    = size;

    pData->size = size;
    onResize(ev);

    repaint();
}

Application& Widget::getApp() const noexcept
{
    return pData->topLevelWidget->getApp();
}

TopLevelWidget* Widget::getTopLevelWidget() const noexcept
{
    return pData->topLevelWidget;
}

void Widget::repaint() noexcept
{
}

uint Widget::getId() const noexcept
{
    return pData->id;
}

void Widget::setId(uint id) noexcept
{
    pData->id = id;
}

bool Widget::onKeyboard(const KeyboardEvent&)
{
    return false;
}

bool Widget::onSpecial(const SpecialEvent&)
{
    return false;
}

bool Widget::onCharacterInput(const CharacterInputEvent&)
{
    return false;
}

bool Widget::onMouse(const MouseEvent&)
{
    return false;
}

bool Widget::onMotion(const MotionEvent&)
{
    return false;
}

bool Widget::onScroll(const ScrollEvent&)
{
    return false;
}

void Widget::onResize(const ResizeEvent&)
{
}

// -----------------------------------------------------------------------

// -----------------------------------------------------------------------

// -----------------------------------------------------------------------

#if 0
Widget::Widget(Widget* groupWidget)
    : pData(new PrivateData(this, groupWidget->getParentWindow(), groupWidget, true))
{
}

Widget::Widget(Widget* groupWidget, bool addToSubWidgets)
    : pData(new PrivateData(this, groupWidget->getParentWindow(), groupWidget, addToSubWidgets))
{
}

Window& Widget::getParentWindow() const noexcept
{
    return pData->parent;
}

void Widget::setNeedsFullViewport()
{
    pData->needsFullViewport = true;
}
#endif

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
