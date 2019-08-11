/*
  Copyright 2012-2014 David Robillard <http://drobilla.net>
  Copyright 2011-2012 Ben Loftis, Harrison Consoles
  Copyright 2013,2015 Robin Gareus <robin@gareus.org>
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
   @file pugl_x11.c X11 Pugl Implementation.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef PUGL_CAIRO
#include <cairo/cairo.h>
#include <cairo/cairo-xlib.h>
#endif
#ifdef PUGL_OPENGL
#include <GL/gl.h>
#include <GL/glx.h>
#endif
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include "pugl_internal.h"

#ifndef DGL_FILE_BROWSER_DISABLED
#define SOFD_HAVE_X11
#include "../sofd/libsofd.h"
#include "../sofd/libsofd.c"
#endif

/* work around buggy re-parent & focus issues on some systems
 * where no keyboard events are passed through even if the
 * app has mouse-focus and all other events are working.
 */
//#define PUGL_GRAB_FOCUS

/* show messages during initalization
 */
//#define PUGL_VERBOSE

struct PuglInternalsImpl {
	Display*   display;
	int        screen;
	Window     win;
#ifdef PUGL_CAIRO
	cairo_t*         xlib_cr;
	cairo_t*         buffer_cr;
	cairo_surface_t* xlib_surface;
	cairo_surface_t* buffer_surface;
#endif
#ifdef PUGL_OPENGL
	GLXContext ctx;
	Bool       doubleBuffered;
#endif
};

#ifdef PUGL_OPENGL
/**
   Attributes for single-buffered RGBA with at least
   4 bits per color and a 16 bit depth buffer.
*/
static int attrListSgl[] = {
	GLX_RGBA,
	GLX_RED_SIZE, 4,
	GLX_GREEN_SIZE, 4,
	GLX_BLUE_SIZE, 4,
	GLX_DEPTH_SIZE, 16,
	GLX_ARB_multisample, 1,
	None
};

/**
   Attributes for double-buffered RGBA with at least
   4 bits per color and a 16 bit depth buffer.
*/
static int attrListDbl[] = {
	GLX_RGBA,
	GLX_DOUBLEBUFFER, True,
	GLX_RED_SIZE, 4,
	GLX_GREEN_SIZE, 4,
	GLX_BLUE_SIZE, 4,
	GLX_DEPTH_SIZE, 16,
	GLX_ARB_multisample, 1,
	None
};

/**
   Attributes for double-buffered RGBA with multi-sampling
   (antialiasing)
*/
static int attrListDblMS[] = {
	GLX_RGBA,
	GLX_DOUBLEBUFFER, True,
	GLX_RED_SIZE, 4,
	GLX_GREEN_SIZE, 4,
	GLX_BLUE_SIZE, 4,
	GLX_ALPHA_SIZE, 4,
	GLX_DEPTH_SIZE, 16,
	GLX_SAMPLE_BUFFERS, 1,
	GLX_SAMPLES, 4,
	None
};
#endif

PuglInternals*
puglInitInternals(void)
{
	return (PuglInternals*)calloc(1, sizeof(PuglInternals));
}

void
puglEnterContext(PuglView* view)
{
#ifdef PUGL_OPENGL
	glXMakeCurrent(view->impl->display, view->impl->win, view->impl->ctx);
#endif
}

void
puglLeaveContext(PuglView* view, bool flush)
{
#ifdef PUGL_OPENGL
	if (flush) {
		glFlush();
		if (view->impl->doubleBuffered) {
			glXSwapBuffers(view->impl->display, view->impl->win);
		}
	}
	glXMakeCurrent(view->impl->display, None, NULL);
#endif
}

