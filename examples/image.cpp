/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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
// DGL Image

#include "Image.hpp"

// ------------------------------------------------------
// DGL Widget and StandaloneWindow

#include "Widget.hpp"
#include "StandaloneWindow.hpp"

// ------------------------------------------------------
// use namespace

using namespace DGL;

// ------------------------------------------------------
// our widget

class ExampleImageWidget : public Widget
{
public:
    ExampleImageWidget(Window& win)
        : Widget(win)
    {
        // TODO: load image
    }

private:
    void onDisplay() override
    {
        fImage.draw();
    }

    Image fImage;
};

// ------------------------------------------------------
// main entry point

int main()
{
    StandaloneWindow appWin;
    Window& win(appWin.getWindow());

    ExampleImageWidget gui(win);
    win.setResizable(false);
    win.setSize(200, 200);
    win.setTitle("Image");
    win.show();

    appWin.exec();

    return 0;
}

// ------------------------------------------------------
