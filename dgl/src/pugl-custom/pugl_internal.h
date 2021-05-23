/*
  Copyright 2012-2014 David Robillard <http://drobilla.net>
  Copyright 2012-2019 Filipe Coelho <falktx@falktx.com>

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
   @file pugl_internal.h Private platform-independent definitions.

   Note this file contains function definitions, so it must be compiled into
   the final binary exactly once.  Each platform specific implementation file
   including it once should achieve this.
*/

#include "pugl.h"

typedef struct PuglInternalsImpl PuglInternals;

struct PuglViewImpl {
	PuglHandle       handle;
	PuglCloseFunc    closeFunc;
	PuglDisplayFunc  displayFunc;
	PuglKeyboardFunc keyboardFunc;
	PuglMotionFunc   motionFunc;
	PuglMouseFunc    mouseFunc;
	PuglReshapeFunc  reshapeFunc;
	PuglResizeFunc   resizeFunc;
	PuglScrollFunc   scrollFunc;
	PuglSpecialFunc  specialFunc;
	PuglFileSelectedFunc fileSelectedFunc;

	PuglInternals*   impl;
	PuglNativeWindow parent;
	uintptr_t        transient_parent;

	int      width;
	int      height;
	int      min_width;
	int      min_height;
	int      mods;
	bool     mouse_in_view;
	bool     ignoreKeyRepeat;
	bool     redisplay;
	bool     user_resizable;
	bool     pending_resize;
	uint32_t event_timestamp_ms;
};

PuglInternals* puglInitInternals(void);

PuglView*
puglInit(void)
{
	PuglView* view = (PuglView*)calloc(1, sizeof(PuglView));
	if (!view) {
		return NULL;
	}

	PuglInternals* impl = puglInitInternals();
	if (!impl) {
		free(view);
		return NULL;
	}

	view->impl   = impl;
	view->width  = 640;
	view->height = 480;

	return view;
}

void
puglInitWindowSize(PuglView* view, int width, int height)
{
	view->width  = width;
	view->height = height;
}

void
puglInitWindowMinSize(PuglView* view, int width, int height)
{
	view->min_width  = width;
	view->min_height = height;
}

void
puglInitWindowParent(PuglView* view, PuglNativeWindow parent)
{
	view->parent = parent;
}

void
puglInitUserResizable(PuglView* view, bool resizable)
{
	view->user_resizable = resizable;
}

void
puglInitTransientFor(PuglView* view, uintptr_t parent)
{
	view->transient_parent = parent;
}

PuglView*
puglCreate(PuglNativeWindow parent,
           const char*      title,
           int              min_width,
           int              min_height,
           int              width,
           int              height,
           bool             resizable,
           unsigned long    transientId)
{
	PuglView* view = puglInit();
	if (!view) {
		return NULL;
	}

	puglInitWindowParent(view, parent);
	puglInitWindowMinSize(view, min_width, min_height);
	puglInitWindowSize(view, width, height);
	puglInitUserResizable(view, resizable);
	puglInitTransientFor(view, transientId);

	if (!puglCreateWindow(view, title)) {
		free(view);
		return NULL;
	}

	return view;
}

void
puglSetHandle(PuglView* view, PuglHandle handle)
{
	view->handle = handle;
}

PuglHandle
puglGetHandle(PuglView* view)
{
	return view->handle;
}

uint32_t
puglGetEventTimestamp(PuglView* view)
{
	return view->event_timestamp_ms;
}

int
puglGetModifiers(PuglView* view)
{
	return view->mods;
}

void
puglIgnoreKeyRepeat(PuglView* view, bool ignore)
{
	view->ignoreKeyRepeat = ignore;
}

void
puglSetCloseFunc(PuglView* view, PuglCloseFunc closeFunc)
{
	view->closeFunc = closeFunc;
}

void
puglSetDisplayFunc(PuglView* view, PuglDisplayFunc displayFunc)
{
	view->displayFunc = displayFunc;
}

void
puglSetKeyboardFunc(PuglView* view, PuglKeyboardFunc keyboardFunc)
{
	view->keyboardFunc = keyboardFunc;
}

void
puglSetMotionFunc(PuglView* view, PuglMotionFunc motionFunc)
{
	view->motionFunc = motionFunc;
}

void
puglSetMouseFunc(PuglView* view, PuglMouseFunc mouseFunc)
{
	view->mouseFunc = mouseFunc;
}

void
puglSetReshapeFunc(PuglView* view, PuglReshapeFunc reshapeFunc)
{
	view->reshapeFunc = reshapeFunc;
}

void
puglSetResizeFunc(PuglView* view, PuglResizeFunc resizeFunc)
{
	view->resizeFunc = resizeFunc;
}

void
puglSetScrollFunc(PuglView* view, PuglScrollFunc scrollFunc)
{
	view->scrollFunc = scrollFunc;
}

void
puglSetSpecialFunc(PuglView* view, PuglSpecialFunc specialFunc)
{
	view->specialFunc = specialFunc;
}

void
puglSetFileSelectedFunc(PuglView* view, PuglFileSelectedFunc fileSelectedFunc)
{
	view->fileSelectedFunc = fileSelectedFunc;
}

void
puglEnterContext(PuglView* view);

void
puglLeaveContext(PuglView* view, bool flush);

static void
puglDefaultReshape(int width, int height)
{
#ifdef PUGL_OPENGL
#ifdef ROBTK_HERE
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
#else
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, height, 0, 0, 1);
	glViewport(0, 0, width, height);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
#endif
#endif // PUGL_OPENGL
}
