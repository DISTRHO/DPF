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

#include "../ImageBase.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

ImageBase::ImageBase()
    : fRawData(nullptr),
      fSize(0, 0) {}

ImageBase::ImageBase(const char* const rawData, const uint width, const uint height)
  : fRawData(rawData),
    fSize(width, height) {}

ImageBase::ImageBase(const char* const rawData, const Size<uint>& size)
  : fRawData(rawData),
    fSize(size) {}

ImageBase::ImageBase(const ImageBase& image)
  : fRawData(image.fRawData),
    fSize(image.fSize) {}

ImageBase::~ImageBase() {}

// -----------------------------------------------------------------------

bool ImageBase::isValid() const noexcept
{
    return (fRawData != nullptr && fSize.isValid());
}

uint ImageBase::getWidth() const noexcept
{
    return fSize.getWidth();
}

uint ImageBase::getHeight() const noexcept
{
    return fSize.getHeight();
}

const Size<uint>& ImageBase::getSize() const noexcept
{
    return fSize;
}

const char* ImageBase::getRawData() const noexcept
{
    return fRawData;
}

// -----------------------------------------------------------------------

void ImageBase::draw()
{
    _drawAt(Point<int>());
}

void ImageBase::drawAt(const int x, const int y)
{
    _drawAt(Point<int>(x, y));
}

void ImageBase::drawAt(const Point<int>& pos)
{
    _drawAt(pos);
}

// -----------------------------------------------------------------------

ImageBase& ImageBase::operator=(const ImageBase& image) noexcept
{
    fRawData = image.fRawData;
    fSize    = image.fSize;
    return *this;
}

bool ImageBase::operator==(const ImageBase& image) const noexcept
{
    return (fRawData == image.fRawData && fSize == image.fSize);
}

bool ImageBase::operator!=(const ImageBase& image) const noexcept
{
    return !operator==(image);
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
