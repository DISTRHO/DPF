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
   @file win.c Windows extra implementation for DPF.
*/

#include "pugl/detail/win.h"

#include "pugl/detail/implementation.h"

void
puglRaiseWindow(PuglView* view)
{
    SetForegroundWindow(view->impl->hwnd);
    SetActiveWindow(view->impl->hwnd);
    return PUGL_SUCCESS;
}

void
puglSetWindowSize(PuglView* view, unsigned int width, unsigned int height)
{
    view->frame.width = width;
    view->frame.height = height;

    // NOTE the following code matches upstream pugl, except we add SWP_NOMOVE flag
    if (view->impl->hwnd) {
        RECT rect = { (long)frame.x,
                      (long)frame.y,
                      (long)frame.x + (long)frame.width,
                      (long)frame.y + (long)frame.height };

        AdjustWindowRectEx(&rect, puglWinGetWindowFlags(view),
                           FALSE,
                           puglWinGetWindowExFlags(view));

        SetWindowPos(view->impl->hwnd,
                     HWND_TOP,
                     rect.left,
                     rect.top,
                     rect.right - rect.left,
                     rect.bottom - rect.top,
                     SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
    }
}

void
puglUpdateGeometryConstraints(PuglView* view, unsigned int width, unsigned int height, bool aspect)
{
    // NOTE this is a combination of puglSetMinSize and puglSetAspectRatio, but stilL TODO on pugl
    Display* display = view->world->impl->display;

    view->minWidth = width;
    view->minHeight = height;

    if (aspect) {
        view->minAspectX = width;
        view->minAspectY = height;
        view->maxAspectX = width;
        view->maxAspectY = height;
    }
}

void
puglWin32RestoreWindow(PuglView* view)
{
    PuglInternals* impl = view->impl;

    ShowWindow(impl->hwnd, SW_RESTORE);
    SetFocus(impl->hwnd);
}

void
puglWin32ShowWindowCentered(PuglView* view)
{
    PuglInternals* impl = view->impl;

    RECT rectChild, rectParent;

    if (impl->transientParent != 0 &&
        GetWindowRect(impl->hwnd, &rectChild) &&
        GetWindowRect(impl->transientParent, &rectParent))
    {
        SetWindowPos(impl->hwnd, (HWND)impl->transientParent,
                     rectParent.left + (rectChild.right-rectChild.left)/2,
                     rectParent.top + (rectChild.bottom-rectChild.top)/2,
                     0, 0, SWP_SHOWWINDOW|SWP_NOSIZE);
    }
    else
    {
        ShowWindow(hwnd, SW_SHOWNORMAL);
    }

    SetFocus(impl->hwnd);
}

void
puglWin32SetWindowResizable(PuglView* view, bool resizable)
{
    PuglInternals* impl = view->impl;

    const int winFlags = resizable ? GetWindowLong(hwnd, GWL_STYLE) |  WS_SIZEBOX
                                   : GetWindowLong(hwnd, GWL_STYLE) & ~WS_SIZEBOX;
    SetWindowLong(impl->hwnd, GWL_STYLE, winFlags);
}
