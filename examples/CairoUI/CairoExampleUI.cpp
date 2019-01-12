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

#include "DistrhoUI.hpp"
#include "DemoWidgetBanner.hpp"
#include "DemoWidgetClickable.hpp"
#include "Window.hpp"

START_NAMESPACE_DISTRHO

class CairoExampleUI : public UI
{
public:
    CairoExampleUI()
        : UI(200, 200)
    {
        DemoWidgetClickable* widgetClickable = new DemoWidgetClickable(this);
        fWidgetClickable = widgetClickable;
        widgetClickable->setSize(50, 50);
        widgetClickable->setAbsolutePos(100, 100);

        DemoWidgetBanner* widgetBanner = new DemoWidgetBanner(this);
        fWidgetBanner = widgetBanner;
        widgetBanner->setSize(180, 80);
        widgetBanner->setAbsolutePos(10, 10);
    }

    ~CairoExampleUI()
    {
    }

    void onDisplay()
    {
        cairo_t* cr = getParentWindow().getGraphicsContext().cairo;

        cairo_set_source_rgb(cr, 1.0, 0.8, 0.5);
        cairo_paint(cr);
    }

    void parameterChanged(uint32_t index, float value)
    {
        // unused
        (void)index;
        (void)value;
    }

private:
    ScopedPointer<DemoWidgetClickable> fWidgetClickable;
    ScopedPointer<DemoWidgetBanner> fWidgetBanner;
};

UI* createUI()
{
    return new CairoExampleUI;
}

END_NAMESPACE_DISTRHO
