/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2022 Filipe Coelho <falktx@falktx.com>
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

#ifndef DGL_CAIRO_HPP_INCLUDED
#define DGL_CAIRO_HPP_INCLUDED

#include "ImageBase.hpp"
#include "ImageBaseWidgets.hpp"

#include <cairo.h>

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

/**
   Cairo Graphics context.
 */
struct CairoGraphicsContext : GraphicsContext
{
    cairo_t* handle;
};

// --------------------------------------------------------------------------------------------------------------------

/**
   Cairo Image class.

   TODO ...
 */
class CairoImage : public ImageBase
{
public:
   /**
      Constructor for a null Image.
    */
    CairoImage();

   /**
      Constructor using raw image data.
      @note @a rawData must remain valid for the lifetime of this Image.
    */
    CairoImage(const char* rawData, uint width, uint height, ImageFormat format);

   /**
      Constructor using raw image data.
      @note @a rawData must remain valid for the lifetime of this Image.
    */
    CairoImage(const char* rawData, const Size<uint>& size, ImageFormat format);

   /**
      Constructor using another image data.
    */
    CairoImage(const CairoImage& image);

   /**
      Destructor.
    */
    ~CairoImage() override;

   /**
      Load raw image data from memory.
      @note @a rawData must remain valid for the lifetime of this Image.
    */
    void loadFromMemory(const char* rawData,
                        const Size<uint>& size,
                        ImageFormat format = kImageFormatBGRA) noexcept override;

   /**
      Load PNG image from memory.
      Image size is read from PNG contents.
      @note @a pngData must remain valid for the lifetime of this Image.
    */
    void loadFromPNG(const char* pngData, uint dataSize) noexcept;

   /**
      Draw this image at position @a pos using the graphics context @a context.
    */
    void drawAt(const GraphicsContext& context, const Point<int>& pos) override;

   /**
      Get the cairo surface currently associated with this image.
      FIXME might be removed
    */
    inline cairo_surface_t* getSurface() const noexcept
    {
        return surface;
    }

   /**
      TODO document this.
    */
    CairoImage& operator=(const CairoImage& image) noexcept;

    // FIXME this should not be needed
    inline void loadFromMemory(const char* rdata, uint w, uint h, ImageFormat fmt = kImageFormatBGRA)
    { loadFromMemory(rdata, Size<uint>(w, h), fmt); };
    inline void draw(const GraphicsContext& context)
    { drawAt(context, Point<int>(0, 0)); };
    inline void drawAt(const GraphicsContext& context, int x, int y)
    { drawAt(context, Point<int>(x, y)); };

private:
    cairo_surface_t* surface;
    uchar* surfacedata;
    int* datarefcount;
};

// --------------------------------------------------------------------------------------------------------------------

/**
   CairoWidget, handy class that takes graphics context during onDisplay and passes it in a new function.
 */
template <class BaseWidget>
class CairoBaseWidget : public BaseWidget
{
public:
   /**
      Constructor for a CairoSubWidget.
    */
    explicit CairoBaseWidget(Widget* const parentGroupWidget);

   /**
      Constructor for a CairoTopLevelWidget.
    */
    explicit CairoBaseWidget(Window& windowToMapTo);

   /**
      Constructor for a CairoStandaloneWindow without parent window.
    */
    explicit CairoBaseWidget(Application& app);

   /**
      Constructor for a CairoStandaloneWindow with parent window.
    */
    explicit CairoBaseWidget(Application& app, Window& parentWindow);

   /**
      Destructor.
    */
    ~CairoBaseWidget() override {}

protected:
   /**
      New virtual onDisplay function.
      @see onDisplay
    */
    virtual void onCairoDisplay(const CairoGraphicsContext& context) = 0;

private:
   /**
      Widget display function.
      Implemented internally to pass context into the drawing function.
    */
    void onDisplay() override
    {
        const CairoGraphicsContext& context((const CairoGraphicsContext&)BaseWidget::getGraphicsContext());
        onCairoDisplay(context);
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CairoBaseWidget);
};

typedef CairoBaseWidget<SubWidget> CairoSubWidget;
typedef CairoBaseWidget<TopLevelWidget> CairoTopLevelWidget;
typedef CairoBaseWidget<StandaloneWindow> CairoStandaloneWindow;

// --------------------------------------------------------------------------------------------------------------------

typedef ImageBaseAboutWindow<CairoImage> CairoImageAboutWindow;
typedef ImageBaseButton<CairoImage> CairoImageButton;
typedef ImageBaseKnob<CairoImage> CairoImageKnob;
typedef ImageBaseSlider<CairoImage> CairoImageSlider;
typedef ImageBaseSwitch<CairoImage> CairoImageSwitch;

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif
