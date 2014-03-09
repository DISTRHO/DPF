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
// Pics

#include "images_src/CatPics.cpp"

// ------------------------------------------------------
// DGL Stuff

#include "Image.hpp"
#include "Widget.hpp"
#include "StandaloneWindow.hpp"

// ------------------------------------------------------
// use namespace

using namespace DGL;

// ------------------------------------------------------
// our widget

class ExampleImagesWidget : public App::IdleCallback,
                                   Widget
{
public:
    static const int kImg1y = 0;
    static const int kImg2y = 500/2-CatPics::cat2Height/2;
    static const int kImg3x = 400/3-CatPics::cat3Width/3;

    static const int kImg1max = 500-CatPics::cat1Width;
    static const int kImg2max = 500-CatPics::cat2Width;
    static const int kImg3max = 400-CatPics::cat3Height;

    ExampleImagesWidget(Window& win)
        : Widget(win),
          fImgTop1st(1),
          fImgTop2nd(2),
          fImgTop3rd(3),
          fImg1x(0),
          fImg2x(kImg2max),
          fImg3y(kImg3max),
          fImg1rev(false),
          fImg2rev(true),
          fImg3rev(true),
          fImg1(CatPics::cat1Data, CatPics::cat1Width, CatPics::cat1Height, GL_BGR),
          fImg2(CatPics::cat2Data, CatPics::cat2Width, CatPics::cat2Height, GL_BGR),
          fImg3(CatPics::cat3Data, CatPics::cat3Width, CatPics::cat3Height, GL_BGR)
    {
    }

private:
    void idleCallback() override
    {
        if (fImg1rev)
        {
            fImg1x -= 2;
            if (fImg1x <= -50)
            {
                fImg1rev = false;
                setNewTopImg(1);
            }
        }
        else
        {
            fImg1x += 2;
            if (fImg1x >= kImg1max+50)
            {
                fImg1rev = true;
                setNewTopImg(1);
            }
        }

        if (fImg2rev)
        {
            fImg2x -= 1;
            if (fImg2x <= -50)
            {
                fImg2rev = false;
                setNewTopImg(2);
            }
        }
        else
        {
            fImg2x += 4;
            if (fImg2x >= kImg2max+50)
            {
                fImg2rev = true;
                setNewTopImg(2);
            }
        }

        if (fImg3rev)
        {
            fImg3y -= 3;
            if (fImg3y <= -50)
            {
                fImg3rev = false;
                setNewTopImg(3);
            }
        }
        else
        {
            fImg3y += 3;
            if (fImg3y >= kImg3max+50)
            {
                fImg3rev = true;
                setNewTopImg(3);
            }
        }

        repaint();
    }

    void setNewTopImg(const int imgId)
    {
        if (fImgTop1st == imgId)
            return;

        if (fImgTop2nd == imgId)
        {
            fImgTop2nd = fImgTop1st;
            fImgTop1st =  imgId;
            return;
        }

        fImgTop3rd = fImgTop2nd;
        fImgTop2nd = fImgTop1st;
        fImgTop1st =  imgId;
    }

    void onDisplay() override
    {
        switch (fImgTop3rd)
        {
        case 1:
            fImg1.draw(fImg1x, kImg1y);
            break;
        case 2:
            fImg2.draw(fImg2x, kImg2y);
            break;
        case 3:
            fImg3.draw(kImg3x, fImg3y);
            break;
        };

        switch (fImgTop2nd)
        {
        case 1:
            fImg1.draw(fImg1x, kImg1y);
            break;
        case 2:
            fImg2.draw(fImg2x, kImg2y);
            break;
        case 3:
            fImg3.draw(kImg3x, fImg3y);
            break;
        };

        switch (fImgTop1st)
        {
        case 1:
            fImg1.draw(fImg1x, kImg1y);
            break;
        case 2:
            fImg2.draw(fImg2x, kImg2y);
            break;
        case 3:
            fImg3.draw(kImg3x, fImg3y);
            break;
        };
    }

    int fImgTop1st, fImgTop2nd, fImgTop3rd;
    int fImg1x, fImg2x, fImg3y;
    bool fImg1rev, fImg2rev, fImg3rev;
    Image fImg1, fImg2, fImg3;
};

// ------------------------------------------------------
// main entry point

int main()
{
    App app;
    Window win(app);
    ExampleImagesWidget images(win);

    app.addIdleCallback(&images);

    win.setResizable(false);
    win.setSize(500, 400);
    win.setTitle("Images");
    win.show();
    app.exec();

    return 0;
}

// ------------------------------------------------------
