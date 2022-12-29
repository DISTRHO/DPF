// Copyright 2012-2022 David Robillard <d@drobilla.net>
// Copyright 2021-2022 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#include "wasm.h"

#include "../pugl-upstream/src/internal.h"

#include <stdio.h>

#include <emscripten/html5.h>

#ifdef __cplusplus
#  define PUGL_INIT_STRUCT \
    {}
#else
#  define PUGL_INIT_STRUCT \
    {                      \
      0                    \
    }
#endif

#ifdef __MOD_DEVICES__
#  define MOD_SCALE_FACTOR_MULT 1
#endif

PuglWorldInternals*
puglInitWorldInternals(const PuglWorldType type, const PuglWorldFlags flags)
{
  PuglWorldInternals* impl =
    (PuglWorldInternals*)calloc(1, sizeof(PuglWorldInternals));

  impl->scaleFactor = emscripten_get_device_pixel_ratio();
#ifdef __MOD_DEVICES__
  impl->scaleFactor *= MOD_SCALE_FACTOR_MULT;
#endif

  printf("DONE: %s %d | -> %f\n", __func__, __LINE__, impl->scaleFactor);

  return impl;
}

void*
puglGetNativeWorld(PuglWorld*)
{
  printf("DONE: %s %d\n", __func__, __LINE__);
  return NULL;
}

PuglInternals*
puglInitViewInternals(PuglWorld* const world)
{
  printf("DONE: %s %d\n", __func__, __LINE__);
  PuglInternals* impl = (PuglInternals*)calloc(1, sizeof(PuglInternals));

  impl->buttonPressTimeout = -1;
  impl->supportsTouch = PUGL_DONT_CARE; // not yet known

#ifdef PUGL_WASM_ASYNC_CLIPBOARD
  impl->supportsClipboardRead = (PuglViewHintValue)EM_ASM_INT({
    if (typeof(navigator.clipboard) !== 'undefined' && typeof(navigator.clipboard.readText) === 'function' && window.isSecureContext) {
      return 1; // PUGL_TRUE
    }
    return 0; // PUGL_FALSE
  });

  impl->supportsClipboardWrite = (PuglViewHintValue)EM_ASM_INT({
    if (typeof(navigator.clipboard) !== 'undefined' && typeof(navigator.clipboard.writeText) === 'function' && window.isSecureContext) {
      return 1; // PUGL_TRUE
    }
    if (typeof(document.queryCommandSupported) !== 'undefined' && document.queryCommandSupported("copy")) {
      return 1; // PUGL_TRUE
    }
    return 0; // PUGL_FALSE
  });
#endif

  return impl;
}

static PuglStatus
puglDispatchEventWithContext(PuglView* const view, const PuglEvent* event)
{
  PuglStatus st0 = PUGL_SUCCESS;
  PuglStatus st1 = PUGL_SUCCESS;

  if (!(st0 = view->backend->enter(view, NULL))) {
    st0 = view->eventFunc(view, event);
    st1 = view->backend->leave(view, NULL);
  }

  return st0 ? st0 : st1;
}

static PuglKey
keyCodeToSpecial(const unsigned long code, const unsigned long location)
{
  switch (code) {
  case 0x08: return PUGL_KEY_BACKSPACE;
  case 0x1B: return PUGL_KEY_ESCAPE;
  case 0x2E: return PUGL_KEY_DELETE;
  case 0x70: return PUGL_KEY_F1;
  case 0x71: return PUGL_KEY_F2;
  case 0x72: return PUGL_KEY_F3;
  case 0x73: return PUGL_KEY_F4;
  case 0x74: return PUGL_KEY_F5;
  case 0x75: return PUGL_KEY_F6;
  case 0x76: return PUGL_KEY_F7;
  case 0x77: return PUGL_KEY_F8;
  case 0x78: return PUGL_KEY_F9;
  case 0x79: return PUGL_KEY_F10;
  case 0x7A: return PUGL_KEY_F11;
  case 0x7B: return PUGL_KEY_F12;
  case 0x25: return PUGL_KEY_LEFT;
  case 0x26: return PUGL_KEY_UP;
  case 0x27: return PUGL_KEY_RIGHT;
  case 0x28: return PUGL_KEY_DOWN;
  case 0x21: return PUGL_KEY_PAGE_UP;
  case 0x22: return PUGL_KEY_PAGE_DOWN;
  case 0x24: return PUGL_KEY_HOME;
  case 0x23: return PUGL_KEY_END;
  case 0x2D: return PUGL_KEY_INSERT;
  case 0x10: return location == DOM_KEY_LOCATION_RIGHT ? PUGL_KEY_SHIFT_R : PUGL_KEY_SHIFT_L;
  case 0x11: return location == DOM_KEY_LOCATION_RIGHT ? PUGL_KEY_CTRL_R : PUGL_KEY_CTRL_L;
  case 0x12: return location == DOM_KEY_LOCATION_RIGHT ? PUGL_KEY_ALT_R : PUGL_KEY_ALT_L;
  case 0xE0: return location == DOM_KEY_LOCATION_RIGHT ? PUGL_KEY_SUPER_R : PUGL_KEY_SUPER_L;
  case 0x5D: return PUGL_KEY_MENU;
  case 0x14: return PUGL_KEY_CAPS_LOCK;
  case 0x91: return PUGL_KEY_SCROLL_LOCK;
  case 0x90: return PUGL_KEY_NUM_LOCK;
  case 0x2C: return PUGL_KEY_PRINT_SCREEN;
  case 0x13: return PUGL_KEY_PAUSE;
  case '\r': return (PuglKey)'\r';
  default: break;
  }

  return (PuglKey)0;
}

