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

private:
    void onDisplay() override
    {
        glColor3b(r, g, b);

        // full size
        const int width  = getWidth();
        const int height = getHeight();

        glBegin(GL_QUADS);
          glTexCoord2i(0, height);
          glVertex2i(0, height);

          glTexCoord2i(width, height);
          glVertex2i(width, height);

          glTexCoord2i(width, 0);
          glVertex2i(width, 0);

          glTexCoord2i(0, 0);
          glVertex2i(0, 0);
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
