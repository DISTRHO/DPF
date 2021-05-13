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

// -----------------------------------------------------------------------
// Rectangle

template<typename T>
void Rectangle<T>::_draw(const bool outline)
{
    DISTRHO_SAFE_ASSERT_RETURN(fSize.isValid(),);

    glBegin(outline ? GL_LINE_LOOP : GL_QUADS);

    {
        glTexCoord2f(0.0f, 0.0f);
        glVertex2d(fPos.fX, fPos.fY);

        glTexCoord2f(1.0f, 0.0f);
        glVertex2d(fPos.fX+fSize.fWidth, fPos.fY);

        glTexCoord2f(1.0f, 1.0f);
        glVertex2d(fPos.fX+fSize.fWidth, fPos.fY+fSize.fHeight);

        glTexCoord2f(0.0f, 1.0f);
        glVertex2d(fPos.fX, fPos.fY+fSize.fHeight);
    }

    glEnd();
}

// -----------------------------------------------------------------------
// Possible template data types

template class Line<double>;
template class Line<float>;
template class Line<int>;
template class Line<uint>;
template class Line<short>;
template class Line<ushort>;

template class Circle<double>;
template class Circle<float>;
template class Circle<int>;
template class Circle<uint>;
template class Circle<short>;
template class Circle<ushort>;

template class Triangle<double>;
template class Triangle<float>;
template class Triangle<int>;
template class Triangle<uint>;
template class Triangle<short>;
template class Triangle<ushort>;

template class Rectangle<double>;
template class Rectangle<float>;
template class Rectangle<int>;
template class Rectangle<uint>;
template class Rectangle<short>;
template class Rectangle<ushort>;

// -----------------------------------------------------------------------

OpenGLImage::OpenGLImage()
    : ImageBase(),
      fFormat(0),
      fType(0),
      fTextureId(0),
      setupCalled(false) {}

OpenGLImage::OpenGLImage(const OpenGLImage& image)
    : ImageBase(image),
      fFormat(image.fFormat),
      fType(image.fType),
      fTextureId(0),
      setupCalled(false) {}

OpenGLImage::OpenGLImage(const char* const rawData, const uint width, const uint height, const GLenum format, const GLenum type)
    : ImageBase(rawData, width, height),
      fFormat(format),
      fType(type),
      fTextureId(0),
      setupCalled(false) {}

OpenGLImage::OpenGLImage(const char* const rawData, const Size<uint>& size, const GLenum format, const GLenum type)
    : ImageBase(rawData, size),
      fFormat(format),
      fType(type),
      fTextureId(0),
      setupCalled(false) {}

OpenGLImage::~OpenGLImage()
{
    if (setupCalled) {
    // FIXME test if this is still necessary with new pugl
#ifndef DISTRHO_OS_MAC
        if (fTextureId != 0)
            cleanup();
#endif
        DISTRHO_SAFE_ASSERT(fTextureId == 0);
    }
}

void OpenGLImage::loadFromMemory(const char* const rawData,
                                 const uint width,
                                 const uint height,
                                 const GLenum format,
                                 const GLenum type) noexcept
{
    loadFromMemory(rawData, Size<uint>(width, height), format, type);
}

void OpenGLImage::loadFromMemory(const char* const rdata,
                                 const Size<uint>& s,
                                 const GLenum format,
                                 const GLenum type) noexcept
{
    rawData = rdata;
    size    = s;
    fFormat  = format;
    fType    = type;
    setupCalled = false;
}

void OpenGLImage::setup()
{
    setupCalled = true;
    DISTRHO_SAFE_ASSERT_RETURN(fTextureId == 0,);
    DISTRHO_SAFE_ASSERT_RETURN(isValid(),);

    glGenTextures(1, &fTextureId);
    DISTRHO_SAFE_ASSERT_RETURN(fTextureId != 0,);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, fTextureId);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    static const float trans[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, trans);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 static_cast<GLsizei>(size.getWidth()), static_cast<GLsizei>(size.getHeight()), 0,
                 fFormat, fType, rawData);

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

