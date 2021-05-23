/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2021 Filipe Coelho <falktx@falktx.com>
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

#include "tests.hpp"

#include "dgl/NanoVG.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

class NanoFilePicker : public NanoStandaloneWindow
{
    Rectangle<uint> buttonBounds;
    bool buttonClick = false;
    bool buttonHover = false;

public:
    NanoFilePicker(Application& app)
      : NanoStandaloneWindow(app)
    {
#ifndef DGL_NO_SHARED_RESOURCES
        loadSharedResources();
#endif

    }

protected:
    void onNanoDisplay() override
    {
        Color labelColor(255, 255, 255);
        Color backgroundColor(32,
                              buttonClick ? 128 : 32,
                              buttonHover ? 128 : 32);
        Color borderColor;

        // Button background
        beginPath();
        fillColor(backgroundColor);
        strokeColor(borderColor);
        rect(buttonBounds.getX(), buttonBounds.getY(), buttonBounds.getWidth(), buttonBounds.getHeight());
        fill();
        stroke();
        closePath();

        // Label
        beginPath();
        fontSize(14);
        fillColor(labelColor);
        Rectangle<float> buttonTextBounds;
        textBounds(0, 0, "Press me", NULL, buttonTextBounds);
        textAlign(ALIGN_CENTER | ALIGN_MIDDLE);

        fillColor(255, 255, 255, 255);
        text(buttonBounds.getX() + buttonBounds.getWidth()/2,
             buttonBounds.getY() + buttonBounds.getHeight()/2,
             "Press me", NULL);
        closePath();
    }

    bool onMotion(const MotionEvent& ev) override
    {
        const bool newButtonHover = buttonBounds.contains(ev.pos);

        if (newButtonHover != buttonHover)
        {
            buttonHover = newButtonHover;
            repaint();
            return true;
        }

        return newButtonHover;
    }

    bool onMouse(const MouseEvent& ev) override
    {
        if (ev.button != 1)
            return false;

        if (! buttonBounds.contains(ev.pos))
        {
            if (buttonClick)
            {
                buttonClick = false;
                repaint();
                return true;
            }
            return false;
        }

        const bool newButtonClick = ev.press;

        if (newButtonClick != buttonClick)
        {
            buttonClick = newButtonClick;
            repaint();

            if (newButtonClick)
            {
                FileBrowserOptions opts;
                opts.title = "Look at me";
                openFileBrowser(opts);
            }

            return true;
        }

        return newButtonClick;
    }

    void onResize(const ResizeEvent& ev) override
    {
        const uint width  = ev.size.getWidth();
        const uint height = ev.size.getHeight();

        buttonBounds = Rectangle<uint>(width - 120, height/2 - 20, 100, 40);
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

int main()
{
    USE_NAMESPACE_DGL;

    Application app(true);
    NanoFilePicker win(app);
    win.setSize(500, 200);
    win.setTitle("FileBrowserDialog");
    win.show();
    app.exec();

    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
