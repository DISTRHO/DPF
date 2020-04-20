/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2020 Filipe Coelho <falktx@falktx.com>
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
#include "../Window.hpp"

#include "pugl-upstream/pugl/pugl.h"

#include <list>

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

struct Application::PrivateData {
    bool isQuitting;
    bool isStandalone;
    uint visibleWindows;
    std::list<Window*> windows;
    std::list<IdleCallback*> idleCallbacks;
    PuglWorld* const world;

    PrivateData(const bool standalone)
        : isQuitting(false),
          isStandalone(standalone),
          visibleWindows(0),
          windows(),
          idleCallbacks(),
          world(puglNewWorld(isStandalone ? PUGL_PROGRAM : PUGL_MODULE,
                             isStandalone ? PUGL_WORLD_THREADS : 0x0))
    {
        puglSetWorldHandle(world, this);

        puglSetLogLevel(world, PUGL_LOG_LEVEL_DEBUG);

        // TODO puglSetClassName
    }

    ~PrivateData()
    {
        DISTRHO_SAFE_ASSERT(isQuitting);
        DISTRHO_SAFE_ASSERT(visibleWindows == 0);

        windows.clear();
        idleCallbacks.clear();

        d_stdout("calling puglFreeWorld");
        puglFreeWorld(world);
    }

    void oneWindowShown() noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(isStandalone,);

        if (++visibleWindows == 1)
            isQuitting = false;
    }

    void oneWindowHidden() noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(isStandalone,);
        DISTRHO_SAFE_ASSERT_RETURN(visibleWindows > 0,);

        if (--visibleWindows == 0)
            isQuitting = true;
    }

    void idle(const uint timeout)
    {
        puglUpdate(world, timeout == 0 ? 0.0 : (1.0 / static_cast<double>(timeout)));

        for (std::list<Window*>::iterator it = windows.begin(), ite = windows.end(); it != ite; ++it)
        {
            Window* const window(*it);
            window->_idle();
        }

        for (std::list<IdleCallback*>::iterator it = idleCallbacks.begin(), ite = idleCallbacks.end(); it != ite; ++it)
        {
            IdleCallback* const idleCallback(*it);
            idleCallback->idleCallback();
        }
    }

    void quit()
    {
        isQuitting = true;

        for (std::list<Window*>::reverse_iterator rit = windows.rbegin(), rite = windows.rend(); rit != rite; ++rit)
        {
            Window* const window(*rit);
            window->close();
        }
    }

    DISTRHO_DECLARE_NON_COPY_STRUCT(PrivateData)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_APP_PRIVATE_DATA_HPP_INCLUDED
