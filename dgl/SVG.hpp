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

#ifndef DGL_SVG_HPP_INCLUDED
#define DGL_SVG_HPP_INCLUDED

#include "src/nanosvg/nanosvg.h"
#include "Geometry.hpp"

struct NSVGrasterizer;

START_NAMESPACE_DGL

/**
 * Utility class for loading SVGs.
 */
class SVG
{
public:

   /**
      Constructor for a null SVG.
    */
    SVG();

   /**
      Constructor using raw SVG data.
    */
    SVG(const char* const data, const uint width, const uint height, const float scaling = 1.0f);

   /**
      Destructor.
    */
    ~SVG();
    
    /**
       Check if this SVG is valid.
    */
    const Size<uint>& getSize() const noexcept;

   /**
      Get the RGBA data of the rasterized SVG.
    */
    const unsigned char* getRGBAData() const noexcept;

   /**
      Check if this SVG is valid.
    */
    bool isValid() const noexcept;

private:
    Size<uint> fSize;
    NSVGrasterizer* fRasterizer;
    NSVGimage* fImage;
    unsigned char* fRGBAData;
};

END_NAMESPACE_DGL

#endif
