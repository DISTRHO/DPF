// Copyright 2012-2022 David Robillard <d@drobilla.net>
// Copyright 2021-2022 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#include "haiku.h"

#include "../pugl-upstream/src/internal.h"

class PuglHaikuView : public BView
{
    PuglView* const view;

public:
    PuglHaikuView(PuglView* const v)
        : BView(NULL, B_FULL_UPDATE_ON_RESIZE|B_FRAME_EVENTS|B_NAVIGABLE|B_INPUT_METHOD_AWARE),
          view(v) {}

protected:
    void GetPreferredSize(float* width, float* height) override
    {
        d_stdout("%s %i", __func__, __LINE__);
        if (width != nullptr)
            *width = view->frame.width;
        if (height != nullptr)
            *height = view->frame.height;
        d_stdout("%s %i", __func__, __LINE__);
    }
};

class PuglHaikuWindow : public BWindow
{
    PuglView* const view;

public:
    PuglHaikuWindow(PuglView* const v)
        : BWindow(BRect(1.0f), "DPF-Window", B_TITLED_WINDOW, 0x0),
          view(v) {}

// protected:
//     bool QuitRequested() override
//     {
//         d_stdout("%s %i", __func__, __LINE__);
//         if (puglView->closeFunc) {
//             puglView->closeFunc(puglView);
//             puglView->redisplay = false;
//         }
//         needsQuit = false;
//         d_stdout("%s %i", __func__, __LINE__);
//         return true;
//     }
};

BApplication* s_app = NULL;

PuglWorldInternals*
puglInitWorldInternals(const PuglWorldType type, const PuglWorldFlags flags)
{
  if (!s_app) {
    status_t status;
    s_app = new BApplication("application/x-vnd.pugl-application", &status);
    
    if (status != B_OK) {
      delete s_app;
      return NULL;
    }
  }

  PuglWorldInternals* impl =
    (PuglWorldInternals*)calloc(1, sizeof(PuglWorldInternals));

  impl->app = s_app;
  return impl;
}

void*
puglGetNativeWorld(PuglWorld* const world)
{
  return world->impl->app;
}

PuglInternals*
puglInitViewInternals(PuglWorld* const world)
{
  PuglInternals* impl = (PuglInternals*)calloc(1, sizeof(PuglInternals));

  return impl;
}

PuglStatus
puglRealize(PuglView* const view)
{
  PuglInternals* const impl    = view->impl;
  PuglStatus           st      = PUGL_SUCCESS;

  // Ensure that we're unrealized and that a reasonable backend has been set
  if (impl->view) {
    return PUGL_FAILURE;
  }
  if (!view->backend || !view->backend->configure) {
    return PUGL_BAD_BACKEND;
  }

  // Set the size to the default if it has not already been set
  if (view->frame.width <= 0.0 || view->frame.height <= 0.0) {
    const PuglViewSize defaultSize = view->sizeHints[PUGL_DEFAULT_SIZE];
    if (!defaultSize.width || !defaultSize.height) {
      return PUGL_BAD_CONFIGURATION;
    }

    view->frame.width  = defaultSize.width;
    view->frame.height = defaultSize.height;
  }

  // Center top-level windows if a position has not been set
  if (!view->parent && !view->frame.x && !view->frame.y) {
    // TODO
  }

  if (!view->parent) {
    impl->window = new PuglHaikuWindow(view);
    impl->window->Lock();
  }

  impl->view = new PuglHaikuView(view);

  if (view->parent) {
    BView* const pview = (BView*)view->parent;
    pview->AddChild(impl->view);
  } else {
    impl->window->AddChild(impl->view);
  }

  // Configure and create the backend
  if ((st = view->backend->configure(view)) || (st = view->backend->create(view))) {
    view->backend->destroy(view);
    return st;
  }

  if (view->title) {
    puglSetWindowTitle(view, view->title);
  }

  if (view->transientParent) {
    puglSetTransientParent(view, view->transientParent);
  }

  puglDispatchSimpleEvent(view, PUGL_CREATE);

  if (impl->window) {
    impl->window->Unlock();
  }

  return PUGL_SUCCESS;
}

PuglStatus
puglShow(PuglView* const view)
{
  PuglInternals* const impl = view->impl;
  if (impl->window) {
    impl->window->Show();
  } else {
    impl->view->Show();
  }
  return PUGL_SUCCESS;
}

