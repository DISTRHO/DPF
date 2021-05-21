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
#include "../ImageBaseWidgets.hpp"

#include "Common.hpp"
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

    // TODO use arc
    /*
    cairo_arc(handle, origx, origy, size, sin, cos);
    */

    cairo_move_to(handle, x + origx, y + origy);

    for (uint i=1; i<numSegments; ++i)
    {
        cairo_line_to(handle, x + origx, y + origy);

        t = x;
        x = cos * x - sin * y;
        y = sin * t + cos * y;
    }

    cairo_line_to(handle, x + origx, y + origy);

    if (outline)
        cairo_stroke(handle);
    else
        cairo_fill(handle);
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

    cairo_move_to(handle, pos1.getX(), pos1.getY());
    cairo_line_to(handle, pos2.getX(), pos2.getY());
    cairo_line_to(handle, pos3.getX(), pos3.getY());
    cairo_line_to(handle, pos1.getX(), pos1.getY());

    if (outline)
        cairo_stroke(handle);
    else
        cairo_fill(handle);
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
    cairo_rectangle(handle, rect.getX(), rect.getY(), rect.getWidth(), rect.getHeight());

    if (outline)
        cairo_stroke(handle);
    else
        cairo_fill(handle);
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

static cairo_format_t asCairoImageFormat(const ImageFormat format)
{
    switch (format)
    {
    case kImageFormatNull:
        break;
    case kImageFormatGrayscale:
        return CAIRO_FORMAT_A8;
    case kImageFormatBGR:
    case kImageFormatRGB:
        return CAIRO_FORMAT_RGB24;
    case kImageFormatBGRA:
    case kImageFormatRGBA:
        return CAIRO_FORMAT_ARGB32;
    }

    return CAIRO_FORMAT_INVALID;
}

CairoImage::CairoImage()
    : ImageBase(),
      surface(nullptr),
      surfacedata(nullptr),
      datarefcount(nullptr) {}

CairoImage::CairoImage(const char* const rawData, const uint width, const uint height, const ImageFormat format)
    : ImageBase(rawData, width, height, format),
      surface(nullptr),
      surfacedata(nullptr),
      datarefcount(nullptr)
{
    loadFromMemory(rawData, width, height, format);
}

CairoImage::CairoImage(const char* const rawData, const Size<uint>& size, const ImageFormat format)
    : ImageBase(rawData, size, format),
      surface(nullptr),
      surfacedata(nullptr),
      datarefcount(nullptr)
{
    loadFromMemory(rawData, size, format);
}

CairoImage::CairoImage(const CairoImage& image)
    : ImageBase(image.rawData, image.size, image.format),
      surface(cairo_surface_reference(image.surface)),
      surfacedata(image.surfacedata),
      datarefcount(image.datarefcount)
{
    if (datarefcount != nullptr)
        ++(*datarefcount);
}

CairoImage::~CairoImage()
{
    cairo_surface_destroy(surface);

    if (datarefcount != nullptr && --(*datarefcount) == 0)
    {
        std::free(surfacedata);
        std::free(datarefcount);
    }
}

void CairoImage::loadFromMemory(const char* const rdata, const Size<uint>& s, const ImageFormat fmt) noexcept
{
    const cairo_format_t cairoformat = asCairoImageFormat(fmt);
    const uint width  = s.getWidth();
    const uint height = s.getHeight();
    const int stride  = cairo_format_stride_for_width(cairoformat, width);

    uchar* const newdata = (uchar*)std::malloc(width * height * stride * 4);
    DISTRHO_SAFE_ASSERT_RETURN(newdata != nullptr,);

    cairo_surface_t* const newsurface = cairo_image_surface_create_for_data(newdata, cairoformat, width, height, stride);
    DISTRHO_SAFE_ASSERT_RETURN(newsurface != nullptr,);
    DISTRHO_SAFE_ASSERT_RETURN(s.getWidth() == cairo_image_surface_get_width(newsurface),);
    DISTRHO_SAFE_ASSERT_RETURN(s.getHeight() == cairo_image_surface_get_height(newsurface),);

    cairo_surface_destroy(surface);

    if (datarefcount != nullptr && --(*datarefcount) == 0)
        std::free(surfacedata);
    else
        datarefcount = (int*)malloc(sizeof(*datarefcount));

    surface = newsurface;
    surfacedata = newdata;
    *datarefcount = 1;

    switch (fmt)
    {
    case kImageFormatNull:
        break;
    case kImageFormatGrayscale:
        // Grayscale to A8
        // TODO
        break;
    case kImageFormatBGR:
        // BGR8 to CAIRO_FORMAT_RGB24
        for (uint h = 0; h < height; ++h)
        {
            for (uint w = 0; w < width; ++w)
            {
                newdata[h*width*4+w*4+0] = rdata[h*width*3+w*3+0];
                newdata[h*width*4+w*4+1] = rdata[h*width*3+w*3+1];
                newdata[h*width*4+w*4+2] = rdata[h*width*3+w*3+2];
                newdata[h*width*4+w*4+3] = 0;
            }
        }
        break;
    case kImageFormatBGRA:
        // RGB8 to CAIRO_FORMAT_ARGB32
        // TODO
        break;
    case kImageFormatRGB:
        // RGB8 to CAIRO_FORMAT_RGB24
        // TODO
        break;
    case kImageFormatRGBA:
        // RGBA8 to CAIRO_FORMAT_ARGB32
        // TODO
        break;
    }

    ImageBase::loadFromMemory(rdata, s, fmt);
}

