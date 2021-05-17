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

#include "../Cairo.hpp"
#include "../Color.hpp"

#include "SubWidgetPrivateData.hpp"
#include "TopLevelWidgetPrivateData.hpp"
#include "WidgetPrivateData.hpp"
#include "WindowPrivateData.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

static void notImplemented(const char* const name)
{
    d_stderr2("cairo function not implemented: %s", name);
}

// -----------------------------------------------------------------------
// Color

void Color::setFor(const GraphicsContext& context, const bool includeAlpha)
{
    cairo_t* const handle = ((const CairoGraphicsContext&)context).handle;

    if (includeAlpha)
        cairo_set_source_rgba(handle, red, green, blue, alpha);
    else
        cairo_set_source_rgb(handle, red, green, blue);
}

// -----------------------------------------------------------------------
// Line

template<typename T>
void Line<T>::draw(const GraphicsContext& context, const T width)
{
    DISTRHO_SAFE_ASSERT_RETURN(posStart != posEnd,);
    DISTRHO_SAFE_ASSERT_RETURN(width != 0,);

    cairo_t* const handle = ((const CairoGraphicsContext&)context).handle;

    cairo_set_line_width(handle, width);
    cairo_move_to(handle, posStart.getX(), posStart.getY());
    cairo_line_to(handle, posEnd.getX(), posEnd.getY());
    cairo_stroke(handle);
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
static void drawCircle(cairo_t* const handle,
                       const Point<T>& pos,
                       const uint numSegments,
                       const float size,
                       const float sin,
                       const float cos,
                       const bool outline)
{
    DISTRHO_SAFE_ASSERT_RETURN(numSegments >= 3 && size > 0.0f,);

    const T origx = pos.getX();
    const T origy = pos.getY();
    double t, x = size, y = 0.0;

    /*
    glBegin(outline ? GL_LINE_LOOP : GL_POLYGON);

    for (uint i=0; i<numSegments; ++i)
    {
        glVertex2d(x + origx, y + origy);

        t = x;
        x = cos * x - sin * y;
        y = sin * t + cos * y;
    }

    glEnd();
    */
}

template<typename T>
void Circle<T>::draw(const GraphicsContext& context)
{
    cairo_t* const handle = ((const CairoGraphicsContext&)context).handle;

    drawCircle<T>(handle, fPos, fNumSegments, fSize, fSin, fCos, false);
}

template<typename T>
void Circle<T>::drawOutline(const GraphicsContext& context, const T lineWidth)
{
    DISTRHO_SAFE_ASSERT_RETURN(lineWidth != 0,);

    cairo_t* const handle = ((const CairoGraphicsContext&)context).handle;

    cairo_set_line_width(handle, lineWidth);
    drawCircle<T>(handle, fPos, fNumSegments, fSize, fSin, fCos, true);
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
static void drawTriangle(cairo_t* const handle,
                         const Point<T>& pos1,
                         const Point<T>& pos2,
                         const Point<T>& pos3,
                         const bool outline)
{
    DISTRHO_SAFE_ASSERT_RETURN(pos1 != pos2 && pos1 != pos3,);

    // TODO
}

template<typename T>
void Triangle<T>::draw(const GraphicsContext& context)
{
    cairo_t* const handle = ((const CairoGraphicsContext&)context).handle;

    drawTriangle<T>(handle, pos1, pos2, pos3, false);
}

template<typename T>
void Triangle<T>::drawOutline(const GraphicsContext& context, const T lineWidth)
{
    DISTRHO_SAFE_ASSERT_RETURN(lineWidth != 0,);

    cairo_t* const handle = ((const CairoGraphicsContext&)context).handle;

    cairo_set_line_width(handle, lineWidth);
    drawTriangle<T>(handle, pos1, pos2, pos3, true);
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
static void drawRectangle(cairo_t* const handle, const Rectangle<T>& rect, const bool outline)
{
    // TODO
}

template<typename T>
void Rectangle<T>::draw(const GraphicsContext& context)
{
    DISTRHO_SAFE_ASSERT_RETURN(isValid(),);

    cairo_t* const handle = ((const CairoGraphicsContext&)context).handle;

    drawRectangle(handle, *this, false);
}

template<typename T>
void Rectangle<T>::drawOutline(const GraphicsContext& context, const T lineWidth)
{
    DISTRHO_SAFE_ASSERT_RETURN(isValid(),);
    DISTRHO_SAFE_ASSERT_RETURN(lineWidth != 0,);

    cairo_t* const handle = ((const CairoGraphicsContext&)context).handle;

    cairo_set_line_width(handle, lineWidth);
    drawRectangle(handle, *this, true);
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
// CairoImage

CairoImage::CairoImage()
    : ImageBase() {}

CairoImage::CairoImage(const char* const rawData, const uint width, const uint height, const ImageFormat format)
    : ImageBase(rawData, width, height, format) {}

CairoImage::CairoImage(const char* const rawData, const Size<uint>& size, const ImageFormat format)
    : ImageBase(rawData, size, format) {}

CairoImage::CairoImage(const CairoImage& image)
    : ImageBase(image.rawData, image.size, image.format) {}

CairoImage::~CairoImage()
{
}

void CairoImage::drawAt(const GraphicsContext&, const Point<int>&)
{
}

// -----------------------------------------------------------------------
// CairoSubWidget

template <>
CairoWidget<SubWidget>::CairoWidget(Widget* const parent)
    : SubWidget(parent) {}

template class CairoWidget<SubWidget>;

// -----------------------------------------------------------------------
// CairoTopLevelWidget

template <>
CairoWidget<TopLevelWidget>::CairoWidget(Window& windowToMapTo)
    : TopLevelWidget(windowToMapTo) {}

template class CairoWidget<TopLevelWidget>;

// -----------------------------------------------------------------------
// CairoStandaloneWindow

template <>
CairoWidget<StandaloneWindow>::CairoWidget(Application& app)
    : StandaloneWindow(app) {}

template <>
CairoWidget<StandaloneWindow>::CairoWidget(Application& app, Window& parentWindow)
    : StandaloneWindow(app, parentWindow) {}

template class CairoWidget<StandaloneWindow>;

// -----------------------------------------------------------------------

template <>
void ImageBaseAboutWindow<CairoImage>::onDisplay()
{
    img.draw(getGraphicsContext());
}

template class ImageBaseAboutWindow<CairoImage>;

// -----------------------------------------------------------------------

void SubWidget::PrivateData::display(const uint width, const uint height, const double autoScaleFactor)
{
    /*
    if ((skipDisplay && ! renderingSubWidget) || size.isInvalid() || ! visible)
        return;
    */

    cairo_t* cr = static_cast<const CairoGraphicsContext&>(self->getGraphicsContext()).handle;
    cairo_matrix_t matrix;
    cairo_get_matrix(cr, &matrix);
    cairo_translate(cr, absolutePos.getX(), absolutePos.getY());
    // TODO: autoScaling and cropping

    // display widget
    self->onDisplay();

    cairo_set_matrix(cr, &matrix);

    selfw->pData->displaySubWidgets(width, height, autoScaleFactor);
}

// -----------------------------------------------------------------------

void TopLevelWidget::PrivateData::display()
{
    const Size<uint> size(window.getSize());
    const uint width  = size.getWidth();
    const uint height = size.getHeight();

    const double autoScaleFactor = window.pData->autoScaleFactor;

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
    GraphicsContext& context((GraphicsContext&)graphicsContext);
    ((CairoGraphicsContext&)context).handle = (cairo_t*)puglGetContext(view);
    return context;
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

// -----------------------------------------------------------------------
// templated classes

#include "ImageBaseWidgets.cpp"

// -----------------------------------------------------------------------
