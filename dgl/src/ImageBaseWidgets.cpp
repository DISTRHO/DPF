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
#include "../Color.hpp"
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
void ImageBaseAboutWindow<ImageType>::onDisplay()
{
    img.draw(getGraphicsContext());
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

    DISTRHO_DECLARE_NON_COPYABLE(PrivateData)
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

template <class ImageType>
ImageBaseKnob<ImageType>::PrivateData::PrivateData(const ImageType& img, const Orientation o)
    : image(img),
        minimum(0.0f),
        maximum(1.0f),
        step(0.0f),
        value(0.5f),
        valueDef(value),
        valueTmp(value),
        usingDefault(false),
        usingLog(false),
        orientation(o),
        rotationAngle(0),
        dragging(false),
        lastX(0.0),
        lastY(0.0),
        callback(nullptr),
        alwaysRepaint(false),
        isImgVertical(img.getHeight() > img.getWidth()),
        imgLayerWidth(isImgVertical ? img.getWidth() : img.getHeight()),
        imgLayerHeight(imgLayerWidth),
        imgLayerCount(isImgVertical ? img.getHeight()/imgLayerHeight : img.getWidth()/imgLayerWidth),
        isReady(false)
{
    init();
}

template <class ImageType>
ImageBaseKnob<ImageType>::PrivateData::PrivateData(PrivateData* const other)
    : image(other->image),
        minimum(other->minimum),
        maximum(other->maximum),
        step(other->step),
        value(other->value),
        valueDef(other->valueDef),
        valueTmp(value),
        usingDefault(other->usingDefault),
        usingLog(other->usingLog),
        orientation(other->orientation),
        rotationAngle(other->rotationAngle),
        dragging(false),
        lastX(0.0),
        lastY(0.0),
        callback(other->callback),
        alwaysRepaint(other->alwaysRepaint),
        isImgVertical(other->isImgVertical),
        imgLayerWidth(other->imgLayerWidth),
        imgLayerHeight(other->imgLayerHeight),
        imgLayerCount(other->imgLayerCount),
        isReady(false)
{
    init();
}

template <class ImageType>
void ImageBaseKnob<ImageType>::PrivateData::assignFrom(PrivateData* const other)
{
    cleanup();
    image          = other->image;
    minimum        = other->minimum;
    maximum        = other->maximum;
    step           = other->step;
    value          = other->value;
    valueDef       = other->valueDef;
    valueTmp       = value;
    usingDefault   = other->usingDefault;
    usingLog       = other->usingLog;
    orientation    = other->orientation;
    rotationAngle  = other->rotationAngle;
    dragging       = false;
    lastX          = 0.0;
    lastY          = 0.0;
    callback       = other->callback;
    alwaysRepaint  = other->alwaysRepaint;
    isImgVertical  = other->isImgVertical;
    imgLayerWidth  = other->imgLayerWidth;
    imgLayerHeight = other->imgLayerHeight;
    imgLayerCount  = other->imgLayerCount;
    isReady        = false;
    init();
}

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
ImageBaseKnob<ImageType>::ImageBaseKnob(Widget* const parentWidget, const ImageType& image, const Orientation orientation) noexcept
    : SubWidget(parentWidget),
      pData(new PrivateData(image, orientation))
{
    setSize(pData->imgLayerWidth, pData->imgLayerHeight);
}

template <class ImageType>
ImageBaseKnob<ImageType>::ImageBaseKnob(const ImageBaseKnob<ImageType>& imageKnob)
    : SubWidget(imageKnob.getParentWidget()),
      pData(new PrivateData(imageKnob.pData))
{
    setSize(pData->imgLayerWidth, pData->imgLayerHeight);
}

template <class ImageType>
ImageBaseKnob<ImageType>& ImageBaseKnob<ImageType>::operator=(const ImageBaseKnob<ImageType>& imageKnob)
{
    pData->assignFrom(imageKnob.pData);
    setSize(pData->imgLayerWidth, pData->imgLayerHeight);
    return *this;
}

template <class ImageType>
ImageBaseKnob<ImageType>::~ImageBaseKnob()
{
    delete pData;
}

template <class ImageType>
float ImageBaseKnob<ImageType>::getValue() const noexcept
{
    return pData->value;
}

// NOTE: value is assumed to be scaled if using log
template <class ImageType>
void ImageBaseKnob<ImageType>::setDefault(float value) noexcept
{
    pData->valueDef = value;
    pData->usingDefault = true;
}

template <class ImageType>
void ImageBaseKnob<ImageType>::setRange(float min, float max) noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(max > min,);

    if (pData->value < min)
    {
        pData->value = min;
        repaint();

        if (pData->callback != nullptr)
        {
            try {
                pData->callback->imageKnobValueChanged(this, pData->value);
            } DISTRHO_SAFE_EXCEPTION("ImageBaseKnob<ImageType>::setRange < min");
        }
    }
    else if (pData->value > max)
    {
        pData->value = max;
        repaint();

        if (pData->callback != nullptr)
        {
            try {
                pData->callback->imageKnobValueChanged(this, pData->value);
            } DISTRHO_SAFE_EXCEPTION("ImageBaseKnob<ImageType>::setRange > max");
        }
    }

    pData->minimum = min;
    pData->maximum = max;
}