int
puglCreateWindow(PuglView* view, const char* title)
{
	PuglInternals* impl = view->impl;
	if (!impl) {
		return 1;
	}

	view->impl = impl;
	impl->display = XOpenDisplay(NULL);
	if (!impl->display) {
		free(impl);
		return 1;
	}
	impl->screen = DefaultScreen(impl->display);

	XVisualInfo*         vi   = NULL;

#ifdef PUGL_OPENGL
	impl->doubleBuffered = True;
	vi = glXChooseVisual(impl->display, impl->screen, attrListDblMS);

	if (!vi) {
		vi = glXChooseVisual(impl->display, impl->screen, attrListDbl);
#ifdef PUGL_VERBOSE
		printf("puGL: multisampling (antialiasing) is not available\n");
#endif
	}

	if (!vi) {
		vi = glXChooseVisual(impl->display, impl->screen, attrListSgl);
		impl->doubleBuffered = False;
	}
#endif
#ifdef PUGL_CAIRO
	XVisualInfo pat;
	int         n;
	pat.screen = impl->screen;
	vi         = XGetVisualInfo(impl->display, VisualScreenMask, &pat, &n);
#endif

	if (!vi) {
		XCloseDisplay(impl->display);
		free(impl);
		return 1;
	}

#ifdef PUGL_VERBOSE
#ifdef PUGL_OPENGL
	int glxMajor, glxMinor;
	glXQueryVersion(impl->display, &glxMajor, &glxMinor);
	printf("puGL: GLX-Version : %d.%d\n", glxMajor, glxMinor);
#endif
#endif

#ifdef PUGL_OPENGL
	impl->ctx = glXCreateContext(impl->display, vi, 0, GL_TRUE);

	if (!impl->ctx) {
		XFree(vi);
		XCloseDisplay(impl->display);
		free(impl);
		return 1;
	}
#endif

	Window xParent = view->parent
		? (Window)view->parent
		: RootWindow(impl->display, impl->screen);

	Colormap cmap = XCreateColormap(
		impl->display, xParent, vi->visual, AllocNone);

	XSetWindowAttributes attr;
	memset(&attr, 0, sizeof(XSetWindowAttributes));
	attr.border_pixel = BlackPixel(impl->display, impl->screen);
	attr.colormap     = cmap;
	attr.event_mask   = (ExposureMask | StructureNotifyMask |
	                     EnterWindowMask | LeaveWindowMask |
	                     KeyPressMask | KeyReleaseMask |
	                     ButtonPressMask | ButtonReleaseMask |
	                     PointerMotionMask | FocusChangeMask);

	impl->win = XCreateWindow(
		impl->display, xParent,
		0, 0, view->width, view->height, 0, vi->depth, InputOutput, vi->visual,
		CWBorderPixel | CWColormap | CWEventMask, &attr);

	if (!impl->win) {
#ifdef PUGL_OPENGL
		glXDestroyContext(impl->display, impl->ctx);
#endif
		XFree(vi);
		XCloseDisplay(impl->display);
		free(impl);
		return 1;
	}

#ifdef PUGL_CAIRO
	impl->xlib_surface = cairo_xlib_surface_create(
		impl->display, impl->win, vi->visual, view->width, view->height);
	if (impl->xlib_surface == NULL || cairo_surface_status(impl->xlib_surface) != CAIRO_STATUS_SUCCESS) {
		printf("puGL: failed to create cairo surface\n");
	}
	else {
		impl->xlib_cr = cairo_create(impl->xlib_surface);
	}
	if (impl->xlib_cr == NULL || cairo_status(impl->xlib_cr) != CAIRO_STATUS_SUCCESS) {
		cairo_destroy(impl->xlib_cr);
		cairo_surface_destroy(impl->xlib_surface);
		XDestroyWindow(impl->display, impl->win);
		XFree(vi);
		XCloseDisplay(impl->display);
		free(impl);
		printf("puGL: failed to create cairo context\n");
		return 1;
	}
#endif

	if (view->width > 1 || view->height > 1) {
		puglUpdateGeometryConstraints(view, view->min_width, view->min_height, view->min_width != view->width);
		XResizeWindow(view->impl->display, view->impl->win, view->width, view->height);
	}

	if (title) {
		XStoreName(impl->display, impl->win, title);
		Atom netWmName = XInternAtom(impl->display, "_NET_WM_NAME", False);
		Atom utf8String = XInternAtom(impl->display, "UTF8_STRING", False);
		XChangeProperty(impl->display, impl->win, netWmName, utf8String, 8, PropModeReplace, (unsigned char *)title, strlen(title));
	}

	if (view->transient_parent > 0) {
		XSetTransientForHint(impl->display, impl->win, (Window)view->transient_parent);
	}

	if (view->parent) {
		XMapRaised(impl->display, impl->win);
	} else {
		Atom wmDelete = XInternAtom(impl->display, "WM_DELETE_WINDOW", True);
		XSetWMProtocols(impl->display, impl->win, &wmDelete, 1);
	}

#ifdef PUGL_VERBOSE
#ifdef PUGL_OPENGL
	if (glXIsDirect(impl->display, impl->ctx)) {
		printf("puGL: DRI enabled (to disable, set LIBGL_ALWAYS_INDIRECT=1\n");
	} else {
		printf("puGL: No DRI available\n");
	}
#endif
#endif

	XFree(vi);
	return 0;
}