void CairoImage::drawAt(const GraphicsContext& context, const Point<int>& pos)
{
    if (surface == nullptr)
        return;

    cairo_t* const handle = ((const CairoGraphicsContext&)context).handle;

    cairo_set_source_surface(handle, surface, pos.getX(), pos.getY());
    cairo_paint(handle);
}

CairoImage& CairoImage::operator=(const CairoImage& image) noexcept
{
    cairo_surface_t* newsurface = cairo_surface_reference(image.surface);
    cairo_surface_destroy(surface);
    surface = newsurface;
    rawData = image.rawData;
    size    = image.size;
    format  = image.format;
    surfacedata = image.surfacedata;
    datarefcount = image.datarefcount;
    if (datarefcount != nullptr)
        ++(*datarefcount);
    return *this;
}

// -----------------------------------------------------------------------
// CairoSubWidget

template <>
CairoBaseWidget<SubWidget>::CairoBaseWidget(Widget* const parent)
    : SubWidget(parent) {}

template class CairoBaseWidget<SubWidget>;

// -----------------------------------------------------------------------
// CairoTopLevelWidget

template <>
CairoBaseWidget<TopLevelWidget>::CairoBaseWidget(Window& windowToMapTo)
    : TopLevelWidget(windowToMapTo) {}

template class CairoBaseWidget<TopLevelWidget>;

// -----------------------------------------------------------------------
// CairoStandaloneWindow

template <>
CairoBaseWidget<StandaloneWindow>::CairoBaseWidget(Application& app)
    : StandaloneWindow(app) {}

template <>
CairoBaseWidget<StandaloneWindow>::CairoBaseWidget(Application& app, Window& parentWindow)
    : StandaloneWindow(app, parentWindow) {}

template class CairoBaseWidget<StandaloneWindow>;

// -----------------------------------------------------------------------
// ImageBaseAboutWindow

#if 0
template <>
void ImageBaseAboutWindow<CairoImage>::onDisplay()
{
    img.draw(getGraphicsContext());
}
#endif

template class ImageBaseAboutWindow<CairoImage>;

// -----------------------------------------------------------------------
// ImageBaseButton

template class ImageBaseButton<CairoImage>;

// -----------------------------------------------------------------------
// ImageBaseKnob

template <>
void ImageBaseKnob<CairoImage>::PrivateData::init()
{
    notImplemented("ImageBaseKnob::PrivateData::init");
}

template <>
void ImageBaseKnob<CairoImage>::PrivateData::cleanup()
{
    notImplemented("ImageBaseKnob::PrivateData::cleanup");
}

template <>
void ImageBaseKnob<CairoImage>::onDisplay()
{
    notImplemented("ImageBaseKnob::onDisplay");
}

template class ImageBaseKnob<CairoImage>;

// -----------------------------------------------------------------------
// ImageBaseSlider

template class ImageBaseSlider<CairoImage>;

// -----------------------------------------------------------------------
// ImageBaseSwitch

template class ImageBaseSwitch<CairoImage>;

// -----------------------------------------------------------------------

void SubWidget::PrivateData::display(const uint width, const uint height, const double autoScaleFactor)
{
    cairo_t* const handle = static_cast<const CairoGraphicsContext&>(self->getGraphicsContext()).handle;

    bool needsResetClip = false;

    cairo_matrix_t matrix;
    cairo_get_matrix(handle, &matrix);

    if (needsFullViewportForDrawing || (absolutePos.isZero() && self->getSize() == Size<uint>(width, height)))
    {
        // full viewport size
        cairo_translate(handle, 0, 0);
    }
    else if (needsViewportScaling)
    {
        // limit viewport to widget bounds
        // NOTE only used for nanovg for now, which is not relevant here
        cairo_translate(handle, 0, 0);
    }
    else
    {
        // set viewport pos
        cairo_translate(handle, absolutePos.getX(), absolutePos.getY());

        // then cut the outer bounds
        cairo_rectangle(handle,
                        0,
                        0,
                        std::round(self->getWidth() * autoScaleFactor),
                        std::round(self->getHeight() * autoScaleFactor));

        cairo_clip(handle);
        needsResetClip = true;
    }

    // display widget
    self->onDisplay();

    if (needsResetClip)
        cairo_reset_clip(handle);

    cairo_set_matrix(handle, &matrix);

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
