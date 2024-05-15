/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2024 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#if !defined(DISTRHO_WEB_VIEW_HPP_INCLUDED) && !defined(DGL_WEB_VIEW_HPP_INCLUDED)
# error bad include
#endif

#if !defined(DGL_UI_USE_WEB_VIEW) && defined(DISTRHO_UI_WEB_VIEW) && DISTRHO_UI_WEB_VIEW == 0
# error To use WebViews in DPF plugins please set DISTRHO_UI_WEB_VIEW to 1
#endif

// --------------------------------------------------------------------------------------------------------------------
// Web View stuff

struct WebViewData;
typedef WebViewData* WebViewHandle;
typedef void (*WebViewMessageCallback)(void* arg, char* msg);

// --------------------------------------------------------------------------------------------------------------------

/**
  Web view options, for customizing web view details.
*/
struct WebViewOptions {
   /**
      Position offset, for cases of mixing regular widgets with web views.
    */
    struct PositionOffset {
      /** Horizontal offset, with scale factor pre-applied */
      int x;

      /** Vertical offset, with scale factor pre-applied */
      int y;

      /** Constructor for default values */
      PositionOffset() : x(0), y(0) {}
    } offset;

   /**
      Set some JavaScript to evalute on every new page load.
    */
    const char* initialJS;

   /**
      Message callback triggered from JavaScript code inside the WebView.
    */
    WebViewMessageCallback callback;
    void* callbackPtr;

    /** Constructor for default values */
    WebViewOptions()
      : offset(),
        initialJS(nullptr),
        callback(nullptr),
        callbackPtr(nullptr) {}

    /** Constructor providing a callback */
    WebViewOptions(const WebViewMessageCallback cb, void* const ptr)
      : offset(),
        initialJS(nullptr),
        callback(cb),
        callbackPtr(ptr) {}
};

// --------------------------------------------------------------------------------------------------------------------

/**
  Create a new web view.

  The web view will be added on top of an existing platform-specific view/window.
  This means it will draw on top of whatever is below it,
  something to take into consideration if mixing regular widgets with web views.

  Provided metrics must have scale factor pre-applied.

  @p windowId:    The native window id to attach this view to (X11 Window, HWND or NSView*)
  @p scaleFactor: Scale factor in use
  @p options:     Extra options, optional
*/
WebViewHandle webViewCreate(const char* url,
                            uintptr_t windowId,
                            uint initialWidth,
                            uint initialHeight,
                            double scaleFactor,
                            const WebViewOptions& options = WebViewOptions());

/**
  Destroy the web view, handle must not be used afterwards.
*/
void webViewDestroy(WebViewHandle webview);

/**
  Idle the web view, to be called on regular intervals.
  Can cause callbacks to trigger.
*/
void webViewIdle(WebViewHandle webview);

/**
  Evaluate/run JavaScript on the web view.
*/
void webViewEvaluateJS(WebViewHandle webview, const char* js);

/**
  Reload the web view current page.
*/
void webViewReload(WebViewHandle webview);

/**
  Resize the web view.
*/
void webViewResize(WebViewHandle webview, uint width, uint height, double scaleFactor);

// --------------------------------------------------------------------------------------------------------------------
