// Copyright 2012-2022 David Robillard <d@drobilla.net>
// Copyright 2021-2022 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#ifndef PUGL_SRC_HAIKU_H
#define PUGL_SRC_HAIKU_H

#include "../pugl-upstream/src/types.h"

#include "pugl/pugl.h"

#include <Application.h>
#include <Window.h>

struct PuglWorldInternalsImpl {
  BApplication* app;
};

struct PuglInternalsImpl {
  PuglSurface* surface;
  BView* view;
  BWindow* window;
};

#endif // PUGL_SRC_HAIKU_H
