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
#include "SubWidgetPrivateData.hpp"
#include "../TopLevelWidget.hpp"

START_NAMESPACE_DGL

#define FOR_EACH_SUBWIDGET(it) \
  for (std::list<SubWidget*>::iterator it = subWidgets.begin(); it != subWidgets.end(); ++it)

#define FOR_EACH_SUBWIDGET_INV(rit) \
  for (std::list<SubWidget*>::reverse_iterator rit = subWidgets.rbegin(); rit != subWidgets.rend(); ++rit)

// -----------------------------------------------------------------------

Widget::PrivateData::PrivateData(Widget* const s, TopLevelWidget* const tlw)
    : self(s),
      topLevelWidget(tlw),
      parentWidget(nullptr),
      id(0),
      needsScaling(false),
      visible(true),
      size(0, 0),
      subWidgets() {}

Widget::PrivateData::PrivateData(Widget* const s, Widget* const pw)
    : self(s),
      topLevelWidget(findTopLevelWidget(pw)),
      parentWidget(pw),
      id(0),
      needsScaling(false),
      visible(true),
      size(0, 0),
      subWidgets() {}

Widget::PrivateData::~PrivateData()
{
    subWidgets.clear();
}

void Widget::PrivateData::displaySubWidgets(const uint width, const uint height, const double scaling)
{
    if (subWidgets.size() == 0)
        return;

    for (std::list<SubWidget*>::iterator it = subWidgets.begin(); it != subWidgets.end(); ++it)
    {
        SubWidget* const subwidget(*it);

        if (subwidget->isVisible())
            subwidget->pData->display(width, height, scaling);
    }
}

void Widget::PrivateData::giveMouseEventForSubWidgets(Events::MouseEvent& ev)
{
    if (! visible)
        return;
    if (subWidgets.size() == 0)
        return;

    const double x = ev.pos.getX();
    const double y = ev.pos.getY();

    FOR_EACH_SUBWIDGET_INV(rit)
    {
        SubWidget* const widget(*rit);

        if (! widget->isVisible())
            continue;

        ev.pos = Point<double>(x - widget->getAbsoluteX(),
                               y - widget->getAbsoluteY());

        if (widget->onMouse(ev))
            return;
    }
}

// -----------------------------------------------------------------------

TopLevelWidget* Widget::PrivateData::findTopLevelWidget(Widget* const pw)
{
    if (pw->pData->topLevelWidget != nullptr)
        return pw->pData->topLevelWidget;
    if (pw->pData->parentWidget != nullptr)
        return findTopLevelWidget(pw->pData->parentWidget);
    return nullptr;
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
