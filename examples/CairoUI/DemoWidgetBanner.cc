//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "DemoWidgetBanner.h"

#include "Cairo.hpp"
#include "Window.hpp"

static const char *banner =
"                                                                        "
"  *     *               *                                 *     *       "
"  **   **               *                           *     *     *       "
"  * * * *               *                                 *     *       "
"  *  *  *   ****    *** *   ****         *     *   **    ****   * ***   "
"  *     *       *  *   **  *    *        *     *    *     *     **   *  "
"  *     *   *****  *    *  ******        *  *  *    *     *     *    *  "
"  *     *  *    *  *    *  *             *  *  *    *     *     *    *  "
"  *     *  *   **  *   **  *    *        *  *  *    *     *  *  *    *  "
"  *     *   *** *   *** *   ****          ** **   *****    **   *    *  "
"                                                                        "
"                                                                        "
"                                                                        "
"                          *****   ****   *****                          "
"                           *   *  *   *  *                              "
"                           *   *  *   *  *                              "
"                           *   *  *   *  *                              "
"                           *   *  ****   ****                           "
"                           *   *  *      *                              "
"                           *   *  *      *                              "
"                           *   *  *      *                              "
"                          *****   *      *                              "
"                                                                        ";

enum {
    rows = 23,
    columns = 72,
};

DemoWidgetBanner::DemoWidgetBanner(Widget *group)
    : Widget(group)
{
}

void DemoWidgetBanner::onDisplay()
{
    cairo_t *cr = getParentWindow().getGraphicsContext().cairo;

    Point<int> pt = getAbsolutePos();
    Size<uint> sz = getSize();

    int x = pt.getX();
    int y = pt.getY();
    int w = sz.getWidth();
    int h = sz.getHeight();

    double diameter = (double)w / columns;
    double radius = 0.5 * diameter;
    double xoff = 0;
    double yoff = 0.5 * (h - rows * diameter);
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < columns; ++c) {
            double cx = x + xoff + radius + c * diameter;
            double cy = y + yoff + radius + r * diameter;

            char ch = banner[c + r * columns];
            if (ch != ' ')
                cairo_set_source_rgb(cr, 0.5, 0.9, 0.2);
            else
                cairo_set_source_rgb(cr, 0.5, 0.5, 0.5);

            cairo_save(cr);
            cairo_translate(cr, cx, cy);
            cairo_scale(cr, radius, radius);
            cairo_arc(cr, 0.0, 0.0, 1.0, 0.0, 2 * M_PI);
            cairo_restore(cr);

            cairo_fill(cr);
        }
    }
}
