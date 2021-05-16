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
#include "../ImageWidgets.hpp"

#include "SubWidgetPrivateData.hpp"
#include "TopLevelWidgetPrivateData.hpp"
#include "WidgetPrivateData.hpp"
#include "WindowPrivateData.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------
// Line

template<typename T>
void Line<T>::draw()
{
    DISTRHO_SAFE_ASSERT_RETURN(fPosStart != fPosEnd,);

    glBegin(GL_LINES);

    {
        glVertex2d(fPosStart.fX, fPosStart.fY);
        glVertex2d(fPosEnd.fX, fPosEnd.fY);
    }

    glEnd();
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
void Circle<T>::_draw(const bool outline)
{
    DISTRHO_SAFE_ASSERT_RETURN(fNumSegments >= 3 && fSize > 0.0f,);

    double t, x = fSize, y = 0.0;

    glBegin(outline ? GL_LINE_LOOP : GL_POLYGON);

    for (uint i=0; i<fNumSegments; ++i)
    {
        glVertex2d(x + fPos.fX, y + fPos.fY);

        t = x;
        x = fCos * x - fSin * y;
        y = fSin * t + fCos * y;
    }

    glEnd();
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
void Triangle<T>::_draw(const bool outline)
{
    DISTRHO_SAFE_ASSERT_RETURN(fPos1 != fPos2 && fPos1 != fPos3,);

    glBegin(outline ? GL_LINE_LOOP : GL_TRIANGLES);

    {
        glVertex2d(fPos1.fX, fPos1.fY);
        glVertex2d(fPos2.fX, fPos2.fY);
        glVertex2d(fPos3.fX, fPos3.fY);
    }

    glEnd();
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
    drawRectangle(*this, false);
}

template<typename T>
void Rectangle<T>::drawOutline(const GraphicsContext&)
{
    drawRectangle(*this, true);
}

template<typename T>
void Rectangle<T>::draw()
{
    drawRectangle(*this, true);
}

template<typename T>
void Rectangle<T>::drawOutline()
{
    drawRectangle(*this, true);
}

template class Rectangle<double>;
template class Rectangle<float>;
template class Rectangle<int>;
template class Rectangle<uint>;
template class Rectangle<short>;
template class Rectangle<ushort>;

// -----------------------------------------------------------------------

static GLenum asOpenGLImageFormat(const ImageFormat format)
{
    switch (format)
    {
    case kImageFormatBGR:
        return GL_BGR;
    case kImageFormatBGRA:
        return GL_BGRA;
    case kImageFormatRGB:
        return GL_RGB;
    case kImageFormatRGBA:
        return GL_RGBA;
    }

    return GL_BGRA;
}

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
    ImageBase::loadFromMemory(rdata, s, fmt);
    setupCalled = false;
}

void OpenGLImage::drawAt(const GraphicsContext&, const Point<int>& pos)
{
    drawAt(pos);
}

OpenGLImage& OpenGLImage::operator=(const OpenGLImage& image) noexcept
{
    rawData = image.rawData;
    size    = image.size;
    format  = image.format;
    setupCalled = false;
    return *this;
}

void OpenGLImage::draw()
{
    drawAt(Point<int>(0, 0));
}

void OpenGLImage::drawAt(const int x, const int y)
{
    drawAt(Point<int>(x, y));
}

void OpenGLImage::drawAt(const Point<int>& pos)
{
    if (textureId == 0 || isInvalid())
        return;

    if (! setupCalled)
    {
        setupOpenGLImage(*this, textureId);
        setupCalled = true;
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glBegin(GL_QUADS);

    {
        const int x = pos.getX();
        const int y = pos.getY();
        const int w = static_cast<int>(size.getWidth());
        const int h = static_cast<int>(size.getHeight());

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

// -----------------------------------------------------------------------

template <>
void ImageBaseAboutWindow<OpenGLImage>::onDisplay()
{
    img.draw();
}

template class ImageBaseAboutWindow<OpenGLImage>;

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
        // only set viewport pos
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
    {
        glDisable(GL_SCISSOR_TEST);
        needsDisableScissor = false;
    }

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
