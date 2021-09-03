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

#ifndef EXAMPLE_COLOR_WIDGET_HPP_INCLUDED
#define EXAMPLE_COLOR_WIDGET_HPP_INCLUDED

// ------------------------------------------------------
// DGL Stuff

#include "../../dgl/Color.hpp"
#include "../../dgl/StandaloneWindow.hpp"
#include "../../dgl/SubWidget.hpp"

START_NAMESPACE_DGL

// ------------------------------------------------------
// our widget

template <class BaseWidget>
class ExampleColorWidget : public BaseWidget,
                           public IdleCallback
{
    char cur;
    bool reverse;
    int r, g, b;

    Rectangle<uint> bgFull, bgSmall;

public:
    static constexpr const char* kExampleWidgetName = "Color";

    // SubWidget
    explicit ExampleColorWidget(Widget* const parent);

    // TopLevelWidget
    explicit ExampleColorWidget(Window& windowToMapTo);

    // StandaloneWindow
    explicit ExampleColorWidget(Application& app);

protected:
    void idleCallback() noexcept override
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

        BaseWidget::repaint();
    }

    void onDisplay() override
    {
        const GraphicsContext& context(BaseWidget::getGraphicsContext());

        // paint bg color (in full size)
        Color(r, g, b).setFor(context);
        bgFull.draw(context);

        // paint inverted color (in 2/3 size)
        Color(100-r, 100-g, 100-b).setFor(context);
        bgSmall.draw(context);
    }

    void onResize(const Widget::ResizeEvent& ev) override
    {
        const uint width  = ev.size.getWidth();
        const uint height = ev.size.getHeight();

        // full bg
        bgFull = Rectangle<uint>(0, 0, width, height);

        // small bg, centered 2/3 size
        bgSmall = Rectangle<uint>(width/6, height/6, width*2/3, height*2/3);
    }
};

// SubWidget
template<> inline
ExampleColorWidget<SubWidget>::ExampleColorWidget(Widget* const parent)
    : SubWidget(parent),
      cur('r'),
      reverse(false),
      r(0), g(0), b(0)
{
    setSize(300, 300);
    parent->getApp().addIdleCallback(this);
}

// TopLevelWidget
template<> inline
ExampleColorWidget<TopLevelWidget>::ExampleColorWidget(Window& windowToMapTo)
    : TopLevelWidget(windowToMapTo),
      cur('r'),
      reverse(false),
      r(0), g(0), b(0)
{
    setSize(300, 300);
    addIdleCallback(this);
}

// StandaloneWindow
template<> inline
ExampleColorWidget<StandaloneWindow>::ExampleColorWidget(Application& app)
    : StandaloneWindow(app),
      cur('r'),
      reverse(false),
      r(0), g(0), b(0)
{
    setSize(300, 300);
    addIdleCallback(this);
    done();
}

typedef ExampleColorWidget<SubWidget> ExampleColorSubWidget;
typedef ExampleColorWidget<TopLevelWidget> ExampleColorTopLevelWidget;
typedef ExampleColorWidget<StandaloneWindow> ExampleColorStandaloneWindow;

// ------------------------------------------------------

END_NAMESPACE_DGL

#endif // EXAMPLE_COLOR_WIDGET_HPP_INCLUDED