template <class ImageType>
void ImageBaseKnob<ImageType>::setStep(float step) noexcept
{
    pData->step = step;
}

// NOTE: value is assumed to be scaled if using log
template <class ImageType>
void ImageBaseKnob<ImageType>::setValue(float value, bool sendCallback) noexcept
{
    if (d_isEqual(pData->value, value))
        return;

    pData->value = value;

    if (d_isZero(pData->step))
        pData->valueTmp = value;

    if (pData->rotationAngle == 0 || pData->alwaysRepaint)
        pData->isReady = false;

    repaint();

    if (sendCallback && pData->callback != nullptr)
    {
        try {
            pData->callback->imageKnobValueChanged(this, pData->value);
        } DISTRHO_SAFE_EXCEPTION("ImageBaseKnob<ImageType>::setValue");
    }
}

template <class ImageType>
void ImageBaseKnob<ImageType>::setUsingLogScale(bool yesNo) noexcept
{
    pData->usingLog = yesNo;
}

template <class ImageType>
void ImageBaseKnob<ImageType>::setCallback(Callback* callback) noexcept
{
    pData->callback = callback;
}

template <class ImageType>
void ImageBaseKnob<ImageType>::setOrientation(Orientation orientation) noexcept
{
    if (pData->orientation == orientation)
        return;

    pData->orientation = orientation;
}

template <class ImageType>
void ImageBaseKnob<ImageType>::setRotationAngle(int angle)
{
    if (pData->rotationAngle == angle)
        return;

    pData->rotationAngle = angle;
    pData->isReady = false;
}

template <class ImageType>
void ImageBaseKnob<ImageType>::setImageLayerCount(uint count) noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(count > 1,);

    pData->imgLayerCount = count;

    if (pData->isImgVertical)
        pData->imgLayerHeight = pData->image.getHeight()/count;
    else
        pData->imgLayerWidth = pData->image.getWidth()/count;

    setSize(pData->imgLayerWidth, pData->imgLayerHeight);
}

template <class ImageType>
bool ImageBaseKnob<ImageType>::onMouse(const MouseEvent& ev)
{
    if (ev.button != 1)
        return false;

    if (ev.press)
    {
        if (! contains(ev.pos))
            return false;

        if ((ev.mod & kModifierShift) != 0 && pData->usingDefault)
        {
            setValue(pData->valueDef, true);
            pData->valueTmp = pData->value;
            return true;
        }

        pData->dragging = true;
        pData->lastX = ev.pos.getX();
        pData->lastY = ev.pos.getY();

        if (pData->callback != nullptr)
            pData->callback->imageKnobDragStarted(this);

        return true;
    }
    else if (pData->dragging)
    {
        if (pData->callback != nullptr)
            pData->callback->imageKnobDragFinished(this);

        pData->dragging = false;
        return true;
    }

    return false;
}

