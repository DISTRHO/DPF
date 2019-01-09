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

#include "PluginUI.h"
#include "DemoWidgetClickable.h"
#include "DemoWidgetBanner.h"

#include "Window.hpp"

ExampleUI::ExampleUI()
    : UI(200, 200)
{
    DemoWidgetClickable *widget_clickable = new DemoWidgetClickable(this);
    widget_clickable_.reset(widget_clickable);
    widget_clickable->setSize(50, 50);
    widget_clickable->setAbsolutePos(100, 100);

    DemoWidgetBanner *widget_banner = new DemoWidgetBanner(this);
    widget_banner_.reset(widget_banner);
    widget_banner->setSize(180, 80);
    widget_banner->setAbsolutePos(10, 10);
}

ExampleUI::~ExampleUI()
{
}

void ExampleUI::onDisplay()
{
    cairo_t *cr = getParentWindow().getGraphicsContext().cairo;

    cairo_set_source_rgb(cr, 1.0, 0.8, 0.5);
    cairo_paint(cr);
}

void ExampleUI::parameterChanged(uint32_t, float)
{
}

UI *DISTRHO::createUI()
{
    return new ExampleUI;
}
