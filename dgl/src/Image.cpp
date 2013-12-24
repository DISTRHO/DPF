/*
 * DISTRHO Plugin Toolkit (DPT)
 * Copyright (C) 2012-2013 Filipe Coelho <falktx@falktx.com>
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

#include "../Image.hpp"

#include <cstdio>

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

Image::Image() noexcept
    : fRawData(nullptr),
      fSize(0, 0),
      fFormat(0),
      fType(0),
      fTextureId(0)
{
}

Image::Image(const char* rawData, int width, int height, GLenum format, GLenum type) noexcept
    : fRawData(rawData),
      fSize(width, height),
      fFormat(format),
      fType(type),
      fTextureId(0)
{
}

Image::Image(const char* rawData, const Size<int>& size, GLenum format, GLenum type) noexcept
    : fRawData(rawData),
      fSize(size),
      fFormat(format),
      fType(type),
      fTextureId(0)
{
}

Image::Image(const Image& image) noexcept
    : fRawData(image.fRawData),
      fSize(image.fSize),
      fFormat(image.fFormat),
      fType(image.fType),
      fTextureId(0)
{
}

Image::~Image()
{
    if (fTextureId != 0)
    {
        glDeleteTextures(1, &fTextureId);
        fTextureId = 0;
    }
}

void Image::loadFromMemory(const char* rawData, int width, int height, GLenum format, GLenum type) noexcept
{
    loadFromMemory(rawData, Size<int>(width, height), format, type);
}

void Image::loadFromMemory(const char* rawData, const Size<int>& size, GLenum format, GLenum type) noexcept
{
    fRawData = rawData;
    fSize    = size;
    fFormat  = format;
    fType    = type;
}

bool Image::isValid() const noexcept
{
    return (fRawData != nullptr && getWidth() > 0 && getHeight() > 0);
}

int Image::getWidth() const noexcept
{
    return fSize.getWidth();
}

int Image::getHeight() const noexcept
{
    return fSize.getHeight();
}

const Size<int>& Image::getSize() const noexcept
{
    return fSize;
}

const char* Image::getRawData() const noexcept
{
    return fRawData;
}

GLenum Image::getFormat() const noexcept
{
    return fFormat;
}

GLenum Image::getType() const noexcept
{
    return fType;
}

void Image::draw() const
{
    draw(0, 0);
}

void Image::draw(int x, int y) const
{
    if (! isValid())
        return;
    if (fTextureId == 0)
        glGenTextures(1, &fTextureId);
    if (fTextureId == 0)
    {
        printf("texture Id still 0\n");
        return;
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, fTextureId);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, getWidth(), getHeight(), 0, fFormat, fType, fRawData);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    float trans[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, trans);

    glPushMatrix();

    const GLint w2 = getWidth()/2;
    const GLint h2 = getHeight()/2;

    glTranslatef(x+w2, y+h2, 0.0f);

    glBegin(GL_QUADS);
      glTexCoord2f(0.0f, 1.0f);
      glVertex2i(-w2, -h2);

      glTexCoord2f(1.0f, 1.0f);
      glVertex2i(getWidth()-w2, -h2);

      glTexCoord2f(1.0f, 0.0f);
      glVertex2i(getWidth()-w2, getHeight()-h2);

      glTexCoord2f(0.0f, 0.0f);
      glVertex2i(-w2, getHeight()-h2);
    glEnd();

    glPopMatrix();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

//     glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//     glPixelStorei(GL_PACK_ALIGNMENT, 1);
//     glRasterPos2i(x, fSize.getHeight()+y);
//     glDrawPixels(fSize.getWidth(), fSize.getHeight(), fFormat, fType, fRawData);
}

void Image::draw(const Point<int>& pos) const
{
    draw(pos.getX(), pos.getY());
}

// -----------------------------------------------------------------------

Image& Image::operator=(const Image& image) noexcept
{
    fRawData = image.fRawData;
    fSize    = image.fSize;
    fFormat  = image.fFormat;
    fType    = image.fType;
    return *this;
}

bool Image::operator==(const Image& image) const noexcept
{
    return (fRawData == image.fRawData);
}

bool Image::operator!=(const Image& image) const noexcept
{
    return (fRawData != image.fRawData);
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
