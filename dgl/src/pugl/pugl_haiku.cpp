/*
  Copyright 2019 Filipe Coelho <falktx@falktx.com>

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
   @file pugl_haiku.cpp BeOS/HaikuOS Pugl Implementation.
*/

#include <interface/Window.h>

#ifdef PUGL_CAIRO
#include <cairo/cairo.h>
#endif
#ifdef PUGL_OPENGL
#include <GL/gl.h>
#endif

#include "pugl_internal.h"

struct PuglInternalsImpl {
    BWindow* window;
};

PuglInternals*
puglInitInternals()
{
	return (PuglInternals*)calloc(1, sizeof(PuglInternals));
}

void
puglEnterContext(PuglView* view)
{
}

void
puglLeaveContext(PuglView* view, bool flush)
{
}

class DWindow : public BWindow
{
public:
    DWindow(PuglInternals* const i)
        : BWindow(BRect(), "", B_TITLED_WINDOW, 0x0),
          impl(i)
    {
    }

private:
    PuglInternals* const impl;
};

int
puglCreateWindow(PuglView* view, const char* title)
{
	PuglInternals* impl = view->impl;
    
    impl->window = new DWindow(impl);
    return 0;
}

void
puglShowWindow(PuglView* view)
{
}

void
puglHideWindow(PuglView* view)
{
}

void
puglDestroy(PuglView* view)
{
}

PuglStatus
puglProcessEvents(PuglView* view)
{
	return PUGL_SUCCESS;
}

void
puglPostRedisplay(PuglView* view)
{
	view->redisplay = true;
}

PuglNativeWindow
puglGetNativeWindow(PuglView* view)
{
	return NULL;
}

void*
puglGetContext(PuglView* view)
{
	return NULL;
}

int
puglUpdateGeometryConstraints(PuglView* view, int min_width, int min_height, bool aspect)
{
	// TODO
	return 1;

	(void)view;
	(void)min_width;
	(void)min_height;
	(void)aspect;
}
