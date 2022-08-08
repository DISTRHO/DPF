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

#ifndef EXAMPLE_TEXT_WIDGET_HPP_INCLUDED
#define EXAMPLE_TEXT_WIDGET_HPP_INCLUDED

// ------------------------------------------------------
// DGL Stuff

#include "../../dgl/NanoVG.hpp"

START_NAMESPACE_DGL

// ------------------------------------------------------
// our widget

template <class BaseWidget>
class ExampleTextWidget : public BaseWidget
{
public:
    static constexpr const char* kExampleWidgetName = "Text";

    // SubWidget
    explicit ExampleTextWidget(Widget* const parent);

    // TopLevelWidget
    explicit ExampleTextWidget(Window& windowToMapTo);

    // StandaloneWindow
    explicit ExampleTextWidget(Application& app);

    // helper
    double getScaleFactor();

protected:
    void onNanoDisplay() override
    {
        const int width  = BaseWidget::getWidth();
        const int height = BaseWidget::getHeight();
        const double scaleFactor = getScaleFactor();

        NanoVG::fontSize(40.0f * scaleFactor);
        NanoVG::textAlign(NanoVG::Align(NanoVG::ALIGN_CENTER|NanoVG::ALIGN_MIDDLE));
        NanoVG::textLineHeight(20.0f * scaleFactor);

        NanoVG::beginPath();
        NanoVG::fillColor(220,220,220,255);
        NanoVG::roundedRect(10 * scaleFactor, height/4 + 10 * scaleFactor,
                            width - 20 * scaleFactor, height/2 - 20 * scaleFactor, 3 * scaleFactor);
        NanoVG::fill();

        NanoVG::fillColor(0,150,0,220);
        NanoVG::textBox(10 * scaleFactor, height/2, width - 20 * scaleFactor, "Hello World!", nullptr);
    }
};

// SubWidget
template<> inline
ExampleTextWidget<NanoSubWidget>::ExampleTextWidget(Widget* const parent)
    : NanoSubWidget(parent)
{
    loadSharedResources();
    setSize(500, 300);
}

template<> inline
double ExampleTextWidget<NanoSubWidget>::getScaleFactor()
{
    return getWindow().getScaleFactor();
}

// TopLevelWidget
template<> inline
ExampleTextWidget<NanoTopLevelWidget>::ExampleTextWidget(Window& windowToMapTo)
    : NanoTopLevelWidget(windowToMapTo)
{
    loadSharedResources();
    setSize(500, 300);
}

template<> inline
double ExampleTextWidget<NanoTopLevelWidget>::getScaleFactor()
{
    return NanoTopLevelWidget::getScaleFactor();
}

// StandaloneWindow
template<> inline
ExampleTextWidget<NanoStandaloneWindow>::ExampleTextWidget(Application& app)
    : NanoStandaloneWindow(app)
{
    loadSharedResources();
    setSize(500, 300);
    done();
}

template<> inline
double ExampleTextWidget<NanoStandaloneWindow>::getScaleFactor()
{
    return Window::getScaleFactor();
}

template class ExampleTextWidget<NanoSubWidget>;
template class ExampleTextWidget<NanoTopLevelWidget>;
template class ExampleTextWidget<NanoStandaloneWindow>;

typedef ExampleTextWidget<NanoSubWidget> ExampleTextSubWidget;
typedef ExampleTextWidget<NanoTopLevelWidget> ExampleTextTopLevelWidget;
typedef ExampleTextWidget<NanoStandaloneWindow> ExampleTextStandaloneWindow;

// ------------------------------------------------------

END_NAMESPACE_DGL

#endif // EXAMPLE_TEXT_WIDGET_HPP_INCLUDED
