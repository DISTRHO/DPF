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

#ifdef DGL_CAIRO
# include "../Cairo.hpp"
#endif
#ifdef DGL_OPENGL
# include "../OpenGL.hpp"
#endif

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

Widget::PrivateData::PrivateData(Widget* const s, TopLevelWidget* const tlw)
    : self(s),
      topLevelWidget(tlw),
      groupWidget(nullptr),
      id(0),
      needsScaling(false),
      visible(true),
      size(0, 0),
      subWidgets()
{
}

Widget::PrivateData::PrivateData(Widget* const s, Widget* const g)
    : self(s),
      topLevelWidget(findTopLevelWidget(g)),
      groupWidget(g),
      id(0),
      needsScaling(false),
      visible(true),
      size(0, 0),
      subWidgets()
{
    groupWidget->pData->subWidgets.push_back(self);
}

Widget::PrivateData::~PrivateData()
{
    if (groupWidget != nullptr)
        groupWidget->pData->subWidgets.remove(self);

    subWidgets.clear();
}

void Widget::PrivateData::displaySubWidgets(const uint width, const uint height, const double scaling)
{
    for (std::list<Widget*>::iterator it = subWidgets.begin(); it != subWidgets.end(); ++it)
    {
        Widget* const widget(*it);
        DISTRHO_SAFE_ASSERT_CONTINUE(widget->pData != this);

        widget->pData->display(width, height, scaling, true);
    }
}

void Widget::PrivateData::repaint()
{
    if (groupWidget != nullptr)
        groupWidget->repaint();
    else if (topLevelWidget != nullptr)
        topLevelWidget->repaint();
}

// -----------------------------------------------------------------------

TopLevelWidget* Widget::PrivateData::findTopLevelWidget(Widget* const w)
{
    if (w->pData->topLevelWidget != nullptr)
        return w->pData->topLevelWidget;
    if (w->pData->groupWidget != nullptr)
        return findTopLevelWidget(w->pData->groupWidget);
    return nullptr;
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