void
puglDestroy(PuglView* view)
{
	if (!view) {
		return;
	}

	PuglInternals* const impl = view->impl;

#ifndef DGL_FILE_BROWSER_DISABLED
	x_fib_close(impl->display);
#endif

#ifdef PUGL_OPENGL
	glXDestroyContext(impl->display, impl->ctx);
#endif
#ifdef PUGL_CAIRO
	cairo_destroy(impl->xlib_cr);
	cairo_destroy(impl->buffer_cr);
	cairo_surface_destroy(impl->xlib_surface);
	cairo_surface_destroy(impl->buffer_surface);
#endif
	XDestroyWindow(impl->display, impl->win);
	XCloseDisplay(impl->display);
	free(impl);
	free(view);
}

void
puglShowWindow(PuglView* view)
{
	XMapRaised(view->impl->display, view->impl->win);
}

void
puglHideWindow(PuglView* view)
{
	XUnmapWindow(view->impl->display, view->impl->win);
}

static void
puglReshape(PuglView* view, int width, int height)
{
	puglEnterContext(view);

	if (view->reshapeFunc) {
		view->reshapeFunc(view, width, height);
	} else {
		puglDefaultReshape(width, height);
	}

	puglLeaveContext(view, false);

	view->width  = width;
	view->height = height;
}

static void
puglDisplay(PuglView* view)
{
	PuglInternals* impl = view->impl;

	puglEnterContext(view);

#ifdef PUGL_CAIRO
	cairo_t* bc = impl->buffer_cr;
	cairo_surface_t* xs = impl->xlib_surface;
	cairo_surface_t* bs = impl->buffer_surface;
	int w = cairo_xlib_surface_get_width(xs);
	int h = cairo_xlib_surface_get_height(xs);

	int bw = bs ? cairo_image_surface_get_width(bs) : -1;
	int bh = bs ? cairo_image_surface_get_height(bs) : -1;
	if (!bc || bw != w || bh != h) {
		cairo_destroy(bc);
		cairo_surface_destroy(bs);
		bs = cairo_surface_create_similar_image(xs, CAIRO_FORMAT_ARGB32, w, h);
		bc = bs ? cairo_create(bs) : NULL;
		impl->buffer_cr = bc;
		impl->buffer_surface = bs;
	}

	if (!bc) {
		puglLeaveContext(view, false);
		return;
	}
#endif

	view->redisplay = false;
	if (view->displayFunc) {
		view->displayFunc(view);
	}

#ifdef PUGL_CAIRO
	cairo_t* xc = impl->xlib_cr;
	cairo_set_source_surface(xc, impl->buffer_surface, 0, 0);
	cairo_paint(xc);
#endif

	puglLeaveContext(view, true);
	(void)impl;
}

static void
puglResize(PuglView* view)
{
	int set_hints = 1;
	view->pending_resize = false;
	if (!view->resizeFunc) { return; }
	/* ask the plugin about the new size */
	view->resizeFunc(view, &view->width, &view->height, &set_hints);

	if (set_hints) {
		XSizeHints sizeHints;
		memset(&sizeHints, 0, sizeof(sizeHints));
		sizeHints.flags      = PMinSize|PMaxSize;
		sizeHints.min_width  = view->width;
		sizeHints.min_height = view->height;
		sizeHints.max_width  = view->user_resizable ? 4096 : view->width;
		sizeHints.max_height = view->user_resizable ? 4096 : view->height;
		XSetWMNormalHints(view->impl->display, view->impl->win, &sizeHints);
	}
	XResizeWindow(view->impl->display, view->impl->win, view->width, view->height);
	XFlush(view->impl->display);

#ifdef PUGL_VERBOSE
	printf("puGL: window resize (%dx%d)\n", view->width, view->height);
#endif

	/* and call Reshape in glX context */
	puglReshape(view, view->width, view->height);
}

