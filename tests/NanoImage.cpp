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

#include "tests.hpp"

#include "dgl/NanoVG.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------
// Images

#include "images_res/CatPics.cpp"

// --------------------------------------------------------------------------------------------------------------------

class NanoImageExample : public NanoStandaloneWindow,
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
    NanoImage img1, img2, img3;

public:
    NanoImageExample(Application& app)
      : NanoStandaloneWindow(app),
        imgTop1st(1),
        imgTop2nd(2),
        imgTop3rd(3),
        img1x(0),
        img2x(kImg2max),
        img3y(kImg3max),
        img1rev(false),
        img2rev(true),
        img3rev(true),
        img1(createImageFromRawMemory(CatPics::cat1Width, CatPics::cat1Height, (uchar*)CatPics::cat1Data, 0, kImageFormatBGR)),
        img2(createImageFromRawMemory(CatPics::cat2Width, CatPics::cat2Height, (uchar*)CatPics::cat2Data, 0, kImageFormatBGR)),
        img3(createImageFromRawMemory(CatPics::cat3Width, CatPics::cat3Height, (uchar*)CatPics::cat3Data, 0, kImageFormatBGR))
    {
        DISTRHO_SAFE_ASSERT(img1.isValid());
        DISTRHO_SAFE_ASSERT(img2.isValid());
        DISTRHO_SAFE_ASSERT(img3.isValid());

        DISTRHO_SAFE_ASSERT_UINT2(img1.getSize().getWidth() == CatPics::cat1Width,
                                  img1.getSize().getWidth(), CatPics::cat1Width);

        DISTRHO_SAFE_ASSERT_UINT2(img1.getSize().getHeight() == CatPics::cat1Height,
                                  img1.getSize().getHeight(), CatPics::cat1Height);

        DISTRHO_SAFE_ASSERT_UINT2(img2.getSize().getWidth() == CatPics::cat2Width,
                                  img2.getSize().getWidth(), CatPics::cat2Width);

        DISTRHO_SAFE_ASSERT_UINT2(img2.getSize().getHeight() == CatPics::cat2Height,
                                  img2.getSize().getHeight(), CatPics::cat2Height);

        DISTRHO_SAFE_ASSERT_UINT2(img3.getSize().getWidth() == CatPics::cat3Width,
                                  img3.getSize().getWidth(), CatPics::cat3Width);

        DISTRHO_SAFE_ASSERT_UINT2(img3.getSize().getHeight() == CatPics::cat3Height,
                                  img3.getSize().getHeight(), CatPics::cat3Height);

        setResizable(true);
        setSize(500, 500);
        setGeometryConstraints(500, 500, false, true);
        setTitle("NanoImage");
        done();

        addIdleCallback(this);
    }

protected:
    void onNanoDisplay() override
    {
        // bottom image
        beginPath();
        fillPaint(setupImagePaint(imgTop3rd));
        fill();

        // middle image
        beginPath();
        fillPaint(setupImagePaint(imgTop2nd));
        fill();

        // top image
        beginPath();
        fillPaint(setupImagePaint(imgTop1st));
        fill();
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

        repaint();
    }

private:
    Paint setupImagePaint(const int imgId) noexcept
    {
        switch (imgId)
        {
        case 1:
            rect(img1x, kImg1y, CatPics::cat1Width, CatPics::cat1Height);
            return imagePattern(img1x, kImg1y, CatPics::cat1Width, CatPics::cat1Height, 0, img1, 1.0f);
        case 2:
            rect(img2x, kImg2y, CatPics::cat2Width, CatPics::cat2Height);
            return imagePattern(img2x, kImg2y, CatPics::cat2Width, CatPics::cat2Height, 0, img2, 1.0f);
        case 3:
            rect(kImg3x, img3y, CatPics::cat3Width, CatPics::cat3Height);
            return imagePattern(kImg3x, img3y, CatPics::cat3Width, CatPics::cat3Height, 0, img3, 1.0f);
        };

        return Paint();
    }

    void setNewTopImg(const int imgId) noexcept
    {
        if (imgTop1st == imgId)
            return;

        if (imgTop2nd == imgId)
        {
            imgTop2nd = imgTop1st;
            imgTop1st = imgId;
            return;
        }

        imgTop3rd = imgTop2nd;
        imgTop2nd = imgTop1st;
        imgTop1st = imgId;
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

int main()
{
    USE_NAMESPACE_DGL;

    Application app(true);
    NanoImageExample win(app);
    win.show();
    app.exec();

    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