PuglStatus
puglHide(PuglView* const view)
{
  PuglInternals* const impl = view->impl;
  if (impl->window) {
    impl->window->Hide();
  } else {
    impl->view->Hide();
  }
  return PUGL_SUCCESS;
}

void
puglFreeViewInternals(PuglView* const view)
{
  if (view && view->impl) {
    PuglInternals* const impl = view->impl;
    if (view->backend) {
      view->backend->destroy(view);
    }
    if (impl->view) {
      if (impl->window) {
        impl->window->RemoveChild(impl->view);
      }
      delete impl->view;
      delete impl->window;
    }
    free(impl);
  }
}

void
puglFreeWorldInternals(PuglWorld* const world)
{
//   if (world->impl->app != nullptr && world->impl->app->CountWindows() == 0) {
//     delete world->impl->app;
//     s_app = NULL;
//   }
  free(world->impl);
}

PuglStatus
puglGrabFocus(PuglView*)
{
  return PUGL_UNSUPPORTED;
}

double
puglGetScaleFactor(const PuglView* const view)
{
  return 1.0;
}

double
puglGetTime(const PuglWorld* const world)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ((double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0) -
         world->startTime;
}

PuglStatus
puglUpdate(PuglWorld* const world, const double timeout)
{
  return PUGL_UNSUPPORTED;
}

PuglStatus
puglPostRedisplay(PuglView* const view)
{
  return PUGL_UNSUPPORTED;
}

PuglStatus
puglPostRedisplayRect(PuglView* const view, const PuglRect rect)
{
  return PUGL_UNSUPPORTED;
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
  return PUGL_UNSUPPORTED;
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

PuglStatus
puglStartTimer(PuglView* const view, const uintptr_t id, const double timeout)
{
  return PUGL_UNSUPPORTED;
}

PuglStatus
puglStopTimer(PuglView* const view, const uintptr_t id)
{
  return PUGL_UNSUPPORTED;
}

PuglStatus
puglPaste(PuglView* const view)
{
  return PUGL_UNSUPPORTED;
}

PuglStatus
puglAcceptOffer(PuglView* const                 view,
                const PuglDataOfferEvent* const offer,
                const uint32_t                  typeIndex)
{
  return PUGL_UNSUPPORTED;
}

uint32_t
puglGetNumClipboardTypes(const PuglView* const view)
{
  return 0u;
}

const char*
puglGetClipboardType(const PuglView* const view, const uint32_t typeIndex)
{
  return NULL;
}

const void*
puglGetClipboard(PuglView* const view,
                 const uint32_t  typeIndex,
                 size_t* const   len)
{
  return NULL;
}

PuglStatus
puglSetClipboard(PuglView* const   view,
                 const char* const type,
                 const void* const data,
                 const size_t      len)
{
  return PUGL_FAILURE;
}

PuglStatus
puglSetCursor(PuglView* const view, const PuglCursor cursor)
{
  return PUGL_FAILURE;
}

PuglStatus
puglSetTransientParent(PuglView* const view, const PuglNativeView parent)
{
  return PUGL_FAILURE;
}

PuglStatus
puglSetPosition(PuglView* const view, const int x, const int y)
{
  return PUGL_FAILURE;
}

#if 0
PuglStatus
puglGrabFocus(PuglView* view)
{
    view->impl->bView->MakeFocus(true);
    return PUGL_SUCCESS;
}

// extras follow

void
puglRaiseWindow(PuglView* view)
{
}

void
puglSetWindowSize(PuglView* view, unsigned int width, unsigned int height)
{
    bView->ResizeTo(width, height);

    if (bWindow != nullptr && bWindow->LockLooper())
    {
        bWindow->MoveTo(50, 50);
        bWindow->ResizeTo(width, height);

        if (! forced)
            bWindow->Flush();

        bWindow->UnlockLooper();
    }
    // TODO resizable
}

void setVisible(const bool yesNo)
{
    if (bWindow != nullptr)
    {
        if (bWindow->LockLooper())
        {
            if (yesNo)
                bWindow->Show();
            else
                bWindow->Hide();

            // TODO use flush?
            bWindow->Sync();
            bWindow->UnlockLooper();
        }
    }
    else
    {
        if (yesNo)
            bView->Show();
        else
            bView->Hide();
    }
}
#endif