template <class ImageType>
bool ImageBaseKnob<ImageType>::onMotion(const MotionEvent& ev)
{
    if (! pData->dragging)
        return false;

    bool doVal = false;
    float d, value = 0.0f;

    if (pData->orientation == ImageBaseKnob<ImageType>::Horizontal)
    {
        if (const double movX = ev.pos.getX() - pData->lastX)
        {
            d     = (ev.mod & kModifierControl) ? 2000.0f : 200.0f;
            value = (pData->usingLog ? pData->invlogscale(pData->valueTmp) : pData->valueTmp) + (float(pData->maximum - pData->minimum) / d * float(movX));
            doVal = true;
        }
    }
    else if (pData->orientation == ImageBaseKnob<ImageType>::Vertical)
    {
        if (const double movY = pData->lastY - ev.pos.getY())
        {
            d     = (ev.mod & kModifierControl) ? 2000.0f : 200.0f;
            value = (pData->usingLog ? pData->invlogscale(pData->valueTmp) : pData->valueTmp) + (float(pData->maximum - pData->minimum) / d * float(movY));
            doVal = true;
        }
    }

    if (! doVal)
        return false;

    if (pData->usingLog)
        value = pData->logscale(value);

    if (value < pData->minimum)
    {
        pData->valueTmp = value = pData->minimum;
    }
    else if (value > pData->maximum)
    {
        pData->valueTmp = value = pData->maximum;
    }
    else if (d_isNotZero(pData->step))
    {
        pData->valueTmp = value;
        const float rest = std::fmod(value, pData->step);
        value = value - rest + (rest > pData->step/2.0f ? pData->step : 0.0f);
    }

    setValue(value, true);

    pData->lastX = ev.pos.getX();
    pData->lastY = ev.pos.getY();

    return true;
}

template <class ImageType>
bool ImageBaseKnob<ImageType>::onScroll(const ScrollEvent& ev)
{
    if (! contains(ev.pos))
        return false;

    const float dir   = (ev.delta.getY() > 0.f) ? 1.f : -1.f;
    const float d     = (ev.mod & kModifierControl) ? 2000.0f : 200.0f;
    float       value = (pData->usingLog ? pData->invlogscale(pData->valueTmp) : pData->valueTmp)
                      + ((pData->maximum - pData->minimum) / d * 10.f * dir);

    if (pData->usingLog)
        value = pData->logscale(value);

    if (value < pData->minimum)
    {
        pData->valueTmp = value = pData->minimum;
    }
    else if (value > pData->maximum)
    {
        pData->valueTmp = value = pData->maximum;
    }
    else if (d_isNotZero(pData->step))
    {
        pData->valueTmp = value;
        const float rest = std::fmod(value, pData->step);
        value = value - rest + (rest > pData->step/2.0f ? pData->step : 0.0f);
    }

    setValue(value, true);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
struct ImageBaseSlider<ImageType>::PrivateData {
    ImageType image;
    float minimum;
    float maximum;
    float step;
    float value;
    float valueDef;
    float valueTmp;
    bool  usingDefault;

    bool dragging;
    bool inverted;
    bool valueIsSet;
    double startedX;
    double startedY;

    Callback* callback;

    Point<int> startPos;
    Point<int> endPos;
    Rectangle<double> sliderArea;

    PrivateData(const ImageType& img)
        : image(img),
          minimum(0.0f),
          maximum(1.0f),
          step(0.0f),
          value(0.5f),
          valueDef(value),
          valueTmp(value),
          usingDefault(false),
          dragging(false),
          inverted(false),
          valueIsSet(false),
          startedX(0.0),
          startedY(0.0),
          callback(nullptr),
          startPos(),
          endPos(),
          sliderArea() {}

    void recheckArea() noexcept
    {
        if (startPos.getY() == endPos.getY())
        {
            // horizontal
            sliderArea = Rectangle<double>(startPos.getX(),
                                           startPos.getY(),
                                           endPos.getX() + static_cast<int>(image.getWidth()) - startPos.getX(),
                                           static_cast<int>(image.getHeight()));
        }
        else
        {
            // vertical
            sliderArea = Rectangle<double>(startPos.getX(),
                                           startPos.getY(),
                                           static_cast<int>(image.getWidth()),
                                           endPos.getY() + static_cast<int>(image.getHeight()) - startPos.getY());
        }
    }

    DISTRHO_DECLARE_NON_COPYABLE(PrivateData)
};

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
ImageBaseSlider<ImageType>::ImageBaseSlider(Widget* const parentWidget, const ImageType& image) noexcept
    : SubWidget(parentWidget),
      pData(new PrivateData(image))
{
    setNeedsFullViewportDrawing();
}

template <class ImageType>
ImageBaseSlider<ImageType>::~ImageBaseSlider()
{
    delete pData;
}

template <class ImageType>
float ImageBaseSlider<ImageType>::getValue() const noexcept
{
    return pData->value;
}

template <class ImageType>
void ImageBaseSlider<ImageType>::setValue(float value, bool sendCallback) noexcept
{
    if (! pData->valueIsSet)
        pData->valueIsSet = true;

    if (d_isEqual(pData->value, value))
        return;

    pData->value = value;

    if (d_isZero(pData->step))
        pData->valueTmp = value;

    repaint();

    if (sendCallback && pData->callback != nullptr)
    {
        try {
            pData->callback->imageSliderValueChanged(this, pData->value);
        } DISTRHO_SAFE_EXCEPTION("ImageBaseSlider::setValue");
    }
}

template <class ImageType>
void ImageBaseSlider<ImageType>::setStartPos(const Point<int>& startPos) noexcept
{
    pData->startPos = startPos;
    pData->recheckArea();
}

template <class ImageType>
void ImageBaseSlider<ImageType>::setStartPos(int x, int y) noexcept
{
    setStartPos(Point<int>(x, y));
}

template <class ImageType>
void ImageBaseSlider<ImageType>::setEndPos(const Point<int>& endPos) noexcept
{
    pData->endPos = endPos;
    pData->recheckArea();
}

template <class ImageType>
void ImageBaseSlider<ImageType>::setEndPos(int x, int y) noexcept
{
    setEndPos(Point<int>(x, y));
}

template <class ImageType>
void ImageBaseSlider<ImageType>::setInverted(bool inverted) noexcept
{
    if (pData->inverted == inverted)
        return;

    pData->inverted = inverted;
    repaint();
}

template <class ImageType>
void ImageBaseSlider<ImageType>::setDefault(float value) noexcept
{
    pData->valueDef = value;
    pData->usingDefault = true;
}

template <class ImageType>
void ImageBaseSlider<ImageType>::setRange(float min, float max) noexcept
{
    pData->minimum = min;
    pData->maximum = max;

    if (pData->value < min)
    {
        pData->value = min;
        repaint();

        if (pData->callback != nullptr && pData->valueIsSet)
        {
            try {
                pData->callback->imageSliderValueChanged(this, pData->value);
            } DISTRHO_SAFE_EXCEPTION("ImageBaseSlider::setRange < min");
        }
    }
    else if (pData->value > max)
    {
        pData->value = max;
        repaint();

        if (pData->callback != nullptr && pData->valueIsSet)
        {
            try {
                pData->callback->imageSliderValueChanged(this, pData->value);
            } DISTRHO_SAFE_EXCEPTION("ImageBaseSlider::setRange > max");
        }
    }
}

template <class ImageType>
void ImageBaseSlider<ImageType>::setStep(float step) noexcept
{
    pData->step = step;
}

template <class ImageType>
void ImageBaseSlider<ImageType>::setCallback(Callback* callback) noexcept
{
    pData->callback = callback;
}

template <class ImageType>
void ImageBaseSlider<ImageType>::onDisplay()
{
    const GraphicsContext& context(getGraphicsContext());

#if 0 // DEBUG, paints slider area
    Color(1.0f, 1.0f, 1.0f, 0.5f).setFor(context, true);
    Rectangle<int>(pData->sliderArea.getX(),
                   pData->sliderArea.getY(),
                   pData->sliderArea.getX()+pData->sliderArea.getWidth(),
                   pData->sliderArea.getY()+pData->sliderArea.getHeight()).draw(context);
    Color(1.0f, 1.0f, 1.0f, 1.0f).setFor(context, true);
#endif

    const float normValue = (pData->value - pData->minimum) / (pData->maximum - pData->minimum);

    int x, y;

    if (pData->startPos.getY() == pData->endPos.getY())
    {
        // horizontal
        if (pData->inverted)
            x = pData->endPos.getX() - static_cast<int>(normValue*static_cast<float>(pData->endPos.getX()-pData->startPos.getX()));
        else
            x = pData->startPos.getX() + static_cast<int>(normValue*static_cast<float>(pData->endPos.getX()-pData->startPos.getX()));

        y = pData->startPos.getY();
    }
    else
    {
        // vertical
        x = pData->startPos.getX();

        if (pData->inverted)
            y = pData->endPos.getY() - static_cast<int>(normValue*static_cast<float>(pData->endPos.getY()-pData->startPos.getY()));
        else
            y = pData->startPos.getY() + static_cast<int>(normValue*static_cast<float>(pData->endPos.getY()-pData->startPos.getY()));
    }

    pData->image.drawAt(context, x, y);
}

template <class ImageType>
bool ImageBaseSlider<ImageType>::onMouse(const MouseEvent& ev)
{
    if (ev.button != 1)
        return false;

    if (ev.press)
    {
        if (! pData->sliderArea.contains(ev.pos))
            return false;

        if ((ev.mod & kModifierShift) != 0 && pData->usingDefault)
        {
            setValue(pData->valueDef, true);
            pData->valueTmp = pData->value;
            return true;
        }

        float vper;
        const double x = ev.pos.getX();
        const double y = ev.pos.getY();

        if (pData->startPos.getY() == pData->endPos.getY())
        {
            // horizontal
            vper = float(x - pData->sliderArea.getX()) / float(pData->sliderArea.getWidth());
        }
        else
        {
            // vertical
            vper = float(y - pData->sliderArea.getY()) / float(pData->sliderArea.getHeight());
        }

        float value;

        if (pData->inverted)
            value = pData->maximum - vper * (pData->maximum - pData->minimum);
        else
            value = pData->minimum + vper * (pData->maximum - pData->minimum);

        if (value < pData->minimum)
        {
            pData->valueTmp = value = pData->minimum;
        }
        else if (value > pData->maximum)
        {
            pData->valueTmp = value = pData->maximum;
        }
        else if (d_isNotZero(pData->step))
        {
            pData->valueTmp = value;
            const float rest = std::fmod(value, pData->step);
            value = value - rest + (rest > pData->step/2.0f ? pData->step : 0.0f);
        }

        pData->dragging = true;
        pData->startedX = x;
        pData->startedY = y;

        if (pData->callback != nullptr)
            pData->callback->imageSliderDragStarted(this);

        setValue(value, true);

        return true;
    }
    else if (pData->dragging)
    {
        if (pData->callback != nullptr)
            pData->callback->imageSliderDragFinished(this);

        pData->dragging = false;
        return true;
    }

    return false;
}

template <class ImageType>
bool ImageBaseSlider<ImageType>::onMotion(const MotionEvent& ev)
{
    if (! pData->dragging)
        return false;

    const bool horizontal = pData->startPos.getY() == pData->endPos.getY();
    const double x = ev.pos.getX();
    const double y = ev.pos.getY();

    if ((horizontal && pData->sliderArea.containsX(x)) || (pData->sliderArea.containsY(y) && ! horizontal))
    {
        float vper;

        if (horizontal)
        {
            // horizontal
            vper = float(x - pData->sliderArea.getX()) / float(pData->sliderArea.getWidth());
        }
        else
        {
            // vertical
            vper = float(y - pData->sliderArea.getY()) / float(pData->sliderArea.getHeight());
        }

        float value;

        if (pData->inverted)
            value = pData->maximum - vper * (pData->maximum - pData->minimum);
        else
            value = pData->minimum + vper * (pData->maximum - pData->minimum);

        if (value < pData->minimum)
        {
            pData->valueTmp = value = pData->minimum;
        }
        else if (value > pData->maximum)
        {
            pData->valueTmp = value = pData->maximum;
        }
        else if (d_isNotZero(pData->step))
        {
            pData->valueTmp = value;
            const float rest = std::fmod(value, pData->step);
            value = value - rest + (rest > pData->step/2.0f ? pData->step : 0.0f);
        }

        setValue(value, true);
    }
    else if (horizontal)
    {
        if (x < pData->sliderArea.getX())
            setValue(pData->inverted ? pData->maximum : pData->minimum, true);
        else
            setValue(pData->inverted ? pData->minimum : pData->maximum, true);
    }
    else
    {
        if (y < pData->sliderArea.getY())
            setValue(pData->inverted ? pData->maximum : pData->minimum, true);
        else
            setValue(pData->inverted ? pData->minimum : pData->maximum, true);
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
struct ImageBaseSwitch<ImageType>::PrivateData {
    ImageType imageNormal;
    ImageType imageDown;
    bool isDown;
    Callback* callback;

    PrivateData(const ImageType& normal, const ImageType& down)
        : imageNormal(normal),
          imageDown(down),
          isDown(false),
          callback(nullptr)
    {
        DISTRHO_SAFE_ASSERT(imageNormal.getSize() == imageDown.getSize());
    }

    PrivateData(PrivateData* const other)
        : imageNormal(other->imageNormal),
          imageDown(other->imageDown),
          isDown(other->isDown),
          callback(other->callback)
    {
        DISTRHO_SAFE_ASSERT(imageNormal.getSize() == imageDown.getSize());
    }

    void assignFrom(PrivateData* const other)
    {
        imageNormal = other->imageNormal;
        imageDown   = other->imageDown;
        isDown      = other->isDown;
        callback    = other->callback;
        DISTRHO_SAFE_ASSERT(imageNormal.getSize() == imageDown.getSize());
    }

    DISTRHO_DECLARE_NON_COPYABLE(PrivateData)
};

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
ImageBaseSwitch<ImageType>::ImageBaseSwitch(Widget* const parentWidget, const ImageType& imageNormal, const ImageType& imageDown) noexcept
    : SubWidget(parentWidget),
      pData(new PrivateData(imageNormal, imageDown))
{
    setSize(imageNormal.getSize());
}

template <class ImageType>
ImageBaseSwitch<ImageType>::ImageBaseSwitch(const ImageBaseSwitch<ImageType>& imageSwitch) noexcept
    : SubWidget(imageSwitch.getParentWidget()),
      pData(new PrivateData(imageSwitch.pData))
{
    setSize(pData->imageNormal.getSize());
}

template <class ImageType>
ImageBaseSwitch<ImageType>& ImageBaseSwitch<ImageType>::operator=(const ImageBaseSwitch<ImageType>& imageSwitch) noexcept
{
    pData->assignFrom(imageSwitch.pData);
    setSize(pData->imageNormal.getSize());
    return *this;
}

template <class ImageType>
ImageBaseSwitch<ImageType>::~ImageBaseSwitch()
{
    delete pData;
}

template <class ImageType>
bool ImageBaseSwitch<ImageType>::isDown() const noexcept
{
    return pData->isDown;
}

template <class ImageType>
void ImageBaseSwitch<ImageType>::setDown(const bool down) noexcept
{
    if (pData->isDown == down)
        return;

    pData->isDown = down;
    repaint();
}

template <class ImageType>
void ImageBaseSwitch<ImageType>::setCallback(Callback* const callback) noexcept
{
    pData->callback = callback;
}

template <class ImageType>
void ImageBaseSwitch<ImageType>::onDisplay()
{
    const GraphicsContext& context(getGraphicsContext());

    if (pData->isDown)
        pData->imageDown.draw(context);
    else
        pData->imageNormal.draw(context);
}

template <class ImageType>
bool ImageBaseSwitch<ImageType>::onMouse(const MouseEvent& ev)
{
    if (ev.press && contains(ev.pos))
    {
        pData->isDown = !pData->isDown;

        repaint();

        if (pData->callback != nullptr)
            pData->callback->imageSwitchClicked(this, pData->isDown);

        return true;
    }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
