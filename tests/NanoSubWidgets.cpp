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

#include "../dgl/NanoVG.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

class NanoRectangle : public NanoSubWidget
{
public:
    explicit NanoRectangle(Widget* const parent)
        : NanoSubWidget(parent),
          color() {}

    void setColor(const Color c) noexcept
    {
        color = c;
    }

protected:
    void onNanoDisplay() override
    {
        beginPath();

        fillColor(color);
        rect(0, 0, getWidth(), getHeight());
        fill();

        closePath();
    }

private:
    Color color;
};

// --------------------------------------------------------------------------------------------------------------------

class NanoRectanglesContainer : public NanoTopLevelWidget
{
public:
    explicit NanoRectanglesContainer(Window& parent)
        : NanoTopLevelWidget(parent),
          rect1(this),
          rect2(this),
          rect3(this)
    {
        rect1.setAbsolutePos(100, 100);
        rect1.setSize(25, 25);
        rect1.setColor(Color(255, 0, 0));

        rect2.setAbsolutePos(200, 200);
        rect2.setSize(25, 25);
        rect2.setColor(Color(0, 255, 0));

        rect3.setAbsolutePos(300, 300);
        rect3.setSize(25, 25);
        rect3.setColor(Color(0, 0, 255));
    }

protected:
    void onNanoDisplay() override
    {
    }

private:
    NanoRectangle rect1, rect2, rect3;
};

// --------------------------------------------------------------------------------------------------------------------

class NanoExampleWindow : public Window
{
public:
    explicit NanoExampleWindow(Application& app)
        : Window(app),
          container(*this)
    {
        const uint targetWidth = 1000;
        const uint targetHeight = 600;

        setSize(targetWidth, targetHeight);
        // container.setSize(width, height);

        setTitle("NanoVG SubWidgets test");
    }

    /*
protected:
    void onReshape(uint width, uint height) override
    {
        container.setSize(width, height);

        Window::onReshape(width, height);
    }
    */

private:
    NanoRectanglesContainer container;
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

int main()
{
    USE_NAMESPACE_DGL;

    Application app;
    NanoExampleWindow win(app);

    win.show();
    app.exec();

    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
