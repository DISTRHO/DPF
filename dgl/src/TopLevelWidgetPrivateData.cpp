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
#include "WidgetPrivateData.hpp"
#include "WindowPrivateData.hpp"
#include "pugl.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

TopLevelWidget::PrivateData::PrivateData(TopLevelWidget* const s, Window& w)
    : self(s),
      selfw(s),
      window(w)
{
    window.pData->topLevelWidget = self;
}

TopLevelWidget::PrivateData::~PrivateData()
{
    // FIXME?
    window.pData->topLevelWidget = nullptr;
}

void TopLevelWidget::PrivateData::mouseEvent(const Events::MouseEvent& ev)
{
    Events::MouseEvent rev = ev;

    const double autoScaling = window.pData->autoScaling;

    if (autoScaling != 1.0)
    {
        rev.pos.setX(ev.pos.getX() / autoScaling);
        rev.pos.setY(ev.pos.getY() / autoScaling);
    }

    // give top-level widget chance to catch this event first
    if (self->onMouse(ev))
        return;

    // propagate event to all subwidgets recursively
    selfw->pData->giveMouseEventForSubWidgets(rev);
}

void TopLevelWidget::PrivateData::fallbackOnResize()
{
    puglFallbackOnResize(window.pData->view);
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
