/*
 * DISTRHO Nekobi Plugin, based on Nekobee by Sean Bolton and others.
 * Copyright (C) 2013 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the doc/GPL.txt file.
 */

#ifndef DISTRHO_UI_NEKOBI_HPP_INCLUDED
#define DISTRHO_UI_NEKOBI_HPP_INCLUDED

#include "ImageAboutWindow.hpp"
#include "ImageButton.hpp"
#include "ImageKnob.hpp"
#include "ImageSlider.hpp"

#include "DistrhoArtworkNekobi.hpp"
#include "NekoWidget.hpp"

using DGL::ImageAboutWindow;
using DGL::ImageButton;
using DGL::ImageKnob;
using DGL::ImageSlider;

// -----------------------------------------------------------------------

class DistrhoUINekobi : public DGL::Widget,
                        public ImageButton::Callback
{
public:
    DistrhoUINekobi(DGL::Window& parent);
    ~DistrhoUINekobi() override;

    unsigned int getWidth() const noexcept
    {
        return DistrhoArtworkNekobi::backgroundWidth;
    }

    unsigned int getHeight() const noexcept
    {
        return DistrhoArtworkNekobi::backgroundHeight;
    }

    void idle();

protected:
    // -------------------------------------------------------------------
    // Widget Callbacks

    void imageButtonClicked(ImageButton* button, int) override;
    void onDisplay() override;

private:
    Image      fImgBackground;
    NekoWidget fNeko;

    ImageKnob* fKnobTuning;
    ImageKnob* fKnobCutoff;
    ImageKnob* fKnobResonance;
    ImageKnob* fKnobEnvMod;
    ImageKnob* fKnobDecay;
    ImageKnob* fKnobAccent;
    ImageKnob* fKnobVolume;

    ImageButton* fButtonAbout;
    ImageSlider* fSliderWaveform;
    ImageAboutWindow fAboutWindow;
};

// -----------------------------------------------------------------------

#endif // DISTRHO_UI_NEKOBI_HPP_INCLUDED
