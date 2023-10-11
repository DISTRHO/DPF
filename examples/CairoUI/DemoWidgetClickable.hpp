/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2019-2021 Jean Pierre Cimalando <jp-dev@inbox.ru>
 * Copyright (C) 2012-2023 Filipe Coelho <falktx@falktx.com>
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

#pragma once

#include "Cairo.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

class DemoWidgetClickable : public CairoSubWidget,
                            public ButtonEventHandler,
                            public ButtonEventHandler::Callback
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void demoWidgetClicked(DemoWidgetClickable* widget, uint8_t colorId) = 0;
    };

    explicit DemoWidgetClickable(SubWidget* const parent)
        : CairoSubWidget(parent),
          ButtonEventHandler(this),
          fCallback(nullptr),
          fColorId(0)
    {
        ButtonEventHandler::setCallback(this);
    }

    explicit DemoWidgetClickable(TopLevelWidget* const parent)
        : CairoSubWidget(parent),
          ButtonEventHandler(this),
          fCallback(nullptr),
          fColorId(0)
    {
        ButtonEventHandler::setCallback(this);
    }

    void setCallback(Callback* const callback) noexcept
    {
        fCallback = callback;
    }

    uint8_t getColorId() const noexcept
    {
        return fColorId;
    }

    void setColorId(uint8_t colorId) noexcept
    {
        fColorId = colorId;
        repaint();
    }

protected:
    void onCairoDisplay(const CairoGraphicsContext& context) override
    {
        cairo_t* const cr = context.handle;

        const Size<uint> sz = getSize();
        const int w = sz.getWidth();
        const int h = sz.getHeight();

        switch (fColorId)
        {
        case 0:
            cairo_set_source_rgb(cr, 0.75, 0.0, 0.0);
            break;
        case 1:
            cairo_set_source_rgb(cr, 0.0, 0.75, 0.0);
            break;
        case 2:
            cairo_set_source_rgb(cr, 0.0, 0.0, 0.75);
            break;
        }

        cairo_rectangle(cr, 0, 0, w, h);
        cairo_fill(cr);

        cairo_set_source_rgb(cr, 0.9, 0.9, 0.9);
        cairo_new_path(cr);
        cairo_move_to(cr, 0.25 * w, 0.25 * h);
        cairo_line_to(cr, 0.75 * w, 0.75 * h);
        cairo_stroke(cr);
        cairo_new_path(cr);
        cairo_move_to(cr, 0.75 * w, 0.25 * h);
        cairo_line_to(cr, 0.25 * w, 0.75 * h);
        cairo_stroke(cr);
    }

    bool onMouse(const MouseEvent& event) override
    {
        return ButtonEventHandler::mouseEvent(event);
    }

    void buttonClicked(SubWidget*, int) override
    {
        fColorId = (fColorId + 1) % 3;
        repaint();

        if (fCallback != nullptr)
            fCallback->demoWidgetClicked(this, fColorId);
    }

private:
    Callback* fCallback;
    uint8_t fColorId;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

using DGL_NAMESPACE::DemoWidgetClickable;
