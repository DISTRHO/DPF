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

#ifndef DISTRHO_EXTERNAL_WINDOW_HPP_INCLUDED
#define DISTRHO_EXTERNAL_WINDOW_HPP_INCLUDED

#include "String.hpp"

#ifdef DISTRHO_OS_WINDOWS
# error Unsupported platform!
#else
# include <cerrno>
# include <signal.h>
# include <sys/wait.h>
# include <unistd.h>
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// ExternalWindow class

/**
   External Window class.

   This is a standalone TopLevelWidget-compatible class, but without any real event handling.
   Being compatible with TopLevelWidget, it allows to be used as DPF UI target.

   It can be used to embed non-DPF things or to run a tool in a new process as the "UI".
 */
class ExternalWindow
{
    struct PrivateData;

public:
   /**
      Constructor.
    */
    explicit ExternalWindow()
       : pData() {}

   /**
      Constructor.
    */
    explicit ExternalWindow(const PrivateData& data)
       : pData(data) {}

   /**
      Destructor.
    */
    virtual ~ExternalWindow()
    {
        DISTRHO_SAFE_ASSERT(!pData.visible);
    }

   /* --------------------------------------------------------------------------------------------------------
    * ExternalWindow specific calls */

    virtual bool isRunning() const
    {
        if (ext.inUse)
            return ext.isRunning();

        return isVisible();
    }

    virtual bool isQuiting() const
    {
        if (ext.inUse)
            return ext.isQuiting;

        return !isVisible();
    }

   /**
      Hide the UI and gracefully terminate.
    */
    virtual void close()
    {
        hide();

        if (ext.inUse)
            terminateAndWaitForExternalProcess();
    }

   /**
      Grab the keyboard input focus.
    */
    virtual void focus() {}

   /**
      Get the transient window that we should attach ourselves to.
      TODO what id? also NSView* on macOS, or NSWindow?
    */
    uintptr_t getTransientWindowId() const noexcept
    {
        return pData.transientWinId;
    }

   /**
      Called by the host to set the transient window that we should attach ourselves to.
      TODO what id? also NSView* on macOS, or NSWindow?
    */
    void setTransientWindowId(uintptr_t winId)
    {
        if (pData.transientWinId == winId)
            return;
        pData.transientWinId = winId;
        transientWindowChanged(winId);
    }

#if DISTRHO_PLUGIN_HAS_EMBED_UI
   /**
      Whether this Window is embed into another (usually not DGL-controlled) Window.
    */
    bool isEmbed() const noexcept
    {
        return pData.parentWindowHandle != 0;
    }

   /**
      Get the "native" window handle.
      This can be reimplemented in order to pass the child window to hosts that can use such informaton.

      Returned value type depends on the platform:
       - HaikuOS: This is a pointer to a `BView`.
       - MacOS: This is a pointer to an `NSView*`.
       - Windows: This is a `HWND`.
       - Everything else: This is an [X11] `Window`.
    */
    virtual uintptr_t getNativeWindowHandle() const noexcept
    {
        return 0;
    }

   /**
      Get the "native" window handle that this window should embed itself into.
      Returned value type depends on the platform:
       - HaikuOS: This is a pointer to a `BView`.
       - MacOS: This is a pointer to an `NSView*`.
       - Windows: This is a `HWND`.
       - Everything else: This is an [X11] `Window`.
    */
    uintptr_t getParentWindowHandle() const noexcept
    {
        return pData.parentWindowHandle;
    }
#endif

   /* --------------------------------------------------------------------------------------------------------
    * TopLevelWidget-like calls */

   /**
      Check if this window is visible.
      @see setVisible(bool)
    */
    bool isVisible() const noexcept
    {
        return pData.visible;
    }

   /**
      Set window visible (or not) according to @a visible.
      @see isVisible(), hide(), show()
    */
    void setVisible(bool visible)
    {
        if (pData.visible == visible)
            return;
        pData.visible = visible;
        visibilityChanged(visible);
    }

   /**
      Show window.
      This is the same as calling setVisible(true).
      @see isVisible(), setVisible(bool)
    */
    void show()
    {
        setVisible(true);
    }

   /**
      Hide window.
      This is the same as calling setVisible(false).
      @see isVisible(), setVisible(bool)
    */
    void hide()
    {
        setVisible(false);
    }

   /**
      Get width.
    */
    uint getWidth() const noexcept
    {
        return pData.width;
    }

   /**
      Get height.
    */
    uint getHeight() const noexcept
    {
        return pData.height;
    }

