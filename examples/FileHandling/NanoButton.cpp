/*
 * Copyright (C) 2018-2019 Rob van den Berg <rghvdberg at gmail dot org>
 * Copyright (C) 2020-2021 Filipe Coelho <falktx@falktx.com>
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

START_NAMESPACE_DGL

Button::Button(Widget* const parent, ButtonEventHandler::Callback* const cb)
    : NanoWidget(parent),
      ButtonEventHandler(this),
      backgroundColor(32, 32, 32),
      labelColor(255, 255, 255),
      label("button"),
      fontScale(1.0f)
{
#ifdef DGL_NO_SHARED_RESOURCES
    createFontFromFile("sans", "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSans.ttf");
#else
    loadSharedResources();
#endif
    ButtonEventHandler::setCallback(cb);
}

Button::~Button()
{
}

void Button::setBackgroundColor(const Color color)
{
    backgroundColor = color;
}

void Button::setFontScale(const float scale)
{
    fontScale = scale;
}

void Button::setLabel(const std::string& label2)
{
    label = label2;
}

void Button::setLabelColor(const Color color)
{
    labelColor = color;
}

void Button::onNanoDisplay()
{
    const uint w = getWidth();
    const uint h = getHeight();
    const float margin = 1.0f;

    // Background
    beginPath();
    fillColor(backgroundColor);
    strokeColor(labelColor);
    rect(margin, margin, w - 2 * margin, h - 2 * margin);
    fill();
    stroke();
    closePath();

    // Label
    beginPath();
    fontSize(14 * fontScale);
    fillColor(labelColor);
    Rectangle<float> bounds;
    textBounds(0, 0, label.c_str(), NULL, bounds);
    float tx = w / 2.0f ;
    float ty = h / 2.0f;
    textAlign(ALIGN_CENTER | ALIGN_MIDDLE);

    fillColor(255, 255, 255, 255);
    text(tx, ty, label.c_str(), NULL);
    closePath();
}

bool Button::onMouse(const MouseEvent& ev)
{
    return ButtonEventHandler::mouseEvent(ev);
}

bool Button::onMotion(const MotionEvent& ev)
{
    return ButtonEventHandler::motionEvent(ev);
}

END_NAMESPACE_DGL
