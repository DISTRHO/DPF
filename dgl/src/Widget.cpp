/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2015 Filipe Coelho <falktx@falktx.com>
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

#include "../Widget.hpp"
#include "../Window.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------
// Widget

Widget::Widget(Window& parent)
    : fParent(parent),
      fNeedsFullViewport(false),
      fNeedsScaling(false),
      fSkipDisplay(false),
      fVisible(true),
      fId(0),
      fAbsolutePos(0, 0),
      fSize(0, 0),
      leakDetector_Widget()
{
    fParent._addWidget(this);
}

Widget::Widget(Widget* groupWidget)
    : fParent(groupWidget->getParentWindow()),
      fNeedsFullViewport(false),
      fNeedsScaling(false),
      fSkipDisplay(true),
      fVisible(true),
      fId(0),
      fAbsolutePos(0, 0),
      fSize(0, 0),
      leakDetector_Widget()
{
    fParent._addWidget(this);
    groupWidget->fSubWidgets.push_back(this);
}

Widget::Widget(Widget* groupWidget, bool addToSubWidgets)
    : fParent(groupWidget->getParentWindow()),
      fNeedsFullViewport(false),
      fNeedsScaling(false),
      fSkipDisplay(true),
      fVisible(true),
      fId(0),
      fAbsolutePos(0, 0),
      fSize(0, 0),
      leakDetector_Widget()
{
    fParent._addWidget(this);

    if (addToSubWidgets)
        groupWidget->fSubWidgets.push_back(this);
}

Widget::~Widget()
{
    fParent._removeWidget(this);
    fSubWidgets.clear();
}

bool Widget::isVisible() const noexcept
{
    return fVisible;
}

void Widget::setVisible(bool yesNo)
{
    if (fVisible == yesNo)
        return;

    fVisible = yesNo;
    fParent.repaint();
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
    return fSize.getWidth();
}

uint Widget::getHeight() const noexcept
{
    return fSize.getHeight();
}

const Size<uint>& Widget::getSize() const noexcept
{
    return fSize;
}

void Widget::setWidth(uint width) noexcept
{
    if (fSize.getWidth() == width)
        return;

    ResizeEvent ev;
    ev.oldSize = fSize;
    ev.size    = Size<uint>(width, fSize.getHeight());

    fSize.setWidth(width);
    onResize(ev);

    fParent.repaint();
}

void Widget::setHeight(uint height) noexcept
{
    if (fSize.getHeight() == height)
        return;

    ResizeEvent ev;
    ev.oldSize = fSize;
    ev.size    = Size<uint>(fSize.getWidth(), height);

    fSize.setHeight(height);
    onResize(ev);

    fParent.repaint();
}

void Widget::setSize(uint width, uint height) noexcept
{
    setSize(Size<uint>(width, height));
}

void Widget::setSize(const Size<uint>& size) noexcept
{
    if (fSize == size)
        return;

    ResizeEvent ev;
    ev.oldSize = fSize;
    ev.size    = size;

    fSize = size;
    onResize(ev);

    fParent.repaint();
}

int Widget::getAbsoluteX() const noexcept
{
    return fAbsolutePos.getX();
}

int Widget::getAbsoluteY() const noexcept
{
    return fAbsolutePos.getY();
}

const Point<int>& Widget::getAbsolutePos() const noexcept
{
    return fAbsolutePos;
}

void Widget::setAbsoluteX(int x) noexcept
{
    if (fAbsolutePos.getX() == x)
        return;

    fAbsolutePos.setX(x);
    fParent.repaint();
}

void Widget::setAbsoluteY(int y) noexcept
{
    if (fAbsolutePos.getY() == y)
        return;

    fAbsolutePos.setY(y);
    fParent.repaint();
}

void Widget::setAbsolutePos(int x, int y) noexcept
{
    setAbsolutePos(Point<int>(x, y));
}

void Widget::setAbsolutePos(const Point<int>& pos) noexcept
{
    if (fAbsolutePos == pos)
        return;

    fAbsolutePos = pos;
    fParent.repaint();
}

Application& Widget::getParentApp() const noexcept
{
    return fParent.getApp();
}

Window& Widget::getParentWindow() const noexcept
{
    return fParent;
}

bool Widget::contains(int x, int y) const noexcept
{
    return (x >= 0 && y >= 0 && static_cast<uint>(x) < fSize.getWidth() && static_cast<uint>(y) < fSize.getHeight());
}

bool Widget::contains(const Point<int>& pos) const noexcept
{
    return contains(pos.getX(), pos.getY());
}

void Widget::repaint() noexcept
{
    fParent.repaint();
}

uint Widget::getId() const noexcept
{
    return fId;
}

void Widget::setId(uint id) noexcept
{
    fId = id;
}

bool Widget::onKeyboard(const KeyboardEvent&)
{
    return false;
}

bool Widget::onSpecial(const SpecialEvent&)
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

void Widget::_displaySubWidgets()
{
    for (std::vector<Widget*>::iterator it = fSubWidgets.begin(); it != fSubWidgets.end(); ++it)
    {
        Widget* const widget(*it);

        if (widget->fNeedsScaling)
        {
            // limit viewport to widget bounds
            glViewport(widget->getAbsoluteX(),
                       fParent.getHeight() - static_cast<int>(widget->getHeight()) - widget->getAbsoluteY(),
                       static_cast<GLsizei>(widget->getWidth()),
                       static_cast<GLsizei>(widget->getHeight()));
        }
        else
        {
            // only set viewport pos
            glViewport(widget->getAbsoluteX(),
                       /*fParent.getHeight() - static_cast<int>(widget->getHeight())*/ - widget->getAbsoluteY(),
                       static_cast<GLsizei>(fParent.getWidth()),
                       static_cast<GLsizei>(fParent.getHeight()));

            // then cut the outer bounds
            glScissor(widget->getAbsoluteX(),
                      fParent.getHeight() - static_cast<int>(widget->getHeight()) - widget->getAbsoluteY(),
                      static_cast<GLsizei>(widget->getWidth()),
                      static_cast<GLsizei>(widget->getHeight()));

            glEnable(GL_SCISSOR_TEST);
        }

        widget->onDisplay();

        if (! fNeedsScaling)
            glDisable(GL_SCISSOR_TEST);
    }
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
