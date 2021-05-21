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

#if 0
ImageKnob::ImageKnob(Widget* const parentWidget, const Image& image, Orientation orientation) noexcept
    : SubWidget(parentWidget),
      fImage(image),
      fMinimum(0.0f),
      fMaximum(1.0f),
      fStep(0.0f),
      fValue(0.5f),
      fValueDef(fValue),
      fValueTmp(fValue),
      fUsingDefault(false),
      fUsingLog(false),
      fOrientation(orientation),
      fRotationAngle(0),
      fDragging(false),
      fLastX(0),
      fLastY(0),
      fCallback(nullptr),
      fIsImgVertical(image.getHeight() > image.getWidth()),
      fImgLayerWidth(fIsImgVertical ? image.getWidth() : image.getHeight()),
      fImgLayerHeight(fImgLayerWidth),
      fImgLayerCount(fIsImgVertical ? image.getHeight()/fImgLayerHeight : image.getWidth()/fImgLayerWidth),
      fIsReady(false),
      fTextureId(0)
{
    glGenTextures(1, &fTextureId);
    setSize(fImgLayerWidth, fImgLayerHeight);
}

ImageKnob::ImageKnob(const ImageKnob& imageKnob)
    : SubWidget(imageKnob.getParentWidget()),
      fImage(imageKnob.fImage),
      fMinimum(imageKnob.fMinimum),
      fMaximum(imageKnob.fMaximum),
      fStep(imageKnob.fStep),
      fValue(imageKnob.fValue),
      fValueDef(imageKnob.fValueDef),
      fValueTmp(fValue),
      fUsingDefault(imageKnob.fUsingDefault),
      fUsingLog(imageKnob.fUsingLog),
      fOrientation(imageKnob.fOrientation),
      fRotationAngle(imageKnob.fRotationAngle),
      fDragging(false),
      fLastX(0),
      fLastY(0),
      fCallback(imageKnob.fCallback),
      fIsImgVertical(imageKnob.fIsImgVertical),
      fImgLayerWidth(imageKnob.fImgLayerWidth),
      fImgLayerHeight(imageKnob.fImgLayerHeight),
      fImgLayerCount(imageKnob.fImgLayerCount),
      fIsReady(false),
      fTextureId(0)
{
    glGenTextures(1, &fTextureId);
    setSize(fImgLayerWidth, fImgLayerHeight);
}

ImageKnob& ImageKnob::operator=(const ImageKnob& imageKnob)
{
    fImage    = imageKnob.fImage;
    fMinimum  = imageKnob.fMinimum;
    fMaximum  = imageKnob.fMaximum;
    fStep     = imageKnob.fStep;
    fValue    = imageKnob.fValue;
    fValueDef = imageKnob.fValueDef;
    fValueTmp = fValue;
    fUsingDefault  = imageKnob.fUsingDefault;
    fUsingLog      = imageKnob.fUsingLog;
    fOrientation   = imageKnob.fOrientation;
    fRotationAngle = imageKnob.fRotationAngle;
    fDragging = false;
    fLastX    = 0;
    fLastY    = 0;
    fCallback = imageKnob.fCallback;
    fIsImgVertical  = imageKnob.fIsImgVertical;
    fImgLayerWidth  = imageKnob.fImgLayerWidth;
    fImgLayerHeight = imageKnob.fImgLayerHeight;
    fImgLayerCount  = imageKnob.fImgLayerCount;
    fIsReady  = false;

    if (fTextureId != 0)
    {
        glDeleteTextures(1, &fTextureId);
        fTextureId = 0;
    }

    glGenTextures(1, &fTextureId);
    setSize(fImgLayerWidth, fImgLayerHeight);

    return *this;
}

ImageKnob::~ImageKnob()
{
    if (fTextureId != 0)
    {
        glDeleteTextures(1, &fTextureId);
        fTextureId = 0;
    }
}

float ImageKnob::getValue() const noexcept
{
    return fValue;
}

// NOTE: value is assumed to be scaled if using log
void ImageKnob::setDefault(float value) noexcept
{
    fValueDef = value;
    fUsingDefault = true;
}

void ImageKnob::setRange(float min, float max) noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(max > min,);

    if (fValue < min)
    {
        fValue = min;
        repaint();

        if (fCallback != nullptr)
        {
            try {
                fCallback->imageKnobValueChanged(this, fValue);
            } DISTRHO_SAFE_EXCEPTION("ImageKnob::setRange < min");
        }
    }
    else if (fValue > max)
    {
        fValue = max;
        repaint();

        if (fCallback != nullptr)
        {
            try {
                fCallback->imageKnobValueChanged(this, fValue);
            } DISTRHO_SAFE_EXCEPTION("ImageKnob::setRange > max");
        }
    }

    fMinimum = min;
    fMaximum = max;
}

void ImageKnob::setStep(float step) noexcept
{
    fStep = step;
}

// NOTE: value is assumed to be scaled if using log
void ImageKnob::setValue(float value, bool sendCallback) noexcept
{
    if (d_isEqual(fValue, value))
        return;

    fValue = value;

    if (d_isZero(fStep))
        fValueTmp = value;

    if (fRotationAngle == 0)
        fIsReady = false;

    repaint();

    if (sendCallback && fCallback != nullptr)
    {
        try {
            fCallback->imageKnobValueChanged(this, fValue);
        } DISTRHO_SAFE_EXCEPTION("ImageKnob::setValue");
    }
}

void ImageKnob::setUsingLogScale(bool yesNo) noexcept
{
    fUsingLog = yesNo;
}

void ImageKnob::setCallback(Callback* callback) noexcept
{
    fCallback = callback;
}

void ImageKnob::setOrientation(Orientation orientation) noexcept
{
    if (fOrientation == orientation)
        return;

    fOrientation = orientation;
}

void ImageKnob::setRotationAngle(int angle)
{
    if (fRotationAngle == angle)
        return;

    fRotationAngle = angle;
    fIsReady = false;
}

void ImageKnob::setImageLayerCount(uint count) noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(count > 1,);

    fImgLayerCount = count;

    if (fIsImgVertical)
        fImgLayerHeight = fImage.getHeight()/count;
    else
        fImgLayerWidth = fImage.getWidth()/count;

    setSize(fImgLayerWidth, fImgLayerHeight);
}

