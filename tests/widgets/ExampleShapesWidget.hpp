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

#ifndef EXAMPLE_SHAPES_WIDGET_HPP_INCLUDED
#define EXAMPLE_SHAPES_WIDGET_HPP_INCLUDED

// ------------------------------------------------------
// DGL Stuff

#include "../../dgl/Color.hpp"
#include "../../dgl/StandaloneWindow.hpp"
#include "../../dgl/SubWidget.hpp"

START_NAMESPACE_DGL

// ------------------------------------------------------
// our widget

template <class BaseWidget>
class ExampleShapesWidget : public BaseWidget
{
    Rectangle<int> bg;
    Rectangle<int> rect;
    Triangle<int> tri;
    Circle<int> cir;

public:
    static constexpr const char* const kExampleWidgetName = "Shapes";

    // SubWidget
    explicit ExampleShapesWidget(Widget* const parent);

    // TopLevelWidget
    explicit ExampleShapesWidget(Window& windowToMapTo);

    // StandaloneWindow
    explicit ExampleShapesWidget(Application& app);

protected:
    void onDisplay() override
    {
        const GraphicsContext& context(BaseWidget::getGraphicsContext());

        Color(0.302f, 0.337f, 0.361f).setFor(context);;
        bg.draw(context);

        Color(0.235f, 0.271f, 0.294f).setFor(context);
        rect.draw(context);

        Color(0.176f, 0.212f, 0.235f).setFor(context);
        rect.drawOutline(context, 1);

        Color(0.302f*2, 0.337f*2, 0.361f*2).setFor(context);
        tri.draw(context);

        Color(0.302f/2.0f, 0.337f/2.0f, 0.361f/2.0f).setFor(context);
        tri.drawOutline(context, 3);

        Color(0.235f, 0.271f, 0.294f).setFor(context);
        cir.draw(context);

        Color(0.176f/4, 0.212f/4, 0.235f/4).setFor(context);
        cir.drawOutline(context, 2);
    }

    void onResize(const Widget::ResizeEvent& ev) override
    {
        const int width  = ev.size.getWidth();
        const int height = ev.size.getHeight();

        // background
        bg = Rectangle<int>(0, 0, width, height);

        // rectangle
        rect = Rectangle<int>(20, 10, width-40, height-20);

        // center triangle
        tri = Triangle<int>(width*0.5, height*0.1, width*0.1, height*0.9, width*0.9, height*0.9);

        // circle
        cir = Circle<int>(width/2, height*2/3, height/6, 300);
    }
};

// SubWidget
template<> inline
ExampleShapesWidget<SubWidget>::ExampleShapesWidget(Widget* const parentWidget)
    : SubWidget(parentWidget)
{
    setSize(300, 300);
}

// TopLevelWidget
template<> inline
ExampleShapesWidget<TopLevelWidget>::ExampleShapesWidget(Window& windowToMapTo)
    : TopLevelWidget(windowToMapTo)
{
    setSize(300, 300);
}

// StandaloneWindow
template<> inline
ExampleShapesWidget<StandaloneWindow>::ExampleShapesWidget(Application& app)
    : StandaloneWindow(app)
{
    setSize(300, 300);
    done();
}

typedef ExampleShapesWidget<SubWidget> ExampleShapesSubWidget;
typedef ExampleShapesWidget<TopLevelWidget> ExampleShapesTopLevelWidget;
typedef ExampleShapesWidget<StandaloneWindow> ExampleShapesStandaloneWindow;

// ------------------------------------------------------

END_NAMESPACE_DGL

#endif // EXAMPLE_SHAPES_WIDGET_HPP_INCLUDED
