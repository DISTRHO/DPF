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

#include "TopLevelWidgetPrivateData.hpp"
#include "../Window.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

TopLevelWidget::TopLevelWidget(Window& windowToMapTo)
    : Widget(this),
      pData(new PrivateData(this, windowToMapTo)) {}

TopLevelWidget::~TopLevelWidget()
{
    delete pData;
}

Application& TopLevelWidget::getApp() const noexcept
{
    return pData->window.getApp();
}

Window& TopLevelWidget::getWindow() const noexcept
{
    return pData->window;
}

void TopLevelWidget::setWidth(const uint width)
{
    pData->window.setWidth(width);
}

void TopLevelWidget::setHeight(const uint height)
{
    pData->window.setHeight(height);
}

void TopLevelWidget::setSize(const uint width, const uint height)
{
    pData->window.setSize(width, height);
}

void TopLevelWidget::setSize(const Size<uint>& size)
{
    pData->window.setSize(size);
}

bool TopLevelWidget::addIdleCallback(IdleCallback* const callback, const uint timerFrequencyInMs)
{
    return pData->window.addIdleCallback(callback, timerFrequencyInMs);
}

bool TopLevelWidget::removeIdleCallback(IdleCallback* const callback)
{
    return pData->window.removeIdleCallback(callback);
}

double TopLevelWidget::getScaleFactor() const noexcept
{
    return pData->window.getScaleFactor();
}

void TopLevelWidget::repaint() noexcept
{
    pData->window.repaint();
}

void TopLevelWidget::repaint(const Rectangle<uint>& rect) noexcept
{
    pData->window.repaint(rect);
}

void TopLevelWidget::setGeometryConstraints(const uint minimumWidth,
                                            const uint minimumHeight,
                                            const bool keepAspectRatio,
                                            const bool automaticallyScale)
{
    pData->window.setGeometryConstraints(minimumWidth, minimumHeight, keepAspectRatio, automaticallyScale);
}

// --------------------------------------------------------------------------------------------------------------------

bool TopLevelWidget::onKeyboard(const KeyboardEvent&)
{
    return false;
}

bool TopLevelWidget::onSpecial(const SpecialEvent&)
{
    return false;
}

bool TopLevelWidget::onCharacterInput(const CharacterInputEvent&)
{
    return false;
}

bool TopLevelWidget::onMouse(const MouseEvent&)
{
    return false;
}

bool TopLevelWidget::onMotion(const MotionEvent&)
{
    return false;
}

bool TopLevelWidget::onScroll(const ScrollEvent&)
{
    return false;
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
