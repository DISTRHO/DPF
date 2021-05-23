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

#include "ApplicationPrivateData.hpp"
#include "../Window.hpp"

#include "pugl.hpp"

#include <ctime>

START_NAMESPACE_DGL

typedef std::list<DGL_NAMESPACE::Window*>::reverse_iterator WindowListReverseIterator;

// --------------------------------------------------------------------------------------------------------------------

Application::PrivateData::PrivateData(const bool standalone)
    : world(puglNewWorld(standalone ? PUGL_PROGRAM : PUGL_MODULE,
                         standalone ? PUGL_WORLD_THREADS : 0x0)),
      isStandalone(standalone),
      isQuitting(false),
      isStarting(true),
      visibleWindows(0),
      windows(),
      idleCallbacks()
{
    DISTRHO_SAFE_ASSERT_RETURN(world != nullptr,);

    puglSetWorldHandle(world, this);

    // FIXME
    static int wc_count = 0;
    char classNameBuf[256];
    std::srand((std::time(NULL)));
    std::snprintf(classNameBuf, sizeof(classNameBuf), "%s_%d-%d-%p",
                  DISTRHO_MACRO_AS_STRING(DGL_NAMESPACE), std::rand(), ++wc_count, this);
    classNameBuf[sizeof(classNameBuf)-1] = '\0';
    d_stderr("--------------------------------------------------------------- className is %s", classNameBuf);

    puglSetClassName(world, classNameBuf);
#ifdef HAVE_X11
    sofdFileDialogSetup(world);
#endif
}

Application::PrivateData::~PrivateData()
{
    DISTRHO_SAFE_ASSERT(isStarting || isQuitting);
    DISTRHO_SAFE_ASSERT(visibleWindows == 0);

    windows.clear();
    idleCallbacks.clear();

    if (world != nullptr)
        puglFreeWorld(world);
}

// --------------------------------------------------------------------------------------------------------------------

void Application::PrivateData::oneWindowShown() noexcept
{
    if (++visibleWindows == 1)
    {
        isQuitting = false;
        isStarting = false;
    }
}

void Application::PrivateData::oneWindowClosed() noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(visibleWindows != 0,);

    if (--visibleWindows == 0)
        isQuitting = true;
}

// --------------------------------------------------------------------------------------------------------------------

void Application::PrivateData::idle(const uint timeoutInMs)
{
    if (world != nullptr)
    {
        const double timeoutInSeconds = timeoutInMs != 0
                                      ? static_cast<double>(timeoutInMs) / 1000.0
                                      : 0.0;

        puglUpdate(world, timeoutInSeconds);
    }

    for (std::list<IdleCallback*>::iterator it = idleCallbacks.begin(), ite = idleCallbacks.end(); it != ite; ++it)
    {
        IdleCallback* const idleCallback(*it);
        idleCallback->idleCallback();
    }
}

void Application::PrivateData::quit()
{
    DISTRHO_SAFE_ASSERT_RETURN(isStandalone,);

    isQuitting = true;

#ifndef DPF_TEST_APPLICATION_CPP
    for (WindowListReverseIterator rit = windows.rbegin(), rite = windows.rend(); rit != rite; ++rit)
    {
        DGL_NAMESPACE::Window* const window(*rit);
        window->close();
    }
#endif
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
