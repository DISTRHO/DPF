/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2024 Filipe Coelho <falktx@falktx.com>
 * Copyright (C) 2019-2021 Jean Pierre Cimalando <jp-dev@inbox.ru>
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

#include "../Cairo.hpp"
#include "../Color.hpp"
#include "../ImageBaseWidgets.hpp"

#include "SubWidgetPrivateData.hpp"
#include "TopLevelWidgetPrivateData.hpp"
#include "WidgetPrivateData.hpp"
#include "WindowPrivateData.hpp"

// templated classes
#include "ImageBaseWidgets.cpp"

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

static cairo_format_t asCairoImageFormat(const ImageFormat format) noexcept
{
    switch (format)
    {
    case kImageFormatNull:
        break;
    case kImageFormatGrayscale:
    case kImageFormatBGR:
    case kImageFormatRGB:
        return CAIRO_FORMAT_RGB24;
    case kImageFormatBGRA:
    case kImageFormatRGBA:
        return CAIRO_FORMAT_ARGB32;
    }

    return CAIRO_FORMAT_INVALID;
}

/*
static ImageFormat asCairoImageFormat(const cairo_format_t format) noexcept
{
    switch (format)
    {
    case CAIRO_FORMAT_INVALID:
        break;
    case CAIRO_FORMAT_ARGB32:
        break;
    case CAIRO_FORMAT_RGB24:
        break;
    case CAIRO_FORMAT_A8:
        break;
    case CAIRO_FORMAT_A1:
        break;
    case CAIRO_FORMAT_RGB16_565:
        break;
    case CAIRO_FORMAT_RGB30:
        break;
    }

    return kImageFormatNull;
}
*/

CairoImage::CairoImage()
    : ImageBase(),
      surface(nullptr),
      surfacedata(nullptr),
      datarefcount(nullptr) {}

CairoImage::CairoImage(const char* const rdata, const uint w, const uint h, const ImageFormat fmt)
    : ImageBase(rdata, w, h, fmt),
      surface(nullptr),
      surfacedata(nullptr),
      datarefcount(nullptr)
{
    loadFromMemory(rdata, w, h, fmt);
}

