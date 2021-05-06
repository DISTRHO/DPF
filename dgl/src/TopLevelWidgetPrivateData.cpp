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

#define FOR_EACH_WIDGET(it) \
  for (std::list<Widget*>::iterator it = widgets.begin(); it != widgets.end(); ++it)

#define FOR_EACH_WIDGET_INV(rit) \
  for (std::list<Widget*>::reverse_iterator rit = widgets.rbegin(); rit != widgets.rend(); ++rit)

// -----------------------------------------------------------------------

TopLevelWidget::PrivateData::PrivateData(TopLevelWidget* const s, Window& w)
    : self(s),
      window(w),
      widgets() {}

void TopLevelWidget::PrivateData::display()
{
    puglFallbackOnDisplay(window.pData->view);

    if (widgets.size() == 0)
        return;

    const Size<uint> size(window.getSize());
    const uint width     = size.getWidth();
    const uint height    = size.getHeight();
    const double scaling = window.pData->autoScaling;

    FOR_EACH_WIDGET(it)
    {
        Widget* const widget(*it);
        widget->pData->display(width, height, scaling, false);
    }
}

void TopLevelWidget::PrivateData::resize(const uint width, const uint height)
{
    if (widgets.size() == 0)
        return;
/*
    FOR_EACH_WIDGET(it)
    {
        Widget* const widget(*it);

        if (widget->pData->needsFullViewport)
            widget->setSize(width, height);
    }*/
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
