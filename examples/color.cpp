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

class ColorWidget : public App::IdleCallback,
                           Widget
{
public:
    ColorWidget(Window& parent)
        : Widget(parent),
          cur('r'),
          reverse(false),
          r(0), g(0), b(0)
    {
    }

private:
    void idleCallback() override
    {
        switch (cur)
        {
        case 'r':
            if (reverse)
            {
                if (--r == 0)
                  cur = 'g';
            }
            else
            {
                if (++r == 100)
                    cur = 'g';
            }
            break;

        case 'g':
            if (reverse)
            {
                if (--g == 0)
                  cur = 'b';
            }
            else
            {
                if (++g == 100)
                  cur = 'b';
            }
            break;

        case 'b':
            if (reverse)
            {
                if (--b == 0)
                {
                    cur = 'r';
                    reverse = false;
                }
            }
            else
            {
                if (++b == 100)
                {
                    cur = 'r';
                    reverse = true;
                }
            }
            break;
        }

        repaint();
    }

    void onDisplay() override
    {
        int x = 0;
        int y = 0;
        int width  = getWidth();
        int height = getHeight();

        // paint bg color (in full size)
        glColor3b(r, g, b);

        glBegin(GL_QUADS);
          glTexCoord2f(0.0f, 0.0f);
          glVertex2i(x, y);

          glTexCoord2f(1.0f, 0.0f);
          glVertex2i(x+width, y);

          glTexCoord2f(1.0f, 1.0f);
          glVertex2i(x+width, y+height);

          glTexCoord2f(0.0f, 1.0f);
          glVertex2i(x, y+height);
        glEnd();

        // centered 2/3 size
        x = width/6;
        y = height/6;
        width  = width*2/3;
        height = height*2/3;

        // paint inverted color (in 2/3 size)
        glColor3b(100-r, 100-g, 100-b);

        glBegin(GL_QUADS);
          glTexCoord2f(0.0f, 0.0f);
          glVertex2i(x, y);

          glTexCoord2f(1.0f, 0.0f);
          glVertex2i(x+width, y);

          glTexCoord2f(1.0f, 1.0f);
          glVertex2i(x+width, y+height);

          glTexCoord2f(0.0f, 1.0f);
          glVertex2i(x, y+height);
        glEnd();
    }

    void onReshape(int width, int height) override
    {
        // make widget same size as window
        setSize(width, height);
        Widget::onReshape(width, height);
    }

    char cur;
    bool reverse;
    int r, g, b;
};

// ------------------------------------------------------
// main entry point

int main()
{
    App app;
    Window win(app);
    ColorWidget color(win);

    app.addIdleCallback(&color);

    win.setSize(300, 300);
    win.setTitle("Color");
    win.show();
    app.exec();

    return 0;
}

// ------------------------------------------------------
