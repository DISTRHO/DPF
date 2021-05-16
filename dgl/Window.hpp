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
      Constructor for a modal window, by having another window as its parent.
      The Application instance must be the same between the 2 windows.
    */
    explicit Window(Application& app, Window& parent);

   /**
      Constructor for an embed Window without known size,
      typically used in modules or plugins that run inside another host.
    */
    explicit Window(Application& app,
                    uintptr_t parentWindowHandle,
                    double scaleFactor,
                    bool resizable);

   /**
      Constructor for an embed Window with known size,
      typically used in modules or plugins that run inside another host.
    */
    explicit Window(Application& app,
                    uintptr_t parentWindowHandle,
                    uint width,
                    uint height,
                    double scaleFactor,
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

    bool isResizable() const noexcept;
    void setResizable(bool resizable);

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

   /**
      Get the title of the window previously set with setTitle().
    */
    const char* getTitle() const noexcept;

   /**
      Set the title of the window, typically displayed in the title bar or in window switchers.

      This only makes sense for non-embedded windows.
    */
    void setTitle(const char* title);

   /**
      Check if key repeat events are ignored.
    */
    bool isIgnoringKeyRepeat() const noexcept;

   /**
      Set to ignore (or not) key repeat events according to @a ignore.
    */
    void setIgnoringKeyRepeat(bool ignore) noexcept;

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

   /**
      Get the scale factor requested for this window.
      This is purely informational, and up to developers to choose what to do with it.

      If you do not want to deal with this yourself,
      consider using setGeometryConstraints() where you can specify to automatically scale the window contents.
      @see setGeometryConstraints
    */
    double getScaleFactor() const noexcept;

   /**
      Grab the keyboard input focus.
    */
    void focus();

   /**
      Request repaint of this window, for the entire area.
    */
    void repaint() noexcept;

   /**
      Request partial repaint of this window, with bounds according to @a rect.
    */
    void repaint(const Rectangle<uint>& rect) noexcept;

   /**
      Run this window as a modal, blocking input events from the parent.
      Only valid for windows that have been created with another window as parent (as passed in the constructor).
      Can optionally block-wait, but such option is only available if the application is running as standalone.
    */
    void runAsModal(bool blockWait = false);

   /**
      Set geometry constraints for the Window when resized by the user, and optionally scale contents automatically.
    */
    void setGeometryConstraints(uint minimumWidth,
                                uint minimumHeight,
                                bool keepAspectRatio = false,
                                bool automaticallyScale = false);

    /*
    void setTransientWinId(uintptr_t winId);
    */

    // TODO deprecated
    inline bool getIgnoringKeyRepeat() const noexcept { return isIgnoringKeyRepeat(); }
    inline double getScaling() const noexcept { return getScaling(); }
    inline void exec(bool blockWait = false) { runAsModal(blockWait); }

protected:
   /**
      A function called when the window is attempted to be closed.
      Returning true closes the window, which is the default behaviour.
      Override this method and return false to prevent the window from being closed by the user.
    */
    virtual bool onClose();

   /**
      A function called when the window gains or loses the keyboard focus.
      The default implementation does nothing.
    */
    virtual void onFocus(bool focus, CrossingMode mode);

   /**
      A function called when the window is resized.
      If there is a top-level widget associated with this window, its size will be set right after this function.
      TODO this seems wrong, top-level widget should be resized here
    */
    virtual void onReshape(uint width, uint height);

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

    void addIdleCallback(IdleCallback* const callback);
    void removeIdleCallback(IdleCallback* const callback);

#ifndef DGL_FILE_BROWSER_DISABLED
    bool openFileBrowser(const FileBrowserOptions& options);
#endif

protected:
#ifndef DGL_FILE_BROWSER_DISABLED
    virtual void fileBrowserSelected(const char* filename);
#endif

    bool handlePluginKeyboard(const bool press, const uint key);
    bool handlePluginSpecial(const bool press, const Key key);
#endif

// -----------------------------------------------------------------------

#endif // DGL_WINDOW_HPP_INCLUDED
