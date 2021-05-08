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
    printf("Widget::PrivateData::displaySubWidgets INIT | %lu\n", subWidgets.size());

    if (subWidgets.size() == 0)
        return;

    for (std::list<SubWidget*>::iterator it = subWidgets.begin(); it != subWidgets.end(); ++it)
    {
        SubWidget* const subwidget(*it);
        printf("Widget::PrivateData::displaySubWidgets %i %i -> %p\n", width, height, subwidget);

        subwidget->pData->display(width, height, scaling);
    }
}

// -----------------------------------------------------------------------

TopLevelWidget* Widget::PrivateData::findTopLevelWidget(Widget* const w)
{
//     if (TopLevelWidget* const tlw = dynamic_cast<TopLevelWidget*>(w))
//         return tlw;
    if (w->pData->topLevelWidget != nullptr)
        return w->pData->topLevelWidget;
    if (w->pData->parentWidget != nullptr)
        return findTopLevelWidget(w->pData->parentWidget);
    return nullptr;
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
