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

#ifdef DISTRHO_DEFINES_H_INCLUDED
START_NAMESPACE_DISTRHO
class UI;
END_NAMESPACE_DISTRHO
#endif

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

   /**
      Get the application associated with this top-level widget's window.
    */
    Application& getApp() const noexcept;

   /**
      Get the window associated with this top-level widget.
    */
    Window& getWindow() const noexcept;

    // TODO group stuff after here, convenience functions present in Window class
    void repaint() noexcept;
    void repaint(const Rectangle<uint>& rect) noexcept;
    void setGeometryConstraints(uint minimumWidth,
                                uint minimumHeight,
                                bool keepAspectRatio = false,
                                bool automaticallyScale = false);

    // TODO deprecated
    Application& getParentApp() const noexcept { return getApp(); }
    Window& getParentWindow() const noexcept { return getWindow(); }

private:
    struct PrivateData;
    PrivateData* const pData;
    friend class Window;
#ifdef DISTRHO_DEFINES_H_INCLUDED
    friend class DISTRHO_NAMESPACE::UI;
#endif

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopLevelWidget)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_TOP_LEVEL_WIDGET_HPP_INCLUDED