CairoImage::CairoImage(const char* const rdata, const Size<uint>& s, const ImageFormat fmt)
    : ImageBase(rdata, s, fmt),
      surface(nullptr),
      surfacedata(nullptr),
      datarefcount(nullptr)
{
    loadFromMemory(rdata, s, fmt);
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
    DISTRHO_SAFE_ASSERT_RETURN(cairoformat != CAIRO_FORMAT_INVALID,);

    const int width  = static_cast<int>(s.getWidth());
    const int height = static_cast<int>(s.getHeight());
    const int stride = cairo_format_stride_for_width(cairoformat, width);

    uchar* const newdata = static_cast<uchar*>(std::malloc(static_cast<size_t>(width * height * stride * 4)));
    DISTRHO_SAFE_ASSERT_RETURN(newdata != nullptr,);

    cairo_surface_t* const newsurface = cairo_image_surface_create_for_data(newdata, cairoformat, width, height, stride);
    DISTRHO_SAFE_ASSERT_RETURN(newsurface != nullptr,);
    DISTRHO_SAFE_ASSERT_RETURN(static_cast<int>(s.getWidth()) == cairo_image_surface_get_width(newsurface),);
    DISTRHO_SAFE_ASSERT_RETURN(static_cast<int>(s.getHeight()) == cairo_image_surface_get_height(newsurface),);

    cairo_surface_destroy(surface);

    if (datarefcount != nullptr && --(*datarefcount) == 0)
        std::free(surfacedata);
    else
        datarefcount = static_cast<int*>(std::malloc(sizeof(int)));

    surface = newsurface;
    surfacedata = newdata;
    *datarefcount = 1;

    const uchar* const urdata = reinterpret_cast<const uchar*>(rdata);

    switch (fmt)
    {
    case kImageFormatNull:
        break;
    case kImageFormatGrayscale:
        // Grayscale to CAIRO_FORMAT_RGB24
        for (int h = 0; h < height; ++h)
        {
            for (int w = 0; w < width; ++w)
            {
                newdata[h*width*4+w*4+0] = urdata[h*width+w];
                newdata[h*width*4+w*4+1] = urdata[h*width+w];
                newdata[h*width*4+w*4+2] = urdata[h*width+w];
                newdata[h*width*4+w*4+3] = 0;
            }
        }
        break;
    case kImageFormatBGR:
        // BGR8 to CAIRO_FORMAT_RGB24
        for (int h = 0; h < height; ++h)
        {
            for (int w = 0; w < width; ++w)
            {
                newdata[h*width*4+w*4+0] = urdata[h*width*3+w*3+0];
                newdata[h*width*4+w*4+1] = urdata[h*width*3+w*3+1];
                newdata[h*width*4+w*4+2] = urdata[h*width*3+w*3+2];
                newdata[h*width*4+w*4+3] = 0;
            }
        }
        break;
    case kImageFormatBGRA:
        // BGRA8 to CAIRO_FORMAT_ARGB32
        for (int h = 0; h < height; ++h)
        {
            for (int w = 0; w < width; ++w)
            {
                const uchar a = urdata[h*width*4+w*4+3];
                newdata[h*width*4+w*4+0] = static_cast<uchar>((urdata[h*width*4+w*4+0] * a) >> 8);
                newdata[h*width*4+w*4+1] = static_cast<uchar>((urdata[h*width*4+w*4+1] * a) >> 8);
                newdata[h*width*4+w*4+2] = static_cast<uchar>((urdata[h*width*4+w*4+2] * a) >> 8);
                newdata[h*width*4+w*4+3] = a;
            }
        }
        break;
    case kImageFormatRGB:
        // RGB8 to CAIRO_FORMAT_RGB24
        for (int h = 0; h < height; ++h)
        {
            for (int w = 0; w < width; ++w)
            {
                newdata[h*width*4+w*4+0] = urdata[h*width*3+w*3+2];
                newdata[h*width*4+w*4+1] = urdata[h*width*3+w*3+1];
                newdata[h*width*4+w*4+2] = urdata[h*width*3+w*3+0];
                newdata[h*width*4+w*4+3] = 0;
            }
        }
        break;
    case kImageFormatRGBA:
        // RGBA8 to CAIRO_FORMAT_ARGB32
        for (int h = 0; h < height; ++h)
        {
            for (int w = 0; w < width; ++w)
            {
                const uchar a = urdata[h*width*4+w*4+3];
                newdata[h*width*4+w*4+0] = static_cast<uchar>((urdata[h*width*4+w*4+2] * a) >> 8);
                newdata[h*width*4+w*4+1] = static_cast<uchar>((urdata[h*width*4+w*4+1] * a) >> 8);
                newdata[h*width*4+w*4+2] = static_cast<uchar>((urdata[h*width*4+w*4+0] * a) >> 8);
                newdata[h*width*4+w*4+3] = a;
            }
        }
        break;
    }

    ImageBase::loadFromMemory(rdata, s, fmt);
}