static PuglMods
translateModifiers(const EM_BOOL ctrlKey,
                   const EM_BOOL shiftKey,
                   const EM_BOOL altKey,
                   const EM_BOOL metaKey)
{
  return (ctrlKey  ? PUGL_MOD_CTRL  : 0u) |
         (shiftKey ? PUGL_MOD_SHIFT : 0u) |
         (altKey   ? PUGL_MOD_ALT   : 0u) |
         (metaKey  ? PUGL_MOD_SUPER : 0u);
}

static bool
decodeCharacterString(const unsigned long keyCode,
                      const EM_UTF8 key[EM_HTML5_SHORT_STRING_LEN_BYTES],
                      char str[8])
{
  if (key[1] == 0)
  {
    str[0] = key[0];
    return true;
  }

  return false;
}

static EM_BOOL
puglKeyCallback(const int eventType, const EmscriptenKeyboardEvent* const keyEvent, void* const userData)
{
  PuglView* const view = (PuglView*)userData;

  if (!view->visible) {
    return EM_FALSE;
  }

  if (keyEvent->repeat && view->hints[PUGL_IGNORE_KEY_REPEAT])
    return EM_TRUE;

  PuglStatus st0 = PUGL_SUCCESS;
  PuglStatus st1 = PUGL_SUCCESS;

  const uint state = translateModifiers(keyEvent->ctrlKey,
                                        keyEvent->shiftKey,
                                        keyEvent->altKey,
                                        keyEvent->metaKey);

  const PuglKey special = keyCodeToSpecial(keyEvent->keyCode, keyEvent->location);

  uint key = keyEvent->key[0] >= ' ' && keyEvent->key[0] <= '~' && keyEvent->key[1] == '\0'
           ? keyEvent->key[0]
           : keyEvent->keyCode;

  if (key >= 'A' && key <= 'Z' && !keyEvent->shiftKey)
      key += 'a' - 'A';

  PuglEvent event = {{PUGL_NOTHING, 0}};
  event.key.type = eventType == EMSCRIPTEN_EVENT_KEYDOWN ? PUGL_KEY_PRESS : PUGL_KEY_RELEASE;
  event.key.time = keyEvent->timestamp / 1e3;
  // event.key.x     = xevent.xkey.x;
  // event.key.y     = xevent.xkey.y;
  // event.key.xRoot = xevent.xkey.x_root;
  // event.key.yRoot = xevent.xkey.y_root;
  event.key.key     = special ? special : key;
  event.key.keycode = keyEvent->keyCode;
  event.key.state   = state;
  st0 = puglDispatchEventWithContext(view, &event);

  d_debug("key event \n"
           "\tdown:     %d\n"
           "\trepeat:   %d\n"
           "\tlocation: %d\n"
           "\tstate:    0x%x\n"
           "\tkey[]:    '%s'\n"
           "\tcode[]:   '%s'\n"
           "\tlocale[]: '%s'\n"
           "\tkeyCode:  0x%lx:'%c' [deprecated, use key]\n"
           "\twhich:    0x%lx:'%c' [deprecated, use key, same as keycode?]\n"
           "\tspecial:  0x%x",
           eventType == EMSCRIPTEN_EVENT_KEYDOWN,
           keyEvent->repeat,
           keyEvent->location,
           state,
           keyEvent->key,
           keyEvent->code,
           keyEvent->locale,
           keyEvent->keyCode, keyEvent->keyCode >= ' ' && keyEvent->keyCode <= '~' ? keyEvent->keyCode : 0,
           keyEvent->which, keyEvent->which >= ' ' && keyEvent->which <= '~' ? keyEvent->which : 0,
           special);

  if (event.type == PUGL_KEY_PRESS && !special && !(keyEvent->ctrlKey|keyEvent->altKey|keyEvent->metaKey)) {
    char str[8] = PUGL_INIT_STRUCT;

    if (decodeCharacterString(keyEvent->keyCode, keyEvent->key, str)) {
      d_debug("resulting string is '%s'", str);

      event.text.type      = PUGL_TEXT;
      event.text.character = event.key.key;
      memcpy(event.text.string, str, sizeof(event.text.string));
      st1 = puglDispatchEventWithContext(view, &event);
    }
  }

  return (st0 ? st0 : st1) == PUGL_SUCCESS ? EM_TRUE : EM_FALSE;
}