static PuglKey
keySymToSpecial(KeySym sym)
{
	switch (sym) {
	case XK_F1:        return PUGL_KEY_F1;
	case XK_F2:        return PUGL_KEY_F2;
	case XK_F3:        return PUGL_KEY_F3;
	case XK_F4:        return PUGL_KEY_F4;
	case XK_F5:        return PUGL_KEY_F5;
	case XK_F6:        return PUGL_KEY_F6;
	case XK_F7:        return PUGL_KEY_F7;
	case XK_F8:        return PUGL_KEY_F8;
	case XK_F9:        return PUGL_KEY_F9;
	case XK_F10:       return PUGL_KEY_F10;
	case XK_F11:       return PUGL_KEY_F11;
	case XK_F12:       return PUGL_KEY_F12;
	case XK_Left:      return PUGL_KEY_LEFT;
	case XK_Up:        return PUGL_KEY_UP;
	case XK_Right:     return PUGL_KEY_RIGHT;
	case XK_Down:      return PUGL_KEY_DOWN;
	case XK_Page_Up:   return PUGL_KEY_PAGE_UP;
	case XK_Page_Down: return PUGL_KEY_PAGE_DOWN;
	case XK_Home:      return PUGL_KEY_HOME;
	case XK_End:       return PUGL_KEY_END;
	case XK_Insert:    return PUGL_KEY_INSERT;
	case XK_Shift_L:   return PUGL_KEY_SHIFT;
	case XK_Shift_R:   return PUGL_KEY_SHIFT;
	case XK_Control_L: return PUGL_KEY_CTRL;
	case XK_Control_R: return PUGL_KEY_CTRL;
	case XK_Alt_L:     return PUGL_KEY_ALT;
	case XK_Alt_R:     return PUGL_KEY_ALT;
	case XK_Super_L:   return PUGL_KEY_SUPER;
	case XK_Super_R:   return PUGL_KEY_SUPER;
	}
	return (PuglKey)0;
}

static void
setModifiers(PuglView* view, unsigned xstate, unsigned xtime)
{
	view->event_timestamp_ms = xtime;

	view->mods = 0;
	view->mods |= (xstate & ShiftMask)   ? PUGL_MOD_SHIFT  : 0;
	view->mods |= (xstate & ControlMask) ? PUGL_MOD_CTRL   : 0;
	view->mods |= (xstate & Mod1Mask)    ? PUGL_MOD_ALT    : 0;
	view->mods |= (xstate & Mod4Mask)    ? PUGL_MOD_SUPER  : 0;
}

static void
dispatchKey(PuglView* view, XEvent* event, bool press)
{
	KeySym    sym;
	char      str[5];
	PuglKey   special;
	const int n = XLookupString(&event->xkey, str, 4, &sym, NULL);

	if (sym == XK_Escape && view->closeFunc && !press && !view->parent) {
		view->closeFunc(view);
		view->redisplay = false;
		return;
	}
	if (n == 0 && sym == 0) {
		goto send_event;
		return;
	}
	if (n > 1) {
		fprintf(stderr, "warning: Unsupported multi-byte key %X\n", (int)sym);
		goto send_event;
		return;
	}

	special = keySymToSpecial(sym);
	if (special && view->specialFunc) {
		if (view->specialFunc(view, press, special) == 0) {
			return;
		}
	} else if (!special && view->keyboardFunc) {
		if (view->keyboardFunc(view, press, str[0]) == 0) {
			return;
		}
	}

send_event:
	if (view->parent != 0) {
		event->xkey.time   = 0; // purposefully set an invalid time, used for feedback detection on bad hosts
		event->xany.window = view->parent;
		XSendEvent(view->impl->display, view->parent, False, NoEventMask, event);
	}
}

