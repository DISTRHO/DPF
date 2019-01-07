//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

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

void ExampleUI::parameterChanged(uint32_t index, float value)
{
}

UI *DISTRHO::createUI()
{
    return new ExampleUI;
}
