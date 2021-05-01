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

// -----------------------------------------------------------------------

class Application;

class Window
{
public:
    explicit Window(Application& app);
    explicit Window(Application& app, uintptr_t parentWindowHandle, double scaling, bool resizable);
    virtual ~Window();

    void close();

   /**
      Get the "native" window handle.
      Returned value depends on the platform:
       - HaikuOS: This is a pointer to a `BView`.
       - MacOS: This is a pointer to an `NSView*`.
       - Windows: This is a `HWND`.
       - Everything else: This is an [X11] `Window`.
    */
    uintptr_t getNativeWindowHandle() const noexcept;

private:
    struct PrivateData;
    PrivateData* const pData;
    friend class Application;

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

    void show();
    void hide();
    void exec(bool lockWait = false);

    void focus();
    void repaint() noexcept;
    void repaint(const Rectangle<uint>& rect) noexcept;

#ifndef DGL_FILE_BROWSER_DISABLED
    bool openFileBrowser(const FileBrowserOptions& options);
#endif

    bool isEmbed() const noexcept;

    bool isVisible() const noexcept;
    void setVisible(bool visible);

    bool isResizable() const noexcept;
    void setResizable(bool resizable);

    bool getIgnoringKeyRepeat() const noexcept;
    void setIgnoringKeyRepeat(bool ignore) noexcept;

    uint getWidth() const noexcept;
    uint getHeight() const noexcept;
    Size<uint> getSize() const noexcept;
    void setSize(uint width, uint height);
    void setSize(Size<uint> size);

    const char* getTitle() const noexcept;
    void setTitle(const char* title);

    void setGeometryConstraints(uint width, uint height, bool aspect);
    void setTransientWinId(uintptr_t winId);

    double getScaling() const noexcept;

#if 0
    // should this be removed?
    Application& getApp() const noexcept;
#endif

    const GraphicsContext& getGraphicsContext() const noexcept;

    void addIdleCallback(IdleCallback* const callback);
    void removeIdleCallback(IdleCallback* const callback);


protected:
    virtual void onDisplayBefore();
    virtual void onDisplayAfter();
    virtual void onReshape(uint width, uint height);
    virtual void onClose();

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
