/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2019 Filipe Coelho <falktx@falktx.com>
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

START_NAMESPACE_DGL

// -----------------------------------------------------------------------
// OpenGL Image class.

#ifdef DGL_OPENGL

ImageOpenGL::ImageOpenGL()
    : ImageBase(),
      fFormat(0),
      fType(0),
      fTextureId(0),
      fIsReady(false)
{
    glGenTextures(1, &fTextureId);
}

ImageOpenGL::ImageOpenGL(const Image& image)
    : ImageBase(image),
      fFormat(image.fFormat),
      fType(image.fType),
      fTextureId(0),
      fIsReady(false)
{
    glGenTextures(1, &fTextureId);
}

ImageOpenGL::ImageOpenGL(const char* const rawData, const uint width, const uint height, const GLenum format, const GLenum type)
    : ImageBase(rawData, width, height),
      fFormat(format),
      fType(type),
      fTextureId(0),
      fIsReady(false)
{
    glGenTextures(1, &fTextureId);
}

ImageOpenGL::ImageOpenGL(const char* const rawData, const Size<uint>& size, const GLenum format, const GLenum type)
    : ImageBase(rawData, size),
      fFormat(format),
      fType(type),
      fTextureId(0),
      fIsReady(false)
{
    glGenTextures(1, &fTextureId);
}

ImageOpenGL::~ImageOpenGL()
{
    if (fTextureId != 0)
    {
#ifndef DISTRHO_OS_MAC // FIXME
        glDeleteTextures(1, &fTextureId);
#endif
        fTextureId = 0;
    }
}

void ImageOpenGL::loadFromMemory(const char* const rawData,
                                 const uint width,
                                 const uint height,
                                 const GLenum format,
                                 const GLenum type) noexcept
{
    loadFromMemory(rawData, Size<uint>(width, height), format, type);
}

void ImageOpenGL::loadFromMemory(const char* const rawData,
                                 const Size<uint>& size,
                                 const GLenum format,
                                 const GLenum type) noexcept
{
    fRawData = rawData;
    fSize    = size;
    fFormat  = format;
    fType    = type;
    fIsReady = false;
}

GLenum ImageOpenGL::getFormat() const noexcept
{
    return fFormat;
}

GLenum ImageOpenGL::getType() const noexcept
{
    return fType;
}

ImageOpenGL& ImageOpenGL::operator=(const Image& image) noexcept
{
    fRawData = image.fRawData;
    fSize    = image.fSize;
    fFormat  = image.fFormat;
    fType    = image.fType;
    fIsReady = false;
    return *this;
}

void ImageOpenGL::_drawAt(const Point<int>& pos)
{
    if (fTextureId == 0 || ! isValid())
        return;

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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     static_cast<GLsizei>(fSize.getWidth()), static_cast<GLsizei>(fSize.getHeight()), 0,
                     fFormat, fType, fRawData);

        fIsReady = true;
    }

    Rectangle<int>(pos, static_cast<int>(fSize.getWidth()), static_cast<int>(fSize.getHeight())).draw();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}
#endif // DGL_OPENGL

// -----------------------------------------------------------------------
// Cairo Image class.

#ifdef DGL_CAIRO

ImageCairo::ImageCairo() noexcept
    : ImageBase(),
      fSurface(nullptr),
      fGc(nullptr)
{
}

ImageCairo::ImageCairo(cairo_surface_t* surface, bool takeReference, const GraphicsContext* gc) noexcept
    : fSurface(surface),
      fGc(gc)
{
    if (takeReference)
        cairo_surface_reference(surface);

    fRawData = (const char*)cairo_image_surface_get_data(surface);
    fSize.setWidth(cairo_image_surface_get_width(surface));
    fSize.setHeight(cairo_image_surface_get_height(surface));
}

ImageCairo::ImageCairo(const ImageCairo& image) noexcept
    : ImageBase(*this),
      fSurface(cairo_surface_reference(image.fSurface)),
      fGc(image.fGc)
{
    cairo_surface_t* surface = fSurface;
    fRawData = (const char*)cairo_image_surface_get_data(surface);
    fSize.setWidth(cairo_image_surface_get_width(surface));
    fSize.setHeight(cairo_image_surface_get_height(surface));
}

ImageCairo::ImageCairo(const ImageCairo& image, const GraphicsContext* gc) noexcept
    : ImageBase(*this),
      fSurface(cairo_surface_reference(image.fSurface)),
      fGc(gc)
{
    cairo_surface_t* surface = fSurface;
    fRawData = (const char*)cairo_image_surface_get_data(surface);
    fSize.setWidth(cairo_image_surface_get_width(surface));
    fSize.setHeight(cairo_image_surface_get_height(surface));
}

