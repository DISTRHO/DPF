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

#ifndef DGL_IMAGE_BASE_WIDGETS_HPP_INCLUDED
#define DGL_IMAGE_BASE_WIDGETS_HPP_INCLUDED

#include "StandaloneWindow.hpp"
#include "SubWidget.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
class ImageBaseAboutWindow : public StandaloneWindow
{
public:
    explicit ImageBaseAboutWindow(Window& parentWindow, const ImageType& image = ImageType());
    explicit ImageBaseAboutWindow(TopLevelWidget* parentTopLevelWidget, const ImageType& image = ImageType());

    void setImage(const ImageType& image);

protected:
    void onDisplay() override;
    bool onKeyboard(const KeyboardEvent&) override;
    bool onMouse(const MouseEvent&) override;

    // FIXME needed?
    void onReshape(uint width, uint height) override;

private:
    ImageType img;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImageBaseAboutWindow)
};

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
class ImageBaseButton : public SubWidget
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void imageButtonClicked(ImageBaseButton* imageButton, int button) = 0;
    };

    explicit ImageBaseButton(Widget* parentWidget, const ImageType& image);
    explicit ImageBaseButton(Widget* parentWidget, const ImageType& imageNormal, const ImageType& imageDown);
    explicit ImageBaseButton(Widget* parentWidget, const ImageType& imageNormal, const ImageType& imageHover, const ImageType& imageDown);

    ~ImageBaseButton() override;

    void setCallback(Callback* callback) noexcept;

protected:
     void onDisplay() override;
     bool onMouse(const MouseEvent&) override;
     bool onMotion(const MotionEvent&) override;

private:
    struct PrivateData;
    PrivateData* const pData;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImageBaseButton)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_IMAGE_BASE_WIDGETS_HPP_INCLUDED
