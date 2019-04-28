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

#include "SVG.hpp"

#define NANOSVG_IMPLEMENTATION
#include "nanosvg/nanosvg.h"

#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvg/nanosvgrast.h"


START_NAMESPACE_DGL

SVG::SVG()
    : fSize(),
      fRasterizer(nullptr),
      fImage(nullptr),
      fRGBAData(nullptr)
{

}

SVG::SVG(const char* const data, const uint width, const uint height, const float scaling) 
    : fSize(width * scaling, height * scaling)
{
    DISTRHO_SAFE_ASSERT_RETURN(data != nullptr,)
    DISTRHO_SAFE_ASSERT_RETURN(fSize.isValid(),)

    fRasterizer = nsvgCreateRasterizer();

    const size_t bufferSize = width * height * 4;

    // nsvgParse modifies the input data, so we must use a temporary buffer
    char tmpBuffer[bufferSize];
    strncpy(tmpBuffer, data, bufferSize);
    
    const float dpi = 96 * scaling;
    fImage = nsvgParse(tmpBuffer, "px", dpi);

    DISTRHO_SAFE_ASSERT_RETURN(fImage != nullptr,)

    const uint scaledWidth = width * scaling;
    const uint scaledHeight = height * scaling;

    fRGBAData = (unsigned char *)malloc(scaledWidth * scaledHeight * 4);

    nsvgRasterize(fRasterizer, fImage, 0, 0, 1, fRGBAData, scaledWidth, scaledHeight, scaledWidth * 4);
}

SVG::~SVG()
{
    nsvgDelete(fImage);
    nsvgDeleteRasterizer(fRasterizer);

    free(fRGBAData);
}
        
const Size<uint>& SVG::getSize() const noexcept
{
    return fSize;
}

const unsigned char* SVG::getRGBAData() const noexcept
{
    return fRGBAData;
}

bool SVG::isValid() const noexcept
{
    return (fRGBAData != nullptr && fSize.isValid());
}

END_NAMESPACE_DGL