void OpenGLImage::cleanup()
{
    DISTRHO_SAFE_ASSERT_RETURN(fTextureId != 0,);
    glDeleteTextures(1, &fTextureId);
    fTextureId = 0;
}

GLenum OpenGLImage::getFormat() const noexcept
{
    return fFormat;
}

GLenum OpenGLImage::getType() const noexcept
{
    return fType;
}

void OpenGLImage::drawAt(const GraphicsContext&, const Point<int>& pos)
{
    drawAt(pos);
}

OpenGLImage& OpenGLImage::operator=(const OpenGLImage& image) noexcept
{
    rawData = image.rawData;
    size    = image.size;
    fFormat  = image.fFormat;
    fType    = image.fType;
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
    if (isInvalid())
        return;

    if (! setupCalled)
    {
        // TODO check if this is valid, give warning about needing to call setup/cleanup manually
        setup();
    }

    if (fTextureId == 0)
        return;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, fTextureId);

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

#if 0
void Widget::PrivateData::display(const uint width,
                                  const uint height,
                                  const double autoScaling,
                                  const bool renderingSubWidget)
{
    printf("Widget::PrivateData::display INIT\n");

    if (/*(skipDisplay && ! renderingSubWidget) ||*/ size.isInvalid() || ! visible)
        return;

    bool needsDisableScissor = false;

    // reset color
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

#if 0
    if (/*needsFullViewport ||*/ (absolutePos.isZero() && size == Size<uint>(width, height)))
#endif
    {
        // full viewport size
        glViewport(0,
                    -(height * autoScaling - height),
                    width * autoScaling,
                    height * autoScaling);
    }
#if 0
    else if (needsScaling)
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
        glViewport(absolutePos.getX() * autoScaling,
                    -std::round((height * autoScaling - height) + (absolutePos.getY() * autoScaling)),
                    std::round(width * autoScaling),
                    std::round(height * autoScaling));

        // then cut the outer bounds
        glScissor(absolutePos.getX() * autoScaling,
                  height - std::round((self->getHeight() + absolutePos.getY()) * autoScaling),
                  std::round(self->getWidth() * autoScaling),
                  std::round(self->getHeight() * autoScaling));

        glEnable(GL_SCISSOR_TEST);
        needsDisableScissor = true;
    }
#endif

    // display widget
    self->onDisplay();

    if (needsDisableScissor)
    {
        glDisable(GL_SCISSOR_TEST);
        needsDisableScissor = false;
    }

    displaySubWidgets(width, height, autoScaling);
}
#endif

void SubWidget::PrivateData::display(const uint width, const uint height, const double autoScaling)
{
    bool needsDisableScissor = false;

    if (absolutePos.isZero() && self->getSize() == Size<uint>(width, height))
    {
        // full viewport size
        glViewport(0,
                   -(height * autoScaling - height),
                   width * autoScaling,
                   height * autoScaling);
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
        glViewport(absolutePos.getX() * autoScaling,
                    -std::round((height * autoScaling - height) + (absolutePos.getY() * autoScaling)),
                    std::round(width * autoScaling),
                    std::round(height * autoScaling));

        // then cut the outer bounds
        glScissor(absolutePos.getX() * autoScaling,
                  height - std::round((self->getHeight() + absolutePos.getY()) * autoScaling),
                  std::round(self->getWidth() * autoScaling),
                  std::round(self->getHeight() * autoScaling));

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

//     displaySubWidgets(width, height, autoScaling);
}

// -----------------------------------------------------------------------

void TopLevelWidget::PrivateData::display()
{
    const Size<uint> size(window.getSize());
    const uint width         = size.getWidth();
    const uint height        = size.getHeight();
    const double autoScaling = window.pData->autoScaling;

    // full viewport size
    glViewport(0, -(height * autoScaling - height), width * autoScaling, height * autoScaling);

    // main widget drawing
    self->onDisplay();

    // now draw subwidgets if there are any
    selfw->pData->displaySubWidgets(width, height, autoScaling);
}

// -----------------------------------------------------------------------

const GraphicsContext& Window::PrivateData::getGraphicsContext() const noexcept
{
    return (const GraphicsContext&)graphicsContext;
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
