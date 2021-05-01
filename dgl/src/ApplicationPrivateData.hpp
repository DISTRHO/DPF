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

#ifndef DGL_APP_PRIVATE_DATA_HPP_INCLUDED
#define DGL_APP_PRIVATE_DATA_HPP_INCLUDED

#include "../Application.hpp"

#include <list>

typedef struct PuglWorldImpl PuglWorld;

START_NAMESPACE_DGL

class Window;

// --------------------------------------------------------------------------------------------------------------------

struct Application::PrivateData {
    /** Pugl world instance. */
    PuglWorld* const world;

    /** Whether the application is running as standalone, otherwise it is part of a plugin. */
    const bool isStandalone;

    /** Whether the applicating is about to quit, or already stopped. Defaults to false. */
    bool isQuitting;

    /** Counter of visible windows, only used in standalone mode.
        If 0->1, application is starting. If 1->0, application is quitting/stopping. */
    uint visibleWindows;

#ifndef DPF_TEST_APPLICATION_CPP
    /** List of windows for this application. Used as a way to call each window `idle`. */
    std::list<Window*> windows;
#endif

    /** List of idle callbacks for this application. Run after all windows `idle`. */
    std::list<IdleCallback*> idleCallbacks;

    /** Constructor and destructor */
    PrivateData(const bool standalone);
    ~PrivateData();

    /** Flag one window shown or hidden status, which modifies @a visibleWindows.
        For standalone mode only.
        Modifies @a isQuitting under certain conditions */
    void oneWindowShown() noexcept;
    void oneWindowHidden() noexcept;

    /** Run Pugl world update for @a timeoutInMs, and then the idle functions for each window and idle callback,
        in order of registration. */
    void idle(const uint timeoutInMs);

    /** Set flag indicating application is quitting, and closes all windows in reverse order of registration. */
    void quit();

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PrivateData)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_APP_PRIVATE_DATA_HPP_INCLUDED
