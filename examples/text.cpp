/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

// ------------------------------------------------------
// DGL Stuff

#include "App.hpp"
#include "Window.hpp"
#include "Widget.hpp"

#include <cstdio>

// ------------------------------------------------------
// use namespace

using namespace DGL;

// ------------------------------------------------------
// Single color widget

class TextWidget : public Widget
{
public:
    TextWidget(Window& parent)
        : Widget(parent)
    {
    }

private:
    void onDisplay() override
    {
    }

    void onReshape(int width, int height) override
    {
        // make widget same size as window
        setSize(width, height);
        Widget::onReshape(width, height);
    }
};

// ------------------------------------------------------
// main entry point

int main()
{
    App app;
    Window win(app);
    TextWidget color(win);

    win.setSize(600, 300);
    win.setTitle("Text");
    win.show();
    app.exec();

    return 0;
}

// ------------------------------------------------------
