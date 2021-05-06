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

#ifndef DGL_OPENGL
#error OpenGL build required for Demo
#endif

#include "tests.hpp"

// #define DPF_TEST_POINT_CPP
#include "dgl/src/pugl.cpp"
// #include "dgl/src/SubWidget.cpp"
#include "dgl/src/Application.cpp"
#include "dgl/src/ApplicationPrivateData.cpp"
#include "dgl/src/Geometry.cpp"
#include "dgl/src/OpenGL.cpp"
#include "dgl/src/SubWidget.cpp"
#include "dgl/src/TopLevelWidget.cpp"
#include "dgl/src/TopLevelWidgetPrivateData.cpp"
#include "dgl/src/Widget.cpp"
#include "dgl/src/WidgetPrivateData.cpp"
#include "dgl/src/Window.cpp"
#include "dgl/src/WindowPrivateData.cpp"
#include "dgl/StandaloneWindow.hpp"

#include "widgets/ExampleColorWidget.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

// ------------------------------------------------------
// Left side tab-like widget

class LeftSideWidget : public SubWidget
{
public:
    static const int kPageCount = 5;

    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void curPageChanged(int curPage) = 0;
    };

    LeftSideWidget(Widget* parent, Callback* const cb)
        : SubWidget(parent),
          callback(cb),
          curPage(0),
          curHover(-1)
    {
        // for text
//         font = nvg.createFontFromFile("sans", "./nanovg_res/Roboto-Regular.ttf");

//         using namespace DemoArtwork;
//         img1.loadFromMemory(ico1Data, ico1Width, ico1Height, GL_BGR);
//         img2.loadFromMemory(ico2Data, ico2Width, ico2Height, GL_BGR);
//         img3.loadFromMemory(ico3Data, ico3Width, ico2Height, GL_BGR);
//         img4.loadFromMemory(ico4Data, ico4Width, ico4Height, GL_BGR);
//         img5.loadFromMemory(ico5Data, ico5Width, ico5Height, GL_BGR);
    }

private:
    Callback* const callback;
    int curPage, curHover;
//     Rectangle<int> bgIcon;
//     Line<int> lineSep;
//     Image img1, img2, img3, img4, img5;

    // for text
//     NanoVG nvg;
//     NanoVG::FontId font;
};

// ------------------------------------------------------
// Our Demo Window

class DemoWindow : public StandaloneWindow,
                   public LeftSideWidget::Callback
{
    static const int kSidebarWidth = 81;

    ExampleColorWidget wColor;
    Widget* curWidget;

public:
    DemoWindow(Application& app)
        : StandaloneWindow(app),
          wColor(this),
          curWidget(nullptr)
    {
        wColor.hide();
//         wImages.hide();
//         wRects.hide();
//         wShapes.hide();
//         wText.hide();
//         //wPerf.hide();

        wColor.setAbsoluteX(kSidebarWidth);
//         wImages.setAbsoluteX(kSidebarWidth);
//         wRects.setAbsoluteX(kSidebarWidth);
//         wShapes.setAbsoluteX(kSidebarWidth);
//         wText.setAbsoluteX(kSidebarWidth);
//         wLeft.setAbsolutePos(2, 2);
//         wPerf.setAbsoluteY(5);

        setSize(600, 500);
        setTitle("DGL Demo");

        curPageChanged(0);
    }

protected:
    void curPageChanged(int curPage) override
    {
        if (curWidget != nullptr)
        {
            curWidget->hide();
            curWidget = nullptr;
        }

        switch (curPage)
        {
        case 0:
            curWidget = &wColor;
            break;
//         case 1:
//             curWidget = &wImages;
//             break;
//         case 2:
//             curWidget = &wRects;
//             break;
//         case 3:
//             curWidget = &wShapes;
//             break;
//         case 4:
//             curWidget = &wText;
//             break;
        }

        if (curWidget != nullptr)
            curWidget->show();
    }

    void onReshape(uint width, uint height) override
    {
        StandaloneWindow::onReshape(width, height);

        if (width < kSidebarWidth)
            return;

        Size<uint> size(width-kSidebarWidth, height);
        wColor.setSize(size);
//         wImages.setSize(size);
//         wRects.setSize(size);
//         wShapes.setSize(size);
//         wText.setSize(size);

//         wLeft.setSize(kSidebarWidth-4, height-4);
//         //wRezHandle.setAbsoluteX(width-wRezHandle.getWidth());
//         //wRezHandle.setAbsoluteY(height-wRezHandle.getHeight());
//
//         wPerf.setAbsoluteX(width-wPerf.getWidth()-5);
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

int main()
{
    USE_NAMESPACE_DGL;

    Application app;
    DemoWindow win(app);

    win.show();
    app.exec();

    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
