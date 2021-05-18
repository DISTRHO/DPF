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

#include "../ImageBaseWidgets.hpp"
#include "Common.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
ImageBaseAboutWindow<ImageType>::ImageBaseAboutWindow(Window& parentWindow, const ImageType& image)
    : StandaloneWindow(parentWindow.getApp(), parentWindow),
      img(image)
{
    setResizable(false);
    setTitle("About");

    if (image.isValid())
        setSize(image.getSize());
}

template <class ImageType>
ImageBaseAboutWindow<ImageType>::ImageBaseAboutWindow(TopLevelWidget* const parentTopLevelWidget, const ImageType& image)
    : StandaloneWindow(parentTopLevelWidget->getApp(), parentTopLevelWidget->getWindow()),
      img(image)
{
    setResizable(false);
    setTitle("About");

    if (image.isValid())
        setSize(image.getSize());
}

template <class ImageType>
void ImageBaseAboutWindow<ImageType>::setImage(const ImageType& image)
{
    if (img == image)
        return;

    img = image;
    setSize(image.getSize());
}

template <class ImageType>
bool ImageBaseAboutWindow<ImageType>::onKeyboard(const KeyboardEvent& ev)
{
    if (ev.press && ev.key == kKeyEscape)
    {
        close();
        return true;
    }

    return false;
}

template <class ImageType>
bool ImageBaseAboutWindow<ImageType>::onMouse(const MouseEvent& ev)
{
    if (ev.press)
    {
        close();
        return true;
    }

    return false;
}

template <class ImageType>
void ImageBaseAboutWindow<ImageType>::onReshape(uint width, uint height)
{
    // FIXME needed?
    TopLevelWidget::setSize(width, height);
    StandaloneWindow::onReshape(width, height);
}

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
struct ImageBaseButton<ImageType>::PrivateData {
    ButtonImpl<ImageType> impl;
    ImageType imageNormal;
    ImageType imageHover;
    ImageType imageDown;

    PrivateData(ImageBaseButton<ImageType>* const s, const ImageType& normal, const ImageType& hover, const ImageType& down)
        : impl(s),
          imageNormal(normal),
          imageHover(hover),
          imageDown(down) {}

    DISTRHO_DECLARE_NON_COPY_STRUCT(PrivateData)
};

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
ImageBaseButton<ImageType>::ImageBaseButton(Widget* const parentWidget, const ImageType& image)
    : SubWidget(parentWidget),
      pData(new PrivateData(this, image, image, image))
{
    setSize(image.getSize());
}

template <class ImageType>
ImageBaseButton<ImageType>::ImageBaseButton(Widget* const parentWidget, const ImageType& imageNormal, const ImageType& imageDown)
    : SubWidget(parentWidget),
      pData(new PrivateData(this, imageNormal, imageNormal, imageDown))
{
    DISTRHO_SAFE_ASSERT(imageNormal.getSize() == imageDown.getSize());

    setSize(imageNormal.getSize());
}

template <class ImageType>
ImageBaseButton<ImageType>::ImageBaseButton(Widget* const parentWidget, const ImageType& imageNormal, const ImageType& imageHover, const ImageType& imageDown)
    : SubWidget(parentWidget),
      pData(new PrivateData(this, imageNormal, imageHover, imageDown))
{
    DISTRHO_SAFE_ASSERT(imageNormal.getSize() == imageHover.getSize() && imageHover.getSize() == imageDown.getSize());

    setSize(imageNormal.getSize());
}

template <class ImageType>
ImageBaseButton<ImageType>::~ImageBaseButton()
{
    delete pData;
}

template <class ImageType>
void ImageBaseButton<ImageType>::setCallback(Callback* callback) noexcept
{
    pData->impl.callback_img = callback;
}

template <class ImageType>
void ImageBaseButton<ImageType>::onDisplay()
{
    const GraphicsContext& context(getGraphicsContext());

    switch (pData->impl.state)
    {
    case ButtonImpl<ImageType>::kStateDown:
        pData->imageDown.draw(context);
        break;
    case ButtonImpl<ImageType>::kStateHover:
        pData->imageHover.draw(context);
        break;
    default:
        pData->imageNormal.draw(context);
        break;
    }
}

template <class ImageType>
bool ImageBaseButton<ImageType>::onMouse(const MouseEvent& ev)
{
    return pData->impl.onMouse(ev);
}

template <class ImageType>
bool ImageBaseButton<ImageType>::onMotion(const MotionEvent& ev)
{
    return pData->impl.onMotion(ev);
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
