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

#ifndef DGL_TOP_LEVEL_WIDGET_HPP_INCLUDED
#define DGL_TOP_LEVEL_WIDGET_HPP_INCLUDED

#include "Widget.hpp"

START_NAMESPACE_DGL

class Window;

// -----------------------------------------------------------------------

/**
   Top-Level Widget class.

   This is the only Widget class that is allowed to be used directly on a Window.

   This widget takes the full size of the Window it is mapped to.
   Sub-widgets can be added on top of this top-level widget, by creating them with this class as parent.
   Doing so allows for custom position and sizes.

   This class is used as the type for DPF Plugin UIs.
   So anything that a plugin UI might need that does not belong in a simple Widget will go here.
 */
class TopLevelWidget : public Widget
{
public:
   /**
      Constructor.
    */
    explicit TopLevelWidget(Window& windowToMapTo);

   /**
      Destructor.
    */
    virtual ~TopLevelWidget();

protected:
   /**
      A function called to draw the widget contents.
      Reimplemented from Widget::onDisplay.
    */
    void onDisplay() override;

   /**
      A function called when the widget is resized.
      Reimplemented from Widget::onResize.
    */
    void onResize(const ResizeEvent&) override;

private:
    struct PrivateData;
    PrivateData* const pData;
    friend class Window;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopLevelWidget)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_TOP_LEVEL_WIDGET_HPP_INCLUDED
