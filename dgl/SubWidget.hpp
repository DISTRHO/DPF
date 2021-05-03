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

#ifndef DGL_SUBWIDGET_HPP_INCLUDED
#define DGL_SUBWIDGET_HPP_INCLUDED

#include "Widget.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

/**
   Sub-Widget class.

   This is a handy Widget class that can be freely positioned to be used directly on a Window.

   This widget takes the full size of the Window it is mapped to.
   Sub-widgets can be added on top of this top-level widget, by creating them with this class as parent.
   Doing so allows for custom position and sizes.
 */
class SubWidget : public Widget
{
public:
   /**
      Constructor.
    */
    explicit SubWidget(Widget* widgetToGroupTo);

   /**
      Destructor.
    */
    virtual ~SubWidget();

   /**
      Check if this widget contains the point defined by @a x and @a y.
    */
    template<typename T>
    bool contains(T x, T y) const noexcept;

   /**
      Check if this widget contains the point @a pos.
    */
    template<typename T>
    bool contains(const Point<T>& pos) const noexcept;

   /**
      Get absolute X.
    */
    int getAbsoluteX() const noexcept;

   /**
      Get absolute Y.
    */
    int getAbsoluteY() const noexcept;

   /**
      Get absolute position.
    */
    const Point<int>& getAbsolutePos() const noexcept;

   /**
      Set absolute X.
    */
    void setAbsoluteX(int x) noexcept;

   /**
      Set absolute Y.
    */
    void setAbsoluteY(int y) noexcept;

   /**
      Set absolute position using @a x and @a y values.
    */
    void setAbsolutePos(int x, int y) noexcept;

   /**
      Set absolute position.
    */
    void setAbsolutePos(const Point<int>& pos) noexcept;

protected:
   /**
      A function called when the subwidget's absolute position is changed.
    */
    virtual void onPositionChanged(const PositionChangedEvent&);

private:
    struct PrivateData;
    PrivateData* const pData;
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SubWidget)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_SUBWIDGET_HPP_INCLUDED
