/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2019-2021 Jean Pierre Cimalando <jp-dev@inbox.ru>
 * Copyright (C) 2012-2024 Filipe Coelho <falktx@falktx.com>
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

#pragma once

#include "Cairo.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

static constexpr const char banner[] =
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

static constexpr const int kNumRows = 23;
static constexpr const int kNumColumns = 72;

class DemoWidgetBanner : public CairoSubWidget
{
public:
    explicit DemoWidgetBanner(SubWidget* const parent)
        : CairoSubWidget(parent) {}

    explicit DemoWidgetBanner(TopLevelWidget* const parent)
        : CairoSubWidget(parent) {}

protected:
    void onCairoDisplay(const CairoGraphicsContext& context) override
    {
        cairo_t* const cr = context.handle;

        Size<uint> sz = getSize();
        int w = sz.getWidth();
        int h = sz.getHeight();

        const double diameter = (double)w / kNumColumns;
        const double radius = 0.5 * diameter;
        const double xoff = 0;
        const double yoff = 0.5 * (h - kNumRows * diameter);

        for (int r = 0; r < kNumRows; ++r)
        {
            for (int c = 0; c < kNumColumns; ++c)
            {
                double cx = xoff + radius + c * diameter;
                double cy = yoff + radius + r * diameter;

                char ch = banner[c + r * kNumColumns];
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
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
