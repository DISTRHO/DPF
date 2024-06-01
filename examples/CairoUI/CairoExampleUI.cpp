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

// We need a few classes from DGL.
using DGL_NAMESPACE::CairoGraphicsContext;
using DGL_NAMESPACE::CairoImage;
using DGL_NAMESPACE::CairoImageKnob;
using DGL_NAMESPACE::CairoImageSwitch;

// And from ourselves
using DGL_NAMESPACE::DemoWidgetBanner;

class CairoExampleUI : public UI,
                       public CairoImageKnob::Callback,
                       public CairoImageSwitch::Callback,
                       public DemoWidgetClickable::Callback
{
    ScopedPointer<CairoImageKnob> fKnob;
    ScopedPointer<CairoImageSwitch> fButton;
    ScopedPointer<DemoWidgetBanner> fWidgetBanner;
    ScopedPointer<DemoWidgetClickable> fWidgetClickable;

public:
    CairoExampleUI()
    {
        CairoImage knobSkin;
        knobSkin.loadFromPNG(Artwork::knobData, Artwork::knobDataSize);

        fWidgetBanner = new DemoWidgetBanner(this);
        fWidgetBanner->setAbsolutePos(10, 10);
        fWidgetBanner->setSize(180, 80);

        fWidgetClickable = new DemoWidgetClickable(this);
        fWidgetClickable->setAbsolutePos(100, 100);
        fWidgetClickable->setSize(50, 50);
        fWidgetClickable->setCallback(this);
        fWidgetClickable->setId(kParameterTriState);

        fKnob = new CairoImageKnob(this, knobSkin);
        fKnob->setAbsolutePos(10, 100);
        fKnob->setSize(80, 80);
        fKnob->setCallback(this);
        fKnob->setId(kParameterKnob);

        CairoImage buttonOn, buttonOff;
        buttonOn.loadFromPNG(Artwork::buttonOnData, Artwork::buttonOnDataSize);
        buttonOff.loadFromPNG(Artwork::buttonOffData, Artwork::buttonOffDataSize);

        fButton = new CairoImageSwitch(this, buttonOff, buttonOn);
        fButton->setAbsolutePos(100, 160);
        fButton->setSize(60, 35);
        fButton->setCallback(this);
        fButton->setId(kParameterButton);

        setGeometryConstraints(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT, true, true);
    }

protected:
    void onCairoDisplay(const CairoGraphicsContext& context) override
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

    void parameterChanged(const uint32_t index, const float value) override
    {
        switch (index)
        {
        case kParameterKnob:
            fKnob->setValue(value);
            break;
        case kParameterTriState:
            fWidgetClickable->setColorId(static_cast<int>(value + 0.5f));
            break;
        case kParameterButton:
            fButton->setDown(value > 0.5f);
            break;
        }
    }

    void demoWidgetClicked(DemoWidgetClickable*, const uint8_t colorId) override
    {
        setParameterValue(kParameterTriState, colorId);
    }

    void imageKnobDragStarted(CairoImageKnob*) override
    {
        editParameter(kParameterKnob, true);
    }

    void imageKnobDragFinished(CairoImageKnob*) override
    {
        editParameter(kParameterKnob, false);
    }

    void imageKnobValueChanged(CairoImageKnob*, const float value) override
    {
        setParameterValue(kParameterKnob, value);
    }

    void imageSwitchClicked(CairoImageSwitch*, bool down) override
    {
        setParameterValue(kParameterButton, down ? 1.f : 0.f);
    }
};

UI* createUI()
{
    return new CairoExampleUI;
}

END_NAMESPACE_DISTRHO
