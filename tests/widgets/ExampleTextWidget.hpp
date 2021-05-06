/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2015 Filipe Coelho <falktx@falktx.com>
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

#ifndef EXAMPLE_TEXT_WIDGET_HPP_INCLUDED
#define EXAMPLE_TEXT_WIDGET_HPP_INCLUDED

// ------------------------------------------------------
// DGL Stuff

#include "NanoVG.hpp"

// ------------------------------------------------------
// our widget

class ExampleTextWidget : public NanoWidget
{
public:
    ExampleTextWidget(Window& parent)
        : NanoWidget(parent),
          fFont(createFontFromFile("sans", "./nanovg_res/Roboto-Regular.ttf"))
    {
        setSize(500, 300);
    }

    ExampleTextWidget(Widget* groupWidget)
        : NanoWidget(groupWidget),
          fFont(createFontFromFile("sans", "./nanovg_res/Roboto-Regular.ttf"))
    {
        setSize(500, 300);
    }

protected:
    void onNanoDisplay() override
    {
        const int width  = getWidth();
        const int height = getHeight();

        fontSize(40.0f);
        textAlign(Align(ALIGN_CENTER|ALIGN_MIDDLE));
        textLineHeight(20.0f);

        beginPath();
        fillColor(220,220,220,255);
        roundedRect(10, height/4+10, width-20, height/2-20, 3);
        fill();

        fillColor(0,200,0,220);
        textBox(10, height/2, width-20, "Hello World!", nullptr);
    }

private:
    FontId fFont;
};

// ------------------------------------------------------

#endif // EXAMPLE_TEXT_WIDGET_HPP_INCLUDED
