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

#ifndef BUTTON_HPP_INCLUDED
#define BUTTON_HPP_INCLUDED

#include "Widget.hpp"
#include "NanoVG.hpp"

#include <string>

START_NAMESPACE_DISTRHO

class Button : public NanoWidget
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void buttonClicked(Button *button, bool value) = 0;
    };
    explicit Button(Widget *parent, Callback *cb);

    void setLabel(std::string label);
    void setLabelColor(Color color);
    void setBackgroundColor(Color color);

protected:
    void onNanoDisplay() override;
    bool onMouse(const MouseEvent &ev) override;

private:
    std::string Label;
    Color labelColor,backgroundColor,borderColor;
    Callback *const fCallback;
    bool buttonActive;
    FontId fNanoFont;
    DISTRHO_LEAK_DETECTOR(Button)
};

END_NAMESPACE_DISTRHO

#endif