// const GraphicsContext& context
void CairoImage::loadFromPNG(const char* const pngData, const uint pngSize) noexcept
{
    struct PngReaderData
    {
        const char* dataPtr;
        uint sizeLeft;

        static cairo_status_t read(void* const closure, uchar* const data, const uint length) noexcept
        {
            PngReaderData& readerData = *reinterpret_cast<PngReaderData*>(closure);

            if (readerData.sizeLeft < length)
                return CAIRO_STATUS_READ_ERROR;

            std::memcpy(data, readerData.dataPtr, length);
            readerData.dataPtr += length;
            readerData.sizeLeft -= length;
            return CAIRO_STATUS_SUCCESS;
        }
    };

    PngReaderData readerData;
    readerData.dataPtr = pngData;
    readerData.sizeLeft = pngSize;

    cairo_surface_t* const newsurface = cairo_image_surface_create_from_png_stream(PngReaderData::read, &readerData);
    DISTRHO_SAFE_ASSERT_RETURN(newsurface != nullptr,);

    const int newwidth = cairo_image_surface_get_width(newsurface);
    const int newheight = cairo_image_surface_get_height(newsurface);
    DISTRHO_SAFE_ASSERT_INT_RETURN(newwidth > 0, newwidth,);
    DISTRHO_SAFE_ASSERT_INT_RETURN(newheight > 0, newheight,);

    cairo_surface_destroy(surface);

    if (datarefcount != nullptr && --(*datarefcount) == 0)
        std::free(surfacedata);
    else
        datarefcount = (int*)malloc(sizeof(*datarefcount));

    surface = newsurface;
    surfacedata = nullptr; // cairo_image_surface_get_data(newsurface);
    *datarefcount = 1;

    rawData = nullptr;
    format = kImageFormatNull; // asCairoImageFormat(cairo_image_surface_get_format(newsurface));
    size = Size<uint>(static_cast<uint>(newwidth), static_cast<uint>(newheight));
}

void CairoImage::drawAt(const GraphicsContext& context, const Point<int>& pos)
{
    DISTRHO_SAFE_ASSERT_RETURN(surface != nullptr,);

    cairo_t* const handle = ((const CairoGraphicsContext&)context).handle;

    cairo_set_source_surface(handle, surface, pos.getX(), pos.getY());
    cairo_paint(handle);
}

CairoImage& CairoImage::operator=(const CairoImage& image) noexcept
{
    cairo_surface_t* newsurface = cairo_surface_reference(image.surface);
    cairo_surface_destroy(surface);

    if (datarefcount != nullptr && --(*datarefcount) == 0)
    {
        std::free(surfacedata);
        std::free(datarefcount);
    }

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
    alwaysRepaint = true;
    cairoSurface = nullptr;
}

template <>
void ImageBaseKnob<CairoImage>::PrivateData::cleanup()
{
    cairo_surface_destroy((cairo_surface_t*)cairoSurface);
    cairoSurface = nullptr;
}

/**
   Get the pixel size in bytes.
   @return pixel size, or 0 if the format is unknown, or pixels are not aligned to bytes.
*/
static int getBytesPerPixel(const cairo_format_t format) noexcept
{
    switch (format)
    {
    case CAIRO_FORMAT_ARGB32:
    case CAIRO_FORMAT_RGB24:
        return 4;
    case CAIRO_FORMAT_A8:
        return 1;
    default:
        DISTRHO_SAFE_ASSERT(false);
        return 0;
    }
}

static cairo_surface_t* getRegion(cairo_surface_t* origsurface, int x, int y, int width, int height) noexcept
{
    const cairo_format_t format = cairo_image_surface_get_format(origsurface);
    DISTRHO_SAFE_ASSERT_RETURN(format != CAIRO_FORMAT_INVALID, nullptr);

    const int bpp = getBytesPerPixel(format);
    DISTRHO_SAFE_ASSERT_RETURN(bpp != 0, nullptr);

    const int fullWidth   = cairo_image_surface_get_width(origsurface);
    const int fullHeight  = cairo_image_surface_get_height(origsurface);
    const int stride      = cairo_image_surface_get_stride(origsurface);
    uchar* const fullData = cairo_image_surface_get_data(origsurface);

    x = (x < fullWidth) ? x : fullWidth;
    y = (y < fullHeight) ? y : fullHeight;
    width = (x + width < fullWidth) ? width : (fullWidth - x);
    height = (x + height < fullHeight) ? height : (fullHeight - x);

    uchar* const data = fullData + (x * bpp + y * stride);
    return cairo_image_surface_create_for_data(data, format, width, height, stride);
}

