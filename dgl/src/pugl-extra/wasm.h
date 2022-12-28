// Copyright 2012-2022 David Robillard <d@drobilla.net>
// Copyright 2021-2022 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#ifndef PUGL_SRC_WASM_H
#define PUGL_SRC_WASM_H

// #include "attributes.h"
#include "types.h"

#include "pugl/pugl.h"

// #define PUGL_WASM_ASYNC_CLIPBOARD

struct PuglTimer {
  PuglView* view;
  uintptr_t id;
};

struct PuglWorldInternalsImpl {
  double scaleFactor;
};

struct LastMotionValues {
  double x, y, xRoot, yRoot;
};

struct PuglInternalsImpl {
  PuglSurface* surface;
  bool needsRepaint;
  bool pointerLocked;
  uint32_t numTimers;
  LastMotionValues lastMotion;
  long buttonPressTimeout;
  PuglEvent nextButtonEvent;
#ifdef PUGL_WASM_ASYNC_CLIPBOARD
  PuglViewHintValue supportsClipboardRead;
  PuglViewHintValue supportsClipboardWrite;
#endif
  PuglViewHintValue supportsTouch;
  char* clipboardData;
  struct PuglTimer* timers;
};

#endif // PUGL_SRC_WASM_H
