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

#ifndef DGL_IMAGE_BASE_HPP_INCLUDED
#define DGL_IMAGE_BASE_HPP_INCLUDED

#include "Geometry.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

/**
   Base DGL Image class.

   This is an Image class that handles raw image data in pixels.
   It is an abstract class that provides the common methods to build on top.
   Cairo and OpenGL Image classes are based upon this one.

   @see Image
 */
class ImageBase
{
protected:
   /**
      Constructor for a null Image.
    */
    ImageBase();

   /**
      Constructor using raw image data.
      @note @a rawData must remain valid for the lifetime of this Image.
    */
    ImageBase(const char* const rawData, const uint width, const uint height);

   /**
      Constructor using raw image data.
      @note @a rawData must remain valid for the lifetime of this Image.
    */
    ImageBase(const char* const rawData, const Size<uint>& size);

   /**
      Constructor using another image data.
    */
    ImageBase(const ImageBase& image);

public:
   /**
      Destructor.
    */
    virtual ~ImageBase();

   /**
      Check if this image is valid.
    */
    bool isValid() const noexcept;

   /**
      Get width.
    */
    uint getWidth() const noexcept;

   /**
      Get height.
    */
    uint getHeight() const noexcept;

   /**
      Get size.
    */
    const Size<uint>& getSize() const noexcept;

   /**
      Get the raw image data.
    */
    const char* getRawData() const noexcept;

   /**
      Draw this image at (0, 0) point.
    */
    void draw();

   /**
      Draw this image at (x, y) point.
    */
    void drawAt(const int x, const int y);

   /**
      Draw this image at position @a pos.
    */
    void drawAt(const Point<int>& pos);

   /**
      TODO document this.
    */
    ImageBase& operator=(const ImageBase& image) noexcept;
    bool operator==(const ImageBase& image) const noexcept;
    bool operator!=(const ImageBase& image) const noexcept;

protected:
   /** @internal */
    virtual void _drawAt(const Point<int>& pos) = 0;

    const char* fRawData;
    Size<uint> fSize;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_IMAGE_HPP_INCLUDED
