/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2022 Filipe Coelho <falktx@falktx.com>
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

#include "widgets/ExampleColorWidget.hpp"
#include "widgets/ExampleImagesWidget.hpp"
#include "widgets/ExampleRectanglesWidget.hpp"
#include "widgets/ExampleShapesWidget.hpp"
#ifdef DGL_OPENGL
#include "widgets/ExampleTextWidget.hpp"
#endif
#include "widgets/ResizeHandle.hpp"

#include "demo_res/DemoArtwork.cpp"
#include "images_res/CatPics.cpp"

#ifdef DGL_CAIRO
#include "../dgl/Cairo.hpp"
typedef DGL_NAMESPACE::CairoImage DemoImage;
#endif
#ifdef DGL_OPENGL
#include "../dgl/OpenGL.hpp"
typedef DGL_NAMESPACE::OpenGLImage DemoImage;
#endif
#ifdef DGL_VULKAN
#include "../dgl/Vulkan.hpp"
typedef DGL_NAMESPACE::VulkanImage DemoImage;
#endif

START_NAMESPACE_DGL

// Partial specialization is not allowed in C++, so we need to define these here
template<> inline
ExampleImagesWidget<SubWidget, DemoImage>::ExampleImagesWidget(Widget* const parentWidget)
: SubWidget(parentWidget) { init(parentWidget->getApp()); }

template<> inline
ExampleImagesWidget<TopLevelWidget, DemoImage>::ExampleImagesWidget(Window& windowToMapTo)
: TopLevelWidget(windowToMapTo) { init(windowToMapTo.getApp()); }

template<>
ExampleImagesWidget<StandaloneWindow, DemoImage>::ExampleImagesWidget(Application& app)
: StandaloneWindow(app) { init(app); done(); }

typedef ExampleImagesWidget<SubWidget, DemoImage> ExampleImagesSubWidget;
typedef ExampleImagesWidget<TopLevelWidget, DemoImage> ExampleImagesTopLevelWidget;
typedef ExampleImagesWidget<StandaloneWindow, DemoImage> ExampleImagesStandaloneWindow;

// --------------------------------------------------------------------------------------------------------------------
// Left side tab-like widget

class LeftSideWidget : public SubWidget
{
public:
#ifdef DGL_OPENGL
    static const int kPageCount = 5;
#else
    static const int kPageCount = 4;
#endif

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

        using namespace DemoArtwork;
        img1.loadFromMemory(ico1Data, ico1Width, ico1Height, kImageFormatBGR);
        img2.loadFromMemory(ico2Data, ico2Width, ico2Height, kImageFormatBGR);
        img3.loadFromMemory(ico3Data, ico3Width, ico2Height, kImageFormatBGR);
        img4.loadFromMemory(ico4Data, ico4Width, ico4Height, kImageFormatBGR);

#ifdef DGL_OPENGL
        img5.loadFromMemory(ico5Data, ico5Width, ico5Height, kImageFormatBGR);

        // for text
        nvg.loadSharedResources();
#endif
    }

protected:
    void onDisplay() override
    {
        const GraphicsContext& context(getGraphicsContext());
        const double scaleFactor = getWindow().getScaleFactor();
        const int iconSize = bgIcon.getWidth();

        Color(0.027f, 0.027f, 0.027f).setFor(context);
        Rectangle<uint>(0, 0, getSize()).draw(context);

        bgIcon.setY(curPage * iconSize + curPage * 3 * scaleFactor);

        Color(0.129f, 0.129f, 0.129f).setFor(context);
        bgIcon.draw(context);

        Color(0.184f, 0.184f, 0.184f).setFor(context);
        bgIcon.drawOutline(context);

        if (curHover != curPage && curHover != -1)
        {
            Rectangle<int> rHover(1 * scaleFactor, curHover * iconSize + curHover * 3 * scaleFactor,
                                  iconSize - 2 * scaleFactor, iconSize - 2 * scaleFactor);

            Color(0.071f, 0.071f, 0.071f).setFor(context);
            rHover.draw(context);

            Color(0.102f, 0.102f, 0.102f).setFor(context);
            rHover.drawOutline(context);
        }

        Color(0.184f, 0.184f, 0.184f).setFor(context);
        lineSep.draw(context);

        // reset color
        Color(1.0f, 1.0f, 1.0f, 1.0f).setFor(context, true);

        const int pad = iconSize/2 - DemoArtwork::ico1Width/2;

        img1.drawAt(context, pad, pad);
        img2.drawAt(context, pad, pad + 3 + iconSize);
        img3.drawAt(context, pad, pad + 6 + iconSize*2);
        img4.drawAt(context, pad, pad + 9 + iconSize*3);

#ifdef DGL_OPENGL
        img5.drawAt(context, pad, pad + 12 + iconSize*4);

        // draw some text
        nvg.beginFrame(this);

        nvg.fontSize(23.0f * scaleFactor);
        nvg.textAlign(NanoVG::ALIGN_LEFT|NanoVG::ALIGN_TOP);
        //nvg.textLineHeight(20.0f);

        nvg.fillColor(220,220,220,220);
        nvg.textBox(10 * scaleFactor, 420 * scaleFactor, iconSize, "Haha,", nullptr);
        nvg.textBox(15 * scaleFactor, 440 * scaleFactor, iconSize, "Look!", nullptr);

        nvg.endFrame();
#endif
    }

    bool onMouse(const MouseEvent& ev) override
    {
        if (ev.button != 1 || ! ev.press)
            return false;
        if (! contains(ev.pos))
            return false;

        const int iconSize = bgIcon.getWidth();

        for (int i=0; i<kPageCount; ++i)
        {
            bgIcon.setY(i*iconSize + i*3);

            if (bgIcon.contains(ev.pos))
            {
                curPage = i;
                callback->curPageChanged(i);
                repaint();
                break;
            }
        }

        return true;
    }

    bool onMotion(const MotionEvent& ev) override
    {
        if (contains(ev.pos))
        {
            const int iconSize = bgIcon.getWidth();

            for (int i=0; i<kPageCount; ++i)
            {
                bgIcon.setY(i*iconSize + i*3);

                if (bgIcon.contains(ev.pos))
                {
                    if (curHover == i)
                        return true;

                    curHover = i;
                    repaint();
                    return true;
                }
            }

            if (curHover == -1)
                return true;

            curHover = -1;
            repaint();
            return true;
        }
        else
        {
            if (curHover == -1)
                return false;

            curHover = -1;
            repaint();
            return true;
        }
    }

    void onResize(const ResizeEvent& ev) override
    {
        const uint width  = ev.size.getWidth();
        const uint height = ev.size.getHeight();
        const double scaleFactor = getWindow().getScaleFactor();

        bgIcon.setWidth(width - 4 * scaleFactor);
        bgIcon.setHeight(width - 4 * scaleFactor);

        lineSep.setStartPos(width, 0);
        lineSep.setEndPos(width, height);
    }

private:
    Callback* const callback;
    int curPage, curHover;
    Rectangle<double> bgIcon;
    Line<int> lineSep;
    DemoImage img1, img2, img3, img4, img5;

#ifdef DGL_OPENGL
    // for text
    NanoVG nvg;
#endif
};

// --------------------------------------------------------------------------------------------------------------------
// Main Demo Window, having a left-side tab-like widget and main area for current widget

class DemoWindow : public StandaloneWindow,
                   public LeftSideWidget::Callback
{
    static const int kSidebarWidth = 81;

public:
#ifdef DGL_CAIRO
    static constexpr const char* const kExampleWidgetName = "Demo - Cairo";
#endif
#ifdef DGL_OPENGL
    static constexpr const char* const kExampleWidgetName = "Demo - OpenGL";
#endif
#ifdef DGL_VULKAN
    static constexpr const char* const kExampleWidgetName = "Demo - Vulkan";
#endif

    DemoWindow(Application& app)
        : StandaloneWindow(app),
          curWidget(nullptr)
    {
        const ScopedGraphicsContext sgc(*this);
        const double scaleFactor = getScaleFactor();

        wColor = new ExampleColorSubWidget(this);
        wColor->hide();
        wColor->setAbsoluteX(kSidebarWidth * scaleFactor);

        wImages = new ExampleImagesSubWidget(this);
        wImages->hide();
        wImages->setAbsoluteX(kSidebarWidth * scaleFactor);

        wRects = new ExampleRectanglesSubWidget(this);
        wRects->hide();
        wRects->setAbsoluteX(kSidebarWidth * scaleFactor);

        wShapes = new ExampleShapesSubWidget(this);
        wShapes->hide();
        wShapes->setAbsoluteX(kSidebarWidth * scaleFactor);

#ifdef DGL_OPENGL
        wText = new ExampleTextSubWidget(this),
        wText->hide();
        wText->setAbsoluteX(kSidebarWidth * scaleFactor);
#endif
        wLeft = new LeftSideWidget(this, this),
        wLeft->setAbsolutePos(2 * scaleFactor, 2 * scaleFactor);

        resizer = new ResizeHandle(this);

        curPageChanged(0);
        done();
    }

protected:
    void curPageChanged(int curPage) override
    {
        if (curWidget != nullptr)
            curWidget->hide();

        switch (curPage)
        {
        case 0:
            curWidget = wColor;
            break;
        case 1:
            curWidget = wImages;
            break;
        case 2:
            curWidget = wRects;
            break;
        case 3:
            curWidget = wShapes;
            break;
#ifdef DGL_OPENGL
        case 4:
            curWidget = wText;
            break;
#endif
        default:
            curWidget = nullptr;
            break;
        }

        if (curWidget != nullptr)
            curWidget->show();
    }

    void onDisplay() override
    {
    }

    void onReshape(uint width, uint height) override
    {
        StandaloneWindow::onReshape(width, height);

        const double scaleFactor = getScaleFactor();

        if (width < kSidebarWidth * scaleFactor)
            return;

        const Size<uint> size(width - kSidebarWidth * scaleFactor, height);
        wColor->setSize(size);
        wImages->setSize(size);
        wRects->setSize(size);
        wShapes->setSize(size);
#ifdef DGL_OPENGL
        wText->setSize(size);
#endif
        wLeft->setSize((kSidebarWidth - 4) * scaleFactor, (height - 4) * scaleFactor);
    }

private:
    ScopedPointer<ExampleColorSubWidget> wColor;
    ScopedPointer<ExampleImagesSubWidget> wImages;
    ScopedPointer<ExampleRectanglesSubWidget> wRects;
    ScopedPointer<ExampleShapesSubWidget> wShapes;
#ifdef DGL_OPENGL
    ScopedPointer<ExampleTextSubWidget> wText;
#endif
    ScopedPointer<LeftSideWidget> wLeft;
    ScopedPointer<ResizeHandle> resizer;

    Widget* curWidget;
};

// --------------------------------------------------------------------------------------------------------------------
// Special handy function that runs a StandaloneWindow inside the function scope

template <class ExampleWidgetStandaloneWindow>
void createAndShowExampleWidgetStandaloneWindow(Application& app)
{
    ExampleWidgetStandaloneWindow swin(app);
    const double scaleFactor = swin.getScaleFactor();
    swin.setGeometryConstraints(128 * scaleFactor, 128 * scaleFactor);
    swin.setResizable(true);
    swin.setSize(600 * scaleFactor, 500 * scaleFactor);
    swin.setTitle(ExampleWidgetStandaloneWindow::kExampleWidgetName);
    swin.show();
    app.exec();
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

int main(int argc, char* argv[])
{
    USE_NAMESPACE_DGL;
    using DGL_NAMESPACE::Window;

    Application app;

    if (argc > 1)
    {
        /**/ if (std::strcmp(argv[1], "color") == 0)
            createAndShowExampleWidgetStandaloneWindow<ExampleColorStandaloneWindow>(app);
        else if (std::strcmp(argv[1], "images") == 0)
            createAndShowExampleWidgetStandaloneWindow<ExampleImagesStandaloneWindow>(app);
        else if (std::strcmp(argv[1], "rectangles") == 0)
            createAndShowExampleWidgetStandaloneWindow<ExampleRectanglesStandaloneWindow>(app);
        else if (std::strcmp(argv[1], "shapes") == 0)
            createAndShowExampleWidgetStandaloneWindow<ExampleShapesStandaloneWindow>(app);
#ifdef DGL_OPENGL
        else if (std::strcmp(argv[1], "text") == 0)
            createAndShowExampleWidgetStandaloneWindow<ExampleTextStandaloneWindow>(app);
#endif
        else
            d_stderr2("Invalid demo mode, must be one of: color, images, rectangles or shapes");
    }
    else
    {
        createAndShowExampleWidgetStandaloneWindow<DemoWindow>(app);
    }

    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
