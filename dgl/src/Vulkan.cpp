/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2025 Filipe Coelho <falktx@falktx.com>
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

#ifdef _MSC_VER
// instantiated template classes whose methods are defined elsewhere
# pragma warning(disable:4661)
#endif

#include "../Vulkan.hpp"
#include "../Color.hpp"
#include "../ImageBaseWidgets.hpp"

#include "SubWidgetPrivateData.hpp"
#include "TopLevelWidgetPrivateData.hpp"
#include "WidgetPrivateData.hpp"
#include "WindowPrivateData.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------
// Check for correct build config

#ifndef DGL_VULKAN
# error Build config error, Vulkan was NOT requested while building Vulkan code
#endif
#ifdef DGL_CAIRO
# error Build config error, Cairo requested while building Vulkan code
#endif
#ifdef DGL_OPENGL
# error Build config error, OpenGL requested while building Vulkan code
#endif
#ifdef DGL_USE_GLES2
# error Build config error, GLESv2 requested while building Vulkan code
#endif
#ifdef DGL_USE_GLES3
# error Build config error, GLESv3 requested while building Vulkan code
#endif
#ifdef DGL_USE_OPENGL3
# error Build config error, OpenGL3 requested while building Vulkan code
#endif

// --------------------------------------------------------------------------------------------------------------------

static void notImplemented(const char* const name)
{
    d_stderr2("Vulkan function not implemented: %s", name);
}

// --------------------------------------------------------------------------------------------------------------------
// Color

void Color::setFor(const GraphicsContext&, bool)
{
    notImplemented("Color::setFor");
}

// -----------------------------------------------------------------------
// Line

template<typename T>
void Line<T>::draw(const GraphicsContext&, T)
{
    notImplemented("Line::draw");
}

#ifdef DGL_ALLOW_DEPRECATED_METHODS
template<typename T>
void Line<T>::draw()
{
    notImplemented("Line::draw");
}
#endif

template class Line<double>;
template class Line<float>;
template class Line<int>;
template class Line<uint>;
template class Line<short>;
template class Line<ushort>;

// -----------------------------------------------------------------------
// Circle

template<typename T>
void Circle<T>::draw(const GraphicsContext&)
{
    notImplemented("Circle::draw");
}

template<typename T>
void Circle<T>::drawOutline(const GraphicsContext&, T)
{
    notImplemented("Circle::drawOutline");
}

#ifdef DGL_ALLOW_DEPRECATED_METHODS
template<typename T>
void Circle<T>::draw()
{
    notImplemented("Circle::draw");
}

template<typename T>
void Circle<T>::drawOutline()
{
    notImplemented("Circle::drawOutline");
}
#endif

template class Circle<double>;
template class Circle<float>;
template class Circle<int>;
template class Circle<uint>;
template class Circle<short>;
template class Circle<ushort>;

// -----------------------------------------------------------------------
// Triangle

template<typename T>
void Triangle<T>::draw(const GraphicsContext&)
{
    notImplemented("Triangle::draw");
}

template<typename T>
void Triangle<T>::drawOutline(const GraphicsContext&, T)
{
    notImplemented("Triangle::drawOutline");
}

#ifdef DGL_ALLOW_DEPRECATED_METHODS
template<typename T>
void Triangle<T>::draw()
{
    notImplemented("Triangle::draw");
}

template<typename T>
void Triangle<T>::drawOutline()
{
    notImplemented("Triangle::drawOutline");
}
#endif

template class Triangle<double>;
template class Triangle<float>;
template class Triangle<int>;
template class Triangle<uint>;
template class Triangle<short>;
template class Triangle<ushort>;

// -----------------------------------------------------------------------
// Rectangle

template<typename T>
void Rectangle<T>::draw(const GraphicsContext&)
{
    notImplemented("Rectangle::draw");
}

template<typename T>
void Rectangle<T>::drawOutline(const GraphicsContext&, T)
{
    notImplemented("Rectangle::drawOutline");
}

#ifdef DGL_ALLOW_DEPRECATED_METHODS
template<typename T>
void Rectangle<T>::draw()
{
    notImplemented("Rectangle::draw");
}
#endif

template<typename T>
void Rectangle<T>::drawOutline()
{
    notImplemented("Rectangle::drawOutline");
}

template class Rectangle<double>;
template class Rectangle<float>;
template class Rectangle<int>;
template class Rectangle<uint>;
template class Rectangle<short>;
template class Rectangle<ushort>;

// -----------------------------------------------------------------------
// VulkanImage

VulkanImage::VulkanImage()
    : ImageBase() {}

VulkanImage::VulkanImage(const char* const rdata, const uint w, const uint h, const ImageFormat fmt)
    : ImageBase(rdata, w, h, fmt) {}

VulkanImage::VulkanImage(const char* const rdata, const Size<uint>& s, const ImageFormat fmt)
    : ImageBase(rdata, s, fmt) {}

VulkanImage::VulkanImage(const VulkanImage& image)
    : ImageBase(image.rawData, image.size, image.format) {}

VulkanImage::~VulkanImage() {}

void VulkanImage::loadFromMemory(const char* const rdata, const Size<uint>& s, const ImageFormat fmt) noexcept
{
    ImageBase::loadFromMemory(rdata, s, fmt);
}

void VulkanImage::drawAt(const GraphicsContext&, const Point<int>&)
{
}

VulkanImage& VulkanImage::operator=(const VulkanImage& image) noexcept
{
    rawData = image.rawData;
    size    = image.size;
    format  = image.format;
    return *this;
}

// -----------------------------------------------------------------------

void SubWidget::PrivateData::display(const uint width, const uint height, const double autoScaleFactor)
{
    // TODO

    selfw->pData->displaySubWidgets(width, height, autoScaleFactor);
}

// -----------------------------------------------------------------------

void TopLevelWidget::PrivateData::display()
{
    if (! selfw->pData->visible)
        return;

    const Size<uint> size(window.getSize());
    const uint width  = size.getWidth();
    const uint height = size.getHeight();

    const double autoScaleFactor = window.pData->autoScaleFactor;

    // TODO

    // main widget drawing
    self->onDisplay();

    // now draw subwidgets if there are any
    selfw->pData->displaySubWidgets(width, height, autoScaleFactor);
}

// -----------------------------------------------------------------------

void Window::PrivateData::renderToPicture(const char*, const GraphicsContext&, uint, uint)
{
    notImplemented("Window::PrivateData::renderToPicture");
}

// -----------------------------------------------------------------------

void Window::PrivateData::createContextIfNeeded()
{
}

void Window::PrivateData::destroyContext()
{
}

void Window::PrivateData::startContext()
{
}

void Window::PrivateData::endContext()
{
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

// -----------------------------------------------------------------------
