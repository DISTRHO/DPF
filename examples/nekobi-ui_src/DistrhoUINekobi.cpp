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

#include "DistrhoUINekobi.hpp"

using DGL::Point;

// -----------------------------------------------------------------------

DistrhoUINekobi::DistrhoUINekobi(DGL::Window& parent)
    : DGL::Widget(parent),
      fAboutWindow(this)
{
    fNeko.setTimerSpeed(15);

    // background
    fImgBackground = Image(DistrhoArtworkNekobi::backgroundData, DistrhoArtworkNekobi::backgroundWidth, DistrhoArtworkNekobi::backgroundHeight, GL_BGR);

    Image imageAbout(DistrhoArtworkNekobi::aboutData, DistrhoArtworkNekobi::aboutWidth, DistrhoArtworkNekobi::aboutHeight, GL_BGR);
    fAboutWindow.setImage(imageAbout);

    // slider
    Image sliderImage(DistrhoArtworkNekobi::sliderData, DistrhoArtworkNekobi::sliderWidth, DistrhoArtworkNekobi::sliderHeight);

    fSliderWaveform = new ImageSlider(this, sliderImage);
    fSliderWaveform->setStartPos(133, 40);
    fSliderWaveform->setEndPos(133, 60);
    fSliderWaveform->setRange(0.0f, 1.0f);
    fSliderWaveform->setValue(0.0f);
    fSliderWaveform->setStep(1.0f);

    // knobs
    Image knobImage(DistrhoArtworkNekobi::knobData, DistrhoArtworkNekobi::knobWidth, DistrhoArtworkNekobi::knobHeight);

    // knob Tuning
    fKnobTuning = new ImageKnob(this, knobImage);
    fKnobTuning->setPos(41, 43);
    fKnobTuning->setRange(-12.0f, 12.0f);
    fKnobTuning->setValue(0.0f);
    fKnobTuning->setRotationAngle(305);

    // knob Cutoff
    fKnobCutoff = new ImageKnob(this, knobImage);
    fKnobCutoff->setPos(185, 43);
    fKnobCutoff->setRange(0.0f, 100.0f);
    fKnobCutoff->setValue(25.0f);
    fKnobCutoff->setRotationAngle(305);

    // knob Resonance
    fKnobResonance = new ImageKnob(this, knobImage);
    fKnobResonance->setPos(257, 43);
    fKnobResonance->setRange(0.0f, 95.0f);
    fKnobResonance->setValue(25.0f);
    fKnobResonance->setRotationAngle(305);

    // knob Env Mod
    fKnobEnvMod = new ImageKnob(this, knobImage);
    fKnobEnvMod->setPos(329, 43);
    fKnobEnvMod->setRange(0.0f, 100.0f);
    fKnobEnvMod->setValue(50.0f);
    fKnobEnvMod->setRotationAngle(305);

    // knob Decay
    fKnobDecay = new ImageKnob(this, knobImage);
    fKnobDecay->setPos(400, 43);
    fKnobDecay->setRange(0.0f, 100.0f);
    fKnobDecay->setValue(75.0f);
    fKnobDecay->setRotationAngle(305);

    // knob Accent
    fKnobAccent = new ImageKnob(this, knobImage);
    fKnobAccent->setPos(473, 43);
    fKnobAccent->setRange(0.0f, 100.0f);
    fKnobAccent->setValue(25.0f);
    fKnobAccent->setRotationAngle(305);

    // knob Volume
    fKnobVolume = new ImageKnob(this, knobImage);
    fKnobVolume->setPos(545, 43);
    fKnobVolume->setRange(0.0f, 100.0f);
    fKnobVolume->setValue(75.0f);
    fKnobVolume->setRotationAngle(305);

    // about button
    Image aboutImageNormal(DistrhoArtworkNekobi::aboutButtonNormalData, DistrhoArtworkNekobi::aboutButtonNormalWidth, DistrhoArtworkNekobi::aboutButtonNormalHeight);
    Image aboutImageHover(DistrhoArtworkNekobi::aboutButtonHoverData, DistrhoArtworkNekobi::aboutButtonHoverWidth, DistrhoArtworkNekobi::aboutButtonHoverHeight);
    fButtonAbout = new ImageButton(this, aboutImageNormal, aboutImageHover, aboutImageHover);
    fButtonAbout->setPos(505, 5);
    fButtonAbout->setCallback(this);
}

DistrhoUINekobi::~DistrhoUINekobi()
{
    delete fSliderWaveform;
    delete fKnobTuning;
    delete fKnobCutoff;
    delete fKnobResonance;
    delete fKnobEnvMod;
    delete fKnobDecay;
    delete fKnobAccent;
    delete fKnobVolume;
    delete fButtonAbout;
}

void DistrhoUINekobi::idle()
{
    if (fNeko.idle())
        repaint();
}

// -----------------------------------------------------------------------
// Widget Callbacks

void DistrhoUINekobi::imageButtonClicked(ImageButton* button, int)
{
    if (button != fButtonAbout)
        return;

    fAboutWindow.exec();
}

void DistrhoUINekobi::onDisplay()
{
    fImgBackground.draw();
    fNeko.draw();
}

// -----------------------------------------------------------------------
