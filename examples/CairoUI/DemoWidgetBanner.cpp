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

#include "DemoWidgetBanner.hpp"

#include "Cairo.hpp"
#include "Window.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

static const char* banner =
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

enum
{
    rows = 23,
    columns = 72,
};

DemoWidgetBanner::DemoWidgetBanner(Widget* group)
    : Widget(group)
{
}

void DemoWidgetBanner::onDisplay()
{
    cairo_t* cr = getParentWindow().getGraphicsContext().cairo;

    Size<uint> sz = getSize();
    int w = sz.getWidth();
    int h = sz.getHeight();

    double diameter = (double)w / columns;
    double radius = 0.5 * diameter;
    double xoff = 0;
    double yoff = 0.5 * (h - rows * diameter);
    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < columns; ++c)
        {
            double cx = xoff + radius + c * diameter;
            double cy = yoff + radius + r * diameter;

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

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
