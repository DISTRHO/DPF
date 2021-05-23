/*
  Copyright (C) 2012-2020 Filipe Coelho <falktx@falktx.com>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/**
   @file x11.c X11 extra implementation for DPF.
*/

// NOTE can't import this file twice!
#ifndef PUGL_DETAIL_X11_H_INCLUDED
#include "../pugl-upstream/src/x11.h"
#endif

#include "../pugl-upstream/src/implementation.h"

#include <sys/types.h>
#include <unistd.h>

void
puglRaiseWindow(PuglView* view)
{
    XRaiseWindow(view->impl->display, view->impl->win);
}

void
puglSetWindowSize(PuglView* view, unsigned int width, unsigned int height)
{
    view->frame.width = width;
    view->frame.height = height;

    if (view->impl->win) {
#if 0
        if (! fResizable)
        {
            XSizeHints sizeHints;
            memset(&sizeHints, 0, sizeof(sizeHints));

            sizeHints.flags      = PSize|PMinSize|PMaxSize;
            sizeHints.width      = static_cast<int>(width);
            sizeHints.height     = static_cast<int>(height);
            sizeHints.min_width  = static_cast<int>(width);
            sizeHints.min_height = static_cast<int>(height);
            sizeHints.max_width  = static_cast<int>(width);
            sizeHints.max_height = static_cast<int>(height);

            XSetWMNormalHints(xDisplay, xWindow, &sizeHints);
        }
#endif

        XResizeWindow(view->world->impl->display, view->impl->win, width, height);
    }
}

void
puglUpdateGeometryConstraints(PuglView* view, unsigned int width, unsigned int height, bool aspect)
{
    // NOTE this is a combination of puglSetMinSize and puglSetAspectRatio
    Display* display = view->world->impl->display;

    view->minWidth = width;
    view->minHeight = height;

    if (aspect) {
        view->minAspectX = width;
        view->minAspectY = height;
        view->maxAspectX = width;
        view->maxAspectY = height;
    }

#if 0
    if (view->impl->win) {
        XSizeHints sizeHints = getSizeHints(view);
        XSetNormalHints(display, view->impl->win, &sizeHints);
        // NOTE old code used this instead
        // XSetWMNormalHints(display, view->impl->win, &sizeHints);
    }
#endif
}

void
puglExtraSetWindowTypeAndPID(PuglView* view)
{
    PuglInternals* const impl = view->impl;

    const pid_t pid = getpid();
    const Atom _nwp = XInternAtom(impl->display, "_NET_WM_PID", False);
    XChangeProperty(impl->display, impl->win, _nwp, XA_CARDINAL, 32, PropModeReplace, (const uchar*)&pid, 1);

    const Atom _wt = XInternAtom(impl->display, "_NET_WM_WINDOW_TYPE", False);

    // Setting the window to both dialog and normal will produce a decorated floating dialog
    // Order is important: DIALOG needs to come before NORMAL
    const Atom _wts[2] = {
        XInternAtom(impl->display, "_NET_WM_WINDOW_TYPE_DIALOG", False),
        XInternAtom(impl->display, "_NET_WM_WINDOW_TYPE_NORMAL", False)
    };

    XChangeProperty(impl->display, impl->win, _wt, XA_ATOM, 32, PropModeReplace, (const uchar*)&_wts, 2);
}
