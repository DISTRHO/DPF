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
    struct PrivateData {
        uintptr_t parentWindowHandle;
        uintptr_t transientWinId;
        uint width;
        uint height;
        double scaleFactor;
        String title;
        bool visible;
        pid_t pid;

        PrivateData()
            : parentWindowHandle(0),
              transientWinId(0),
              width(1),
              height(1),
              scaleFactor(1.0),
              title(),
              visible(false),
              pid(0) {}
    } pData;

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
        /*
        terminateAndWaitForProcess();
        */
    }

   /* --------------------------------------------------------------------------------------------------------
    * ExternalWindow specific calls */

    virtual bool isRunning() const
    {
        return isVisible();
    }

    virtual bool isQuiting() const
    {
        return !isVisible();
    }

    uintptr_t getTransientWindowId() const noexcept
    {
        return pData.transientWinId;
    }

    virtual void setTransientWindowId(uintptr_t winId)
    {
        if (pData.transientWinId == winId)
            return;
        pData.transientWinId = winId;
        transientWindowChanged(winId);
    }

#if DISTRHO_PLUGIN_HAS_EMBED_UI
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
    virtual void setVisible(bool visible)
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
    virtual void setSize(uint width, uint height)
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
    virtual void setTitle(const char* title)
    {
        if (pData.title == title)
            return;
        pData.title = title;
        titleChanged(title);
    }

   /**
      Get the scale factor requested for this window.
      This is purely informational, and up to developers to choose what to do with it.

      If you do not want to deal with this yourself,
      consider using setGeometryConstraints() where you can specify to automatically scale the window contents.
      @see setGeometryConstraints
    */
    double getScaleFactor() const noexcept
    {
        return pData.scaleFactor;
    }

   /**
      Grab the keyboard input focus.
    */
    virtual void focus() {}

protected:
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

    /*
    bool isRunning() noexcept
    {
        if (pid <= 0)
            return false;

        const pid_t p = ::waitpid(pid, nullptr, WNOHANG);

        if (p == pid || (p == -1 && errno == ECHILD))
        {
            printf("NOTICE: Child process exited while idle\n");
            pid = 0;
            return false;
        }

        return true;
    }

    */

protected:
    /*
    bool startExternalProcess(const char* args[])
    {
        terminateAndWaitForProcess();

        pid = vfork();

        switch (pid)
        {
        case 0:
            execvp(args[0], (char**)args);
            _exit(1);
            return false;

        case -1:
            printf("Could not start external ui\n");
            return false;

        default:
            return true;
        }
    }

    void terminateAndWaitForProcess()
    {
        if (pid <= 0)
            return;

        printf("Waiting for previous process to stop,,,\n");

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
                    printf("Done! (no such process)\n");
                    pid = 0;
                    return;
                }
                break;

            default:
                if (p == pid)
                {
                    printf("Done! (clean wait)\n");
                    pid = 0;
                    return;
                }
                break;
            }

            // 5 msec
            usleep(5*1000);
        }
    }
    */

private:
    friend class PluginWindow;
    friend class UI;

    DISTRHO_DECLARE_NON_COPYABLE(ExternalWindow)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_EXTERNAL_WINDOW_HPP_INCLUDED
