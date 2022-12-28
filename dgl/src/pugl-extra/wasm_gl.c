// Copyright 2012-2022 David Robillard <d@drobilla.net>
// Copyright 2021-2022 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#include "stub.h"
#include "wasm.h"

#include "pugl/pugl.h"

#include <stdio.h>
#include <stdlib.h>

#include <EGL/egl.h>

// for performance reasons we can keep a single EGL context always active
#define PUGL_WASM_SINGLE_EGL_CONTEXT

typedef struct {
  EGLDisplay display;
  EGLConfig config;
  EGLContext context;
  EGLSurface surface;
} PuglWasmGlSurface;

static EGLint
puglWasmGlHintValue(const int value)
{
  return value == PUGL_DONT_CARE ? EGL_DONT_CARE : value;
}

static int
puglWasmGlGetAttrib(const EGLDisplay display,
                    const EGLConfig  config,
                    const EGLint     attrib)
{
  EGLint value = 0;
  eglGetConfigAttrib(display, config, attrib, &value);
  return value;
}

static PuglStatus
puglWasmGlConfigure(PuglView* view)
{
  PuglInternals* const impl    = view->impl;

  const EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

  if (display == EGL_NO_DISPLAY) {
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  int major, minor;
  if (eglInitialize(display, &major, &minor) != EGL_TRUE) {
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  EGLConfig config;
  int numConfigs;

  if (eglGetConfigs(display, &config, 1, &numConfigs) != EGL_TRUE || numConfigs != 1) {
    eglTerminate(display);
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  // clang-format off
  const EGLint attrs[] = {
    /*
    GLX_X_RENDERABLE,  True,
    GLX_X_VISUAL_TYPE, GLX_TRUE_COLOR,
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_RENDER_TYPE,   GLX_RGBA_BIT,
    EGL_SAMPLE_BUFFERS, view->hints[PUGL_MULTI_SAMPLE] ? 1 : 0,
    */
    EGL_SAMPLES,       puglWasmGlHintValue(view->hints[PUGL_SAMPLES]),
    EGL_RED_SIZE,      puglWasmGlHintValue(view->hints[PUGL_RED_BITS]),
    EGL_GREEN_SIZE,    puglWasmGlHintValue(view->hints[PUGL_GREEN_BITS]),
    EGL_BLUE_SIZE,     puglWasmGlHintValue(view->hints[PUGL_BLUE_BITS]),
    EGL_ALPHA_SIZE,    puglWasmGlHintValue(view->hints[PUGL_ALPHA_BITS]),
    EGL_DEPTH_SIZE,    puglWasmGlHintValue(view->hints[PUGL_DEPTH_BITS]),
    EGL_STENCIL_SIZE,  puglWasmGlHintValue(view->hints[PUGL_STENCIL_BITS]),
    EGL_NONE
  };
  // clang-format on

  if (eglChooseConfig(display, attrs, &config, 1, &numConfigs) != EGL_TRUE || numConfigs != 1) {
    eglTerminate(display);
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  PuglWasmGlSurface* const surface =
    (PuglWasmGlSurface*)calloc(1, sizeof(PuglWasmGlSurface));
  impl->surface = surface;

  surface->display = display;
  surface->config = config;
  surface->context = EGL_NO_SURFACE;
  surface->surface = EGL_NO_CONTEXT;

  view->hints[PUGL_RED_BITS] =
    puglWasmGlGetAttrib(display, config, EGL_RED_SIZE);
  view->hints[PUGL_GREEN_BITS] =
    puglWasmGlGetAttrib(display, config, EGL_GREEN_SIZE);
  view->hints[PUGL_BLUE_BITS] =
    puglWasmGlGetAttrib(display, config, EGL_BLUE_SIZE);
  view->hints[PUGL_ALPHA_BITS] =
    puglWasmGlGetAttrib(display, config, EGL_ALPHA_SIZE);
  view->hints[PUGL_DEPTH_BITS] =
    puglWasmGlGetAttrib(display, config, EGL_DEPTH_SIZE);
  view->hints[PUGL_STENCIL_BITS] =
    puglWasmGlGetAttrib(display, config, EGL_STENCIL_SIZE);
  view->hints[PUGL_SAMPLES] =
    puglWasmGlGetAttrib(display, config, EGL_SAMPLES);

  // double-buffering is always enabled for EGL
  view->hints[PUGL_DOUBLE_BUFFER] = 1;

  return PUGL_SUCCESS;
}

PUGL_WARN_UNUSED_RESULT
static PuglStatus
puglWasmGlEnter(PuglView* view, const PuglExposeEvent* PUGL_UNUSED(expose))
{
  PuglWasmGlSurface* const surface = (PuglWasmGlSurface*)view->impl->surface;
  if (!surface || !surface->context || !surface->surface) {
    return PUGL_FAILURE;
  }

#ifndef PUGL_WASM_SINGLE_EGL_CONTEXT
  return eglMakeCurrent(surface->display, surface->surface, surface->surface, surface->context) ? PUGL_SUCCESS : PUGL_FAILURE;
#else
  return PUGL_SUCCESS;
#endif
}

PUGL_WARN_UNUSED_RESULT
static PuglStatus
puglWasmGlLeave(PuglView* view, const PuglExposeEvent* expose)
{
  PuglWasmGlSurface* const surface = (PuglWasmGlSurface*)view->impl->surface;

  if (expose) { // note: swap buffers always enabled for EGL
    eglSwapBuffers(surface->display, surface->surface);
  }

#ifndef PUGL_WASM_SINGLE_EGL_CONTEXT
  return eglMakeCurrent(surface->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) ? PUGL_SUCCESS : PUGL_FAILURE;
#else
  return PUGL_SUCCESS;
#endif
}

static PuglStatus
puglWasmGlCreate(PuglView* view)
{
  PuglWasmGlSurface* const surface = (PuglWasmGlSurface*)view->impl->surface;
  const EGLDisplay display = surface->display;
  const EGLConfig  config  = surface->config;

  const EGLint attrs[] = {
    EGL_CONTEXT_CLIENT_VERSION,
    view->hints[PUGL_CONTEXT_VERSION_MAJOR],

    EGL_CONTEXT_MAJOR_VERSION,
    view->hints[PUGL_CONTEXT_VERSION_MAJOR],

    /*
    EGL_CONTEXT_MINOR_VERSION,
    view->hints[PUGL_CONTEXT_VERSION_MINOR],

    EGL_CONTEXT_OPENGL_DEBUG,
    (view->hints[PUGL_USE_DEBUG_CONTEXT] ? EGL_TRUE : EGL_FALSE),

    EGL_CONTEXT_OPENGL_PROFILE_MASK,
    (view->hints[PUGL_USE_COMPAT_PROFILE]
       ? EGL_CONTEXT_OPENGL_COMPATIBILITY_PROFILE_BIT
       : EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT),
    */

    EGL_NONE
  };

  surface->context = eglCreateContext(display, config, EGL_NO_CONTEXT, attrs);

  if (surface->context == EGL_NO_CONTEXT) {
    return PUGL_CREATE_CONTEXT_FAILED;
  }

  surface->surface = eglCreateWindowSurface(display, config, 0, NULL);

  if (surface->surface == EGL_NO_SURFACE) {
    return PUGL_CREATE_CONTEXT_FAILED;
  }

#ifdef PUGL_WASM_SINGLE_EGL_CONTEXT
  eglMakeCurrent(surface->display, surface->surface, surface->surface, surface->context);
#endif

  return PUGL_SUCCESS;
}

static void
puglWasmGlDestroy(PuglView* view)
{
  PuglWasmGlSurface* surface = (PuglWasmGlSurface*)view->impl->surface;
  if (surface) {
    const EGLDisplay display = surface->display;
    if (surface->surface != EGL_NO_SURFACE)
      eglDestroySurface(display, surface->surface);
    if (surface->context != EGL_NO_CONTEXT)
      eglDestroyContext(display, surface->context);
    eglTerminate(display);
    free(surface);
    view->impl->surface = NULL;
  }
}

const PuglBackend*
puglGlBackend(void)
{
  static const PuglBackend backend = {puglWasmGlConfigure,
                                      puglWasmGlCreate,
                                      puglWasmGlDestroy,
                                      puglWasmGlEnter,
                                      puglWasmGlLeave,
                                      puglStubGetContext};
  return &backend;
}