static EM_BOOL
puglMouseCallback(const int eventType, const EmscriptenMouseEvent* const mouseEvent, void* const userData)
{
  PuglView* const view = (PuglView*)userData;

  if (!view->visible) {
    return EM_FALSE;
  }

  PuglEvent event = {{PUGL_NOTHING, 0}};

  const double   time  = mouseEvent->timestamp / 1e3;
  const PuglMods state = translateModifiers(mouseEvent->ctrlKey,
                                            mouseEvent->shiftKey,
                                            mouseEvent->altKey,
                                            mouseEvent->metaKey);

  double scaleFactor = view->world->impl->scaleFactor;
#ifdef __MOD_DEVICES__
  scaleFactor /= EM_ASM_DOUBLE({
    return parseFloat(
      RegExp('^scale\\\((.*)\\\)$')
      .exec(document.getElementById("pedalboard-dashboard").style.transform)[1]
    );
  }) * MOD_SCALE_FACTOR_MULT;
#endif

  // workaround missing pointer lock callback, see https://github.com/emscripten-core/emscripten/issues/9681
  EmscriptenPointerlockChangeEvent e;
  if (emscripten_get_pointerlock_status(&e) == EMSCRIPTEN_RESULT_SUCCESS)
      view->impl->pointerLocked = e.isActive;

  switch (eventType) {
  case EMSCRIPTEN_EVENT_MOUSEDOWN:
  case EMSCRIPTEN_EVENT_MOUSEUP:
    event.button.type  = eventType == EMSCRIPTEN_EVENT_MOUSEDOWN ? PUGL_BUTTON_PRESS : PUGL_BUTTON_RELEASE;
    event.button.time  = time;
    event.button.x     = mouseEvent->targetX * scaleFactor;
    event.button.y     = mouseEvent->targetY * scaleFactor;
    event.button.xRoot = mouseEvent->screenX * scaleFactor;
    event.button.yRoot = mouseEvent->screenY * scaleFactor;
    event.button.state = state;
    switch (mouseEvent->button) {
    case 1:
      event.button.button = 2;
      break;
    case 2:
      event.button.button = 1;
      break;
    default:
      event.button.button = mouseEvent->button;
      break;
    }
    break;
  case EMSCRIPTEN_EVENT_MOUSEMOVE:
    event.motion.type = PUGL_MOTION;
    event.motion.time = time;
    if (view->impl->pointerLocked) {
      // adjust local values for delta
      const double movementX = mouseEvent->movementX * scaleFactor;
      const double movementY = mouseEvent->movementY * scaleFactor;
      view->impl->lastMotion.x += movementX;
      view->impl->lastMotion.y += movementY;
      view->impl->lastMotion.xRoot += movementX;
      view->impl->lastMotion.yRoot += movementY;
      // now set x, y, xRoot and yRoot
      event.motion.x = view->impl->lastMotion.x;
      event.motion.y = view->impl->lastMotion.y;
      event.motion.xRoot = view->impl->lastMotion.xRoot;
      event.motion.yRoot = view->impl->lastMotion.yRoot;
    } else {
      // cache values for possible pointer lock movement later
      view->impl->lastMotion.x = event.motion.x = mouseEvent->targetX * scaleFactor;
      view->impl->lastMotion.y = event.motion.y = mouseEvent->targetY * scaleFactor;
      view->impl->lastMotion.xRoot = event.motion.xRoot = mouseEvent->screenX * scaleFactor;
      view->impl->lastMotion.yRoot = event.motion.yRoot = mouseEvent->screenY * scaleFactor;
    }
    event.motion.state = state;
    break;
  case EMSCRIPTEN_EVENT_MOUSEENTER:
  case EMSCRIPTEN_EVENT_MOUSELEAVE:
    event.crossing.type  = eventType == EMSCRIPTEN_EVENT_MOUSEENTER ? PUGL_POINTER_IN : PUGL_POINTER_OUT;
    event.crossing.time  = time;
    event.crossing.x     = mouseEvent->targetX * scaleFactor;
    event.crossing.y     = mouseEvent->targetY * scaleFactor;
    event.crossing.xRoot = mouseEvent->screenX * scaleFactor;
    event.crossing.yRoot = mouseEvent->screenY * scaleFactor;
    event.crossing.state = state;
    event.crossing.mode  = PUGL_CROSSING_NORMAL;
    break;
  }

  if (event.type == PUGL_NOTHING)
    return EM_FALSE;

  puglDispatchEventWithContext(view, &event);

#ifdef PUGL_WASM_AUTO_POINTER_LOCK
  switch (eventType) {
  case EMSCRIPTEN_EVENT_MOUSEDOWN:
    emscripten_request_pointerlock(view->world->className, false);
    break;
  case EMSCRIPTEN_EVENT_MOUSEUP:
    emscripten_exit_pointerlock();
    break;
  }
#endif

  // note: we must always return false, otherwise canvas never gets keyboard input
  return EM_FALSE;
}

