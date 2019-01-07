/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2019 Filipe Coelho <falktx@falktx.com>
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

#include "DemoWidgetClickable.h"

#include "Cairo.hpp"
#include "Window.hpp"

DemoWidgetClickable::DemoWidgetClickable(Widget *group)
    : Widget(group)
{
}

void DemoWidgetClickable::onDisplay()
{
    cairo_t *cr = getParentWindow().getGraphicsContext().cairo;

    Point<int> pt = getAbsolutePos();
    Size<uint> sz = getSize();

    int x = pt.getX();
    int y = pt.getY();
    int w = sz.getWidth();
    int h = sz.getHeight();

    switch (colorid_) {
    case 0:
        cairo_set_source_rgb(cr, 0.75, 0.0, 0.0);
        break;
    case 1:
        cairo_set_source_rgb(cr, 0.0, 0.75, 0.0);
        break;
    case 2:
        cairo_set_source_rgb(cr, 0.0, 0.0, 0.75);
        break;
    }
    cairo_rectangle(cr, x, y, w, h);
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
    cairo_new_path(cr);
    cairo_move_to(cr, x + 0.25 * w, y + 0.25 * h);
    cairo_line_to(cr, x + 0.75 * w, y + 0.75 * h);
    cairo_stroke(cr);
    cairo_new_path(cr);
    cairo_move_to(cr, x + 0.75 * w, y + 0.25 * h);
    cairo_line_to(cr, x + 0.25 * w, y + 0.75 * h);
    cairo_stroke(cr);
}

bool DemoWidgetClickable::onMouse(const MouseEvent &event)
{
    if (event.press) {
        Point<int> pos = getAbsolutePos();
        Size<uint> size = getSize();

        int mx = event.pos.getX();
        int my = event.pos.getY();
        int px = pos.getX();
        int py = pos.getY();

        bool inside = mx >= 0 && my >= 0 &&
            mx < size.getWidth() && my < size.getHeight();

        if (inside) {
            colorid_ = (colorid_ + 1) % 3;
            repaint();
        }
    }

    return Widget::onMouse(event);
}
