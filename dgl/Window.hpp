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

#ifndef DGL_WINDOW_HPP_INCLUDED
#define DGL_WINDOW_HPP_INCLUDED

#include "Geometry.hpp"

START_NAMESPACE_DGL

class Application;
class TopLevelWidget;

// -----------------------------------------------------------------------

/**
   DGL Window class.

   This is the where all OS-related events initially happen, before being propagated to any widgets.

   A Window MUST have an Application instance tied to it.
   It is not possible to swap Application instances from within the lifetime of a Window.
   But it is possible to completely change the Widgets that a Window contains during its lifetime.

   Typically the event handling functions as following:
   Application -> Window -> Top-Level-Widget -> SubWidgets

   Please note that, unlike many other graphical toolkits out there,
   DGL makes a clear distinction between a Window and a Widget.
   You cannot directly draw in a Window, you need to create a Widget for that.

   Also, a Window MUST have a single top-level Widget.
   The Window will take care of global screen positioning and resizing, everything else is sent for widgets to handle.

   ...
 */
class Window
{
public:
   /**
      Constructor for a regular, standalone window.
    */
    explicit Window(Application& app);

   /**
      Constructor for an embed Window, typically used in modules or plugins that run inside another host.
    */
    explicit Window(Application& app,
                    uintptr_t parentWindowHandle,
                    uint width,
                    uint height,
                    double scaling,
                    bool resizable);

   /**
      Destructor.
    */
    virtual ~Window();

   /**
      Whether this Window is embed into another (usually not DGL-controlled) Window.
    */
    bool isEmbed() const noexcept;

   /**
      Check if this window is visible / mapped.
      Invisible windows do not receive events except resize.
      @see setVisible(bool)
    */
    bool isVisible() const noexcept;

   /**
      Set windows visible (or not) according to @a visible.
      Only valid for standalones, embed windows are always visible.
      @see isVisible(), hide(), show()
    */
    void setVisible(bool visible);

   /**
      Show window.
      This is the same as calling setVisible(true).
      @see isVisible(), setVisible(bool)
    */
    void show();

   /**
      Hide window.
      This is the same as calling setVisible(false).
      @see isVisible(), setVisible(bool)
    */
    void hide();

   /**
      Hide window and notify application of a window close event.
      The application event-loop will stop when all windows have been closed.
      For standalone windows only, has no effect if window is embed.
      @see isEmbed()

      @note It is possible to hide the window while not stopping the event-loop.
            A closed window is always hidden, but the reverse is not always true.
    */
    void close();

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
    Size<uint> getSize() const noexcept;

   /**
      Set width.
    */
    void setWidth(uint width);

   /**
      Set height.
    */
    void setHeight(uint height);

   /**
      Set size using @a width and @a height values.
    */
    void setSize(uint width, uint height);

   /**
      Set size.
    */
    void setSize(const Size<uint>& size);

    const char* getTitle() const noexcept;
    void setTitle(const char* title);

   /**
      Get the application associated with this window.
    */
    Application& getApp() const noexcept;

   /**
      Get the "native" window handle.
      Returned value depends on the platform:
       - HaikuOS: This is a pointer to a `BView`.
       - MacOS: This is a pointer to an `NSView*`.
       - Windows: This is a `HWND`.
       - Everything else: This is an [X11] `Window`.
    */
    uintptr_t getNativeWindowHandle() const noexcept;

    void repaint() noexcept;
    void repaint(const Rectangle<uint>& rect) noexcept;

protected:
   /**
      A function called when the window is resized.
      If there is a top-level widget associated with this window, its size will be set right after this function.
    */
    virtual void onReshape(uint width, uint height);

   /**
      A function called when the window is attempted to be closed.
      Returning true closes the window, which is the default behaviour.
      Override this method and return false to prevent the window from being closed by the user.
    */
    virtual bool onClose();

private:
    struct PrivateData;
    PrivateData* const pData;
    friend class Application;
    friend class TopLevelWidget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Window);
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

/* TODO
 * add focusEvent with CrossingMode arg
 * add eventcrossing/enter-leave event
 */

// class StandaloneWindow;
// class Widget;

// #ifdef DISTRHO_DEFINES_H_INCLUDED
// START_NAMESPACE_DISTRHO
// class UI;
// class UIExporter;
// END_NAMESPACE_DISTRHO
// #endif

#if 0
#ifndef DGL_FILE_BROWSER_DISABLED
   /**
      File browser options.
    */
    struct FileBrowserOptions {
        const char* startDir;
        const char* title;
        uint width;
        uint height;

      /**
         File browser buttons.

         0 means hidden.
         1 means visible and unchecked.
         2 means visible and checked.
        */
        struct Buttons {
            uint listAllFiles;
            uint showHidden;
            uint showPlaces;

            /** Constuctor for default values */
            Buttons()
                : listAllFiles(2),
                  showHidden(1),
                  showPlaces(1) {}
        } buttons;

        /** Constuctor for default values */
        FileBrowserOptions()
            : startDir(nullptr),
              title(nullptr),
              width(0),
              height(0),
              buttons() {}
    };
#endif // DGL_FILE_BROWSER_DISABLED

    static Window& withTransientParentWindow(Window& transientParentWindow);

    void exec(bool lockWait = false);

    void focus();

#ifndef DGL_FILE_BROWSER_DISABLED
    bool openFileBrowser(const FileBrowserOptions& options);
#endif

    bool isResizable() const noexcept;
    void setResizable(bool resizable);

    bool getIgnoringKeyRepeat() const noexcept;
    void setIgnoringKeyRepeat(bool ignore) noexcept;

    void setGeometryConstraints(uint width, uint height, bool aspect);
    void setTransientWinId(uintptr_t winId);

    double getScaling() const noexcept;

    const GraphicsContext& getGraphicsContext() const noexcept;

    void addIdleCallback(IdleCallback* const callback);
    void removeIdleCallback(IdleCallback* const callback);


protected:
#ifndef DGL_FILE_BROWSER_DISABLED
    virtual void fileBrowserSelected(const char* filename);
#endif

    // internal
    void _setAutoScaling(double scaling) noexcept;

    virtual void _addWidget(Widget* const widget);
    virtual void _removeWidget(Widget* const widget);
    void _idle();

    bool handlePluginKeyboard(const bool press, const uint key);
    bool handlePluginSpecial(const bool press, const Key key);

//     friend class Widget;
//     friend class StandaloneWindow;
// #ifdef DISTRHO_DEFINES_H_INCLUDED
//     friend class DISTRHO_NAMESPACE::UI;
//     friend class DISTRHO_NAMESPACE::UIExporter;
// #endif

    // Prevent copies
// #ifdef DISTRHO_PROPER_CPP11_SUPPORT
//     Window& operator=(Window&) = delete;
//     Window& operator=(const Window&) = delete;
// #else
//     Window& operator=(Window&);
//     Window& operator=(const Window&);
// #endif

#endif

// -----------------------------------------------------------------------

#endif // DGL_WINDOW_HPP_INCLUDED
