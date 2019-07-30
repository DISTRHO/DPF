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
   @file pugl.h API for Pugl, a minimal portable API for OpenGL.
*/

#ifndef PUGL_H_INCLUDED
#define PUGL_H_INCLUDED

#include <stdint.h>

/*
  This API is pure portable C and contains no platform specific elements, or
  even a GL dependency.  However, unfortunately GL includes vary across
  platforms so they are included here to allow for pure portable programs.
*/
#ifdef __APPLE__
#    include <OpenGL/gl.h>
#else
#    ifdef _WIN32
#        include <winsock2.h>
#        include <windows.h>  /* Broken Windows GL headers require this */
#    endif
#    include <GL/gl.h>
#endif

#ifdef __cplusplus
extern "C" {
#else
#    include <stdbool.h>
#endif

/**
   @defgroup pugl Pugl
   A minimal portable API for OpenGL.
   @{
*/

/**
   A Pugl view.
*/
typedef struct PuglViewImpl PuglView;

/**
   A native window handle.

   On X11, this is a Window.
   On OSX, this is an NSView*.
   On Windows, this is a HWND.
*/
typedef intptr_t PuglNativeWindow;

/**
   Return status code.
*/
typedef enum {
	PUGL_SUCCESS = 0
} PuglStatus;

/**
   Convenience symbols for ASCII control characters.
*/
typedef enum {
	PUGL_CHAR_BACKSPACE = 0x08,
	PUGL_CHAR_ESCAPE    = 0x1B,
	PUGL_CHAR_DELETE    = 0x7F
} PuglChar;

/**
   Special (non-Unicode) keyboard keys.
*/
typedef enum {
	PUGL_KEY_F1 = 1,
	PUGL_KEY_F2,
	PUGL_KEY_F3,
	PUGL_KEY_F4,
	PUGL_KEY_F5,
	PUGL_KEY_F6,
	PUGL_KEY_F7,
	PUGL_KEY_F8,
	PUGL_KEY_F9,
	PUGL_KEY_F10,
	PUGL_KEY_F11,
	PUGL_KEY_F12,
	PUGL_KEY_LEFT,
	PUGL_KEY_UP,
	PUGL_KEY_RIGHT,
	PUGL_KEY_DOWN,
	PUGL_KEY_PAGE_UP,
	PUGL_KEY_PAGE_DOWN,
	PUGL_KEY_HOME,
	PUGL_KEY_END,
	PUGL_KEY_INSERT,
	PUGL_KEY_SHIFT,
	PUGL_KEY_CTRL,
	PUGL_KEY_ALT,
	PUGL_KEY_SUPER
} PuglKey;

/**
   Keyboard modifier flags.
*/
typedef enum {
	PUGL_MOD_SHIFT = 1 << 0,  /**< Shift key */
	PUGL_MOD_CTRL  = 1 << 1,  /**< Control key */
	PUGL_MOD_ALT   = 1 << 2,  /**< Alt/Option key */
	PUGL_MOD_SUPER = 1 << 3   /**< Mod4/Command/Windows key */
} PuglMod;

/**
   Layout-independent keycodes.
   These keycodes are relative to an US QWERTY keyboard. 
   Therefore, the keycode for the letter 'A' on an AZERTY keyboard will be equal to PUGL_KC_Q.
 */
typedef enum {
    /* Zero, does not correspond to any key. */
    PUGL_KC_None = 0,
    
    PUGL_KC_A = 4,
    PUGL_KC_B = 5,
    PUGL_KC_C = 6,
    PUGL_KC_D = 7,
    PUGL_KC_E = 8,
    PUGL_KC_F = 9,
    PUGL_KC_G = 10,
    PUGL_KC_H = 11,
    PUGL_KC_I = 12,
    PUGL_KC_J = 13,
    PUGL_KC_K = 14,
    PUGL_KC_L = 15,
    PUGL_KC_M = 16,
    PUGL_KC_N = 17,
    PUGL_KC_O = 18,
    PUGL_KC_P = 19,
    PUGL_KC_Q = 20,
    PUGL_KC_R = 21,
    PUGL_KC_S = 22,
    PUGL_KC_T = 23,
    PUGL_KC_U = 24,
    PUGL_KC_V = 25,
    PUGL_KC_W = 26,
    PUGL_KC_X = 27,
    PUGL_KC_Y = 28,
    PUGL_KC_Z = 29,
    PUGL_KC_1 = 30,
    PUGL_KC_2 = 31,
    PUGL_KC_3 = 32,
    PUGL_KC_4 = 33,
    PUGL_KC_5 = 34,
    PUGL_KC_6 = 35,
    PUGL_KC_7 = 36,
    PUGL_KC_8 = 37,
    PUGL_KC_9 = 38,
    PUGL_KC_0 = 39,
    PUGL_KC_Escape = 41,
    PUGL_KC_Delete = 42,
    PUGL_KC_Tab = 43,
    PUGL_KC_Space = 44,
    PUGL_KC_Minus = 45,
    PUGL_KC_Equals = 46,
    PUGL_KC_LeftBracket = 47,
    PUGL_KC_RightBracket = 48,
    PUGL_KC_Backslash = 49,
    PUGL_KC_Semicolon = 51,
    PUGL_KC_Quote = 52,
    PUGL_KC_Grave = 53,
    PUGL_KC_Comma = 54,
    PUGL_KC_Period = 55,
    PUGL_KC_Slash = 56,
    PUGL_KC_CapsLock = 57,
    PUGL_KC_F1 = 58,
    PUGL_KC_F2 = 59,
    PUGL_KC_F3 = 60,
    PUGL_KC_F4 = 61,
    PUGL_KC_F5 = 62,
    PUGL_KC_F6 = 63,
    PUGL_KC_F7 = 64,
    PUGL_KC_F8 = 65,
    PUGL_KC_F9 = 66,
    PUGL_KC_F10 = 67,
    PUGL_KC_F11 = 68,
    PUGL_KC_F12 = 69,
    PUGL_KC_PrintScreen = 70,
    PUGL_KC_ScrollLock = 71,
    PUGL_KC_Pause = 72,
    PUGL_KC_Insert = 73,
    PUGL_KC_Home = 74,
    PUGL_KC_PageUp = 75,
    PUGL_KC_DeleteForward = 76,
    PUGL_KC_End = 77,
    PUGL_KC_PageDown = 78,
    PUGL_KC_Right = 79,
    PUGL_KC_Left = 80,
    PUGL_KC_Down = 81,
    PUGL_KC_Up = 82,
    PUGL_KC_KP_NumLock = 83,
    PUGL_KC_KP_Divide = 84,
    PUGL_KC_KP_Multiply = 85,
    PUGL_KC_KP_Subtract = 86,
    PUGL_KC_KP_Add = 87,
    PUGL_KC_KP_Enter = 88,
    PUGL_KC_KP_1 = 89,
    PUGL_KC_KP_2 = 90,
    PUGL_KC_KP_3 = 91,
    PUGL_KC_KP_4 = 92,
    PUGL_KC_KP_5 = 93,
    PUGL_KC_KP_6 = 94,
    PUGL_KC_KP_7 = 95,
    PUGL_KC_KP_8 = 96,
    PUGL_KC_KP_9 = 97,
    PUGL_KC_KP_0 = 98,
    PUGL_KC_Point = 99,
    PUGL_KC_NonUSBackslash = 100,
    PUGL_KC_KP_Equals = 103,
    PUGL_KC_F13 = 104,
    PUGL_KC_F14 = 105,
    PUGL_KC_F15 = 106,
    PUGL_KC_F16 = 107,
    PUGL_KC_F17 = 108,
    PUGL_KC_F18 = 109,
    PUGL_KC_F19 = 110,
    PUGL_KC_F20 = 111,
    PUGL_KC_F21 = 112,
    PUGL_KC_F22 = 113,
    PUGL_KC_F23 = 114,
    PUGL_KC_F24 = 115,
    PUGL_KC_Help = 117,
    PUGL_KC_Menu = 118,
    PUGL_KC_Mute = 127,
    PUGL_KC_SysReq = 154,
    PUGL_KC_Return = 158,
    PUGL_KC_KP_Clear = 216,
    PUGL_KC_KP_Decimal = 220,
    PUGL_KC_LeftControl = 224,
    PUGL_KC_LeftShift = 225,
    PUGL_KC_LeftAlt = 226,
    PUGL_KC_LeftGUI = 227,
    PUGL_KC_RightControl = 228,
    PUGL_KC_RightShift = 229,
    PUGL_KC_RightAlt = 230,
    PUGL_KC_RightGUI = 231
} PuglKeyCodes;

/**
   Handle for opaque user data.
*/
typedef void* PuglHandle;

/**
   A function called when the window is closed.
*/
typedef void (*PuglCloseFunc)(PuglView* view);

/**
   A function called to draw the view contents with OpenGL.
*/
typedef void (*PuglDisplayFunc)(PuglView* view);

/**
   A function called when a key is pressed or released.
   @param view The view the event occured in.
   @param press True if the key was pressed, false if released.
   @param key Unicode point of the key pressed.
   @return 0 if event was handled, otherwise send event to parent window.
*/
typedef int (*PuglKeyboardFunc)(PuglView* view, bool press, uint32_t key, uint32_t keycode);

/**
   A function called when the pointer moves.
   @param view The view the event occured in.
   @param x The window-relative x coordinate of the pointer.
   @param y The window-relative y coordinate of the pointer.
*/
typedef void (*PuglMotionFunc)(PuglView* view, int x, int y);

/**
   A function called when a mouse button is pressed or released.
   @param view The view the event occured in.
   @param button The button number (1 = left, 2 = middle, 3 = right).
   @param press True if the key was pressed, false if released.
   @param x The window-relative x coordinate of the pointer.
   @param y The window-relative y coordinate of the pointer.
*/
typedef void (*PuglMouseFunc)(
	PuglView* view, int button, bool press, int x, int y);

/**
   A function called when the view is resized.
   @param view The view being resized.
   @param width The new view width.
   @param height The new view height.
*/
typedef void (*PuglReshapeFunc)(PuglView* view, int width, int height);

/**
   A function called outside of gl-context when the plugin schedules a resize via puglPostResize.

   @param view The view being resized.
   @param width The new width to resize to (variable is initialized to current size)
   @param height The new height to resize to (variable is initialized to current size)
   @param set_hints If not null, set window-hints
 */
typedef void (*PuglResizeFunc)(PuglView* view, int *width, int *height, int *set_hints);

/**
   A function called on scrolling (e.g. mouse wheel or track pad).

   The distances used here are in "lines", a single tick of a clicking mouse
   wheel.  For example, @p dy = 1.0 scrolls 1 line up.  Some systems and
   devices support finer resolution and/or higher values for fast scrolls,
   so programs should handle any value gracefully.

   @param view The view being scrolled.
   @param x The window-relative x coordinate of the pointer.
   @param y The window-relative y coordinate of the pointer.
   @param dx The scroll x distance.
   @param dx The scroll y distance.
*/
typedef void (*PuglScrollFunc)(PuglView* view, int x, int y, float dx, float dy);

/**
   A function called when a special key is pressed or released.

   This callback allows the use of keys that do not have unicode points.
   Note that some are non-printable keys.

   @param view The view the event occured in.
   @param press True if the key was pressed, false if released.
   @param key The key pressed.
   @return 0 if event was handled, otherwise send event to parent window.
*/
typedef int (*PuglSpecialFunc)(PuglView* view, bool press, PuglKey key);

/**
   A function called when a filename is selected via file-browser.

   @param view The view the event occured in.
   @param filename The selected file name or NULL if the dialog was canceled.
*/
typedef void (*PuglFileSelectedFunc)(PuglView* view, const char* filename);

/**
   @name Initialization
   Configuration functions which must be called before creating a window.
   @{
*/

/**
   Create a Pugl context.

   To create a window, call the various puglInit* functions as necessary, then
   call puglCreateWindow().
*/
PuglView*
puglInit(void);

/**
   Set the parent window before creating a window (for embedding).
*/
void
puglInitWindowParent(PuglView* view, PuglNativeWindow parent);

/**
   Set the window size before creating a window.
*/
void
puglInitWindowSize(PuglView* view, int width, int height);

/**
   Set the minimum window size before creating a window.
*/
void
puglInitWindowMinSize(PuglView* view, int width, int height);

/**
   Enable or disable resizing before creating a window.
*/
void
puglInitUserResizable(PuglView* view, bool resizable);

/**
   Set transient parent before creating a window.

   On X11, parent_id must be a Window.
   On OSX, parent_id must be an NSView*.
*/
void
puglInitTransientFor(PuglView* view, uintptr_t parent);

/**
   @}
*/

/**
   @name Windows
   Window management functions.
   @{
*/

/**
   Create a window with the settings given by the various puglInit functions.

   @return 1 (pugl does not currently support multiple windows).
*/
int
puglCreateWindow(PuglView* view, const char* title);

/**
   Create a new GL window.
   @param parent Parent window, or 0 for top level.
   @param title Window title, or NULL.
   @param width Window width in pixels.
   @param height Window height in pixels.
   @param resizable Whether window should be user resizable.
*/
PuglView*
puglCreate(PuglNativeWindow parent,
           const char*      title,
           int              min_width,
           int              min_height,
           int              width,
           int              height,
           bool             resizable,
           unsigned long    transientId);

/**
   Show Window (external ui)
*/
void
puglShowWindow(PuglView* view);

/**
   Hide Window (external ui)
*/
void
puglHideWindow(PuglView* view);

/**
   Return the native window handle.
*/
PuglNativeWindow
puglGetNativeWindow(PuglView* view);

/**
   @}
*/

/**
   Set the handle to be passed to all callbacks.

   This is generally a pointer to a struct which contains all necessary state.
   Everything needed in callbacks should be here, not in static variables.

   Note the lack of this facility makes GLUT unsuitable for plugins or
   non-trivial programs; this mistake is largely why Pugl exists.
*/
void
puglSetHandle(PuglView* view, PuglHandle handle);

/**
   Get the handle to be passed to all callbacks.
*/
PuglHandle
puglGetHandle(PuglView* view);

/**
   Get the drawing context.
   For Cairo contexts, this returns a pointer to a cairo_t.
   For everything else, this is unused and returns NULL.
*/
void*
puglGetContext(PuglView* view);

/**
   Return the timestamp (if any) of the currently-processing event.
*/
uint32_t
puglGetEventTimestamp(PuglView* view);

/**
   Get the currently active modifiers (PuglMod flags).

   This should only be called from an event handler.
*/
int
puglGetModifiers(PuglView* view);

/**
   Ignore synthetic repeated key events.
*/
void
puglIgnoreKeyRepeat(PuglView* view, bool ignore);

/**
   @name Event Callbacks
   Functions to set event callbacks for handling user input.
   @{
*/

/**
   Set the function to call when the window is closed.
*/
void
puglSetCloseFunc(PuglView* view, PuglCloseFunc closeFunc);

/**
   Set the display function which should draw the UI using GL.
*/
void
puglSetDisplayFunc(PuglView* view, PuglDisplayFunc displayFunc);

/**
   Set the function to call on keyboard events.
*/
void
puglSetKeyboardFunc(PuglView* view, PuglKeyboardFunc keyboardFunc);

/**
   Set the function to call on mouse motion.
*/
void
puglSetMotionFunc(PuglView* view, PuglMotionFunc motionFunc);

/**
   Set the function to call on mouse button events.
*/
void
puglSetMouseFunc(PuglView* view, PuglMouseFunc mouseFunc);

/**
   Set the function to call on scroll events.
*/
void
puglSetScrollFunc(PuglView* view, PuglScrollFunc scrollFunc);

/**
   Set the function to call on special events.
*/
void
puglSetSpecialFunc(PuglView* view, PuglSpecialFunc specialFunc);

/**
   Set the function to call when the window size changes.
*/
void
puglSetReshapeFunc(PuglView* view, PuglReshapeFunc reshapeFunc);

/**
   Set callback function to change window size.
*/
void
puglSetResizeFunc(PuglView* view, PuglResizeFunc resizeFunc);

/**
   Set the function to call on file-browser selections.
*/
void
puglSetFileSelectedFunc(PuglView* view, PuglFileSelectedFunc fileSelectedFunc);

/**
   @}
*/

/**
   TODO document this.
 */
int
puglUpdateGeometryConstraints(PuglView* view, int min_width, int min_height, bool aspect);

/**
   Grab the input focus.
*/
void
puglGrabFocus(PuglView* view);

/**
   Process all pending window events.

   This handles input events as well as rendering, so it should be called
   regularly and rapidly enough to keep the UI responsive.
*/
PuglStatus
puglProcessEvents(PuglView* view);

/**
   Request a redisplay on the next call to puglProcessEvents().
*/
void
puglPostRedisplay(PuglView* view);

/**
   Request a resize on the next call to puglProcessEvents().
*/
void
puglPostResize(PuglView* view);

/**
   Destroy a GL window.
*/
void
puglDestroy(PuglView* view);

/**
   @}
*/

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* PUGL_H_INCLUDED */