   /**
      Set width.
    */
    void setWidth(uint width)
    {
        setSize(width, getHeight());
    }

   /**
      Set height.
    */
    void setHeight(uint height)
    {
        setSize(getWidth(), height);
    }

   /**
      Set size using @a width and @a height values.
    */
    void setSize(uint width, uint height)
    {
        DISTRHO_SAFE_ASSERT_UINT2_RETURN(width > 1 && height > 1, width, height,);

        if (pData.width == width || pData.height == height)
            return;

        pData.width = width;
        pData.height = height;
        onResize(width, height);
    }

   /**
      Get the title of the window previously set with setTitle().
    */
    const char* getTitle() const noexcept
    {
        return pData.title;
    }

   /**
      Set the title of the window, typically displayed in the title bar or in window switchers.
    */
    void setTitle(const char* title)
    {
        if (pData.title == title)
            return;
        pData.title = title;
        titleChanged(title);
    }

   /**
      Get the scale factor requested for this window.
      This is purely informational, and up to developers to choose what to do with it.
    */
    double getScaleFactor() const noexcept
    {
        return pData.scaleFactor;
    }

protected:
   /* --------------------------------------------------------------------------------------------------------
    * ExternalWindow special calls for running externals tools */

    bool startExternalProcess(const char* args[])
    {
        ext.inUse = true;

        return ext.start(args);
    }

    void terminateAndWaitForExternalProcess()
    {
        ext.isQuiting = true;
        ext.terminateAndWait();
    }

   /* --------------------------------------------------------------------------------------------------------
    * ExternalWindow specific callbacks */

   /**
      A function called when the window is resized.
    */
    virtual void onResize(uint width, uint height)
    {
        // unused, meant for custom implementations
        return;
        (void)width;
        (void)height;
    }

    virtual void titleChanged(const char* title)
    {
        // unused, meant for custom implementations
        return; (void)title;
    }

    virtual void visibilityChanged(bool visible)
    {
        // unused, meant for custom implementations
        return; (void)visible;
    }

    virtual void transientWindowChanged(uintptr_t winId)
    {
        // unused, meant for custom implementations
        return; (void)winId;
    }

private:
    friend class PluginWindow;
    friend class UI;

    struct ExternalProcess {
        bool inUse;
        bool isQuiting;
        mutable pid_t pid;

        ExternalProcess()
            : inUse(false),
              isQuiting(false),
              pid(0) {}

        bool isRunning() const noexcept
        {
            if (pid <= 0)
                return false;

            const pid_t p = ::waitpid(pid, nullptr, WNOHANG);

            if (p == pid || (p == -1 && errno == ECHILD))
            {
                d_stdout("NOTICE: Child process exited while idle");
                pid = 0;
                return false;
            }

            return true;
        }

        bool start(const char* args[])
        {
            terminateAndWait();

            pid = vfork();

            switch (pid)
            {
            case 0:
                execvp(args[0], (char**)args);
                _exit(1);
                return false;

            case -1:
                d_stderr("Could not start external ui");
                return false;

            default:
                return true;
            }
        }

        void terminateAndWait()
        {
            if (pid <= 0)
                return;

            d_stdout("Waiting for external process to stop,,,");

            bool sendTerm = true;

            for (pid_t p;;)
            {
                p = ::waitpid(pid, nullptr, WNOHANG);

                switch (p)
                {
                case 0:
                    if (sendTerm)
                    {
                        sendTerm = false;
                        ::kill(pid, SIGTERM);
                    }
                    break;

                case -1:
                    if (errno == ECHILD)
                    {
                        d_stdout("Done! (no such process)");
                        pid = 0;
                        return;
                    }
                    break;

                default:
                    if (p == pid)
                    {
                        d_stdout("Done! (clean wait)");
                        pid = 0;
                        return;
                    }
                    break;
                }

                // 5 msec
                usleep(5*1000);
            }
        }
    } ext;

    struct PrivateData {
        uintptr_t parentWindowHandle;
        uintptr_t transientWinId;
        uint width;
        uint height;
        double scaleFactor;
        String title;
        bool visible;

        PrivateData()
            : parentWindowHandle(0),
              transientWinId(0),
              width(1),
              height(1),
              scaleFactor(1.0),
              title(),
              visible(false) {}
    } pData;

    DISTRHO_DECLARE_NON_COPYABLE(ExternalWindow)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_EXTERNAL_WINDOW_HPP_INCLUDED