PuglStatus
puglProcessEvents(PuglView* view)
{
	int conf_width = -1;
	int conf_height = -1;

	XEvent event;
	while (XPending(view->impl->display) > 0) {
		XNextEvent(view->impl->display, &event);

#ifndef DGL_FILE_BROWSER_DISABLED
		if (x_fib_handle_events(view->impl->display, &event)) {
			const int status = x_fib_status();

			if (status > 0) {
				char* const filename = x_fib_filename();
				x_fib_close(view->impl->display);
				if (view->fileSelectedFunc) {
					view->fileSelectedFunc(view, filename);
				}
				free(filename);
			} else if (status < 0) {
				x_fib_close(view->impl->display);
				if (view->fileSelectedFunc) {
					view->fileSelectedFunc(view, NULL);
				}
			}
			break;
		}
#endif

		if (event.xany.window != view->impl->win &&
			(view->parent == 0 || event.xany.window != (Window)view->parent)) {
			continue;
		}
		if ((event.type == KeyPress || event.type == KeyRelease) && event.xkey.time == 0) {
			continue;
		}

		switch (event.type) {
		case UnmapNotify:
			if (view->motionFunc) {
				view->motionFunc(view, -1, -1);
			}
			break;
		case MapNotify:
			puglReshape(view, view->width, view->height);
			break;
		case ConfigureNotify:
			if ((event.xconfigure.width != view->width) ||
			    (event.xconfigure.height != view->height)) {
				conf_width = event.xconfigure.width;
				conf_height = event.xconfigure.height;
			}
			break;
		case Expose:
			if (event.xexpose.count != 0) {
				break;
			}
			view->redisplay = true;
			break;
		case MotionNotify:
			setModifiers(view, event.xmotion.state, event.xmotion.time);
			if (view->motionFunc) {
				view->motionFunc(view, event.xmotion.x, event.xmotion.y);
			}
			break;
		case ButtonPress:
			setModifiers(view, event.xbutton.state, event.xbutton.time);
			if (event.xbutton.button >= 4 && event.xbutton.button <= 7) {
				if (view->scrollFunc) {
					float dx = 0, dy = 0;
					switch (event.xbutton.button) {
					case 4: dy =  1.0f; break;
					case 5: dy = -1.0f; break;
					case 6: dx = -1.0f; break;
					case 7: dx =  1.0f; break;
					}
					view->scrollFunc(view, event.xbutton.x, event.xbutton.y, dx, dy);
				}
				break;
			}
			// nobreak
		case ButtonRelease:
			setModifiers(view, event.xbutton.state, event.xbutton.time);
			if (view->mouseFunc &&
			    (event.xbutton.button < 4 || event.xbutton.button > 7)) {
				view->mouseFunc(view,
				                event.xbutton.button, event.type == ButtonPress,
				                event.xbutton.x, event.xbutton.y);
			}
			break;
		case KeyPress:
			setModifiers(view, event.xkey.state, event.xkey.time);
			dispatchKey(view, &event, true);
			break;
		case KeyRelease: {
			setModifiers(view, event.xkey.state, event.xkey.time);
			bool repeated = false;
			if (view->ignoreKeyRepeat &&
			    XEventsQueued(view->impl->display, QueuedAfterReading)) {
				XEvent next;
				XPeekEvent(view->impl->display, &next);
				if (next.type == KeyPress &&
				    next.xkey.time == event.xkey.time &&
				    next.xkey.keycode == event.xkey.keycode) {
					XNextEvent(view->impl->display, &event);
					repeated = true;
				}
			}
			if (!repeated) {
				dispatchKey(view, &event, false);
			}
		}	break;
		case ClientMessage: {
			char* type = XGetAtomName(view->impl->display,
			                          event.xclient.message_type);
			if (!strcmp(type, "WM_PROTOCOLS")) {
				if (view->closeFunc) {
					view->closeFunc(view);
					view->redisplay = false;
				}
			}
			XFree(type);
		}	break;
#ifdef PUGL_GRAB_FOCUS
		case EnterNotify:
			XSetInputFocus(view->impl->display, view->impl->win, RevertToPointerRoot, CurrentTime);
			break;
#endif
		default:
			break;
		}
	}

	if (conf_width != -1) {
#ifdef PUGL_CAIRO
		// Resize surfaces/contexts before dispatching
		view->redisplay = true;
		cairo_xlib_surface_set_size(view->impl->xlib_surface,
		                            conf_width, conf_height);
#endif
		puglReshape(view, conf_width, conf_height);
	}

	if (view->pending_resize) {
		puglResize(view);
	}

	if (view->redisplay) {
		puglDisplay(view);
	}

	return PUGL_SUCCESS;
}

void
puglPostRedisplay(PuglView* view)
{
	view->redisplay = true;
}

void
puglPostResize(PuglView* view)
{
	view->pending_resize = true;
}

PuglNativeWindow
puglGetNativeWindow(PuglView* view)
{
	return view->impl->win;
}

void*
puglGetContext(PuglView* view)
{
#ifdef PUGL_CAIRO
	return view->impl->buffer_cr;
#endif
	return NULL;

	// may be unused
	(void)view;
}

int
puglUpdateGeometryConstraints(PuglView* view, int min_width, int min_height, bool aspect)
{
	XSizeHints sizeHints;
	memset(&sizeHints, 0, sizeof(sizeHints));
	sizeHints.flags      = PMinSize|PMaxSize;
	sizeHints.min_width  = min_width;
	sizeHints.min_height = min_height;
	sizeHints.max_width  = view->user_resizable ? 4096 : min_width;
	sizeHints.max_height = view->user_resizable ? 4096 : min_height;
	if (aspect) {
		sizeHints.flags |= PAspect;
		sizeHints.min_aspect.x = min_width;
		sizeHints.min_aspect.y = min_height;
		sizeHints.max_aspect.x = min_width;
		sizeHints.max_aspect.y = min_height;
	}
	XSetWMNormalHints(view->impl->display, view->impl->win, &sizeHints);
	return 0;
}
