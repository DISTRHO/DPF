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

#ifndef DGL_WIDGET_HPP_INCLUDED
#define DGL_WIDGET_HPP_INCLUDED

#include "Events.hpp"

// -----------------------------------------------------------------------
// Forward class names

// #ifdef DISTRHO_DEFINES_H_INCLUDED
// START_NAMESPACE_DISTRHO
// class UI;
// END_NAMESPACE_DISTRHO
// #endif

START_NAMESPACE_DGL

// class Application;
// class NanoWidget;
// class Window;
// class StandaloneWindow;
// class SubWidget;
class TopLevelWidget;

using namespace Events;

// -----------------------------------------------------------------------

/**
   Base DGL Widget class.

   This is the base Widget class, from which all widgets are built.

   All widgets have a parent Window where they'll be drawn.
   This parent is never changed during the widget lifetime.

   Widgets receive events in relative coordinates.
   (0, 0) means its top-left position.

   Windows paint widgets in the order they are constructed.
   Early widgets are drawn first, at the bottom, then newer ones on top.
   Events are sent in the inverse order so that the top-most widget gets
   a chance to catch the event and stop its propagation.

   All widget event callbacks do nothing by default.
 */
class Widget
{
   /**
      Constructor, reserved for DGL ..
      use TopLevelWidget or SubWidget instead
    */
    explicit Widget(TopLevelWidget& topLevelWidget);

public:
   /**
      Destructor.
    */
    virtual ~Widget();

   /**
      Check if this widget is visible within its parent window.
      Invisible widgets do not receive events except resize.
    */
    bool isVisible() const noexcept;

   /**
      Set widget visible (or not) according to @a yesNo.
    */
    void setVisible(bool yesNo);

   /**
      Show widget.
      This is the same as calling setVisible(true).
    */
    void show();

   /**
      Hide widget.
      This is the same as calling setVisible(false).
    */
    void hide();

   /**
      Get width.
    */
    uint getWidth() const noexcept;

   /**
      Get height.
    */
    uint getHeight() const noexcept;

   /**
      Get size.
    */
    const Size<uint>& getSize() const noexcept;

   /**
      Set width.
    */
    void setWidth(uint width) noexcept;

   /**
      Set height.
    */
    void setHeight(uint height) noexcept;

   /**
      Set size using @a width and @a height values.
    */
    void setSize(uint width, uint height) noexcept;

   /**
      Set size.
    */
    void setSize(const Size<uint>& size) noexcept;

   /**
      Get top-level widget, as passed in the constructor.
    */
    TopLevelWidget& getTopLevelWidget() const noexcept;

   /**
      Tell this widget's window to repaint itself.
      FIXME better description, partial redraw
    */
    void repaint() noexcept;

   /**
      Get the Id associated with this widget.
      @see setId
    */
    uint getId() const noexcept;

   /**
      Set an Id to be associated with this widget.
      @see getId
    */
    void setId(uint id) noexcept;

protected:
   /**
      A function called to draw the view contents with OpenGL.
    */
    virtual void onDisplay() = 0;

   /**
      A function called when a key is pressed or released.
      @return True to stop event propagation, false otherwise.
    */
    virtual bool onKeyboard(const KeyboardEvent&);

   /**
      A function called when a special key is pressed or released.
      @return True to stop event propagation, false otherwise.
    */
    virtual bool onSpecial(const SpecialEvent&);

   /**
      A function called when an UTF-8 character is received.
      @return True to stop event propagation, false otherwise.
    */
    virtual bool onCharacterInput(const CharacterInputEvent&);

   /**
      A function called when a mouse button is pressed or released.
      @return True to stop event propagation, false otherwise.
    */
    virtual bool onMouse(const MouseEvent&);

   /**
      A function called when the pointer moves.
      @return True to stop event propagation, false otherwise.
    */
    virtual bool onMotion(const MotionEvent&);

   /**
      A function called on scrolling (e.g. mouse wheel or track pad).
      @return True to stop event propagation, false otherwise.
    */
    virtual bool onScroll(const ScrollEvent&);

   /**
      A function called when the widget is resized.
    */
    virtual void onResize(const ResizeEvent&);

private:
    struct PrivateData;
    PrivateData* const pData;

//     friend class NanoWidget;
//     friend class Window;
//     friend class StandaloneWindow;
    friend class TopLevelWidget;
// #ifdef DISTRHO_DEFINES_H_INCLUDED
//     friend class DISTRHO_NAMESPACE::UI;
// #endif

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Widget)
};

#if 0
    // TODO: should we remove this?
   /**
      Get this widget's window application.
      Same as calling getParentWindow().getApp().
    */
    Application& getParentApp() const noexcept;

   /**
      Get parent window, as passed in the constructor.
    */
    Window& getParentWindow() const noexcept;
#endif

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_WIDGET_HPP_INCLUDED
