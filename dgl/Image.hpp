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
#include "OpenGL.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

/**
   OpenGL Image class.

   This is an Image class that handles raw image data in pixels.
   You can init the image data on the contructor or later on by calling loadFromMemory().

   To generate raw data useful for this class see the utils/png2rgba.py script.
   Be careful when using a PNG without alpha channel, for those the format is 'GL_BGR'
   instead of the default 'GL_BGRA'.

   Images are drawn on screen via 2D textures.
 */
class Image : public ImageBase
{
public:
   /**
      Constructor for a null Image.
    */
    Image();

   /**
      Constructor using raw image data.
      @note @a rawData must remain valid for the lifetime of this Image.
    */
    Image(const char* const rawData,
          const uint width,
          const uint height,
          const GLenum format = GL_BGRA,
          const GLenum type = GL_UNSIGNED_BYTE);

   /**
      Constructor using raw image data.
      @note @a rawData must remain valid for the lifetime of this Image.
    */
    Image(const char* const rawData,
          const Size<uint>& size,
          const GLenum format = GL_BGRA,
          const GLenum type = GL_UNSIGNED_BYTE);

   /**
      Constructor using another image data.
    */
    Image(const Image& image);

   /**
      Destructor.
    */
    ~Image() override;

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
    Image& operator=(const Image& image) noexcept;

protected:
   /** @internal */
    void _drawAt(const Point<int>& pos) override;

private:
    GLenum fFormat;
    GLenum fType;
    GLuint fTextureId;
    bool fIsReady;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif
