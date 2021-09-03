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

#ifndef EXAMPLE_IMAGES_WIDGET_HPP_INCLUDED
#define EXAMPLE_IMAGES_WIDGET_HPP_INCLUDED

// ------------------------------------------------------
// DGL Stuff

#include "../../dgl/ImageBase.hpp"
#include "../../dgl/StandaloneWindow.hpp"
#include "../../dgl/SubWidget.hpp"

// ------------------------------------------------------
// Images

#include "../images_res/CatPics.hpp"

START_NAMESPACE_DGL

// ------------------------------------------------------
// our widget

template <class BaseWidget, class BaseImage>
class ExampleImagesWidget : public BaseWidget,
                            public IdleCallback
{
    static const int kImg1y = 0;
    static const int kImg2y = 500/2-CatPics::cat2Height/2;
    static const int kImg3x = 400/3-CatPics::cat3Width/3;

    static const int kImg1max = 500-CatPics::cat1Width;
    static const int kImg2max = 500-CatPics::cat2Width;
    static const int kImg3max = 400-CatPics::cat3Height;

    int imgTop1st, imgTop2nd, imgTop3rd;
    int img1x, img2x, img3y;
    bool img1rev, img2rev, img3rev;
    BaseImage img1, img2, img3;

public:
    static constexpr const char* kExampleWidgetName = "Images";

    // SubWidget
    explicit ExampleImagesWidget(Widget* const parent);

    // TopLevelWidget
    explicit ExampleImagesWidget(Window& windowToMapTo);

    // StandaloneWindow
    explicit ExampleImagesWidget(Application& app);

protected:
    void init(Application& app)
    {
        imgTop1st = 1;
        imgTop2nd = 2;
        imgTop3rd = 3;
        img1x = 0;
        img2x = kImg2max;
        img3y = kImg3max;
        img1rev = false;
        img2rev = true;
        img3rev = true;
        img1 = BaseImage(CatPics::cat1Data, CatPics::cat1Width, CatPics::cat1Height, kImageFormatBGR);
        img2 = BaseImage(CatPics::cat2Data, CatPics::cat2Width, CatPics::cat2Height, kImageFormatBGR);
        img3 = BaseImage(CatPics::cat3Data, CatPics::cat3Width, CatPics::cat3Height, kImageFormatBGR);

        BaseWidget::setSize(500, 400);
        app.addIdleCallback(this);
    }

    void idleCallback() noexcept override
    {
        if (img1rev)
        {
            img1x -= 2;
            if (img1x <= -50)
            {
                img1rev = false;
                setNewTopImg(1);
            }
        }
        else
        {
            img1x += 2;
            if (img1x >= kImg1max+50)
            {
                img1rev = true;
                setNewTopImg(1);
            }
        }

        if (img2rev)
        {
            img2x -= 1;
            if (img2x <= -50)
            {
                img2rev = false;
                setNewTopImg(2);
            }
        }
        else
        {
            img2x += 4;
            if (img2x >= kImg2max+50)
            {
                img2rev = true;
                setNewTopImg(2);
            }
        }

        if (img3rev)
        {
            img3y -= 3;
            if (img3y <= -50)
            {
                img3rev = false;
                setNewTopImg(3);
            }
        }
        else
        {
            img3y += 3;
            if (img3y >= kImg3max+50)
            {
                img3rev = true;
                setNewTopImg(3);
            }
        }

        BaseWidget::repaint();
    }

    void onDisplay() override
    {
        const GraphicsContext& context(BaseWidget::getGraphicsContext());

        switch (imgTop3rd)
        {
        case 1:
            img1.drawAt(context, img1x, kImg1y);
            break;
        case 2:
            img2.drawAt(context, img2x, kImg2y);
            break;
        case 3:
            img3.drawAt(context, kImg3x, img3y);
            break;
        };

        switch (imgTop2nd)
        {
        case 1:
            img1.drawAt(context, img1x, kImg1y);
            break;
        case 2:
            img2.drawAt(context, img2x, kImg2y);
            break;
        case 3:
            img3.drawAt(context, kImg3x, img3y);
            break;
        };

        switch (imgTop1st)
        {
        case 1:
            img1.drawAt(context, img1x, kImg1y);
            break;
        case 2:
            img2.drawAt(context, img2x, kImg2y);
            break;
        case 3:
            img3.drawAt(context, kImg3x, img3y);
            break;
        };
    }

private:
    void setNewTopImg(const int imgId) noexcept
    {
        if (imgTop1st == imgId)
            return;

        if (imgTop2nd == imgId)
        {
            imgTop2nd = imgTop1st;
            imgTop1st =  imgId;
            return;
        }

        imgTop3rd = imgTop2nd;
        imgTop2nd = imgTop1st;
        imgTop1st =  imgId;
    }
};

// ------------------------------------------------------

END_NAMESPACE_DGL

#endif // EXAMPLE_IMAGES_WIDGET_HPP_INCLUDED