template <>
void ImageBaseKnob<CairoImage>::onDisplay()
{
    const GraphicsContext& context(getGraphicsContext());
    cairo_t* const handle = ((const CairoGraphicsContext&)context).handle;
    const double normValue = getNormalizedValue();

    cairo_surface_t* surface = (cairo_surface_t*)pData->cairoSurface;

    if (! pData->isReady)
    {
        const int layerW = static_cast<int>(pData->imgLayerWidth);
        const int layerH = static_cast<int>(pData->imgLayerHeight);
        int layerNum = 0;

        if (pData->rotationAngle == 0)
            layerNum = static_cast<int>(normValue * static_cast<double>(pData->imgLayerCount - 1) + 0.5);

        const int layerX = pData->isImgVertical ? 0 : layerNum * layerW;
        const int layerY = !pData->isImgVertical ? 0 : layerNum * layerH;

        cairo_surface_t* newsurface;

        if (pData->rotationAngle == 0)
        {
            newsurface = getRegion(pData->image.getSurface(), layerX, layerY, layerW, layerH);
        }
        else
        {
            newsurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, layerW, layerH);
            cairo_t* const cr = cairo_create(newsurface);
            cairo_translate(cr, 0.5 * layerW, 0.5 * layerH);
            cairo_rotate(cr, normValue * pData->rotationAngle * (M_PI / 180));
            cairo_set_source_surface(cr, pData->image.getSurface(), -0.5 * layerW, -0.5 * layerH);
            cairo_paint(cr);
            cairo_destroy(cr);
        }

        DISTRHO_SAFE_ASSERT_RETURN(newsurface != nullptr,);

        cairo_surface_destroy(surface);
        pData->cairoSurface = surface = newsurface;
        pData->isReady = true;
    }

    if (surface != nullptr)
    {
        cairo_set_source_surface(handle, surface, 0, 0);
        cairo_paint(handle);
    }
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

    if (needsViewportScaling)
    {
        // limit viewport to widget bounds
        // NOTE only used for nanovg for now, which is not relevant here
    }
    else if (needsFullViewportForDrawing || (absolutePos.isZero() && self->getSize() == Size<uint>(width, height)))
    {
        // full viewport size
        cairo_translate(handle, 0, 0);
        cairo_scale(handle, autoScaleFactor, autoScaleFactor);
    }
    else
    {
        // set viewport pos
        cairo_translate(handle, absolutePos.getX() * autoScaleFactor, absolutePos.getY() * autoScaleFactor);

        // then cut the outer bounds
        cairo_rectangle(handle,
                        0,
                        0,
                        std::round(self->getWidth() * autoScaleFactor),
                        std::round(self->getHeight() * autoScaleFactor));

        cairo_clip(handle);
        needsResetClip = true;

        // set viewport scaling
        cairo_scale(handle, autoScaleFactor, autoScaleFactor);
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
    if (! selfw->pData->visible)
        return;

    cairo_t* const handle = static_cast<const CairoGraphicsContext&>(self->getGraphicsContext()).handle;

    const Size<uint> size(window.getSize());
    const uint width  = size.getWidth();
    const uint height = size.getHeight();

    const double autoScaleFactor = window.pData->autoScaleFactor;

    cairo_matrix_t matrix;
    cairo_get_matrix(handle, &matrix);

    // full viewport size
    cairo_translate(handle, 0, 0);

    if (window.pData->autoScaling)
        cairo_scale(handle, autoScaleFactor, autoScaleFactor);
    else
        cairo_scale(handle, 1.0, 1.0);

    // main widget drawing
    self->onDisplay();

    cairo_set_matrix(handle, &matrix);

    // now draw subwidgets if there are any
    selfw->pData->displaySubWidgets(width, height, autoScaleFactor);
}

// -----------------------------------------------------------------------

void Window::PrivateData::renderToPicture(const char*, const GraphicsContext&, uint, uint)
{
    notImplemented("Window::PrivateData::renderToPicture");
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
