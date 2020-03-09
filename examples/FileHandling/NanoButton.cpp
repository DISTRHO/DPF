/*
 * Copyright (C) 2018-2019 Rob van den Berg <rghvdberg at gmail dot org>
 *
 * This file is part of CharacterCompressor
 *
 * Nnjas2 is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CharacterCompressor is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CharacterCompressor.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "NanoButton.hpp"
#include "Window.hpp"

START_NAMESPACE_DISTRHO

Button::Button(Widget *parent, Callback *cb)
    : NanoWidget(parent),
      fCallback(cb),
      buttonActive(false)
{
    loadSharedResources();
    fNanoFont = findFont(NANOVG_DEJAVU_SANS_TTF);
    labelColor = Color(255, 255, 255);
    backgroundColor = Color(32,32,32);
    Label = "button";
}

void Button::onNanoDisplay()
{
    auto w = getWidth();
    auto h = getHeight();
    auto margin = 1.0f;

    // Background
    beginPath();
    fillColor(backgroundColor);
    strokeColor(borderColor);
    rect(margin, margin, w - 2 * margin, h-2*margin);
    fill();
    stroke();
    closePath();

    //Label
    beginPath();
    fontFaceId(fNanoFont);
    fontSize(14);
    fillColor(labelColor);
    Rectangle<float> bounds;
    textBounds(0, 0, Label.c_str(), NULL, bounds);
    // float tw = bounds.getWidth();
    // float th = bounds.getHeight();
    float tx = w / 2.0f ;
    float ty = h / 2.0f;
    textAlign(ALIGN_CENTER | ALIGN_MIDDLE);

    fillColor(255, 255, 255, 255);
    text(tx, ty, Label.c_str(), NULL);
    closePath();
}

void Button::setLabel(std::string label)
{
    Label = label;
}

void Button::setLabelColor(const Color color)
{
    labelColor = color;
    borderColor = color;
}
void Button::setBackgroundColor(const Color color)
{
    backgroundColor = color;
}

bool Button::onMouse(const MouseEvent &ev)
{
    if (ev.press & contains(ev.pos))
    {
        buttonActive = true;
        setLabelColor(labelColor);     
        fCallback->buttonClicked(this, true);
        return true;
    }
    else if (buttonActive)
    {
        buttonActive = false;
        //setLabelColor(Color(128,128,128));
        return true;
    }

    return false;
}

END_NAMESPACE_DISTRHO
