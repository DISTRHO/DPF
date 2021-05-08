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

#ifndef EXAMPLE_RECTANGLES_WIDGET_HPP_INCLUDED
#define EXAMPLE_RECTANGLES_WIDGET_HPP_INCLUDED

// ------------------------------------------------------
// DGL Stuff

#include "../../dgl/SubWidget.hpp"
#include "../../dgl/TopLevelWidget.hpp"

START_NAMESPACE_DGL

// ------------------------------------------------------
// our widget

template <class BaseWidget>
class ExampleRectanglesWidget : public BaseWidget
{
    bool clicked[9];

public:
    static constexpr const char* const kExampleWidgetName = "Rectangles";

    explicit ExampleRectanglesWidget(Widget* const parentWidget)
        : BaseWidget(parentWidget)
    {
        init();
    }

    explicit ExampleRectanglesWidget(Window& windowToMapTo)
        : BaseWidget(windowToMapTo)
    {
        init();
    }

    explicit ExampleRectanglesWidget(Application& app)
        : BaseWidget(app)
    {
        init();
    }

    void init()
    {
        this->setSize(300, 300);

        for (int i=0; i<9; ++i)
            clicked[i] = false;
    }

protected:
    void onDisplay() override
    {
        const uint width  = this->getWidth();
        const uint height = this->getHeight();

        Rectangle<double> r;

        r.setWidth(width/3 - 6);
        r.setHeight(height/3 - 6);

        // draw a 3x3 grid
        for (int i=0; i<3; ++i)
        {
            r.setX(3 + i*width/3);

            // 1st
            r.setY(3);

            if (clicked[0+i])
                glColor3f(0.8f, 0.5f, 0.3f);
            else
                glColor3f(0.3f, 0.5f, 0.8f);

            r.draw();

            // 2nd
            r.setY(3 + height/3);

            if (clicked[3+i])
                glColor3f(0.8f, 0.5f, 0.3f);
            else
                glColor3f(0.3f, 0.5f, 0.8f);

            r.draw();

            // 3rd
            r.setY(3 + height*2/3);

            if (clicked[6+i])
                glColor3f(0.8f, 0.5f, 0.3f);
            else
                glColor3f(0.3f, 0.5f, 0.8f);

            r.draw();
        }
    }

    bool onMouse(const MouseEvent& ev) override
    {
        if (ev.button != 1 || ! ev.press)
            return false;

        const uint width  = this->getWidth();
        const uint height = this->getHeight();

        Rectangle<double> r;

        r.setWidth(width/3 - 6);
        r.setHeight(height/3 - 6);

        // draw a 3x3 grid
        for (int i=0; i<3; ++i)
        {
            r.setX(3 + i*width/3);

            // 1st
            r.setY(3);

            if (r.contains(ev.pos))
            {
                clicked[0+i] = !clicked[0+i];
                this->repaint();
                break;
            }

            // 2nd
            r.setY(3 + height/3);

            if (r.contains(ev.pos))
            {
                clicked[3+i] = !clicked[3+i];
                this->repaint();
                break;
            }

            // 3rd
            r.setY(3 + height*2/3);

            if (r.contains(ev.pos))
            {
                clicked[6+i] = !clicked[6+i];
                this->repaint();
                break;
            }
        }

        return true;
    }
};

typedef ExampleRectanglesWidget<SubWidget> ExampleRectanglesSubWidget;
typedef ExampleRectanglesWidget<TopLevelWidget> ExampleRectanglesTopLevelWidget;
typedef ExampleRectanglesWidget<StandaloneWindow> ExampleRectanglesStandaloneWindow;

// ------------------------------------------------------

END_NAMESPACE_DGL

#endif // EXAMPLE_RECTANGLES_WIDGET_HPP_INCLUDED
