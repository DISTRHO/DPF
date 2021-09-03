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

#include "../../dgl/Color.hpp"
#include "../../dgl/StandaloneWindow.hpp"
#include "../../dgl/SubWidget.hpp"

START_NAMESPACE_DGL

// ------------------------------------------------------
// our widget

template <class BaseWidget>
class ExampleRectanglesWidget : public BaseWidget
{
    bool clicked[9];

public:
    static constexpr const char* const kExampleWidgetName = "Rectangles";

    // SubWidget
    explicit ExampleRectanglesWidget(Widget* const parent);

    // TopLevelWidget
    explicit ExampleRectanglesWidget(Window& windowToMapTo);

    // StandaloneWindow
    explicit ExampleRectanglesWidget(Application& app);

    void init()
    {
        BaseWidget::setSize(300, 300);

        for (int i=0; i<9; ++i)
            clicked[i] = false;
    }

protected:
    void onDisplay() override
    {
        const GraphicsContext& context(BaseWidget::getGraphicsContext());

        const uint width  = BaseWidget::getWidth();
        const uint height = BaseWidget::getHeight();

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
                Color(0.8f, 0.5f, 0.3f).setFor(context);
            else
                Color(0.3f, 0.5f, 0.8f).setFor(context);

            r.draw(context);

            // 2nd
            r.setY(3 + height/3);

            if (clicked[3+i])
                Color(0.8f, 0.5f, 0.3f).setFor(context);
            else
                Color(0.3f, 0.5f, 0.8f).setFor(context);

            r.draw(context);

            // 3rd
            r.setY(3 + height*2/3);

            if (clicked[6+i])
                Color(0.8f, 0.5f, 0.3f).setFor(context);
            else
                Color(0.3f, 0.5f, 0.8f).setFor(context);

            r.draw(context);
        }
    }

    bool onMouse(const Widget::MouseEvent& ev) override
    {
        if (ev.button != 1 || ! ev.press)
            return false;

        const uint width  = BaseWidget::getWidth();
        const uint height = BaseWidget::getHeight();

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
                BaseWidget::repaint();
                break;
            }

            // 2nd
            r.setY(3 + height/3);

            if (r.contains(ev.pos))
            {
                clicked[3+i] = !clicked[3+i];
                BaseWidget::repaint();
                break;
            }

            // 3rd
            r.setY(3 + height*2/3);

            if (r.contains(ev.pos))
            {
                clicked[6+i] = !clicked[6+i];
                BaseWidget::repaint();
                break;
            }
        }

        return true;
    }
};

// SubWidget
template<> inline
ExampleRectanglesWidget<SubWidget>::ExampleRectanglesWidget(Widget* const parentWidget)
    : SubWidget(parentWidget)
{
    init();
}

// TopLevelWidget
template<> inline
ExampleRectanglesWidget<TopLevelWidget>::ExampleRectanglesWidget(Window& windowToMapTo)
    : TopLevelWidget(windowToMapTo)
{
    init();
}

// StandaloneWindow
template<> inline
ExampleRectanglesWidget<StandaloneWindow>::ExampleRectanglesWidget(Application& app)
    : StandaloneWindow(app)
{
    init();
    done();
}

typedef ExampleRectanglesWidget<SubWidget> ExampleRectanglesSubWidget;
typedef ExampleRectanglesWidget<TopLevelWidget> ExampleRectanglesTopLevelWidget;
typedef ExampleRectanglesWidget<StandaloneWindow> ExampleRectanglesStandaloneWindow;

// ------------------------------------------------------

END_NAMESPACE_DGL

#endif // EXAMPLE_RECTANGLES_WIDGET_HPP_INCLUDED
