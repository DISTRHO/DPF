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
# define DISTRHO_WEB_VIEW_INCLUDE_IMPLEMENTATION
# include "WebView.hpp"
#endif

// --------------------------------------------------------------------------------------------------------------------
// Web View stuff

START_NAMESPACE_DISTRHO

class WebView;

WebView* webview_choc_create(const WebViewOptions& opts);
void webview_choc_destroy(WebView*);
void* webview_choc_handle(WebView*);
void webview_choc_eval(WebView*, const char* js);
void webview_choc_navigate(WebView*, const char* url);

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

#ifdef DISTRHO_WEB_VIEW_INCLUDE_IMPLEMENTATION

# define WC_ERR_INVALID_CHARS 0
# include "choc/choc_WebView.h"

START_NAMESPACE_DISTRHO

WebView* webview_choc_create(const WebViewOptions& opts)
{
    WebView::Options wopts;
    wopts.acceptsFirstMouseClick = true;
    wopts.enableDebugMode = true;

    std::unique_ptr<WebView> webview = std::make_unique<WebView>(wopts);
    DISTRHO_SAFE_ASSERT_RETURN(webview->loadedOK(), nullptr);

    if (const WebViewMessageCallback callback = opts.callback)
    {
        webview->addInitScript("function postMessage(m){window.chrome.webview.postMessage(m);}");

        void* const callbackPtr = opts.callbackPtr;
        webview->bind([callback, callbackPtr](const std::string& value) {
            char* const data = strdup(value.data());
            callback(callbackPtr, data);
            std::free(data);
        });
    }
    else
    {
        webview->addInitScript("function postMessage(m){}");
    }

    if (opts.initialJS != nullptr)
        webview->addInitScript(opts.initialJS);

    return webview.release();
}

void webview_choc_destroy(WebView* const webview)
{
    delete webview;
}

void* webview_choc_handle(WebView* const webview)
{
    return webview->getViewHandle();
}

void webview_choc_eval(WebView* const webview, const char* const js)
{
    webview->evaluateJavascript(js);
}

void webview_choc_navigate(WebView* const webview, const char* const url)
{
    webview->navigate(url);
}

END_NAMESPACE_DISTRHO

#endif // DISTRHO_WEB_VIEW_INCLUDE_IMPLEMENTATION

// --------------------------------------------------------------------------------------------------------------------