static void
puglTouchStartDelay(void* const userData)
{
  PuglView*      const view = (PuglView*)userData;
  PuglInternals* const impl  = view->impl;

  impl->buttonPressTimeout = -1;
  impl->nextButtonEvent.button.time += 2000;
  puglDispatchEventWithContext(view, &impl->nextButtonEvent);
}

static EM_BOOL
puglTouchCallback(const int eventType, const EmscriptenTouchEvent* const touchEvent, void* const userData)
{
  if (touchEvent->numTouches <= 0) {
    return EM_FALSE;
  }

  PuglView*      const view = (PuglView*)userData;
  PuglInternals* const impl  = view->impl;

  if (impl->supportsTouch == PUGL_DONT_CARE) {
    impl->supportsTouch = PUGL_TRUE;

    // stop using mouse press events which conflict with touch
    const char* const className = view->world->className;
    emscripten_set_mousedown_callback(className, view, false, NULL);
    emscripten_set_mouseup_callback(className, view, false, NULL);
  }

  if (!view->visible) {
    return EM_FALSE;
  }

  PuglEvent event = {{PUGL_NOTHING, 0}};

  const double   time  = touchEvent->timestamp / 1e3;
  const PuglMods state = translateModifiers(touchEvent->ctrlKey,
                                            touchEvent->shiftKey,
                                            touchEvent->altKey,
                                            touchEvent->metaKey);

  double scaleFactor = view->world->impl->scaleFactor;
#ifdef __MOD_DEVICES__
  scaleFactor /= EM_ASM_DOUBLE({
    return parseFloat(
      RegExp('^scale\\\((.*)\\\)$')
      .exec(document.getElementById("pedalboard-dashboard").style.transform)[1]
    );
  }) * MOD_SCALE_FACTOR_MULT;
#endif

  d_debug("touch %d|%s %d || %ld",
          eventType,
          eventType == EMSCRIPTEN_EVENT_TOUCHSTART ? "start" :
          eventType == EMSCRIPTEN_EVENT_TOUCHEND ? "end" : "cancel",
          touchEvent->numTouches,
          impl->buttonPressTimeout);

  const EmscriptenTouchPoint* point = &touchEvent->touches[0];

  if (impl->buttonPressTimeout != -1 || eventType == EMSCRIPTEN_EVENT_TOUCHCANCEL) {
    // if we received an event while touch is active, trigger initial click now
    if (impl->buttonPressTimeout != -1) {
      emscripten_clear_timeout(impl->buttonPressTimeout);
      impl->buttonPressTimeout = -1;
      if (eventType != EMSCRIPTEN_EVENT_TOUCHCANCEL) {
        impl->nextButtonEvent.button.button = 0;
      }
    }
    impl->nextButtonEvent.button.time = time;
    puglDispatchEventWithContext(view, &impl->nextButtonEvent);
  }

  switch (eventType) {
  case EMSCRIPTEN_EVENT_TOUCHEND:
  case EMSCRIPTEN_EVENT_TOUCHCANCEL:
    event.button.type   = PUGL_BUTTON_RELEASE;
    event.button.time   = time;
    event.button.button = eventType == EMSCRIPTEN_EVENT_TOUCHCANCEL ? 1 : 0;
    event.button.x      = point->targetX * scaleFactor;
    event.button.y      = point->targetY * scaleFactor;
    event.button.xRoot  = point->screenX * scaleFactor;
    event.button.yRoot  = point->screenY * scaleFactor;
    event.button.state  = state;
    break;

  case EMSCRIPTEN_EVENT_TOUCHSTART:
    // this event can be used for a couple of things, store it until we know more
    event.button.type   = PUGL_BUTTON_PRESS;
    event.button.time   = time;
    event.button.button = 1; // if no other event occurs soon, treat it as right-click
    event.button.x      = point->targetX * scaleFactor;
    event.button.y      = point->targetY * scaleFactor;
    event.button.xRoot  = point->screenX * scaleFactor;
    event.button.yRoot  = point->screenY * scaleFactor;
    event.button.state  = state;
    memcpy(&impl->nextButtonEvent, &event, sizeof(PuglEvent));
    impl->buttonPressTimeout = emscripten_set_timeout(puglTouchStartDelay, 2000, view);
    // fall through, moving "mouse" to touch position

  case EMSCRIPTEN_EVENT_TOUCHMOVE:
    event.motion.type  = PUGL_MOTION;
    event.motion.time  = time;
    event.motion.x     = point->targetX * scaleFactor;
    event.motion.y     = point->targetY * scaleFactor;
    event.motion.xRoot = point->screenX * scaleFactor;
    event.motion.yRoot = point->screenY * scaleFactor;
    event.motion.state = state;
    break;
  }

  if (event.type == PUGL_NOTHING)
    return EM_FALSE;

  puglDispatchEventWithContext(view, &event);

  // FIXME we must always return false??
  return EM_FALSE;
}

