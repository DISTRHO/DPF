//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

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
