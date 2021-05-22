/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2021 Filipe Coelho <falktx@falktx.com>
 * Copyright (C) 2019-2021 Jean Pierre Cimalando <jp-dev@inbox.ru>
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

#include "Artwork.hpp"
#include "DemoWidgetBanner.hpp"
#include "DemoWidgetClickable.hpp"

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

        CairoImage knobSkin;
        knobSkin.loadFromPNG(Artwork::knobData, Artwork::knobDataSize);

        CairoImageKnob* knob = new CairoImageKnob(this, knobSkin);
        fKnob = knob;
        knob->setSize(80, 80);
        knob->setAbsolutePos(10, 100);

        CairoImage buttonOn, buttonOff;
        buttonOn.loadFromPNG(Artwork::buttonOnData, Artwork::buttonOnDataSize);
        buttonOff.loadFromPNG(Artwork::buttonOffData, Artwork::buttonOffDataSize);

        CairoImageButton* button = new CairoImageButton(this, buttonOff, buttonOn);
        fButton = button;
        button->setSize(60, 35);
        button->setAbsolutePos(100, 160);
    }

protected:
    void onCairoDisplay(const CairoGraphicsContext& context)
    {
        cairo_t* const cr = context.handle;
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
    ScopedPointer<CairoImageKnob> fKnob;
    ScopedPointer<CairoImageButton> fButton;
};

UI* createUI()
{
    return new CairoExampleUI;
}

END_NAMESPACE_DISTRHO
