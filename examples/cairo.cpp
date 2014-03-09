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
#include "CairoWidget.hpp"
#include "Window.hpp"

#include <cstdio>

// ------------------------------------------------------
// use namespace

using namespace DGL;

// ------------------------------------------------------
// Background widget (cairo will be painted on top)

class BackgroundWidget : public Widget
{
public:
    BackgroundWidget(Window& parent)
        : Widget(parent)
    {
    }

private:
    void onDisplay() override
    {
        int x = 0;
        int y = 0;
        int width  = getWidth();
        int height = getHeight();

        // paint bg color (in full size)
        glColor3b(20, 80, 20);

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
        // make this widget same size as window
        setSize(width, height);
        Widget::onReshape(width, height);
    }
};

// ------------------------------------------------------
// Custom Cairo Widget

class CustomCairoWidget : public App::IdleCallback,
                                 CairoWidget
{
public:
    CustomCairoWidget(Window& parent)
        : CairoWidget(parent),
          value(0.0f),
          pressed(false)
    {
        setSize(100, 100);
    }

private:
    void idleCallback() override
    {
        value += 0.001f;

        if (value > 1.0f)
            value = 0;

        repaint();
    }

    void cairoDisplay(cairo_t* const context) override
    {
        const int w = getWidth();
        const int h = getHeight();

        float radius = 40.0f;

        // * 0.9 for line width to remain inside redraw area
        if (w > h)
            radius = (h / 2.0f)*0.9f;
        else
            radius = (w / 2.0f)*0.9f;

        cairo_save(context);

        cairo_rectangle(context, 0, 0, w, h );
        cairo_set_source_rgba(context, 1.1, 0.1, 0.1, 0 );
        cairo_fill(context);

        cairo_set_line_join(context, CAIRO_LINE_JOIN_ROUND);
        cairo_set_line_cap(context, CAIRO_LINE_CAP_ROUND);

        cairo_set_line_width(context, 5-0.2);
        cairo_move_to(context, w/2, h/2);
        cairo_line_to(context, w/2, h/2);
        cairo_set_source_rgba(context, 0.1, 0.1, 0.1, 0 );
        cairo_stroke(context);

        cairo_arc(context, w/2, h/2, radius, 2.46, 0.75 );
        cairo_set_source_rgb(context, 0.1, 0.1, 0.1 );
        cairo_stroke(context);

        float angle = 2.46 + ( 4.54 * value );
        cairo_set_line_width(context, 5);
        cairo_arc(context, w/2, h/2, radius, 2.46, angle );
        cairo_line_to(context, w/2, h/2);
        cairo_set_source_rgba(context, 1.0, 0.48,   0, 0.8);
        cairo_stroke(context);

        cairo_restore(context);
    }

    bool onMouse(int button, bool press, int x, int y)
    {
        if (button == 1)
        {
            pressed = press;

            if (press)
            {
                setX(x-100/2);
                setY(y-100/2);
            }

            return true;
        }

        return false;
    }

    bool onMotion(int x, int y)
    {
        if (pressed)
        {
            setX(x-100/2);
            setY(y-100/2);
            return true;
        }

        return false;
    }

    float value;
    bool pressed;
};

// ------------------------------------------------------
// Custom window, with bg + cairo + image

class CustomWindow : public Window
{
public:
    CustomWindow(App& app)
        : Window(app),
          bg(*this),
          cairo(*this)
    {
        app.addIdleCallback(&cairo);
    }

private:
    BackgroundWidget bg;
    CustomCairoWidget cairo;
};

// ------------------------------------------------------
// main entry point

int main()
{
    App app;
    CustomWindow win(app);

    win.setSize(300, 300);
    win.setTitle("Cairo");
    win.show();
    app.exec();

    return 0;
}

// ------------------------------------------------------
