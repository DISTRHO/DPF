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
#include "ImageWidgets.hpp"
#include "Window.hpp"
#include "Artwork.hpp"

START_NAMESPACE_DISTRHO

class CairoExampleUI : public UI
{
public:
    CairoExampleUI()
        : UI(200, 200)
    {
        const GraphicsContext& gc = getParentWindow().getGraphicsContext();

        DemoWidgetClickable* widgetClickable = new DemoWidgetClickable(this);
        fWidgetClickable = widgetClickable;
        widgetClickable->setSize(50, 50);
        widgetClickable->setAbsolutePos(100, 100);

        DemoWidgetBanner* widgetBanner = new DemoWidgetBanner(this);
        fWidgetBanner = widgetBanner;
        widgetBanner->setSize(180, 80);
        widgetBanner->setAbsolutePos(10, 10);

        Image knobSkin;
        knobSkin.loadFromPng(Artwork::knobData, Artwork::knobDataSize, &gc);

        ImageKnob* knob = new ImageKnob(this, knobSkin);
        fKnob = knob;
        knob->setSize(80, 80);
        knob->setAbsolutePos(10, 100);
        // knob->setRotationAngle(270);

        Image buttonOn, buttonOff;
        buttonOn.loadFromPng(Artwork::buttonOnData, Artwork::buttonOnDataSize, &gc);
        buttonOff.loadFromPng(Artwork::buttonOffData, Artwork::buttonOffDataSize, &gc);

        ImageButton* button = new ImageButton(this, buttonOff, buttonOn);
        fButton = button;
        button->setSize(60, 35);
        button->setAbsolutePos(100, 160);
    }

    ~CairoExampleUI()
    {
    }

    void onDisplay()
    {
        cairo_t* cr = getParentWindow().getGraphicsContext().cairo;

        cairo_set_source_rgb(cr, 1.0, 0.8, 0.5);
        cairo_paint(cr);

        ImageKnob* knob = fKnob;
        cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.1);
        cairo_rectangle(cr, knob->getAbsoluteX(), knob->getAbsoluteY(), knob->getWidth(), knob->getHeight());
        cairo_fill(cr);

        ImageButton* button = fButton;
        cairo_set_source_rgba(cr, 1.0, 0.0, 0.0, 0.1);
        cairo_rectangle(cr, button->getAbsoluteX(), button->getAbsoluteY(), button->getWidth(), button->getHeight());
        cairo_fill(cr);
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
    ScopedPointer<ImageKnob> fKnob;
    ScopedPointer<ImageButton> fButton;
};

UI* createUI()
{
    return new CairoExampleUI;
}

END_NAMESPACE_DISTRHO
