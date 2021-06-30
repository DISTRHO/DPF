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
#ifndef DGL_FILE_BROWSER_DISABLED
   /**
      File browser options.
      @see Window::openFileBrowser
    */
    struct FileBrowserOptions {
       /**
          File browser button state.
          This allows to customize the behaviour of the file browse dialog buttons.
        */
        enum ButtonState {
            kButtonInvisible,
            kButtonVisibleUnchecked,
            kButtonVisibleChecked,
        };

        /** Start directory, uses current working directory if null */
        const char* startDir;
        /** File browser dialog window title, uses "FileBrowser" if null */
        const char* title;
        /** File browser dialog window width */
        uint width;
        /** File browser dialog window height */
        uint height;
        // TODO file filter

       /**
          File browser buttons.
        */
        struct Buttons {
            /** Whether to list all files vs only those with matching file extension */
            ButtonState listAllFiles;
            /** Whether to show hidden files */
            ButtonState showHidden;
            /** Whether to show list of places (bookmarks) */
            ButtonState showPlaces;

            /** Constructor for default values */
            Buttons()
                : listAllFiles(kButtonVisibleChecked),
                  showHidden(kButtonVisibleUnchecked),
                  showPlaces(kButtonVisibleUnchecked) {}
        } buttons;

        /** Constructor for default values */
        FileBrowserOptions()
            : startDir(nullptr),
              title(nullptr),
              width(0),
              height(0),
              buttons() {}
    };
#endif // DGL_FILE_BROWSER_DISABLED

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

   /**
      Check if this window is resizable (by the user or window manager).
      @see setResizable
    */
    bool isResizable() const noexcept;

   /**
      Set window as resizable (by the user or window manager).
      It is always possible to resize a window programmatically, which is not the same as the user being allowed to it.
      @note This function does nothing for plugins, where the resizable state is set via macro.
      @see DISTRHO_UI_USER_RESIZABLE
    */
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
      Add a callback function to be triggered on every idle cycle or on a specific timer frequency.
      You can add more than one, and remove them at anytime with removeIdleCallback().
      This can be used to perform some action at a regular interval with relatively low frequency.

      If providing a timer frequency, there are a few things to note:
       1. There is a platform-specific limit to the number of supported timers, and overhead associated with each,
          so you should create only a few timers and perform several tasks in one if necessary.
       2. This timer frequency is not guaranteed to have a resolution better than 10ms
          (the maximum timer resolution on Windows) and may be rounded up if it is too short.
          On X11 and MacOS, a resolution of about 1ms can usually be relied on.
    */
    bool addIdleCallback(IdleCallback* callback, uint timerFrequencyInMs = 0);

   /**
      Remove an idle callback previously added via addIdleCallback().
    */
    bool removeIdleCallback(IdleCallback* callback);

   /**
      Get the application associated with this window.
    */
    Application& getApp() const noexcept;

   /**
      Get the graphics context associated with this window.
      GraphicsContext is an empty struct and needs to be casted into a different type in order to be usable,
      for example GraphicsContext.
      @see CairoSubWidget, CairoTopLevelWidget
    */
    const GraphicsContext& getGraphicsContext() const noexcept;

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

#ifndef DGL_FILE_BROWSER_DISABLED
   /**
      Open a file browser dialog with this window as parent.
      A few options can be specified to setup the dialog.

      If a path is selected, onFileSelected() will be called with the user chosen path.
      If the user cancels or does not pick a file, onFileSelected() will be called with nullptr as filename.

      This function does not block the event loop.
    */
    bool openFileBrowser(const FileBrowserOptions& options);
#endif

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

   /** DEPRECATED Use isIgnoringKeyRepeat(). */
    DISTRHO_DEPRECATED_BY("isIgnoringKeyRepeat()")
    inline bool getIgnoringKeyRepeat() const noexcept { return isIgnoringKeyRepeat(); }

   /** DEPRECATED Use getScaleFactor(). */
    DISTRHO_DEPRECATED_BY("getScaleFactor()")
    inline double getScaling() const noexcept { return getScaleFactor(); }

   /** DEPRECATED Use runAsModal(bool). */
    DISTRHO_DEPRECATED_BY("runAsModal(bool)")
    inline void exec(bool blockWait = false) { runAsModal(blockWait); }

    // TESTING, DO NOT USE
    void leaveContext();

protected:
   /**
      A function called when the window is attempted to be closed.
      Returning true closes the window, which is the default behaviour.
      Override this method and return false to prevent the window from being closed by the user.

      This method is not used for embed windows, and not even made available in DISTRHO_NAMESPACE::UI.
      For embed windows, closing is handled by the host/parent process and we have no control over it.
      As such, a close action on embed windows will always succeed and cannot be cancelled.
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
      The default implementation sets up drawing context where necessary.
    */
    virtual void onReshape(uint width, uint height);

   /**
      A function called when scale factor requested for this window changes.
      The default implementation does nothing.
      WARNING function needs a proper name
    */
    virtual void onScaleFactorChanged(double scaleFactor);

#ifndef DGL_FILE_BROWSER_DISABLED
   /**
      A function called when a path is selected by the user, as triggered by openFileBrowser().
      This action happens after the user confirms the action, so the file browser dialog will be closed at this point.
      The default implementation does nothing.
    */
    virtual void onFileSelected(const char* filename);

   /** DEPRECATED Use onFileSelected(). */
    DISTRHO_DEPRECATED_BY("onFileSelected(const char*)")
    inline virtual void fileBrowserSelected(const char* filename) { return onFileSelected(filename); }
#endif

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
 * add eventcrossing/enter-leave event
 */
#if 0
protected:
    bool handlePluginKeyboard(const bool press, const uint key);
    bool handlePluginSpecial(const bool press, const Key key);
#endif

// -----------------------------------------------------------------------

#endif // DGL_WINDOW_HPP_INCLUDED
