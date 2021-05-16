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

#ifndef DGL_CAIRO_HPP_INCLUDED
#define DGL_CAIRO_HPP_INCLUDED

#include "ImageBase.hpp"
#include "ImageBaseWidgets.hpp"
#include "SubWidget.hpp"
#include "TopLevelWidget.hpp"

#include <cairo/cairo.h>

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
      Draw this image at position @a pos using the graphics context @a context.
    */
    void drawAt(const GraphicsContext& context, const Point<int>& pos) override;

    // FIXME this should not be needed
    inline void drawAt(const GraphicsContext& context, int x, int y)
    { drawAt(context, Point<int>(x, y)); };
};

// --------------------------------------------------------------------------------------------------------------------

/**
   Cairo SubWidget, handy class that takes graphics context during onDisplay and passes it in a new function.
 */
class CairoSubWidget : public SubWidget
{
public:
    CairoSubWidget(Widget* widgetToGroupTo)
        : SubWidget(widgetToGroupTo) {}

protected:
    void onDisplay() override
    {
        const CairoGraphicsContext& context((const CairoGraphicsContext&)getGraphicsContext());
        onCairoDisplay(context);
    }

    virtual void onCairoDisplay(const CairoGraphicsContext& context) = 0;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CairoSubWidget);
};

// --------------------------------------------------------------------------------------------------------------------

/**
   Cairo TopLevelWidget, handy class that takes graphics context during onDisplay and passes it in a new function.
 */
class CairoTopLevelWidget : public TopLevelWidget
{
public:
    CairoTopLevelWidget(Window& windowToMapTo)
        : TopLevelWidget(windowToMapTo) {}

protected:
    void onDisplay() override
    {
        const CairoGraphicsContext& context((const CairoGraphicsContext&)getGraphicsContext());
        onCairoDisplay(context);
    }

    virtual void onCairoDisplay(const CairoGraphicsContext& context) = 0;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CairoTopLevelWidget);
};

// --------------------------------------------------------------------------------------------------------------------

typedef ImageBaseAboutWindow<CairoImage> CairoImageAboutWindow;

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif
