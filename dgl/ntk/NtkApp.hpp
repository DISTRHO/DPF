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

#ifndef DGL_NTK_APP_HPP_INCLUDED
#define DGL_NTK_APP_HPP_INCLUDED

#include "../Base.hpp"
#include "../../distrho/DistrhoUI.hpp"
#include "../../distrho/extra/d_thread.hpp"

#ifdef override
# define override_defined
# undef override
#endif

#include <list>
#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/x.H>

#ifdef override_defined
# define override
# undef override_defined
#endif

struct ScopedDisplayLock {
    ScopedDisplayLock()
    {
#ifdef DISTRHO_OS_LINUX
        XLockDisplay(fl_display);
#endif
    }

    ~ScopedDisplayLock()
    {
#ifdef DISTRHO_OS_LINUX
        XUnlockDisplay(fl_display);
#endif
    }
};

// -----------------------------------------------------------------------

namespace DISTRHO_NAMESPACE {
  class UI;
}

START_NAMESPACE_DGL

class NtkWindow;

typedef DISTRHO_NAMESPACE::Mutex       d_Mutex;
typedef DISTRHO_NAMESPACE::MutexLocker d_MutexLocker;
typedef DISTRHO_NAMESPACE::Thread      d_Thread;
typedef DISTRHO_NAMESPACE::UI          d_UI;

// -----------------------------------------------------------------------

/**
   DGL compatible App class that uses NTK instead of OpenGL.
   @see App
 */
class NtkApp : d_Thread
{
public:
   /**
      Constructor.
    */
    NtkApp()
        : d_Thread("NtkApp"),
          fWindows(),
          fWindowMutex(),
          fNextUI(),
          fDoNextUI(false),
          fInitialized(false)
    {
#ifdef DISTRHO_OS_LINUX
        //XInitThreads();
#endif

        startThread();

        for (; ! fInitialized;)
            d_msleep(10);
    }

   /**
      Destructor.
    */
    ~NtkApp()
    {
        stopThread(-1);
        fWindows.clear();
    }

   /**
      Idle function.
      This calls does nothing.
    */
    void idle() {}

   /**
      Run the application event-loop until all Windows are closed.
      @note: This function is meant for standalones only, *never* call this from plugins.
    */
    void exec()
    {
        while (isThreadRunning() && ! shouldThreadExit())
            d_sleep(1);
    }

   /**
      Quit the application.
      This stops the event-loop and closes all Windows.
    */
    void quit()
    {
        signalThreadShouldExit();
    }

   /**
      Check if the application is about to quit.
      Returning true means there's no event-loop running at the moment.
    */
    bool isQuiting() const noexcept
    {
        if (isThreadRunning() && ! shouldThreadExit())
            return false;
        return true;
    }

    // -------------------------------------------------------------------

   /**
      Create UI on our separate thread.
      Blocks until the UI is created and returns it.
    */
    d_UI* createUI(void* const func)
    {
        DISTRHO_SAFE_ASSERT_RETURN(isThreadRunning(), nullptr);
        DISTRHO_SAFE_ASSERT_RETURN(! fDoNextUI, nullptr);

        fNextUI.create = true;
        fNextUI.func   = (NextUI::UiFunc)func;
        fDoNextUI      = true;

        for (; fDoNextUI;)
            d_msleep(10);

        return fNextUI.ui;
    }

   /**
      Delete UI on our separate thread.
      Blocks until the UI is deleted.
    */
    void deleteUI(d_UI* const ui)
    {
        DISTRHO_SAFE_ASSERT_RETURN(! fDoNextUI,);

        fNextUI.create = false;
        fNextUI.ui     = ui;
        fDoNextUI      = true;

        if (isThreadRunning())
        {
            for (; fDoNextUI;)
                d_msleep(10);
        }
        else
        {
            fNextUI.run();
            fDoNextUI = false;
        }
    }

    // -------------------------------------------------------------------

private:
    struct NextUI {
        typedef d_UI* (*UiFunc)();

        bool create;

        union {
            UiFunc func;
            d_UI*  ui;
        };

        NextUI()
            : create(false),
              func(nullptr) {}

        void run();
    };

    std::list<Fl_Double_Window*> fWindows;
    d_Mutex       fWindowMutex;
    NextUI        fNextUI;
    volatile bool fDoNextUI;
    volatile bool fInitialized;

   /** @internal used by NtkWindow. */
    void addWindow(Fl_Double_Window* const window)
    {
        DISTRHO_SAFE_ASSERT_RETURN(window != nullptr,);

        if (fWindows.size() == 0 && ! isThreadRunning())
            startThread();

        const d_MutexLocker sl(fWindowMutex);
        fWindows.push_back(window);
    }

   /** @internal used by NtkWindow. */
    void removeWindow(Fl_Double_Window* const window)
    {
        DISTRHO_SAFE_ASSERT_RETURN(window != nullptr,);

        const d_MutexLocker sl(fWindowMutex);
        fWindows.remove(window);

        if (fWindows.size() == 0)
            signalThreadShouldExit();
    }

   /** @internal */
    void run() override
    {
        static bool initialized = false;

        if (! initialized)
        {
            initialized = true;
            fl_register_images();
#ifdef DISTRHO_OS_LINUX
            fl_open_display();
#endif
        }

        fInitialized = true;

        for (; ! shouldThreadExit();)
        {
            if (fDoNextUI)
            {
                const ScopedDisplayLock csdl;
                fNextUI.run();
                fDoNextUI = false;
            }

            const ScopedDisplayLock csdl;
            Fl::check();
            Fl::flush();

            d_msleep(20);
        }

        const d_MutexLocker sl(fWindowMutex);
        const ScopedDisplayLock csdl;

        for (std::list<Fl_Double_Window*>::reverse_iterator rit = fWindows.rbegin(), rite = fWindows.rend(); rit != rite; ++rit)
        {
            Fl_Double_Window* const window(*rit);
            window->hide();
        }
    }

    friend class NtkWindow;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NtkApp)
};
// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_NTK_APP_HPP_INCLUDED