static EM_BOOL
puglFocusCallback(const int eventType, const EmscriptenFocusEvent* /*const focusEvent*/, void* const userData)
{
  PuglView* const view = (PuglView*)userData;

  if (!view->visible) {
    return EM_FALSE;
  }

  d_debug("focus %d|%s", eventType, eventType == EMSCRIPTEN_EVENT_FOCUSIN ? "focus-in" : "focus-out");

  PuglEvent event = {{eventType == EMSCRIPTEN_EVENT_FOCUSIN ? PUGL_FOCUS_IN : PUGL_FOCUS_OUT, 0}};
  event.focus.mode = PUGL_CROSSING_NORMAL;

  puglDispatchEventWithContext(view, &event);

  // note: we must always return false, otherwise canvas never gets proper focus
  return EM_FALSE;
}

static EM_BOOL
puglPointerLockChangeCallback(const int eventType, const EmscriptenPointerlockChangeEvent* event, void* const userData)
{
  PuglView* const view = (PuglView*)userData;

  view->impl->pointerLocked = event->isActive;
  return EM_TRUE;
}

static EM_BOOL
puglWheelCallback(const int eventType, const EmscriptenWheelEvent* const wheelEvent, void* const userData)
{
  PuglView* const view = (PuglView*)userData;

  if (!view->visible) {
    return EM_FALSE;
  }

  double scaleFactor = view->world->impl->scaleFactor;
#ifdef __MOD_DEVICES__
  scaleFactor /= EM_ASM_DOUBLE({
    return parseFloat(
      RegExp('^scale\\\((.*)\\\)$')
      .exec(document.getElementById("pedalboard-dashboard").style.transform)[1]
    );
  }) * MOD_SCALE_FACTOR_MULT;
#endif

  PuglEvent event = {{PUGL_SCROLL, 0}};
  event.scroll.time  = wheelEvent->mouse.timestamp / 1e3;
  event.scroll.x     = wheelEvent->mouse.targetX;
  event.scroll.y     = wheelEvent->mouse.targetY;
  event.scroll.xRoot = wheelEvent->mouse.screenX;
  event.scroll.yRoot = wheelEvent->mouse.screenY;
  event.scroll.state = translateModifiers(wheelEvent->mouse.ctrlKey,
                                          wheelEvent->mouse.shiftKey,
                                          wheelEvent->mouse.altKey,
                                          wheelEvent->mouse.metaKey);
  event.scroll.direction = PUGL_SCROLL_SMOOTH;
  // FIXME handle wheelEvent->deltaMode
  event.scroll.dx = wheelEvent->deltaX * 0.01 * scaleFactor;
  event.scroll.dy = -wheelEvent->deltaY * 0.01 * scaleFactor;

  return puglDispatchEventWithContext(view, &event) == PUGL_SUCCESS ? EM_TRUE : EM_FALSE;
}

static EM_BOOL
puglUiCallback(const int eventType, const EmscriptenUiEvent* const uiEvent, void* const userData)
{
  PuglView* const view = (PuglView*)userData;
  const char* const className = view->world->className;

  // FIXME
  const int width = EM_ASM_INT({
    var canvasWrapper = document.getElementById(UTF8ToString($0)).parentElement;
    canvasWrapper.style.setProperty("--device-pixel-ratio", window.devicePixelRatio);
    return canvasWrapper.clientWidth;
  }, className);

  const int height = EM_ASM_INT({
    var canvasWrapper = document.getElementById(UTF8ToString($0)).parentElement;
    return canvasWrapper.clientHeight;
  }, className);

  if (!width || !height)
    return EM_FALSE;

  double scaleFactor = emscripten_get_device_pixel_ratio();
#ifdef __MOD_DEVICES__
  scaleFactor *= MOD_SCALE_FACTOR_MULT;
#endif
  view->world->impl->scaleFactor = scaleFactor;

  emscripten_set_canvas_element_size(view->world->className, width * scaleFactor, height * scaleFactor);

  PuglEvent event        = {{PUGL_CONFIGURE, 0}};
  event.configure.x      = view->frame.x;
  event.configure.y      = view->frame.y;
  event.configure.width  = width * scaleFactor;
  event.configure.height = height * scaleFactor;
  puglDispatchEvent(view, &event);
  return EM_TRUE;
}

