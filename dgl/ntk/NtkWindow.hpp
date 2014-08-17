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

#ifndef DGL_NTK_WINDOW_HPP_INCLUDED
#define DGL_NTK_WINDOW_HPP_INCLUDED

#include "NtkApp.hpp"

START_NAMESPACE_DGL

class NtkWidget;

// -----------------------------------------------------------------------

class NtkWindow : public Fl_Double_Window
{
public:
    explicit NtkWindow(NtkApp& app)
        : Fl_Double_Window(100, 100),
          fApp(app),
          fUsingEmbed(false),
          fParent(nullptr),
          fWidgets() {}

    explicit NtkWindow(NtkApp& app, NtkWindow& parent)
        : Fl_Double_Window(100, 100),
          fApp(app),
          fUsingEmbed(false),
          fParent(&parent),
          fWidgets() {}

    explicit NtkWindow(NtkApp& app, intptr_t parentId)
        : Fl_Double_Window(100, 100),
          fApp(app),
          fUsingEmbed(parentId != 0),
          fParent(nullptr),
          fWidgets()
    {
        if (fUsingEmbed)
        {
            fl_embed(this, (Window)parentId);
            Fl_Double_Window::show();
        }
    }

    ~NtkWindow() override
    {
        fWidgets.clear();

        if (fUsingEmbed)
            Fl_Double_Window::hide();
    }

    void show()
    {
        Fl_Double_Window::show();

#ifdef DISTRHO_OS_LINUX
        if (fParent == nullptr)
            return;

        DISTRHO_SAFE_ASSERT_RETURN(fl_display != nullptr,);

        const ::Window ourWindow(fl_xid_(this));
        DISTRHO_SAFE_ASSERT_RETURN(ourWindow != 0,);

        const ::Window parentWindow(fl_xid_(fParent));
        DISTRHO_SAFE_ASSERT_RETURN(parentWindow != 0,);

        XSetTransientForHint(fl_display, ourWindow, parentWindow);
#endif
    }

    void addIdleCallback(IdleCallback* const callback)
    {
        DISTRHO_SAFE_ASSERT_RETURN(callback != nullptr,);

        if (fIdleCallbacks.size() == 0)
            Fl::add_idle(_idleHandler, this);

        fIdleCallbacks.push_back(callback);
    }

    void removeIdleCallback(IdleCallback* const callback)
    {
        DISTRHO_SAFE_ASSERT_RETURN(callback != nullptr,);

        fIdleCallbacks.remove(callback);

        if (fIdleCallbacks.size() == 0)
            Fl::remove_idle(_idleHandler, this);
    }

private:
    NtkApp& fApp;
    bool    fUsingEmbed;

    // transient parent, may be null
    NtkWindow* const fParent;

    std::list<Fl_Group*> fWidgets;
    std::list<IdleCallback*> fIdleCallbacks;

    friend class NtkWidget;

    static void _idleHandler(void* data)
    {
        NtkWindow* const self((NtkWindow*)data);

        for (std::list<IdleCallback*>::iterator it=self->fIdleCallbacks.begin(), ite=self->fIdleCallbacks.end(); it != ite; ++it)
        {
            IdleCallback* const idleCallback(*it);
            idleCallback->idleCallback();
        }
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NtkWindow)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_NTK_WINDOW_HPP_INCLUDED