ImageCairo::~ImageCairo()
{
    cairo_surface_destroy(fSurface);
    fSurface = nullptr;
}

struct PngReaderData
{
    const char* dataPtr;
    uint sizeLeft;
};

static cairo_status_t readSomePngData(void *closure,
                                      unsigned char *data,
                                      unsigned int length) noexcept
{
    PngReaderData& readerData = *reinterpret_cast<PngReaderData*>(closure);
    if (readerData.sizeLeft < length)
        return CAIRO_STATUS_READ_ERROR;

    memcpy(data, readerData.dataPtr, length);
    readerData.dataPtr += length;
    readerData.sizeLeft -= length;
    return CAIRO_STATUS_SUCCESS;
}

void ImageCairo::loadFromPng(const char* const pngData, const uint pngSize, const GraphicsContext* gc) noexcept
{
    PngReaderData readerData;
    readerData.dataPtr = pngData;
    readerData.sizeLeft = pngSize;

    cairo_surface_t* surface = cairo_image_surface_create_from_png_stream(&readSomePngData, &readerData);
    cairo_surface_destroy(fSurface);
    fSurface = surface;
    fGc = gc;

    fRawData = (const char*)cairo_image_surface_get_data(surface);
    fSize.setWidth(cairo_image_surface_get_width(surface));
    fSize.setHeight(cairo_image_surface_get_height(surface));
}

/**
   Get the pixel size in bytes.
   @return pixel size, or 0 if the format is unknown, or pixels are not aligned to bytes.
*/
static uint getBytesPerPixel(cairo_format_t format) noexcept
{
    switch (format)
    {
    case CAIRO_FORMAT_ARGB32:
    case CAIRO_FORMAT_RGB24:
    case CAIRO_FORMAT_RGB30:
        return 4;
    case CAIRO_FORMAT_RGB16_565:
        return 2;
    case CAIRO_FORMAT_A8:
        return 1;
    case CAIRO_FORMAT_A1:
        return 0;
    default:
        DISTRHO_SAFE_ASSERT(false);
        return 0;
    }
}

Image ImageCairo::getRegion(uint x, uint y, uint width, uint height) const noexcept
{
    cairo_surface_t* surface = fSurface;
    cairo_format_t format = cairo_image_surface_get_format(surface);

    const uint bpp = getBytesPerPixel(format);
    if (bpp == 0)
        return Image();

    const uint fullWidth = cairo_image_surface_get_width(surface);
    const uint fullHeight = cairo_image_surface_get_height(surface);

    x = (x < fullWidth) ? x : fullWidth;
    y = (y < fullHeight) ? y : fullHeight;
    width = (x + width < fullWidth) ? width : (fullWidth - x);
    height = (x + height < fullHeight) ? height : (fullHeight - x);

    unsigned char* fullData = cairo_image_surface_get_data(surface);
    const uint stride = cairo_image_surface_get_stride(surface);

    unsigned char* data = fullData + x * bpp + y * stride;
    return Image(cairo_image_surface_create_for_data(data, format, width, height, stride), false, fGc);
}

cairo_surface_t* ImageCairo::getSurface() const noexcept
{
    return fSurface;
}

cairo_format_t ImageCairo::getFormat() const noexcept
{
    return cairo_image_surface_get_format(fSurface);
}

uint ImageCairo::getStride() const noexcept
{
    return cairo_image_surface_get_stride(fSurface);
}

ImageCairo& ImageCairo::operator=(const ImageCairo& image) noexcept
{
    cairo_surface_t* surface = cairo_surface_reference(image.fSurface);
    cairo_surface_destroy(fSurface);
    fSurface = surface;
    fGc = image.fGc;

    fRawData = (const char*)cairo_image_surface_get_data(surface);
    fSize.setWidth(cairo_image_surface_get_width(surface));
    fSize.setHeight(cairo_image_surface_get_height(surface));
    return *this;
}

void ImageCairo::_drawAt(const Point<int>& pos)
{
    const GraphicsContext* gc = fGc;
    DISTRHO_SAFE_ASSERT_RETURN(gc,)

    cairo_t* cr = gc->cairo;
    cairo_surface_t* surface = fSurface;

    if (!fSurface)
        return;

    cairo_set_source_surface(cr, surface, -pos.getX(), -pos.getY());
    cairo_paint(cr);
}

#endif // DGL_CAIRO

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
