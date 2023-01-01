// Copyright 2012-2022 David Robillard <d@drobilla.net>
// Copyright 2021-2022 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#include "../pugl-upstream/src/stub.h"
#include "haiku.h"

#include "pugl/pugl.h"

#include <stdio.h>
#include <stdlib.h>

#include <GL/gl.h>
#include <opengl/GLView.h>

typedef struct {
  BGLView* view;
} PuglHaikuGlSurface;

static PuglStatus
puglHaikuGlConfigure(PuglView* view)
{
  PuglInternals* const impl = view->impl;

  PuglHaikuGlSurface* const surface =
    (PuglHaikuGlSurface*)calloc(1, sizeof(PuglHaikuGlSurface));
  impl->surface = surface;

  return PUGL_SUCCESS;
}

PUGL_WARN_UNUSED_RESULT
static PuglStatus
puglHaikuGlEnter(PuglView* const view, const PuglExposeEvent* PUGL_UNUSED(expose))
{
  PuglHaikuGlSurface* const surface = (PuglHaikuGlSurface*)view->impl->surface;
  if (!surface || !surface->view) {
    return PUGL_FAILURE;
  }

  surface->view->LockGL();
  return PUGL_SUCCESS;
}

PUGL_WARN_UNUSED_RESULT
static PuglStatus
puglHaikuGlLeave(PuglView* const view, const PuglExposeEvent* const expose)
{
  PuglHaikuGlSurface* const surface = (PuglHaikuGlSurface*)view->impl->surface;
  if (!surface || !surface->view) {
    return PUGL_FAILURE;
  }

  if (expose)
      surface->view->SwapBuffers();

  surface->view->UnlockGL();
  return PUGL_SUCCESS;
}

static PuglStatus
puglHaikuGlCreate(PuglView* view)
{
  return PUGL_SUCCESS;
}

static void
puglHaikuGlDestroy(PuglView* view)
{
  PuglHaikuGlSurface* surface = (PuglHaikuGlSurface*)view->impl->surface;
  if (surface) {
    free(surface);
    view->impl->surface = NULL;
  }
}

const PuglBackend*
puglGlBackend(void)
{
  static const PuglBackend backend = {puglHaikuGlConfigure,
                                      puglHaikuGlCreate,
                                      puglHaikuGlDestroy,
                                      puglHaikuGlEnter,
                                      puglHaikuGlLeave,
                                      puglStubGetContext};
  return &backend;
}
