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

/**
  We need a few classes from DGL.
 */
using DGL_NAMESPACE::CairoGraphicsContext;
using DGL_NAMESPACE::CairoImage;
using DGL_NAMESPACE::CairoImageButton;
using DGL_NAMESPACE::CairoImageKnob;

class CairoExampleUI : public UI
{
public:
    CairoExampleUI()
        : UI(200, 200)
    {
        fWidgetClickable = new DemoWidgetClickable(this);
        fWidgetClickable->setSize(50, 50);
        fWidgetClickable->setAbsolutePos(100, 100);

        fWidgetBanner = new DemoWidgetBanner(this);
        fWidgetBanner->setSize(180, 80);
        fWidgetBanner->setAbsolutePos(10, 10);

        CairoImage knobSkin;
        knobSkin.loadFromPNG(Artwork::knobData, Artwork::knobDataSize);

        fKnob = new CairoImageKnob(this, knobSkin);
        fKnob->setSize(80, 80);
        fKnob->setAbsolutePos(10, 100);

        CairoImage buttonOn, buttonOff;
        buttonOn.loadFromPNG(Artwork::buttonOnData, Artwork::buttonOnDataSize);
        buttonOff.loadFromPNG(Artwork::buttonOffData, Artwork::buttonOffDataSize);

        fButton = new CairoImageButton(this, buttonOff, buttonOn);
        fButton->setSize(60, 35);
        fButton->setAbsolutePos(100, 160);

#if 0
        // we can use this if/when our resources are scalable, for now they are PNGs
        const double scaleFactor = getScaleFactor();
        if (scaleFactor != 1.0)
            setSize(200 * scaleFactor, 200 * scaleFactor);
#else
        // without scalable resources, let DPF handle the scaling internally
        setGeometryConstraints(200, 200, true, true);
#endif
    }

protected:
    void onCairoDisplay(const CairoGraphicsContext& context)
    {
        cairo_t* const cr = context.handle;
        cairo_set_source_rgb(cr, 1.0, 0.8, 0.5);
        cairo_paint(cr);
    }

#if 0
    // we can use this if/when our resources are scalable, for now they are PNGs
    void onResize(const ResizeEvent& ev) override
    {
        UI::onResize(ev);

        const double scaleFactor = getScaleFactor();

        fWidgetClickable->setSize(50*scaleFactor, 50*scaleFactor);
        fWidgetClickable->setAbsolutePos(100*scaleFactor, 100*scaleFactor);

        fWidgetBanner->setSize(180*scaleFactor, 80*scaleFactor);
        fWidgetBanner->setAbsolutePos(10*scaleFactor, 10*scaleFactor);

        fKnob->setSize(80*scaleFactor, 80*scaleFactor);
        fKnob->setAbsolutePos(10*scaleFactor, 100*scaleFactor);

        fButton->setSize(60*scaleFactor, 35*scaleFactor);
        fButton->setAbsolutePos(100*scaleFactor, 160*scaleFactor);
    }
#endif

    void parameterChanged(uint32_t index, float value) override
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
