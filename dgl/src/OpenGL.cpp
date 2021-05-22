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

#include "../OpenGL.hpp"
#include "../Color.hpp"
#include "../ImageWidgets.hpp"

#include "Common.hpp"
#include "SubWidgetPrivateData.hpp"
#include "TopLevelWidgetPrivateData.hpp"
#include "WidgetPrivateData.hpp"
#include "WindowPrivateData.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------
// Color

void Color::setFor(const GraphicsContext&, const bool includeAlpha)
{
    if (includeAlpha)
        glColor4f(red, green, blue, alpha);
    else
        glColor3f(red, green, blue);
}

// -----------------------------------------------------------------------
// Line

template<typename T>
static void drawLine(const Point<T>& posStart, const Point<T>& posEnd)
{
    DISTRHO_SAFE_ASSERT_RETURN(posStart != posEnd,);

    glBegin(GL_LINES);

    {
        glVertex2d(posStart.getX(), posStart.getY());
        glVertex2d(posEnd.getX(), posEnd.getY());
    }

    glEnd();
}

template<typename T>
void Line<T>::draw(const GraphicsContext&, const T width)
{
    DISTRHO_SAFE_ASSERT_RETURN(width != 0,);

    glLineWidth(width);
    drawLine<T>(posStart, posEnd);
}

// deprecated calls
template<typename T>
void Line<T>::draw()
{
    drawLine<T>(posStart, posEnd);
}

template class Line<double>;
template class Line<float>;
template class Line<int>;
template class Line<uint>;
template class Line<short>;
template class Line<ushort>;

// -----------------------------------------------------------------------
// Circle

template<typename T>
static void drawCircle(const Point<T>& pos,
                       const uint numSegments,
                       const float size,
                       const float sin,
                       const float cos,
                       const bool outline)
{
    DISTRHO_SAFE_ASSERT_RETURN(numSegments >= 3 && size > 0.0f,);

    const T origx = pos.getX();
    const T origy = pos.getY();
    double t, x = size, y = 0.0;

    glBegin(outline ? GL_LINE_LOOP : GL_POLYGON);

    for (uint i=0; i<numSegments; ++i)
    {
        glVertex2d(x + origx, y + origy);

        t = x;
        x = cos * x - sin * y;
        y = sin * t + cos * y;
    }

    glEnd();
}

template<typename T>
void Circle<T>::draw(const GraphicsContext&)
{
    drawCircle<T>(fPos, fNumSegments, fSize, fSin, fCos, false);
}

template<typename T>
void Circle<T>::drawOutline(const GraphicsContext&, const T lineWidth)
{
    DISTRHO_SAFE_ASSERT_RETURN(lineWidth != 0,);

    glLineWidth(lineWidth);
    drawCircle<T>(fPos, fNumSegments, fSize, fSin, fCos, true);
}

// deprecated calls
template<typename T>
void Circle<T>::draw()
{
    drawCircle<T>(fPos, fNumSegments, fSize, fSin, fCos, false);
}

template<typename T>
void Circle<T>::drawOutline()
{
    drawCircle<T>(fPos, fNumSegments, fSize, fSin, fCos, true);
}

template class Circle<double>;
template class Circle<float>;
template class Circle<int>;
template class Circle<uint>;
template class Circle<short>;
template class Circle<ushort>;

// -----------------------------------------------------------------------
// Triangle

template<typename T>
static void drawTriangle(const Point<T>& pos1,
                         const Point<T>& pos2,
                         const Point<T>& pos3,
                         const bool outline)
{
    DISTRHO_SAFE_ASSERT_RETURN(pos1 != pos2 && pos1 != pos3,);

    glBegin(outline ? GL_LINE_LOOP : GL_TRIANGLES);

    {
        glVertex2d(pos1.getX(), pos1.getY());
        glVertex2d(pos2.getX(), pos2.getY());
        glVertex2d(pos3.getX(), pos3.getY());
    }

    glEnd();
}

template<typename T>
void Triangle<T>::draw(const GraphicsContext&)
{
    drawTriangle<T>(pos1, pos2, pos3, false);
}

template<typename T>
void Triangle<T>::drawOutline(const GraphicsContext&, const T lineWidth)
{
    DISTRHO_SAFE_ASSERT_RETURN(lineWidth != 0,);

    glLineWidth(lineWidth);
    drawTriangle<T>(pos1, pos2, pos3, true);
}

// deprecated calls
template<typename T>
void Triangle<T>::draw()
{
    drawTriangle<T>(pos1, pos2, pos3, false);
}

template<typename T>
void Triangle<T>::drawOutline()
{
    drawTriangle<T>(pos1, pos2, pos3, true);
}

template class Triangle<double>;
template class Triangle<float>;
template class Triangle<int>;
template class Triangle<uint>;
template class Triangle<short>;
template class Triangle<ushort>;

// -----------------------------------------------------------------------
// Rectangle

template<typename T>
static void drawRectangle(const Rectangle<T>& rect, const bool outline)
{
    DISTRHO_SAFE_ASSERT_RETURN(rect.isValid(),);

    glBegin(outline ? GL_LINE_LOOP : GL_QUADS);

    {
        const T x = rect.getX();
        const T y = rect.getY();
        const T w = rect.getWidth();
        const T h = rect.getHeight();

        glTexCoord2f(0.0f, 0.0f);
        glVertex2d(x, y);

        glTexCoord2f(1.0f, 0.0f);
        glVertex2d(x+w, y);

        glTexCoord2f(1.0f, 1.0f);
        glVertex2d(x+w, y+h);

        glTexCoord2f(0.0f, 1.0f);
        glVertex2d(x, y+h);
    }

    glEnd();
}

template<typename T>
void Rectangle<T>::draw(const GraphicsContext&)
{
    drawRectangle<T>(*this, false);
}

template<typename T>
void Rectangle<T>::drawOutline(const GraphicsContext&, const T lineWidth)
{
    DISTRHO_SAFE_ASSERT_RETURN(lineWidth != 0,);

    glLineWidth(lineWidth);
    drawRectangle<T>(*this, true);
}

// deprecated calls
template<typename T>
void Rectangle<T>::draw()
{
    drawRectangle<T>(*this, false);
}

template<typename T>
void Rectangle<T>::drawOutline()
{
    drawRectangle<T>(*this, true);
}

template class Rectangle<double>;
template class Rectangle<float>;
template class Rectangle<int>;
template class Rectangle<uint>;
template class Rectangle<short>;
template class Rectangle<ushort>;

// -----------------------------------------------------------------------
// OpenGLImage

static void setupOpenGLImage(const OpenGLImage& image, GLuint textureId)
{
    DISTRHO_SAFE_ASSERT_RETURN(image.isValid(),);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    static const float trans[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, trans);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 static_cast<GLsizei>(image.getWidth()),
                 static_cast<GLsizei>(image.getHeight()),
                 0,
                 asOpenGLImageFormat(image.getFormat()), GL_UNSIGNED_BYTE, image.getRawData());

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