void ImageKnob::onDisplay()
{
    const float normValue = ((fUsingLog ? _invlogscale(fValue) : fValue) - fMinimum) / (fMaximum - fMinimum);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, fTextureId);

    if (! fIsReady)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        static const float trans[] = { 0.0f, 0.0f, 0.0f, 0.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, trans);

        glPixelStorei(GL_PACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        uint imageDataOffset = 0;

        if (fRotationAngle == 0)
        {
            DISTRHO_SAFE_ASSERT_RETURN(fImgLayerCount > 0,);
            DISTRHO_SAFE_ASSERT_RETURN(normValue >= 0.0f,);

            const uint& v1(fIsImgVertical ? fImgLayerWidth : fImgLayerHeight);
            const uint& v2(fIsImgVertical ? fImgLayerHeight : fImgLayerWidth);

            const uint layerDataSize   = v1 * v2 * ((fImage.getFormat() == kImageFormatBGRA ||
                                                     fImage.getFormat() == kImageFormatRGBA) ? 4 : 3);
            /*      */ imageDataOffset = layerDataSize * uint(normValue * float(fImgLayerCount-1));
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     static_cast<GLsizei>(getWidth()), static_cast<GLsizei>(getHeight()), 0,
                     asOpenGLImageFormat(fImage.getFormat()), GL_UNSIGNED_BYTE, fImage.getRawData() + imageDataOffset);

        fIsReady = true;
    }

    const int w = static_cast<int>(getWidth());
    const int h = static_cast<int>(getHeight());

    if (fRotationAngle != 0)
    {
        glPushMatrix();

        const int w2 = w/2;
        const int h2 = h/2;

        glTranslatef(static_cast<float>(w2), static_cast<float>(h2), 0.0f);
        glRotatef(normValue*static_cast<float>(fRotationAngle), 0.0f, 0.0f, 1.0f);

        Rectangle<int>(-w2, -h2, w, h).draw();

        glPopMatrix();
    }
    else
    {
        Rectangle<int>(0, 0, w, h).draw();
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

bool ImageKnob::onMouse(const MouseEvent& ev)
{
    if (ev.button != 1)
        return false;

    if (ev.press)
    {
        if (! contains(ev.pos))
            return false;

        if ((ev.mod & kModifierShift) != 0 && fUsingDefault)
        {
            setValue(fValueDef, true);
            fValueTmp = fValue;
            return true;
        }

        fDragging = true;
        fLastX = ev.pos.getX();
        fLastY = ev.pos.getY();

        if (fCallback != nullptr)
            fCallback->imageKnobDragStarted(this);

        return true;
    }
    else if (fDragging)
    {
        if (fCallback != nullptr)
            fCallback->imageKnobDragFinished(this);

        fDragging = false;
        return true;
    }

    return false;
}

bool ImageKnob::onMotion(const MotionEvent& ev)
{
    if (! fDragging)
        return false;

    bool doVal = false;
    float d, value = 0.0f;

    if (fOrientation == ImageKnob::Horizontal)
    {
        if (const int movX = ev.pos.getX() - fLastX)
        {
            d     = (ev.mod & kModifierControl) ? 2000.0f : 200.0f;
            value = (fUsingLog ? _invlogscale(fValueTmp) : fValueTmp) + (float(fMaximum - fMinimum) / d * float(movX));
            doVal = true;
        }
    }
    else if (fOrientation == ImageKnob::Vertical)
    {
        if (const int movY = fLastY - ev.pos.getY())
        {
            d     = (ev.mod & kModifierControl) ? 2000.0f : 200.0f;
            value = (fUsingLog ? _invlogscale(fValueTmp) : fValueTmp) + (float(fMaximum - fMinimum) / d * float(movY));
            doVal = true;
        }
    }

    if (! doVal)
        return false;

    if (fUsingLog)
        value = _logscale(value);

    if (value < fMinimum)
    {
        fValueTmp = value = fMinimum;
    }
    else if (value > fMaximum)
    {
        fValueTmp = value = fMaximum;
    }
    else if (d_isNotZero(fStep))
    {
        fValueTmp = value;
        const float rest = std::fmod(value, fStep);
        value = value - rest + (rest > fStep/2.0f ? fStep : 0.0f);
    }

    setValue(value, true);

    fLastX = ev.pos.getX();
    fLastY = ev.pos.getY();

    return true;
}

bool ImageKnob::onScroll(const ScrollEvent& ev)
{
    if (! contains(ev.pos))
        return false;

    const float d     = (ev.mod & kModifierControl) ? 2000.0f : 200.0f;
    float       value = (fUsingLog ? _invlogscale(fValueTmp) : fValueTmp) + (float(fMaximum - fMinimum) / d * 10.f * ev.delta.getY());

    if (fUsingLog)
        value = _logscale(value);

    if (value < fMinimum)
    {
        fValueTmp = value = fMinimum;
    }
    else if (value > fMaximum)
    {
        fValueTmp = value = fMaximum;
    }
    else if (d_isNotZero(fStep))
    {
        fValueTmp = value;
        const float rest = std::fmod(value, fStep);
        value = value - rest + (rest > fStep/2.0f ? fStep : 0.0f);
    }

    setValue(value, true);
    return true;
}

// -----------------------------------------------------------------------

float ImageKnob::_logscale(float value) const
{
    const float b = std::log(fMaximum/fMinimum)/(fMaximum-fMinimum);
    const float a = fMaximum/std::exp(fMaximum*b);
    return a * std::exp(b*value);
}

float ImageKnob::_invlogscale(float value) const
{
    const float b = std::log(fMaximum/fMinimum)/(fMaximum-fMinimum);
    const float a = fMaximum/std::exp(fMaximum*b);
    return std::log(value/a)/b;
}
#endif

// --------------------------------------------------------------------------------------------------------------------

#if 0
ImageSlider::ImageSlider(Widget* const parentWidget, const Image& image) noexcept
    : SubWidget(parentWidget),
      fImage(image),
      fMinimum(0.0f),
      fMaximum(1.0f),
      fStep(0.0f),
      fValue(0.5f),
      fValueDef(fValue),
      fValueTmp(fValue),
      fUsingDefault(false),
      fDragging(false),
      fInverted(false),
      fValueIsSet(false),
      fStartedX(0),
      fStartedY(0),
      fCallback(nullptr),
      fStartPos(),
      fEndPos(),
      fSliderArea()
{
    setNeedsFullViewportDrawing();
}

float ImageSlider::getValue() const noexcept
{
    return fValue;
}

void ImageSlider::setValue(float value, bool sendCallback) noexcept
{
    if (! fValueIsSet)
        fValueIsSet = true;

    if (d_isEqual(fValue, value))
        return;

    fValue = value;

    if (d_isZero(fStep))
        fValueTmp = value;

    repaint();

    if (sendCallback && fCallback != nullptr)
    {
        try {
            fCallback->imageSliderValueChanged(this, fValue);
        } DISTRHO_SAFE_EXCEPTION("ImageSlider::setValue");
    }
}

void ImageSlider::setStartPos(const Point<int>& startPos) noexcept
{
    fStartPos = startPos;
    _recheckArea();
}

void ImageSlider::setStartPos(int x, int y) noexcept
{
    setStartPos(Point<int>(x, y));
}

void ImageSlider::setEndPos(const Point<int>& endPos) noexcept
{
    fEndPos = endPos;
    _recheckArea();
}

void ImageSlider::setEndPos(int x, int y) noexcept
{
    setEndPos(Point<int>(x, y));
}

void ImageSlider::setInverted(bool inverted) noexcept
{
    if (fInverted == inverted)
        return;

    fInverted = inverted;
    repaint();
}

void ImageSlider::setDefault(float value) noexcept
{
    fValueDef = value;
    fUsingDefault = true;
}

void ImageSlider::setRange(float min, float max) noexcept
{
    fMinimum = min;
    fMaximum = max;

    if (fValue < min)
    {
        fValue = min;
        repaint();

        if (fCallback != nullptr && fValueIsSet)
        {
            try {
                fCallback->imageSliderValueChanged(this, fValue);
            } DISTRHO_SAFE_EXCEPTION("ImageSlider::setRange < min");
        }
    }
    else if (fValue > max)
    {
        fValue = max;
        repaint();

        if (fCallback != nullptr && fValueIsSet)
        {
            try {
                fCallback->imageSliderValueChanged(this, fValue);
            } DISTRHO_SAFE_EXCEPTION("ImageSlider::setRange > max");
        }
    }
}

void ImageSlider::setStep(float step) noexcept
{
    fStep = step;
}

void ImageSlider::setCallback(Callback* callback) noexcept
{
    fCallback = callback;
}

void ImageSlider::onDisplay()
{
#if 0 // DEBUG, paints slider area
    glColor3f(0.4f, 0.5f, 0.1f);
    glRecti(fSliderArea.getX(), fSliderArea.getY(), fSliderArea.getX()+fSliderArea.getWidth(), fSliderArea.getY()+fSliderArea.getHeight());
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
#endif

    const float normValue = (fValue - fMinimum) / (fMaximum - fMinimum);

    int x, y;

    if (fStartPos.getY() == fEndPos.getY())
    {
        // horizontal
        if (fInverted)
            x = fEndPos.getX() - static_cast<int>(normValue*static_cast<float>(fEndPos.getX()-fStartPos.getX()));
        else
            x = fStartPos.getX() + static_cast<int>(normValue*static_cast<float>(fEndPos.getX()-fStartPos.getX()));

        y = fStartPos.getY();
    }
    else
    {
        // vertical
        x = fStartPos.getX();

        if (fInverted)
            y = fEndPos.getY() - static_cast<int>(normValue*static_cast<float>(fEndPos.getY()-fStartPos.getY()));
        else
            y = fStartPos.getY() + static_cast<int>(normValue*static_cast<float>(fEndPos.getY()-fStartPos.getY()));
    }

    fImage.drawAt(x, y);
}
#endif

// --------------------------------------------------------------------------------------------------------------------

#if 0
bool ImageSlider::onMouse(const MouseEvent& ev)
{
    if (ev.button != 1)
        return false;

    if (ev.press)
    {
        if (! fSliderArea.contains(ev.pos))
            return false;

        if ((ev.mod & kModifierShift) != 0 && fUsingDefault)
        {
            setValue(fValueDef, true);
            fValueTmp = fValue;
            return true;
        }

        float vper;
        const int x = ev.pos.getX();
        const int y = ev.pos.getY();

        if (fStartPos.getY() == fEndPos.getY())
        {
            // horizontal
            vper = float(x - fSliderArea.getX()) / float(fSliderArea.getWidth());
        }
        else
        {
            // vertical
            vper = float(y - fSliderArea.getY()) / float(fSliderArea.getHeight());
        }

        float value;

        if (fInverted)
            value = fMaximum - vper * (fMaximum - fMinimum);
        else
            value = fMinimum + vper * (fMaximum - fMinimum);

        if (value < fMinimum)
        {
            fValueTmp = value = fMinimum;
        }
        else if (value > fMaximum)
        {
            fValueTmp = value = fMaximum;
        }
        else if (d_isNotZero(fStep))
        {
            fValueTmp = value;
            const float rest = std::fmod(value, fStep);
            value = value - rest + (rest > fStep/2.0f ? fStep : 0.0f);
        }

        fDragging = true;
        fStartedX = x;
        fStartedY = y;

        if (fCallback != nullptr)
            fCallback->imageSliderDragStarted(this);

        setValue(value, true);

        return true;
    }
    else if (fDragging)
    {
        if (fCallback != nullptr)
            fCallback->imageSliderDragFinished(this);

        fDragging = false;
        return true;
    }

    return false;
}
#endif

// --------------------------------------------------------------------------------------------------------------------

#if 0
bool ImageSlider::onMotion(const MotionEvent& ev)
{
    if (! fDragging)
        return false;

    const bool horizontal = fStartPos.getY() == fEndPos.getY();
    const int x = ev.pos.getX();
    const int y = ev.pos.getY();

    if ((horizontal && fSliderArea.containsX(x)) || (fSliderArea.containsY(y) && ! horizontal))
    {
        float vper;

        if (horizontal)
        {
            // horizontal
            vper = float(x - fSliderArea.getX()) / float(fSliderArea.getWidth());
        }
        else
        {
            // vertical
            vper = float(y - fSliderArea.getY()) / float(fSliderArea.getHeight());
        }

        float value;

        if (fInverted)
            value = fMaximum - vper * (fMaximum - fMinimum);
        else
            value = fMinimum + vper * (fMaximum - fMinimum);

        if (value < fMinimum)
        {
            fValueTmp = value = fMinimum;
        }
        else if (value > fMaximum)
        {
            fValueTmp = value = fMaximum;
        }
        else if (d_isNotZero(fStep))
        {
            fValueTmp = value;
            const float rest = std::fmod(value, fStep);
            value = value - rest + (rest > fStep/2.0f ? fStep : 0.0f);
        }

        setValue(value, true);
    }
    else if (horizontal)
    {
        if (x < fSliderArea.getX())
            setValue(fInverted ? fMaximum : fMinimum, true);
        else
            setValue(fInverted ? fMinimum : fMaximum, true);
    }
    else
    {
        if (y < fSliderArea.getY())
            setValue(fInverted ? fMaximum : fMinimum, true);
        else
            setValue(fInverted ? fMinimum : fMaximum, true);
    }

    return true;
}

void ImageSlider::_recheckArea() noexcept
{
    if (fStartPos.getY() == fEndPos.getY())
    {
        // horizontal
        fSliderArea = Rectangle<double>(fStartPos.getX(),
                                        fStartPos.getY(),
                                        fEndPos.getX() + static_cast<int>(fImage.getWidth()) - fStartPos.getX(),
                                        static_cast<int>(fImage.getHeight()));
    }
    else
    {
        // vertical
        fSliderArea = Rectangle<double>(fStartPos.getX(),
                                        fStartPos.getY(),
                                        static_cast<int>(fImage.getWidth()),
                                        fEndPos.getY() + static_cast<int>(fImage.getHeight()) - fStartPos.getY());
    }
}
#endif

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
struct ImageBaseSwitch<ImageType>::PrivateData {
    ImageBaseSwitch<ImageType>* const self;
    ImageType imageNormal;
    ImageType imageDown;
    bool isDown;
    Callback* callback;

    PrivateData(ImageBaseSwitch<ImageType>* const s, const ImageType& normal, const ImageType& down)
        : self(s),
          imageNormal(normal),
          imageDown(down),
          isDown(false),
          callback(nullptr)
    {
        DISTRHO_SAFE_ASSERT(imageNormal.getSize() == imageDown.getSize());
    }

    PrivateData(ImageBaseSwitch<ImageType>* const s, PrivateData* const other)
        : self(s),
          imageNormal(other->imageNormal),
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

    DISTRHO_DECLARE_NON_COPY_STRUCT(PrivateData)
};

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
ImageBaseSwitch<ImageType>::ImageBaseSwitch(Widget* const parentWidget, const ImageType& imageNormal, const ImageType& imageDown) noexcept
    : SubWidget(parentWidget),
      pData(new PrivateData(this, imageNormal, imageDown))
{
    setSize(imageNormal.getSize());
}

template <class ImageType>
ImageBaseSwitch<ImageType>::ImageBaseSwitch(const ImageBaseSwitch<ImageType>& imageSwitch) noexcept
    : SubWidget(imageSwitch.getParentWidget()),
      pData(new PrivateData(this, imageSwitch.pData))
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
