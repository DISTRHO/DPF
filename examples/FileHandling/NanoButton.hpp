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

#ifndef NANO_BUTTON_HPP_INCLUDED
#define NANO_BUTTON_HPP_INCLUDED

#include "NanoVG.hpp"
#include "EventHandlers.hpp"

#include <string>

START_NAMESPACE_DGL

class Button : public NanoSubWidget,
               public ButtonEventHandler
{
public:
    explicit Button(Widget* parent, ButtonEventHandler::Callback* cb);
    ~Button() override;

    void setBackgroundColor(Color color);
    void setFontScale(float scale);
    void setLabel(const std::string& label);
    void setLabelColor(Color color);

protected:
    void onNanoDisplay() override;
    bool onMouse(const MouseEvent& ev) override;
    bool onMotion(const MotionEvent& ev) override;

private:
    Color backgroundColor;
    Color labelColor;
    std::string label;
    float fontScale;

    DISTRHO_LEAK_DETECTOR(Button)
};

END_NAMESPACE_DGL

#endif // NANO_BUTTON_HPP_INCLUDED