static EM_BOOL
puglVisibilityChangeCallback(const int eventType, const EmscriptenVisibilityChangeEvent* const visibilityChangeEvent, void* const userData)
{
  PuglView* const view = (PuglView*)userData;

  view->visible = visibilityChangeEvent->hidden == EM_FALSE;
  PuglEvent event = {{ view->visible ? PUGL_MAP : PUGL_UNMAP, 0}};
  puglDispatchEvent(view, &event);
  return EM_FALSE;
}

PuglStatus
puglRealize(PuglView* const view)
{
  printf("TODO: %s %d\n", __func__, __LINE__);
  PuglStatus st = PUGL_SUCCESS;

  // Ensure that we do not have a parent
  if (view->parent) {
  printf("TODO: %s %d\n", __func__, __LINE__);
    return PUGL_FAILURE;
  }

  if (!view->backend || !view->backend->configure) {
  printf("TODO: %s %d\n", __func__, __LINE__);
    return PUGL_BAD_BACKEND;
  }

  const char* const className = view->world->className;
  d_stdout("className is %s", className);

  // Set the size to the default if it has not already been set
  if (view->frame.width <= 0.0 && view->frame.height <= 0.0) {
    PuglViewSize defaultSize = view->sizeHints[PUGL_DEFAULT_SIZE];
    if (!defaultSize.width || !defaultSize.height) {
      return PUGL_BAD_CONFIGURATION;
    }

    view->frame.width  = defaultSize.width;
    view->frame.height = defaultSize.height;
  }

  // Configure and create the backend
  if ((st = view->backend->configure(view)) || (st = view->backend->create(view))) {
    view->backend->destroy(view);
    return st;
  }

  if (view->title) {
    puglSetWindowTitle(view, view->title);
  }

  puglDispatchSimpleEvent(view, PUGL_CREATE);

  PuglEvent event        = {{PUGL_CONFIGURE, 0}};
  event.configure.x      = view->frame.x;
  event.configure.y      = view->frame.y;
  event.configure.width  = view->frame.width;
  event.configure.height = view->frame.height;
  puglDispatchEvent(view, &event);

  EM_ASM({
   var canvasWrapper = document.getElementById(UTF8ToString($0)).parentElement;
   canvasWrapper.style.setProperty("--device-pixel-ratio", window.devicePixelRatio);
  }, className);

  emscripten_set_canvas_element_size(className, view->frame.width, view->frame.height);
//   emscripten_set_keypress_callback(className, view, false, puglKeyCallback);
  emscripten_set_keydown_callback(className, view, false, puglKeyCallback);
  emscripten_set_keyup_callback(className, view, false, puglKeyCallback);
  emscripten_set_touchstart_callback(className, view, false, puglTouchCallback);
  emscripten_set_touchend_callback(className, view, false, puglTouchCallback);
  emscripten_set_touchmove_callback(className, view, false, puglTouchCallback);
  emscripten_set_touchcancel_callback(className, view, false, puglTouchCallback);
  emscripten_set_mousedown_callback(className, view, false, puglMouseCallback);
  emscripten_set_mouseup_callback(className, view, false, puglMouseCallback);
  emscripten_set_mousemove_callback(className, view, false, puglMouseCallback);
  emscripten_set_mouseenter_callback(className, view, false, puglMouseCallback);
  emscripten_set_mouseleave_callback(className, view, false, puglMouseCallback);
  emscripten_set_focusin_callback(className, view, false, puglFocusCallback);
  emscripten_set_focusout_callback(className, view, false, puglFocusCallback);
  emscripten_set_wheel_callback(className, view, false, puglWheelCallback);
  emscripten_set_pointerlockchange_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, view, false, puglPointerLockChangeCallback);
  emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, view, false, puglUiCallback);
  emscripten_set_visibilitychange_callback(view, false, puglVisibilityChangeCallback);

  printf("TODO: %s %d\n", __func__, __LINE__);
  return PUGL_SUCCESS;
}

PuglStatus
puglShow(PuglView* const view)
{
  view->visible = true;
  view->impl->needsRepaint = true;
  return puglPostRedisplay(view);
}

PuglStatus
puglHide(PuglView* const view)
{
  view->visible = false;
  return PUGL_FAILURE;
}

