/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef DGL_NTK_WIDGET_HPP_INCLUDED
#define DGL_NTK_WIDGET_HPP_INCLUDED

#include "NtkWindow.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

/**
   DGL compatible Widget class that uses NTK instead of OpenGL.
   @see Widget
 */
class NtkWidget : public Fl_Double_Window
{
public:
   /**
      Constructor.
    */
    explicit NtkWidget(NtkWindow& parent)
        : Fl_Double_Window(100, 100),
          fParent(parent)
    {
        fParent.add(this);
        show();
    }

   /**
      Destructor.
    */
    ~NtkWidget() override
    {
        hide();
        fParent.remove(this);
    }

   /**
      Check if this widget is visible within its parent window.
      Invisible widgets do not receive events except resize.
    */
    bool isVisible() const
    {
        return visible();
    }

   /**
      Set widget visible (or not) according to @a yesNo.
    */
    void setVisible(bool yesNo)
    {
        if (yesNo)
            show();
        else
            hide();
    }

   /**
      Get width.
    */
    int getWidth() const
    {
        return w();
    }

   /**
      Get height.
    */
    int getHeight() const
    {
        return h();
    }

   /**
      Set width.
    */
    void setWidth(int width)
    {
        resize(x(), y(), width, h());
    }

   /**
      Set height.
    */
    void setHeight(int height)
    {
        resize(x(), y(), w(), height);
    }

   /**
      Set size using @a width and @a height values.
    */
    void setSize(int width, int height)
    {
        resize(x(), y(), width, height);
    }

   /**
      Get absolute X.
    */
    int getAbsoluteX() const
    {
        return x();
    }

   /**
      Get absolute Y.
    */
    int getAbsoluteY() const
    {
        return y();
    }

   /**
      Set absolute X.
    */
    void setAbsoluteX(int x)
    {
        resize(x, y(), w(), h());
    }

   /**
      Set absolute Y.
    */
    void setAbsoluteY(int y)
    {
        resize(x(), y, w(), h());
    }

   /**
      Set absolute position using @a x and @a y values.
    */
    void setAbsolutePos(int x, int y)
    {
        resize(x, y, w(), h());
    }

   /**
      Get this widget's window application.
      Same as calling getParentWindow().getApp().
    */
    NtkApp& getParentApp() const noexcept
    {
        return fParent.getApp();
    }

   /**
      Get parent window, as passed in the constructor.
    */
    NtkWindow& getParentWindow() const noexcept
    {
        return fParent;
    }

   /**
      Check if this widget contains the point defined by @a x and @a y.
    */
    bool contains(int x, int y) const
    {
        return (x >= 0 && y >= 0 && x < w() && y < h());
    }

   /**
      Tell this widget's window to repaint itself.
    */
    void repaint()
    {
        redraw();
    }

protected:
   /** @internal used for DGL compatibility. */
    void setNeedsFullViewport(bool) noexcept {}
   /** @internal used for DGL compatibility. */
    void setNeedsScaling(bool) noexcept {}

private:
    NtkWindow& fParent;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NtkWidget)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_NTK_WIDGET_HPP_INCLUDED