static void drawOpenGLImage(const OpenGLImage& image, const Point<int>& pos, const GLuint textureId, bool& setupCalled)
{
    if (textureId == 0 || image.isInvalid())
        return;

    if (! setupCalled)
    {
        setupOpenGLImage(image, textureId);
        setupCalled = true;
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glBegin(GL_QUADS);

    {
        const int x = pos.getX();
        const int y = pos.getY();
        const int w = static_cast<int>(image.getWidth());
        const int h = static_cast<int>(image.getHeight());

        glTexCoord2f(0.0f, 0.0f);
        glVertex2d(x, y);

        glTexCoord2f(1.0f, 0.0f);
        glVertex2d(x+w, y);

        glTexCoord2f(1.0f, 1.0f);
        glVertex2d(x+w, y+h);

        glTexCoord2f(0.0f, 1.0f);
        glVertex2d(x, y+h);
    }

    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

OpenGLImage::OpenGLImage()
    : ImageBase(),
      textureId(0),
      setupCalled(false)
{
    glGenTextures(1, &textureId);
    DISTRHO_SAFE_ASSERT(textureId != 0);
}

OpenGLImage::OpenGLImage(const char* const rawData, const uint width, const uint height, const ImageFormat format)
    : ImageBase(rawData, width, height, format),
      textureId(0),
      setupCalled(false)
{
    glGenTextures(1, &textureId);
    DISTRHO_SAFE_ASSERT(textureId != 0);
}

OpenGLImage::OpenGLImage(const char* const rawData, const Size<uint>& size, const ImageFormat format)
    : ImageBase(rawData, size, format),
      textureId(0),
      setupCalled(false)
{
    glGenTextures(1, &textureId);
    DISTRHO_SAFE_ASSERT(textureId != 0);
}

OpenGLImage::OpenGLImage(const OpenGLImage& image)
    : ImageBase(image),
      textureId(0),
      setupCalled(false)
{
    glGenTextures(1, &textureId);
    DISTRHO_SAFE_ASSERT(textureId != 0);
}

OpenGLImage::~OpenGLImage()
{
    if (textureId != 0)
        glDeleteTextures(1, &textureId);
}

void OpenGLImage::loadFromMemory(const char* const rdata, const Size<uint>& s, const ImageFormat fmt) noexcept
{
    setupCalled = false;
    ImageBase::loadFromMemory(rdata, s, fmt);
}

void OpenGLImage::drawAt(const GraphicsContext&, const Point<int>& pos)
{
    drawOpenGLImage(*this, pos, textureId, setupCalled);
}

OpenGLImage& OpenGLImage::operator=(const OpenGLImage& image) noexcept
{
    rawData = image.rawData;
    size    = image.size;
    format  = image.format;
    setupCalled = false;
    return *this;
}

// deprecated calls
OpenGLImage::OpenGLImage(const char* const rawData, const uint width, const uint height, const GLenum format)
    : ImageBase(rawData, width, height, asDISTRHOImageFormat(format)),
      textureId(0),
      setupCalled(false)
{
    glGenTextures(1, &textureId);
    DISTRHO_SAFE_ASSERT(textureId != 0);
}

OpenGLImage::OpenGLImage(const char* const rawData, const Size<uint>& size, const GLenum format)
    : ImageBase(rawData, size, asDISTRHOImageFormat(format)),
      textureId(0),
      setupCalled(false)
{
    glGenTextures(1, &textureId);
    DISTRHO_SAFE_ASSERT(textureId != 0);
}

void OpenGLImage::draw()
{
    drawOpenGLImage(*this, Point<int>(0, 0), textureId, setupCalled);
}

void OpenGLImage::drawAt(const int x, const int y)
{
    drawOpenGLImage(*this, Point<int>(x, y), textureId, setupCalled);
}

void OpenGLImage::drawAt(const Point<int>& pos)
{
    drawOpenGLImage(*this, pos, textureId, setupCalled);
}

// -----------------------------------------------------------------------
// ImageBaseAboutWindow

#if 0
template <>
void ImageBaseAboutWindow<OpenGLImage>::onDisplay()
{
    const GraphicsContext& context(getGraphicsContext());
    img.draw(context);
}
#endif

template class ImageBaseAboutWindow<OpenGLImage>;

// -----------------------------------------------------------------------
// ImageBaseButton

template class ImageBaseButton<OpenGLImage>;

// -----------------------------------------------------------------------
// ImageBaseKnob

template <>
void ImageBaseKnob<OpenGLImage>::PrivateData::init()
{
    glTextureId = 0;
    glGenTextures(1, &glTextureId);
}

template <>
void ImageBaseKnob<OpenGLImage>::PrivateData::cleanup()
{
    if (glTextureId == 0)
        return;

    glDeleteTextures(1, &glTextureId);
    glTextureId = 0;
}

template <>
void ImageBaseKnob<OpenGLImage>::onDisplay()
{
    const GraphicsContext& context(getGraphicsContext());
    const float normValue = ((pData->usingLog ? pData->invlogscale(pData->value) : pData->value) - pData->minimum)
        / (pData->maximum - pData->minimum);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, pData->glTextureId);

    if (! pData->isReady)
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

        if (pData->rotationAngle == 0)
        {
            DISTRHO_SAFE_ASSERT_RETURN(pData->imgLayerCount > 0,);
            DISTRHO_SAFE_ASSERT_RETURN(normValue >= 0.0f,);

            const uint& v1(pData->isImgVertical ? pData->imgLayerWidth : pData->imgLayerHeight);
            const uint& v2(pData->isImgVertical ? pData->imgLayerHeight : pData->imgLayerWidth);

            // TODO kImageFormatGreyscale
            const uint layerDataSize   = v1 * v2 * ((pData->image.getFormat() == kImageFormatBGRA ||
                                                     pData->image.getFormat() == kImageFormatRGBA) ? 4 : 3);
            /*      */ imageDataOffset = layerDataSize * uint(normValue * float(pData->imgLayerCount-1));
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     static_cast<GLsizei>(getWidth()), static_cast<GLsizei>(getHeight()), 0,
                     asOpenGLImageFormat(pData->image.getFormat()), GL_UNSIGNED_BYTE, pData->image.getRawData() + imageDataOffset);

        pData->isReady = true;
    }

    const int w = static_cast<int>(getWidth());
    const int h = static_cast<int>(getHeight());

    if (pData->rotationAngle != 0)
    {
        glPushMatrix();

        const int w2 = w/2;
        const int h2 = h/2;

        glTranslatef(static_cast<float>(w2), static_cast<float>(h2), 0.0f);
        glRotatef(normValue*static_cast<float>(pData->rotationAngle), 0.0f, 0.0f, 1.0f);

        Rectangle<int>(-w2, -h2, w, h).draw(context);

        glPopMatrix();
    }
    else
    {
        Rectangle<int>(0, 0, w, h).draw(context);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

template class ImageBaseKnob<OpenGLImage>;

// -----------------------------------------------------------------------
// ImageBaseSlider

template class ImageBaseSlider<OpenGLImage>;

// -----------------------------------------------------------------------
// ImageBaseSwitch

template class ImageBaseSwitch<OpenGLImage>;

// -----------------------------------------------------------------------

void SubWidget::PrivateData::display(const uint width, const uint height, const double autoScaleFactor)
{
    bool needsDisableScissor = false;

    if (needsFullViewportForDrawing || (absolutePos.isZero() && self->getSize() == Size<uint>(width, height)))
    {
        // full viewport size
        glViewport(0,
                   -(height * autoScaleFactor - height),
                   width * autoScaleFactor,
                   height * autoScaleFactor);
    }
    else if (needsViewportScaling)
    {
        // limit viewport to widget bounds
        glViewport(absolutePos.getX(),
                   height - self->getHeight() - absolutePos.getY(),
                   self->getWidth(),
                   self->getHeight());
    }
    else
    {
        // set viewport pos
        glViewport(absolutePos.getX() * autoScaleFactor,
                    -std::round((height * autoScaleFactor - height) + (absolutePos.getY() * autoScaleFactor)),
                    std::round(width * autoScaleFactor),
                    std::round(height * autoScaleFactor));

        // then cut the outer bounds
        glScissor(absolutePos.getX() * autoScaleFactor,
                  height - std::round((self->getHeight() + absolutePos.getY()) * autoScaleFactor),
                  std::round(self->getWidth() * autoScaleFactor),
                  std::round(self->getHeight() * autoScaleFactor));

        glEnable(GL_SCISSOR_TEST);
        needsDisableScissor = true;
    }

    // display widget
    self->onDisplay();

    if (needsDisableScissor)
        glDisable(GL_SCISSOR_TEST);

    selfw->pData->displaySubWidgets(width, height, autoScaleFactor);
}

// -----------------------------------------------------------------------

void TopLevelWidget::PrivateData::display()
{
    const Size<uint> size(window.getSize());
    const uint width  = size.getWidth();
    const uint height = size.getHeight();

    const double autoScaleFactor = window.pData->autoScaleFactor;

    // full viewport size
    if (window.pData->autoScaling)
        glViewport(0, -(height * autoScaleFactor - height), width * autoScaleFactor, height * autoScaleFactor);
    else
        glViewport(0, 0, width, height);

    // main widget drawing
    self->onDisplay();

    // now draw subwidgets if there are any
    selfw->pData->displaySubWidgets(width, height, autoScaleFactor);
}

// -----------------------------------------------------------------------

const GraphicsContext& Window::PrivateData::getGraphicsContext() const noexcept
{
    return (const GraphicsContext&)graphicsContext;
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

// -----------------------------------------------------------------------
// templated classes

#include "ImageBaseWidgets.cpp"

// -----------------------------------------------------------------------
