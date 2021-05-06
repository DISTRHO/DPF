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
// #include "pugl.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

TopLevelWidget::TopLevelWidget(Window& windowToMapTo)
    : Widget(this),
      pData(new PrivateData(this, windowToMapTo)) {}

TopLevelWidget::~TopLevelWidget()
{
    delete pData;
}

void TopLevelWidget::onDisplay()
{
    pData->display();
}

void TopLevelWidget::onResize(const ResizeEvent& ev)
{
    pData->resize(ev.size.getWidth(), ev.size.getHeight());
    Widget::onResize(ev);
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