void
puglFreeViewInternals(PuglView* const view)
{
  printf("DONE: %s %d\n", __func__, __LINE__);
  if (view && view->impl) {
    if (view->backend) {
      view->backend->destroy(view);
    }
    free(view->impl->clipboardData);
    free(view->impl->timers);
    free(view->impl);
  }
}

void
puglFreeWorldInternals(PuglWorld* const world)
{
  printf("DONE: %s %d\n", __func__, __LINE__);
  free(world->impl);
}

PuglStatus
puglGrabFocus(PuglView*)
{
  return PUGL_FAILURE;
}

double
puglGetScaleFactor(const PuglView* const view)
{
  printf("DONE: %s %d\n", __func__, __LINE__);
  return view->world->impl->scaleFactor;
}

double
puglGetTime(const PuglWorld*)
{
  return emscripten_get_now() / 1e3;
}

PuglStatus
puglUpdate(PuglWorld* const world, const double timeout)
{
  for (size_t i = 0; i < world->numViews; ++i) {
    PuglView* const view = world->views[i];

    if (!view->visible) {
      continue;
    }

    puglDispatchSimpleEvent(view, PUGL_UPDATE);

    if (!view->impl->needsRepaint) {
      continue;
    }

    view->impl->needsRepaint = false;

    PuglEvent event     = {{PUGL_EXPOSE, 0}};
    event.expose.x      = view->frame.x;
    event.expose.y      = view->frame.y;
    event.expose.width  = view->frame.width;
    event.expose.height = view->frame.height;
    puglDispatchEvent(view, &event);
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglPostRedisplay(PuglView* const view)
{
  view->impl->needsRepaint = true;
  return PUGL_SUCCESS;
}

PuglStatus
puglPostRedisplayRect(PuglView* const view, const PuglRect rect)
{
  view->impl->needsRepaint = true;
  return PUGL_FAILURE;
}

PuglNativeView
puglGetNativeView(PuglView* const view)
{
  return 0;
}

PuglStatus
puglSetWindowTitle(PuglView* const view, const char* const title)
{
  puglSetString(&view->title, title);
  emscripten_set_window_title(title);
  return PUGL_SUCCESS;
}

PuglStatus
puglSetSizeHint(PuglView* const    view,
                const PuglSizeHint hint,
                const PuglSpan     width,
                const PuglSpan     height)
{
  view->sizeHints[hint].width  = width;
  view->sizeHints[hint].height = height;
  return PUGL_SUCCESS;
}

static EM_BOOL
puglTimerLoopCallback(double timeout, void* const arg)
{
  PuglTimer*     const timer = (PuglTimer*)arg;
  PuglInternals* const impl  = timer->view->impl;

  // only handle active timers
  for (uint32_t i=0; i<impl->numTimers; ++i)
  {
    if (impl->timers[i].id == timer->id)
    {
      PuglEvent event = {{PUGL_TIMER, 0}};
      event.timer.id  = timer->id;
      puglDispatchEventWithContext(timer->view, &event);
      return EM_TRUE;
    }
  }

  return EM_FALSE;

  // unused
  (void)timeout;
}

PuglStatus
puglStartTimer(PuglView* const view, const uintptr_t id, const double timeout)
{
  printf("DONE: %s %d\n", __func__, __LINE__);
  PuglInternals* const impl = view->impl;
  const uint32_t timerIndex = impl->numTimers++;

  if (impl->timers == NULL)
    impl->timers = (PuglTimer*)malloc(sizeof(PuglTimer));
  else
    impl->timers = (PuglTimer*)realloc(impl->timers, sizeof(PuglTimer) * timerIndex);

  PuglTimer* const timer = &impl->timers[timerIndex];
  timer->view = view;
  timer->id = id;

  emscripten_set_timeout_loop(puglTimerLoopCallback, timeout * 1e3, timer);
  return PUGL_SUCCESS;
}

PuglStatus
puglStopTimer(PuglView* const view, const uintptr_t id)
{
  printf("DONE: %s %d\n", __func__, __LINE__);
  PuglInternals* const impl = view->impl;

  if (impl->timers == NULL || impl->numTimers == 0)
    return PUGL_FAILURE;

  for (uint32_t i=0; i<impl->numTimers; ++i)
  {
    if (impl->timers[i].id == id)
    {
      memmove(impl->timers + i, impl->timers + (i + 1), sizeof(PuglTimer) * (impl->numTimers - 1));
      --impl->numTimers;
      return PUGL_SUCCESS;
    }
  }

  return PUGL_FAILURE;
}

#ifdef PUGL_WASM_ASYNC_CLIPBOARD
EM_JS(char*, puglGetAsyncClipboardData, (), {
  var text = Asyncify.handleSleep(function(wakeUp) {
    navigator.clipboard.readText()
      .then(function(text) {
        wakeUp(text);
      })
      .catch(function() {
        wakeUp("");
      });
  });
  if (!text.length) {
    return null;
  }
  var length = lengthBytesUTF8(text) + 1;
  var str = _malloc(length);
  stringToUTF8(text, str, length);
  return str;
});
#endif

PuglStatus
puglPaste(PuglView* const view)
{
#ifdef PUGL_WASM_ASYNC_CLIPBOARD
  // abort early if we already know it is not supported
  if (view->impl->supportsClipboardRead == PUGL_FALSE) {
    return PUGL_UNSUPPORTED;
  }

  free(view->impl->clipboardData);
  view->impl->clipboardData = puglGetAsyncClipboardData();
#endif

  if (view->impl->clipboardData == NULL) {
    return PUGL_FAILURE;
  }

  const PuglDataOfferEvent offer = {
    PUGL_DATA_OFFER,
    0,
    emscripten_get_now() / 1e3,
  };

  PuglEvent offerEvent;
  offerEvent.offer = offer;
  puglDispatchEvent(view, &offerEvent);
  return PUGL_SUCCESS;
}

PuglStatus
puglAcceptOffer(PuglView* const                 view,
                const PuglDataOfferEvent* const offer,
                const uint32_t                  typeIndex)
{
  if (typeIndex != 0) {
    return PUGL_UNSUPPORTED;
  }

  const PuglDataEvent data = {
    PUGL_DATA,
    0,
    emscripten_get_now() / 1e3,
    0,
  };

  PuglEvent dataEvent;
  dataEvent.data = data;
  puglDispatchEvent(view, &dataEvent);
  return PUGL_SUCCESS;
}

uint32_t
puglGetNumClipboardTypes(const PuglView* const view)
{
  return view->impl->clipboardData != NULL ? 1u : 0u;
}

const char*
puglGetClipboardType(const PuglView* const view, const uint32_t typeIndex)
{
  return (typeIndex == 0 && view->impl->clipboardData != NULL)
           ? "text/plain"
           : NULL;
}

const void*
puglGetClipboard(PuglView* const view,
                 const uint32_t  typeIndex,
                 size_t* const   len)
{
  return view->impl->clipboardData;
}

PuglStatus
puglSetClipboard(PuglView* const   view,
                 const char* const type,
                 const void* const data,
                 const size_t      len)
{
  // only utf8 text supported for now
  if (type != NULL && strcmp(type, "text/plain") != 0) {
    return PUGL_UNSUPPORTED;
  }

  const char* const className = view->world->className;
  const char* const text      = (const char*)data;

#ifdef PUGL_WASM_ASYNC_CLIPBOARD
  // abort early if we already know it is not supported
  if (view->impl->supportsClipboardWrite == PUGL_FALSE) {
    return PUGL_UNSUPPORTED;
  }
#else
  puglSetString(&view->impl->clipboardData, text);
#endif

  EM_ASM({
    if (typeof(navigator.clipboard) !== 'undefined' && typeof(navigator.clipboard.writeText) === 'function' && window.isSecureContext) {
      navigator.clipboard.writeText(UTF8ToString($1));
    } else {
      var canvasClipboardObjName = UTF8ToString($0) + "_clipboard";
      var canvasClipboardElem = document.getElementById(canvasClipboardObjName);

      if (!canvasClipboardElem) {
        canvasClipboardElem = document.createElement('textarea');
        canvasClipboardElem.id = canvasClipboardObjName;
        canvasClipboardElem.style.position = 'fixed';
        canvasClipboardElem.style.whiteSpace = 'pre';
        canvasClipboardElem.style.zIndex = '-1';
        canvasClipboardElem.setAttribute('readonly', true);
        document.body.appendChild(canvasClipboardElem);
      }

      canvasClipboardElem.textContent = UTF8ToString($1);
      canvasClipboardElem.select();
      document.execCommand("copy");
    }
  }, className, text);

  // FIXME proper return status
  return PUGL_SUCCESS;
}

PuglStatus
puglSetCursor(PuglView* const view, const PuglCursor cursor)
{
  printf("TODO: %s %d\n", __func__, __LINE__);
  return PUGL_FAILURE;
}

PuglStatus
puglSetTransientParent(PuglView* const view, const PuglNativeView parent)
{
  printf("TODO: %s %d\n", __func__, __LINE__);
  view->transientParent = parent;
  return PUGL_FAILURE;
}

PuglStatus
puglSetPosition(PuglView* const view, const int x, const int y)
{
  printf("TODO: %s %d\n", __func__, __LINE__);

  if (x > INT16_MAX || y > INT16_MAX) {
    return PUGL_BAD_PARAMETER;
  }

  view->frame.x = (PuglCoord)x;
  view->frame.y = (PuglCoord)y;
  return PUGL_FAILURE;
}
