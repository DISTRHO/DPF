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

#ifndef DGL_IMAGE_HPP_INCLUDED
#define DGL_IMAGE_HPP_INCLUDED

#include "ImageBase.hpp"
#ifdef DGL_OPENGL
# include "OpenGL.hpp"
#endif
#ifdef DGL_CAIRO
# include "Cairo.hpp"
#endif

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

#ifdef DGL_OPENGL
/**
   OpenGL Image class.

   This is an Image class that handles raw image data in pixels.
   You can init the image data on the contructor or later on by calling loadFromMemory().

   To generate raw data useful for this class see the utils/png2rgba.py script.
   Be careful when using a PNG without alpha channel, for those the format is 'GL_BGR'
   instead of the default 'GL_BGRA'.

   Images are drawn on screen via 2D textures.
 */
class ImageOpenGL : public ImageBase
{
public:
   /**
      Constructor for a null Image.
    */
    ImageOpenGL();

   /**
      Constructor using raw image data.
      @note @a rawData must remain valid for the lifetime of this Image.
    */
    ImageOpenGL(const char* const rawData,
                const uint width,
                const uint height,
                const GLenum format = GL_BGRA,
                const GLenum type = GL_UNSIGNED_BYTE);

   /**
      Constructor using raw image data.
      @note @a rawData must remain valid for the lifetime of this Image.
    */
    ImageOpenGL(const char* const rawData,
                const Size<uint>& size,
                const GLenum format = GL_BGRA,
                const GLenum type = GL_UNSIGNED_BYTE);

   /**
      Constructor using another image data.
    */
    ImageOpenGL(const ImageOpenGL& image);

   /**
      Destructor.
    */
    ~ImageOpenGL() override;

   /**
      Load image data from memory.
      @note @a rawData must remain valid for the lifetime of this Image.
    */
    void loadFromMemory(const char* const rawData,
                        const uint width,
                        const uint height,
                        const GLenum format = GL_BGRA,
                        const GLenum type = GL_UNSIGNED_BYTE) noexcept;

   /**
      Load image data from memory.
      @note @a rawData must remain valid for the lifetime of this Image.
    */
    void loadFromMemory(const char* const rawData,
                        const Size<uint>& size,
                        const GLenum format = GL_BGRA,
                        const GLenum type = GL_UNSIGNED_BYTE) noexcept;

   /**
      Get the image format.
    */
    GLenum getFormat() const noexcept;

   /**
      Get the image type.
    */
    GLenum getType() const noexcept;

   /**
      TODO document this.
    */
    ImageOpenGL& operator=(const ImageOpenGL& image) noexcept;

protected:
   /** @internal */
    void _drawAt(const Point<int>& pos) override;

private:
    GLenum fFormat;
    GLenum fType;
    GLuint fTextureId;
    bool fIsReady;
};
#endif // DGL_OPENGL

// -----------------------------------------------------------------------

#ifdef DGL_CAIRO
/**
   Cairo Image class.
 */
class ImageCairo : public ImageBase
{
public:
   /**
      Constructor for a null Image.
    */
    ImageCairo() noexcept;

   /**
      Constructor for a cairo surface.
      @note If @takeReference is true, this increments the reference counter
      of the cairo surface.
    */
    ImageCairo(cairo_surface_t* surface, bool takeReference, const GraphicsContext* gc) noexcept;

   /**
      Constructor using another image data.
    */
    ImageCairo(const ImageCairo& image) noexcept;

   /**
      Constructor using another image data, transferring to a different graphics context.
    */
    ImageCairo(const ImageCairo& image, const GraphicsContext* gc) noexcept;

   /**
      Destructor.
      @note Decrements the reference counter of the cairo surface.
    */
    ~ImageCairo() override;

   /**
      Load image data from memory.
      @note @a rawData must remain valid for the lifetime of this Image.
    */
    void loadFromPng(const char* const pngData, const uint pngSize, const GraphicsContext* gc) noexcept;

   /**
      Extract a part of the image bounded by the given rectangle.
      @note @a rawData must remain valid for the lifetime of the new Image.
    */
    Image getRegion(uint x, uint y, uint width, uint height) const noexcept;

   /**
      Get the surface.
    */
    cairo_surface_t* getSurface() const noexcept;

   /**
      Get the format.
    */
    cairo_format_t getFormat() const noexcept;

   /**
      Get the stride.
    */
    uint getStride() const noexcept;

   /**
      TODO document this.
    */
    ImageCairo& operator=(const ImageCairo& image) noexcept;

protected:
   /** @internal */
    void _drawAt(const Point<int>& pos) override;

private:
    cairo_surface_t* fSurface;
    const GraphicsContext* fGc;
};
#endif // DGL_CAIRO

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif
