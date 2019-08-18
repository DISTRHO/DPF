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

#include <Application.h>
#include <interface/Window.h>

#ifdef PUGL_CAIRO
#include <cairo/cairo.h>
typedef BView BViewType;
#endif
#ifdef PUGL_OPENGL
#include <GL/gl.h>
#include <opengl/GLView.h>
typedef BGLView BViewType;
#endif

#include "pugl_internal.h"

class DWindow;

struct PuglInternalsImpl {
    BApplication* app;
    BViewType* view;
    DWindow* window;
};

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
	puglEnterContext(view);

	view->redisplay = false;
	if (view->displayFunc) {
		view->displayFunc(view);
	}

	puglLeaveContext(view, true);
}

void
puglEnterContext(PuglView* view)
{
	PuglInternals* impl = view->impl;

#ifdef PUGL_OPENGL
    // FIXME without the first unlock we freeze
    impl->view->UnlockGL();
    impl->view->LockGL();
#endif
}

void
puglLeaveContext(PuglView* view, bool flush)
{
	PuglInternals* impl = view->impl;

#ifdef PUGL_OPENGL
    if (flush)
       impl->view->SwapBuffers();

    impl->view->UnlockGL();
#endif
}

PuglInternals*
puglInitInternals()
{
	return (PuglInternals*)calloc(1, sizeof(PuglInternals));
}

class DView : public BViewType
{
public:
#ifdef PUGL_CAIRO
    DView(PuglView* const v)
        : BView(nullptr, 
                B_FULL_UPDATE_ON_RESIZE|B_WILL_DRAW|B_FRAME_EVENTS|B_NAVIGABLE|B_INPUT_METHOD_AWARE),
          puglView(v) {}
#endif

#ifdef PUGL_OPENGL
    DView(PuglView* const v)
        : BGLView(BRect(), // causes "bitmap bounds is much too large: BRect(0.0, 0.0, 4294967296.0, 4294967296.0)"
                  "DPF-GLView",
                  0x0, // resize mode
                  B_FULL_UPDATE_ON_RESIZE|B_WILL_DRAW|B_NAVIGABLE_JUMP|B_FRAME_EVENTS|B_NAVIGABLE|B_INPUT_METHOD_AWARE,
                  BGL_RGB|BGL_DOUBLE|BGL_ALPHA|BGL_DEPTH|BGL_STENCIL),
          puglView(v)
    {
    }
#endif

protected:
    void GetPreferredSize(float* width, float* height) override
    {
        d_stdout("%s %i", __func__, __LINE__);
        if (width != nullptr)
            *width = puglView->width;
        if (height != nullptr)
            *height = puglView->height;
        d_stdout("%s %i", __func__, __LINE__);
    }
    
    void Draw(BRect updateRect) override
    {
        d_stdout("%s %i", __func__, __LINE__);
        puglDisplay(puglView);
#ifdef PUGL_OPENGL
        BGLView::Draw(updateRect);
        d_stdout("%s %i", __func__, __LINE__);
#endif
    }

    void MessageReceived(BMessage* message)
    {
        d_stdout("MessageReceived %p", message);
        BViewType::MessageReceived(message);
    }

    void MouseDown(BPoint where) override
    {
        if (puglView->mouseFunc) {
			// puglView->event_timestamp_ms = GetMessageTime();
            d_stdout("MouseDown mask %u", EventMask());
            puglView->mouseFunc(puglView, 1, true, where.x, where.y);
            SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
        }
        //BViewType::MouseDown(where);
    }

    void MouseUp(BPoint where) override
    {
        if (puglView->mouseFunc) {
            d_stdout("MouseUp mask %u", EventMask());
			// puglView->event_timestamp_ms = GetMessageTime();
            puglView->mouseFunc(puglView, 1, false, where.x, where.y);
        }
        //BViewType::MouseUp(where);
    }

    void MouseMoved(BPoint where, uint32, const BMessage*) override
    {
		if (puglView->motionFunc) {
			// puglView->event_timestamp_ms = GetMessageTime();
			puglView->motionFunc(puglView, where.x, where.y);
		}
    }

    void KeyDown(const char* bytes, int32 numBytes) override
    {
        d_stdout("KeyDown %i", numBytes);
        if (numBytes != 1)
            return; // TODO
        
        if (puglView->keyboardFunc) {
            puglView->keyboardFunc(puglView, true, bytes[0]);
        }
    }

    void KeyUp(const char* bytes, int32 numBytes) override
    {
        d_stdout("KeyUp %i", numBytes);
        if (numBytes != 1)
            return; // TODO
        
        if (puglView->keyboardFunc) {
            puglView->keyboardFunc(puglView, false, bytes[0]);
        }
    }
    
    void ScrollTo(BPoint where) override
    {
            d_stdout("ScrollTo mask %u", EventMask());
        BViewType::ScrollTo(where);
    }

    void FrameResized(float newWidth, float newHeight) override
    {
        d_stdout("%s %i", __func__, __LINE__);
		puglReshape(puglView, static_cast<int>(newWidth), static_cast<int>(newHeight));
#ifdef PUGL_OPENGL
        BGLView::FrameResized(newWidth, newHeight);
#endif
        d_stdout("%s %i", __func__, __LINE__);
    }

private:
    PuglView* const puglView;
};

class DWindow : public BWindow
{
public:
    DWindow(PuglView* const v)
        : BWindow(BRect(1.0f), "DPF-Window", B_TITLED_WINDOW, 0x0),
          puglView(v),
          needsQuit(true)
    {
    }
    
    bool NeedsQuit() const
    {
        return needsQuit;
    }

protected:
    bool QuitRequested() override
    {
        d_stdout("%s %i", __func__, __LINE__);
		if (puglView->closeFunc) {
			puglView->closeFunc(puglView);
			puglView->redisplay = false;
		}
		needsQuit = false;
        d_stdout("%s %i", __func__, __LINE__);
		return true;
    }

private:
    PuglView* const puglView;
    bool needsQuit;
};

int
puglCreateWindow(PuglView* view, const char* title)
{
	PuglInternals* impl = view->impl;

    if (be_app == nullptr)
    {
        d_stdout("creating app");
        status_t status;
        BApplication* const app = new BApplication("application/x-vnd.dpf-application", &status);

        if (status != B_OK)
        {
            d_stdout("app status error %u", status);
            delete app;
            return 1;
        }

        impl->app = app;
    }
    else
    {
        d_stdout("using existing app");
    }
    
	if (view->parent == 0) {
        impl->window = new DWindow(view);
        impl->window->Lock();
    }

    impl->view = new DView(view);

	if (view->parent != 0) {
        BView* const pview = (BView*)view->parent;
        pview->AddChild(impl->view);
        impl->view->LockGL();
        return 0;
    }

	if (title != nullptr) {
        impl->window->SetTitle(title);
    }
    
    impl->window->AddChild(impl->view);
    impl->view->LockGL();
    //puglEnterContext(view);
    impl->window->Unlock();
    return 0;
}

void
puglShowWindow(PuglView* view)
{
	PuglInternals* impl = view->impl;

    if (impl->window != nullptr)
    {
        if (impl->window->LockLooper())
        {
            impl->window->Show();
            impl->window->UnlockLooper();
        }
    }
    else
    {
        impl->view->Show();
    }
}

void
puglHideWindow(PuglView* view)
{
	PuglInternals* impl = view->impl;

    if (impl->window != nullptr)
    {
        if (impl->window->LockLooper())
        {
            impl->window->Hide();
            impl->window->UnlockLooper();
        }
    }
    else
    {
        impl->view->Show();
    }
}

void
puglDestroy(PuglView* view)
{
	PuglInternals* impl = view->impl;

    if (impl->window != nullptr)
    {
        // impl->window->Lock();
        puglLeaveContext(view, false);
        impl->window->RemoveChild(impl->view);     
        // impl->window->Unlock();

        if (impl->window->NeedsQuit())
            impl->window->Quit();
    }

    delete impl->view;
    impl->view = nullptr;
    impl->window = nullptr;

    if (impl->app != nullptr && impl->app->CountWindows() == 0)
    {
        d_stdout("deleting app");
        delete impl->app;
        impl->app = nullptr;
    } else
        d_stdout("NOT deleting app");
}

PuglStatus
puglProcessEvents(PuglView* view)
{
	return PUGL_SUCCESS;
}

void
puglPostRedisplay(PuglView* view)
{
	PuglInternals* impl = view->impl;

	view->redisplay = true;

    if (impl->window != nullptr)
    {
        if (impl->window->LockLooper())
        {
            impl->view->Invalidate();
            impl->window->UnlockLooper();
        }
    }
    else
    {
        impl->view->Invalidate();
    }
}

PuglNativeWindow
puglGetNativeWindow(PuglView* view)
{
	PuglInternals* impl = view->impl;

#ifdef PUGL_OPENGL
    // return (PuglNativeWindow)impl->view->EmbeddedView();
#endif

	return (PuglNativeWindow)(BView*)impl->view;
}

void*
puglGetContext(PuglView* view)
{
	return NULL;
}

int
puglUpdateGeometryConstraints(PuglView* view, int min_width, int min_height, bool aspect)
{
	PuglInternals* impl = view->impl;

    d_stdout("puglUpdateGeometryConstraints %i %i %i %i", min_width, min_height, view->width, view->height);
    if (impl->window->LockLooper())
    {
        impl->window->SetSizeLimits(min_width,
                                    view->user_resizable ? 4096 : min_width,
                                    min_height,
                                    view->user_resizable ? 4096 : min_height);

        impl->window->UnlockLooper();
        return 0;
    }

    return 1;

    // TODO
	(void)aspect;
}
