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

#include "../Vulkan.hpp"
#include "../Color.hpp"

#include "SubWidgetPrivateData.hpp"
#include "TopLevelWidgetPrivateData.hpp"
#include "WidgetPrivateData.hpp"
#include "WindowPrivateData.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

static void notImplemented(const char* const name)
{
    d_stderr2("vulkan function not implemented: %s", name);
}

// -----------------------------------------------------------------------
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

template<typename T>
void Line<T>::draw()
{
    notImplemented("Line::draw");
}

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

template<typename T>
void Rectangle<T>::draw()
{
    notImplemented("Rectangle::draw");
}

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

void SubWidget::PrivateData::display(const uint width, const uint height, const double autoScaleFactor)
{
    // TODO

    selfw->pData->displaySubWidgets(width, height, autoScaleFactor);
}

// -----------------------------------------------------------------------

void TopLevelWidget::PrivateData::display()
{
    const Size<uint> size(window.getSize());
    const uint width  = size.getWidth();
    const uint height = size.getHeight();

    const double autoScaleFactor = window.pData->autoScaleFactor;

    // FIXME anything needed here?
#if 0
    // full viewport size
    if (window.pData->autoScaling)
        glViewport(0, -(height * autoScaleFactor - height), width * autoScaleFactor, height * autoScaleFactor);
    else
        glViewport(0, 0, width, height);
#endif

    // main widget drawing
    self->onDisplay();

    // now draw subwidgets if there are any
    selfw->pData->displaySubWidgets(width, height, autoScaleFactor);
}

// -----------------------------------------------------------------------

const GraphicsContext& Window::PrivateData::getGraphicsContext() const noexcept
{
    return (const GraphicsContext&)graphicsContext;
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

// -----------------------------------------------------------------------
