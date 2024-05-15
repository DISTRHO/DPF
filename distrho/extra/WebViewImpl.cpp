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
#if !defined(WEB_VIEW_DISTRHO_NAMESPACE) && !defined(WEB_VIEW_DGL_NAMESPACE)
# error bad usage
#endif

// #include <gtk/gtk.h>
// #include <gtk/gtkx.h>
// #include <webkit2/webkit2.h>

#ifdef DISTRHO_OS_MAC
# define WEB_VIEW_USING_MACOS_WEBKIT 1
#else
# define WEB_VIEW_USING_MACOS_WEBKIT 0
#endif

#ifdef DISTRHO_OS_WINDOWS
# define WEB_VIEW_USING_CHOC 1
#else
# define WEB_VIEW_USING_CHOC 0
#endif

#if defined(HAVE_X11) && defined(DISTRHO_OS_LINUX)
# define WEB_VIEW_USING_X11_IPC 1
#else
# define WEB_VIEW_USING_X11_IPC 0
#endif

#if WEB_VIEW_USING_CHOC
# include "WebViewWin32.hpp"
# include "String.hpp"
#elif WEB_VIEW_USING_MACOS_WEBKIT
# include <Cocoa/Cocoa.h>
# include <WebKit/WebKit.h>
#elif WEB_VIEW_USING_X11_IPC
// #define QT_NO_VERSION_TAGGING
// #include <QtCore/QChar>
// #include <QtCore/QPoint>
// #include <QtCore/QSize>
// #undef signals
# include "ChildProcess.hpp"
# include "RingBuffer.hpp"
# include "String.hpp"
# include <clocale>
# include <cstdio>
# include <functional>
# include <dlfcn.h>
# include <fcntl.h>
# include <pthread.h>
# include <unistd.h>
# include <sys/mman.h>
# include <X11/Xlib.h>
# ifdef __linux__
#  include <syscall.h>
#  include <linux/futex.h>
#  include <linux/limits.h>
# else
#  include <semaphore.h>
# endif
#endif

// -----------------------------------------------------------------------------------------------------------

#if WEB_VIEW_USING_MACOS_WEBKIT

#define MACRO_NAME2(a, b, c) a ## b ## c
#define MACRO_NAME(a, b, c) MACRO_NAME2(a, b, c)

#define WEB_VIEW_DELEGATE_CLASS_NAME \
    MACRO_NAME(WebViewDelegate_, _, DISTRHO_NAMESPACE)

@interface WEB_VIEW_DELEGATE_CLASS_NAME : NSObject<WKNavigationDelegate, WKScriptMessageHandler, WKUIDelegate>
@end

@implementation WEB_VIEW_DELEGATE_CLASS_NAME {
@public
    WebViewMessageCallback callback;
    void* callbackPtr;
    bool loaded;
}

- (void)webView:(WKWebView *)webview
    didFinishNavigation:(WKNavigation*)navigation
{
    d_stdout("page loaded");
    loaded = true;
}

- (void)webView:(WKWebView*)webview
    runJavaScriptAlertPanelWithMessage:(NSString*)message
                      initiatedByFrame:(WKFrameInfo*)frame
                     completionHandler:(void (^)(void))completionHandler
{
    NSAlert* const alert = [[NSAlert alloc] init];
    [alert addButtonWithTitle:@"OK"];
    [alert setInformativeText:message];
    [alert setMessageText:@"Alert"];

    dispatch_async(dispatch_get_main_queue(), ^
    {
        [alert beginSheetModalForWindow:[webview window]
                      completionHandler:^(NSModalResponse)
        {
            completionHandler();
            [alert release];
        }];
    });
}

- (void)webView:(WKWebView*)webview
    runJavaScriptConfirmPanelWithMessage:(NSString*)message
                        initiatedByFrame:(WKFrameInfo*)frame
                       completionHandler:(void (^)(BOOL))completionHandler
{
    NSAlert* const alert = [[NSAlert alloc] init];
    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];
    [alert setInformativeText:message];
    [alert setMessageText:@"Confirm"];

    dispatch_async(dispatch_get_main_queue(), ^
    {
        [alert beginSheetModalForWindow:[webview window]
                      completionHandler:^(NSModalResponse result)
        {
            completionHandler(result == NSAlertFirstButtonReturn);
            [alert release];
        }];
    });
}

- (void)webView:(WKWebView*)webview
    runJavaScriptTextInputPanelWithPrompt:(NSString*)prompt
                              defaultText:(NSString*)defaultText
                         initiatedByFrame:(WKFrameInfo*)frame
                        completionHandler:(void (^)(NSString*))completionHandler
{
    NSTextField* const input = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 250, 30)];
    [input setStringValue:defaultText];

    NSAlert* const alert = [[NSAlert alloc] init];
    [alert setAccessoryView:input];
    [alert addButtonWithTitle:@"OK"];
    [alert addButtonWithTitle:@"Cancel"];
    [alert setInformativeText:prompt];
    [alert setMessageText: @"Prompt"];

    dispatch_async(dispatch_get_main_queue(), ^
    {
        [alert beginSheetModalForWindow:[webview window]
                      completionHandler:^(NSModalResponse result)
        {
            [input validateEditing];
            completionHandler(result == NSAlertFirstButtonReturn ? [input stringValue] : nil);
            [alert release];
        }];
    });
}

- (void)webView:(WKWebView*)webview
    runOpenPanelWithParameters:(WKOpenPanelParameters*)params
              initiatedByFrame:(WKFrameInfo*)frame
             completionHandler:(void (^)(NSArray<NSURL*>*))completionHandler
{
    NSOpenPanel* const panel = [[NSOpenPanel alloc] init];

    [panel setAllowsMultipleSelection:[params allowsMultipleSelection]];
    // [panel setAllowedFileTypes:(NSArray<NSString*>*)[params _allowedFileExtensions]];
    [panel setCanChooseDirectories:[params allowsDirectories]];
    [panel setCanChooseFiles:![params allowsDirectories]];

    dispatch_async(dispatch_get_main_queue(), ^
    {
        [panel beginSheetModalForWindow:[webview window]
                      completionHandler:^(NSModalResponse result)
        {
            completionHandler(result == NSModalResponseOK ? [panel URLs] : nil);
            [panel release];
        }];
    });
}

- (void)userContentController:(WKUserContentController*)userContentController
      didReceiveScriptMessage:(WKScriptMessage*)message
{
    NSString* const nsstring = static_cast<NSString*>([message body]);
    char* const string = strdup([nsstring UTF8String]);
    d_debug("JS call received '%s' %p", string, callback);

    if (callback != nullptr)
        callback(callbackPtr, string);

    std::free(string);
}

@end

#endif // WEB_VIEW_USING_MACOS_WEBKIT

// -----------------------------------------------------------------------------------------------------------

#ifdef WEB_VIEW_DGL_NAMESPACE
START_NAMESPACE_DGL
using DISTRHO_NAMESPACE::String;
#else
START_NAMESPACE_DISTRHO
#endif

// -----------------------------------------------------------------------------------------------------------

#if WEB_VIEW_USING_X11_IPC

#ifdef __linux__
typedef int32_t ipc_sem_t;
#else
typedef sem_t ipc_sem_t;
#endif

enum WebViewMessageType {
    kWebViewMessageNull,
    kWebViewMessageInitData,
    kWebViewMessageEvaluateJS,
    kWebViewMessageCallback,
    kWebViewMessageReload
};

struct WebViewSharedBuffer {
    static constexpr const uint32_t size = 0x100000;
    ipc_sem_t sem;
    uint32_t head, tail, wrtn;
    bool     invalidateCommit;
    uint8_t  buf[size];
};

struct WebViewRingBuffer {
    WebViewSharedBuffer server;
    WebViewSharedBuffer client;
    bool valid;
};

static void webview_wake(ipc_sem_t* const sem)
{
   #ifdef __linux__
    if (__sync_bool_compare_and_swap(sem, 0, 1))
        syscall(SYS_futex, sem, FUTEX_WAKE, 1, nullptr, nullptr, 0);
   #else
    sem_post(sem);
   #endif
}

static bool webview_timedwait(ipc_sem_t* const sem)
{
   #ifdef __linux__
    const struct timespec timeout = { 1, 0 };
    for (;;)
    {
        if (__sync_bool_compare_and_swap(sem, 1, 0))
            return true;
        if (syscall(SYS_futex, sem, FUTEX_WAIT, 0, &timeout, nullptr, 0) != 0)
            if (errno != EAGAIN && errno != EINTR)
                return false;
    }
   #else
    struct timespec timeout;
    if (clock_gettime(CLOCK_REALTIME, &timeout) != 0)
        return false;

    timeout.tv_sec += 1;

    for (int r;;)
    {
        r = sem_timedwait(sem, &timeout);

        if (r < 0)
            r = errno;

        if (r == EINTR)
            continue;

        return r == 0;
    }
   #endif
}

static void getFilenameFromFunctionPtr(char filename[PATH_MAX], const void* const ptr)
{
    Dl_info info = {};
    dladdr(ptr, &info);

    if (info.dli_fname[0] == '.' && info.dli_fname[1] != '.')
    {
        if (getcwd(filename, PATH_MAX - 1) == nullptr)
            filename[0] = '\0';
        std::strncat(filename, info.dli_fname + 1, PATH_MAX - 1);
    }
    else if (info.dli_fname[0] != '/')
    {
        if (getcwd(filename, PATH_MAX - 1) == nullptr)
            filename[0] = '\0';
        else
            std::strncat(filename, "/", PATH_MAX - 1);
        std::strncat(filename, info.dli_fname, PATH_MAX - 1);
    }
    else
    {
        std::strncpy(filename, info.dli_fname, PATH_MAX - 1);
    }
}

#endif // WEB_VIEW_USING_X11_IPC

// -----------------------------------------------------------------------------------------------------------

struct WebViewData {
   #if WEB_VIEW_USING_CHOC
    WebView* webview;
    String url;
   #elif WEB_VIEW_USING_MACOS_WEBKIT
    NSView* view;
    WKWebView* webview;
    NSURLRequest* urlreq;
    WEB_VIEW_DELEGATE_CLASS_NAME* delegate;
   #elif WEB_VIEW_USING_X11_IPC
    int shmfd = 0;
    char shmname[128] = {};
    WebViewRingBuffer* shmptr = nullptr;
    WebViewMessageCallback callback = nullptr;
    void* callbackPtr = nullptr;
    ChildProcess p;
    RingBufferControl<WebViewSharedBuffer> rbctrl, rbctrl2;
    ::Display* display = nullptr;
    ::Window childWindow = 0;
    ::Window ourWindow = 0;
   #endif
    WebViewData() {}
    DISTRHO_DECLARE_NON_COPYABLE(WebViewData);
};

// -----------------------------------------------------------------------------------------------------------

WebViewHandle webViewCreate(const char* const url,
                            const uintptr_t windowId,
                            const uint initialWidth,
                            const uint initialHeight,
                            const double scaleFactor,
                            const WebViewOptions& options)
{
#if WEB_VIEW_USING_CHOC
    WebView* const webview = webview_choc_create(options);
    if (webview == nullptr)
        return nullptr;

    const HWND hwnd = static_cast<HWND>(webview_choc_handle(webview));

    LONG_PTR flags = GetWindowLongPtr(hwnd, -16);
    flags = (flags & ~WS_POPUP) | WS_CHILD;
    SetWindowLongPtr(hwnd, -16, flags);

    SetParent(hwnd, reinterpret_cast<HWND>(windowId));
    SetWindowPos(hwnd, nullptr,
                 options.offset.x,
                 options.offset.y,
                 initialWidth - options.offset.x,
                 initialHeight - options.offset.y,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    ShowWindow(hwnd, SW_SHOW);

    WebViewData* const whandle = new WebViewData;
    whandle->webview = webview;
    whandle->url = url;

    webview_choc_navigate(webview, url);

    return whandle;
#elif WEB_VIEW_USING_MACOS_WEBKIT
    NSView* const view = reinterpret_cast<NSView*>(windowId);

    WKPreferences* const prefs = [[WKPreferences alloc] init];
    [prefs setValue:@YES forKey:@"javaScriptCanAccessClipboard"];
    [prefs setValue:@YES forKey:@"DOMPasteAllowed"];

    // if (debug)
    {
        [prefs setValue:@YES forKey:@"developerExtrasEnabled"];
        // TODO enable_write_console_messages_to_stdout
    }

    WKWebViewConfiguration* const config = [[WKWebViewConfiguration alloc] init];
    config.limitsNavigationsToAppBoundDomains = false;
    config.preferences = prefs;

    const CGRect rect = CGRectMake(options.offset.x / scaleFactor,
                                   options.offset.y / scaleFactor,
                                   initialWidth / scaleFactor,
                                   initialHeight / scaleFactor);

    WKWebView* const webview = [[WKWebView alloc] initWithFrame:rect
                                                  configuration:config];
    [webview setHidden:YES];
    [view addSubview:webview];

    // TODO webkit_web_view_set_background_color

    WEB_VIEW_DELEGATE_CLASS_NAME* const delegate = [[WEB_VIEW_DELEGATE_CLASS_NAME alloc] init];
    delegate->callback = options.callback;
    delegate->callbackPtr = options.callbackPtr;
    delegate->loaded = false;

    if (WKUserContentController* const controller = [config userContentController])
    {
        [controller addScriptMessageHandler:delegate name:@"external"];

        WKUserScript* const mscript = [[WKUserScript alloc]
            initWithSource:(options.callback != nullptr
                ? @"function postMessage(m){window.webkit.messageHandlers.external.postMessage(m)}"
                : @"function postMessage(m){}")
             injectionTime:WKUserScriptInjectionTimeAtDocumentStart
          forMainFrameOnly:true
        ];
        [controller addUserScript:mscript];
        [mscript release];

        if (options.initialJS != nullptr)
        {
            NSString* const nsInitialJS = [[NSString alloc] initWithBytes:options.initialJS
                                                                   length:std::strlen(options.initialJS)
                                                                 encoding:NSUTF8StringEncoding];

            WKUserScript* const script = [[WKUserScript alloc] initWithSource:nsInitialJS
                                                                injectionTime:WKUserScriptInjectionTimeAtDocumentStart
                                                             forMainFrameOnly:true];

            [controller addUserScript:script];

            [script release];
            [nsInitialJS release];
        }
    }

    [webview setNavigationDelegate:delegate];
    [webview setUIDelegate:delegate];

    NSString* const nsurl = [[NSString alloc] initWithBytes:url
                                                     length:std::strlen(url)
                                                   encoding:NSUTF8StringEncoding];
    NSURLRequest* const urlreq = [[NSURLRequest alloc] initWithURL: [NSURL URLWithString: nsurl]];

    d_stdout("url is '%s'", url);
    if (std::strncmp(url, "file://", 7) == 0)
    {
        const char* const lastsep = std::strrchr(url + 7, '/');

        NSString* const urlpath = [[NSString alloc] initWithBytes:url
                                                           length:(lastsep - url)
                                                         encoding:NSUTF8StringEncoding];

        [webview loadFileRequest:urlreq
         allowingReadAccessToURL:[NSURL URLWithString:urlpath]];

         [urlpath release];
    }
    else
    {
        [webview loadRequest:urlreq];
    }

    d_stdout("waiting for load");

    if (! delegate->loaded)
    {
        NSAutoreleasePool* const pool = [[NSAutoreleasePool alloc] init];
        NSDate* const date = [NSDate dateWithTimeIntervalSinceNow:0.05];
        NSEvent* event;

        while (! delegate->loaded)
        {
            event = [NSApp
                    #ifdef __MAC_10_12
                     nextEventMatchingMask:NSEventMaskAny
                    #else
                     nextEventMatchingMask:NSAnyEventMask
                    #endif
                                 untilDate:date
                                    inMode:NSDefaultRunLoopMode
                                   dequeue:YES];

            if (event == nil)
                break;

            [NSApp sendEvent: event];
        }

        [pool release];
    }

    d_stdout("waiting done");

    [webview setHidden:NO];

    [nsurl release];
    [config release];
    [prefs release];

    WebViewData* const handle = new WebViewData;
    handle->view = view;
    handle->webview = webview;
    handle->urlreq = urlreq;
    handle->delegate = delegate;
    return handle;
#elif WEB_VIEW_USING_X11_IPC
    // get startup paths
    char ldlinux[PATH_MAX] = {};
    getFilenameFromFunctionPtr(ldlinux, dlsym(nullptr, "_rtld_global"));

    char filename[PATH_MAX] = {};
    getFilenameFromFunctionPtr(filename, reinterpret_cast<const void*>(webViewCreate));

    d_stdout("ld-linux is '%s'", ldlinux);
    d_stdout("filename is '%s'", filename);

    // setup shared memory
    int shmfd;
    char shmname[128];
    void* shmptr;

    for (int i = 0; i < 9999; ++i)
    {
        snprintf(shmname, sizeof(shmname) - 1, "/dpf-webview-%d", i + 1);
        shmfd = shm_open(shmname, O_CREAT|O_EXCL|O_RDWR, 0666);

        if (shmfd < 0)
            continue;

        if (ftruncate(shmfd, sizeof(WebViewRingBuffer)) != 0)
        {
            close(shmfd);
            shm_unlink(shmname);
            continue;
        }

        break;
    }

    if (shmfd < 0)
    {
        d_stderr("shm_open failed: %s", strerror(errno));
        return nullptr;
    }

    shmptr = mmap(nullptr, sizeof(WebViewRingBuffer), PROT_READ|PROT_WRITE, MAP_SHARED, shmfd, 0);

    if (shmptr == nullptr || shmptr == MAP_FAILED)
    {
        d_stderr("mmap failed: %s", strerror(errno));
        close(shmfd);
        shm_unlink(shmname);
        return nullptr;
    }

   #ifndef __linux__
    sem_init(&handle->shmptr->client.sem, 1, 0);
    sem_init(&handle->shmptr->server.sem, 1, 0);
   #endif

    ::Display* const display = XOpenDisplay(nullptr);
    DISTRHO_SAFE_ASSERT_RETURN(display != nullptr, nullptr);

    // set up custom child environment
    uint envsize = 0;
    while (environ[envsize] != nullptr)
        ++envsize;

    char** const envp = new char*[envsize + 5];
    {
        uint e = 0;
        for (uint i = 0; i < envsize; ++i)
        {
            if (std::strncmp(environ[i], "LD_PRELOAD=", 11) == 0)
                continue;
            if (std::strncmp(environ[i], "LD_LIBRARY_PATH=", 16) == 0)
                continue;
            envp[e++] = strdup(environ[i]);
        }

        envp[e++] = strdup("LANG=en_US.UTF-8");
        envp[e++] = ("DPF_WEB_VIEW_SCALE_FACTOR=" + String(scaleFactor)).getAndReleaseBuffer();
        envp[e++] = ("DPF_WEB_VIEW_WIN_ID=" +String(windowId)).getAndReleaseBuffer();

        for (uint i = e; i < envsize + 5; ++i)
            envp[e++] = nullptr;
    }

    WebViewData* const handle = new WebViewData;
    handle->callback = options.callback;
    handle->callbackPtr = options.callbackPtr;
    handle->shmfd = shmfd;
    handle->shmptr = static_cast<WebViewRingBuffer*>(shmptr);
    handle->display = display;
    handle->ourWindow = windowId;
    std::memcpy(handle->shmname, shmname, sizeof(shmname));

    handle->shmptr->valid = true;

    handle->rbctrl.setRingBuffer(&handle->shmptr->client, false);
    handle->rbctrl.flush();

    handle->rbctrl2.setRingBuffer(&handle->shmptr->server, false);
    handle->rbctrl2.flush();

    const char* const args[] = { ldlinux, filename, "dpf-ld-linux-webview", shmname, nullptr };
    handle->p.start(args, envp);

    for (uint i = 0; envp[i] != nullptr; ++i)
        std::free(envp[i]);
    delete[] envp;

    const size_t urllen = std::strlen(url);
    const size_t initjslen = options.initialJS != nullptr ? std::strlen(options.initialJS) + 1 : 0;
    handle->rbctrl.writeUInt(kWebViewMessageInitData) &&
    handle->rbctrl.writeULong(windowId) &&
    handle->rbctrl.writeUInt(initialWidth) &&
    handle->rbctrl.writeUInt(initialHeight) &&
    handle->rbctrl.writeDouble(scaleFactor) &&
    handle->rbctrl.writeInt(options.offset.x) &&
    handle->rbctrl.writeInt(options.offset.y) &&
    handle->rbctrl.writeUInt(urllen) &&
    handle->rbctrl.writeCustomData(url, urllen) &&
    handle->rbctrl.writeUInt(initjslen) &&
    initjslen != 0 &&
    handle->rbctrl.writeCustomData(options.initialJS, initjslen);
    handle->rbctrl.commitWrite();
    webview_wake(&handle->shmptr->client.sem);

    for (int i = 0; i < 5 && handle->p.isRunning(); ++i)
    {
        if (webview_timedwait(&handle->shmptr->server.sem))
            return handle;
    }

    d_stderr("webview client side failed to start");
    webViewDestroy(handle);
    return nullptr;
#endif

    // maybe unused
    (void)windowId;
    (void)initialWidth;
    (void)initialHeight;
    (void)scaleFactor;
    (void)options;
    return nullptr;
}

void webViewDestroy(const WebViewHandle handle)
{
   #if WEB_VIEW_USING_CHOC
    webview_choc_destroy(handle->webview);
   #elif WEB_VIEW_USING_MACOS_WEBKIT
    [handle->webview setHidden:YES];
    [handle->webview removeFromSuperview];
    [handle->urlreq release];
    // [handle->delegate release];
   #elif WEB_VIEW_USING_X11_IPC
   #ifndef __linux__
    sem_destroy(&handle->shmptr->client.sem);
    sem_destroy(&handle->shmptr->server.sem);
   #endif
    munmap(handle->shmptr, sizeof(WebViewRingBuffer));
    close(handle->shmfd);
    shm_unlink(handle->shmname);
    XCloseDisplay(handle->display);
   #endif
    delete handle;
}

void webViewIdle(const WebViewHandle handle)
{
   #if WEB_VIEW_USING_X11_IPC
    uint32_t size = 0;
    void* buffer = nullptr;

    while (handle->rbctrl2.isDataAvailableForReading())
    {
        switch (handle->rbctrl2.readUInt())
        {
        case kWebViewMessageCallback:
            if (const uint32_t len = handle->rbctrl2.readUInt())
            {
                if (len > size)
                {
                    size = len;
                    buffer = std::realloc(buffer, len);

                    if (buffer == nullptr)
                    {
                        d_stderr("server out of memory, abort!");
                        handle->rbctrl2.flush();
                        return;
                    }
                }

                if (handle->rbctrl2.readCustomData(buffer, len))
                {
                    d_debug("server kWebViewMessageCallback -> '%s'", static_cast<char*>(buffer));
                    if (handle->callback != nullptr)
                        handle->callback(handle->callbackPtr, static_cast<char*>(buffer));
                    continue;
                }
            }
            break;
        }

        d_stderr("server ringbuffer data race, abort!");
        handle->rbctrl2.flush();
        return;
    }
   #else
    // unused
    (void)handle;
   #endif
}

void webViewEvaluateJS(const WebViewHandle handle, const char* const js)
{
   #if WEB_VIEW_USING_CHOC
    webview_choc_eval(handle->webview, js);
   #elif WEB_VIEW_USING_MACOS_WEBKIT
    NSString* const nsjs = [[NSString alloc] initWithBytes:js
                                                    length:std::strlen(js)
                                                  encoding:NSUTF8StringEncoding];
    [handle->webview evaluateJavaScript:nsjs completionHandler:nil];
    [nsjs release];
   #elif WEB_VIEW_USING_X11_IPC
    d_debug("evaluateJS '%s'", js);
    const size_t len = std::strlen(js) + 1;
    handle->rbctrl.writeUInt(kWebViewMessageEvaluateJS) &&
    handle->rbctrl.writeUInt(len) &&
    handle->rbctrl.writeCustomData(js, len);
    if (handle->rbctrl.commitWrite())
        webview_wake(&handle->shmptr->client.sem);
   #endif

    // maybe unused
    (void)handle;
    (void)js;
}

void webViewReload(const WebViewHandle handle)
{
   #if WEB_VIEW_USING_CHOC
    webview_choc_navigate(handle->webview, handle->url);
   #elif WEB_VIEW_USING_MACOS_WEBKIT
    [handle->webview loadRequest:handle->urlreq];
   #elif WEB_VIEW_USING_X11_IPC
    d_stdout("reload");
    handle->rbctrl.writeUInt(kWebViewMessageReload);
    if (handle->rbctrl.commitWrite())
        webview_wake(&handle->shmptr->client.sem);
   #endif

    // maybe unused
    (void)handle;
}

void webViewResize(const WebViewHandle handle, const uint width, const uint height, const double scaleFactor)
{
   #if WEB_VIEW_USING_CHOC
    const HWND hwnd = static_cast<HWND>(webview_choc_handle(handle->webview));
    SetWindowPos(hwnd, nullptr, 0, 0, width, height, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER);
   #elif WEB_VIEW_USING_MACOS_WEBKIT
    [handle->webview setFrameSize:NSMakeSize(width / scaleFactor, height / scaleFactor)];
   #elif WEB_VIEW_USING_X11_IPC
    if (handle->childWindow == 0)
    {
        ::Window rootWindow, parentWindow;
        ::Window* childWindows = nullptr;
        uint numChildren = 0;

        XFlush(handle->display);
        XQueryTree(handle->display, handle->ourWindow, &rootWindow, &parentWindow, &childWindows, &numChildren);

        if (numChildren == 0 || childWindows == nullptr)
            return;

        handle->childWindow = childWindows[0];
        XFree(childWindows);
    }

    XResizeWindow(handle->display, handle->childWindow, width, height);
    XFlush(handle->display);
   #endif

    // maybe unused
    (void)handle;
    (void)width;
    (void)height;
    (void)scaleFactor;
}

#if WEB_VIEW_USING_X11_IPC

// -----------------------------------------------------------------------------------------------------------

static std::function<void(const char* js)> evaluateFn;
static std::function<void()> reloadFn;
static std::function<void()> terminateFn;
static std::function<void(WebViewRingBuffer* rb)> wakeFn;

// -----------------------------------------------------------------------------------------------------------

struct GtkContainer;
struct GtkPlug;
struct GtkWidget;
struct GtkWindow;
struct JSCValue;
struct WebKitJavascriptResult;
struct WebKitSettings;
struct WebKitUserContentManager;
struct WebKitUserScript;
struct WebKitWebView;
typedef int gboolean;

#define G_CALLBACK(p) reinterpret_cast<void*>(p)
#define GTK_CONTAINER(p) reinterpret_cast<GtkContainer*>(p)
#define GTK_PLUG(p) reinterpret_cast<GtkPlug*>(p)
#define GTK_WINDOW(p) reinterpret_cast<GtkWindow*>(p)
#define WEBKIT_WEB_VIEW(p) reinterpret_cast<WebKitWebView*>(p)

// struct QApplication;
// struct QUrl;
// struct QWebEngineView;
// struct QWindow;

// -----------------------------------------------------------------------------------------------------------

#define JOIN(A, B) A ## B

#define AUTOSYM(S) \
    using JOIN(gtk3_, S) = decltype(&S); \
    JOIN(gtk3_, S) S = reinterpret_cast<JOIN(gtk3_, S)>(dlsym(nullptr, #S)); \
    DISTRHO_SAFE_ASSERT_RETURN(S != nullptr, false);

#define CSYM(S, NAME) \
    S NAME = reinterpret_cast<S>(dlsym(nullptr, #NAME)); \
    DISTRHO_SAFE_ASSERT_RETURN(NAME != nullptr, false);

#define CPPSYM(S, NAME, SN) \
    S NAME = reinterpret_cast<S>(dlsym(nullptr, #SN)); \
    DISTRHO_SAFE_ASSERT_RETURN(NAME != nullptr, false);

// -----------------------------------------------------------------------------------------------------------
// gtk3 variant

static void gtk3_idle(void* const ptr)
{
    WebViewRingBuffer* const shmptr = static_cast<WebViewRingBuffer*>(ptr);

    RingBufferControl<WebViewSharedBuffer> rbctrl;
    rbctrl.setRingBuffer(&shmptr->client, false);

    uint32_t size = 0;
    void* buffer = nullptr;

    while (rbctrl.isDataAvailableForReading())
    {
        switch (rbctrl.readUInt())
        {
        case kWebViewMessageEvaluateJS:
            if (const uint32_t len = rbctrl.readUInt())
            {
                if (len > size)
                {
                    size = len;
                    buffer = realloc(buffer, len);

                    if (buffer == nullptr)
                    {
                        d_stderr("lv2ui client out of memory, abort!");
                        abort();
                    }
                }

                if (rbctrl.readCustomData(buffer, len))
                {
                    d_debug("client kWebViewMessageEvaluateJS -> '%s'", static_cast<char*>(buffer));
                    evaluateFn(static_cast<char*>(buffer));
                    continue;
                }
            }
            break;
        case kWebViewMessageReload:
            d_debug("client kWebViewMessageReload");
            reloadFn();
            continue;
        }

        d_stderr("client ringbuffer data race, abort!");
        abort();
    }

    free(buffer);
}

static int gtk3_js_cb(WebKitUserContentManager*, WebKitJavascriptResult* const result, void* const arg)
{
    WebViewRingBuffer* const shmptr = static_cast<WebViewRingBuffer*>(arg);

    using g_free_t = void (*)(void*);
    using jsc_value_to_string_t = char* (*)(JSCValue*);
    using webkit_javascript_result_get_js_value_t = JSCValue* (*)(WebKitJavascriptResult*);

    CSYM(g_free_t, g_free)
    CSYM(jsc_value_to_string_t, jsc_value_to_string)
    CSYM(webkit_javascript_result_get_js_value_t, webkit_javascript_result_get_js_value)

    JSCValue* const value = webkit_javascript_result_get_js_value(result);
    DISTRHO_SAFE_ASSERT_RETURN(value != nullptr, false);

    char* const string = jsc_value_to_string(value);
    DISTRHO_SAFE_ASSERT_RETURN(string != nullptr, false);

    d_debug("js call received with data '%s'", string);

    const size_t len = std::strlen(string);
    RingBufferControl<WebViewSharedBuffer> rbctrl2;
    rbctrl2.setRingBuffer(&shmptr->server, false);
    rbctrl2.writeUInt(kWebViewMessageCallback) &&
    rbctrl2.writeUInt(len) &&
    rbctrl2.writeCustomData(string, len);
    rbctrl2.commitWrite();

    g_free(string);
    return 0;
}

static bool gtk3(Display* const display,
                 const Window winId,
                 const int x,
                 const int y,
                 const uint width,
                 const uint height,
                 double scaleFactor,
                 const char* const url,
                 const char* const initialJS,
                 WebViewRingBuffer* const shmptr)
{
    void* lib;
    if ((lib = dlopen("libwebkit2gtk-4.0.so.37", RTLD_NOW|RTLD_GLOBAL)) == nullptr ||
        (lib = dlopen("libwebkit2gtk-4.0.so", RTLD_NOW|RTLD_GLOBAL)) == nullptr)
        return false;

    using g_main_context_invoke_t = void (*)(void*, void*, void*);
    using g_signal_connect_data_t = ulong (*)(void*, const char*, void*, void*, void*, int);
    using gdk_set_allowed_backends_t = void (*)(const char*);
    using gtk_container_add_t = void (*)(GtkContainer*, GtkWidget*);
    using gtk_init_check_t = gboolean (*)(int*, char***);
    using gtk_main_t = void (*)();
    using gtk_main_quit_t = void (*)();
    using gtk_plug_get_id_t = Window (*)(GtkPlug*);
    using gtk_plug_new_t = GtkWidget* (*)(Window);
    using gtk_widget_show_all_t = void (*)(GtkWidget*);
    using gtk_window_move_t = void (*)(GtkWindow*, int, int);
    using gtk_window_set_default_size_t = void (*)(GtkWindow*, int, int);
    using webkit_settings_new_t = WebKitSettings* (*)();
    using webkit_settings_set_enable_developer_extras_t = void (*)(WebKitSettings*, gboolean);
    using webkit_settings_set_enable_write_console_messages_to_stdout_t = void (*)(WebKitSettings*, gboolean);
    using webkit_settings_set_hardware_acceleration_policy_t = void (*)(WebKitSettings*, int);
    using webkit_settings_set_javascript_can_access_clipboard_t = void (*)(WebKitSettings*, gboolean);
    using webkit_user_content_manager_add_script_t = void (*)(WebKitUserContentManager*, WebKitUserScript*);
    using webkit_user_content_manager_register_script_message_handler_t = gboolean (*)(WebKitUserContentManager*, const char*);
    using webkit_user_script_new_t = WebKitUserScript* (*)(const char*, int, int, const char* const*, const char* const*);
    using webkit_web_view_evaluate_javascript_t = void* (*)(WebKitWebView*, const char*, ssize_t, const char*, const char*, void*, void*, void*);
    using webkit_web_view_get_user_content_manager_t = WebKitUserContentManager* (*)(WebKitWebView*);
    using webkit_web_view_load_uri_t = void (*)(WebKitWebView*, const char*);
    using webkit_web_view_new_with_settings_t = GtkWidget* (*)(WebKitSettings*);
    using webkit_web_view_run_javascript_t = void* (*)(WebKitWebView*, const char*, void*, void*, void*);
    using webkit_web_view_set_background_color_t = void (*)(WebKitWebView*, const double*);

    CSYM(g_main_context_invoke_t, g_main_context_invoke)
    CSYM(g_signal_connect_data_t, g_signal_connect_data)
    CSYM(gdk_set_allowed_backends_t, gdk_set_allowed_backends)
    CSYM(gtk_container_add_t, gtk_container_add)
    CSYM(gtk_init_check_t, gtk_init_check)
    CSYM(gtk_main_t, gtk_main)
    CSYM(gtk_main_quit_t, gtk_main_quit)
    CSYM(gtk_plug_get_id_t, gtk_plug_get_id)
    CSYM(gtk_plug_new_t, gtk_plug_new)
    CSYM(gtk_widget_show_all_t, gtk_widget_show_all)
    CSYM(gtk_window_move_t, gtk_window_move)
    CSYM(gtk_window_set_default_size_t, gtk_window_set_default_size)
    CSYM(webkit_settings_new_t, webkit_settings_new)
    CSYM(webkit_settings_set_enable_developer_extras_t, webkit_settings_set_enable_developer_extras)
    CSYM(webkit_settings_set_enable_write_console_messages_to_stdout_t, webkit_settings_set_enable_write_console_messages_to_stdout)
    CSYM(webkit_settings_set_hardware_acceleration_policy_t, webkit_settings_set_hardware_acceleration_policy)
    CSYM(webkit_settings_set_javascript_can_access_clipboard_t, webkit_settings_set_javascript_can_access_clipboard)
    CSYM(webkit_user_content_manager_add_script_t, webkit_user_content_manager_add_script)
    CSYM(webkit_user_content_manager_register_script_message_handler_t, webkit_user_content_manager_register_script_message_handler)
    CSYM(webkit_user_script_new_t, webkit_user_script_new)
    CSYM(webkit_web_view_get_user_content_manager_t, webkit_web_view_get_user_content_manager)
    CSYM(webkit_web_view_load_uri_t, webkit_web_view_load_uri)
    CSYM(webkit_web_view_new_with_settings_t, webkit_web_view_new_with_settings)
    CSYM(webkit_web_view_set_background_color_t, webkit_web_view_set_background_color)

    // special case for legacy API handling
    webkit_web_view_evaluate_javascript_t webkit_web_view_evaluate_javascript = reinterpret_cast<webkit_web_view_evaluate_javascript_t>(dlsym(nullptr, "webkit_web_view_evaluate_javascript"));
    webkit_web_view_run_javascript_t webkit_web_view_run_javascript = reinterpret_cast<webkit_web_view_run_javascript_t>(dlsym(nullptr, "webkit_web_view_run_javascript"));
    DISTRHO_SAFE_ASSERT_RETURN(webkit_web_view_evaluate_javascript != nullptr || webkit_web_view_run_javascript != nullptr, false);

    const int gdkScale = std::fmod(scaleFactor, 1.0) >= 0.75
                       ? static_cast<int>(scaleFactor + 0.5)
                       : static_cast<int>(scaleFactor);

    if (gdkScale != 1)
    {
        char scale[8] = {};
        std::snprintf(scale, 7, "%d", gdkScale);
        setenv("GDK_SCALE", scale, 1);

        std::snprintf(scale, 7, "%.2f", (1.0 / scaleFactor) * 1.2);
        setenv("GDK_DPI_SCALE", scale, 1);
    }
    else if (scaleFactor > 1.0)
    {
        char scale[8] = {};
        std::snprintf(scale, 7, "%.2f", (1.0 / scaleFactor) * 1.4);
        setenv("GDK_DPI_SCALE", scale, 1);
    }

    scaleFactor /= gdkScale;

    gdk_set_allowed_backends("x11");

    if (! gtk_init_check (nullptr, nullptr))
        return false;

    GtkWidget* const window = gtk_plug_new(winId);
    DISTRHO_SAFE_ASSERT_RETURN(window != nullptr, false);

    gtk_window_set_default_size(GTK_WINDOW(window),
                                (width - x) * scaleFactor,
                                (height - y) * scaleFactor);
    gtk_window_move(GTK_WINDOW(window), x * scaleFactor, y * scaleFactor);

    WebKitSettings* const settings = webkit_settings_new();
    DISTRHO_SAFE_ASSERT_RETURN(settings != nullptr, false);

    // TODO DOMPasteAllowed
    webkit_settings_set_javascript_can_access_clipboard(settings, true);
    webkit_settings_set_hardware_acceleration_policy(settings, 2 /* WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER */);

    // if (debug)
    {
        webkit_settings_set_enable_developer_extras(settings, true);
        webkit_settings_set_enable_write_console_messages_to_stdout(settings, true);
    }

    GtkWidget* const webview = webkit_web_view_new_with_settings(settings);
    DISTRHO_SAFE_ASSERT_RETURN(webview != nullptr, false);

    const double color[] = {49.0/255, 54.0/255, 59.0/255, 1};
    webkit_web_view_set_background_color(WEBKIT_WEB_VIEW(webview), color);

    if (WebKitUserContentManager* const manager = webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(webview)))
    {
        g_signal_connect_data(manager, "script-message-received::external", G_CALLBACK(gtk3_js_cb), shmptr, nullptr, 0);
        webkit_user_content_manager_register_script_message_handler(manager, "external");

        WebKitUserScript* const mscript = webkit_user_script_new(
            "function postMessage(m){window.webkit.messageHandlers.external.postMessage(m)}", 0, 0, nullptr, nullptr);
        webkit_user_content_manager_add_script(manager, mscript);

        if (initialJS != nullptr)
        {
            WebKitUserScript* const script = webkit_user_script_new(initialJS, 0, 0, nullptr, nullptr);
            webkit_user_content_manager_add_script(manager, script);
        }
    }

    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webview), url);

    gtk_container_add(GTK_CONTAINER(window), webview);

    gtk_widget_show_all(window);

    Window wid = gtk_plug_get_id(GTK_PLUG(window));
    XMapWindow(display, wid);
    XFlush(display);

    evaluateFn = [=](const char* const js){
        if (webkit_web_view_evaluate_javascript != nullptr)
            webkit_web_view_evaluate_javascript(WEBKIT_WEB_VIEW(webview), js, -1,
                                                nullptr, nullptr, nullptr, nullptr, nullptr);
        else
            webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(webview), js, nullptr, nullptr, nullptr);
    };

    reloadFn = [=](){
        webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webview), url);
    };

    terminateFn = [=](){
        d_stdout("terminateFn");
        static bool quit = true;
        if (quit)
        {
            quit = false;
            gtk_main_quit();
        }
    };

    wakeFn = [=](WebViewRingBuffer* const rb){
        g_main_context_invoke(NULL, G_CALLBACK(gtk3_idle), rb);
    };

    // notify server we started ok
    webview_wake(&shmptr->server.sem);

    gtk_main();
    d_stdout("quit");

    dlclose(lib);
    return true;
}

#if 0
// -----------------------------------------------------------------------------------------------------------
// qt5webengine variant

static bool qt5webengine(const Window winId, const double scaleFactor, const char* const url)
{
    void* lib;
    if ((lib = dlopen("libQt5WebEngineWidgets.so.5", RTLD_NOW|RTLD_GLOBAL)) == nullptr ||
        (lib = dlopen("libQt5WebEngineWidgets.so", RTLD_NOW|RTLD_GLOBAL)) == nullptr)
        return false;

    using QApplication__init_t = void (*)(QApplication*, int&, char**, int);
    using QApplication_exec_t = void (*)();
    using QApplication_setAttribute_t = void (*)(Qt::ApplicationAttribute, bool);
    using QString__init_t = void (*)(void*, const QChar*, ptrdiff_t);
    using QUrl__init_t = void (*)(void*, const QString&, int /* QUrl::ParsingMode */);
    using QWebEngineView__init_t = void (*)(QWebEngineView*, void*);
    using QWebEngineView_move_t = void (*)(QWebEngineView*, const QPoint&);
    using QWebEngineView_resize_t = void (*)(QWebEngineView*, const QSize&);
    using QWebEngineView_setUrl_t = void (*)(QWebEngineView*, const QUrl&);
    using QWebEngineView_show_t = void (*)(QWebEngineView*);
    using QWebEngineView_winId_t = ulonglong (*)(QWebEngineView*);
    using QWebEngineView_windowHandle_t = QWindow* (*)(QWebEngineView*);
    using QWindow_fromWinId_t = QWindow* (*)(ulonglong);
    using QWindow_setParent_t = void (*)(QWindow*, void*);

    CPPSYM(QApplication__init_t, QApplication__init, _ZN12QApplicationC1ERiPPci)
    CPPSYM(QApplication_exec_t, QApplication_exec, _ZN15QGuiApplication4execEv)
    CPPSYM(QApplication_setAttribute_t, QApplication_setAttribute, _ZN16QCoreApplication12setAttributeEN2Qt20ApplicationAttributeEb)
    CPPSYM(QString__init_t, QString__init, _ZN7QStringC2EPK5QChari)
    CPPSYM(QUrl__init_t, QUrl__init, _ZN4QUrlC1ERK7QStringNS_11ParsingModeE)
    CPPSYM(QWebEngineView__init_t, QWebEngineView__init, _ZN14QWebEngineViewC1EP7QWidget)
    CPPSYM(QWebEngineView_move_t, QWebEngineView_move, _ZN7QWidget4moveERK6QPoint)
    CPPSYM(QWebEngineView_resize_t, QWebEngineView_resize, _ZN7QWidget6resizeERK5QSize)
    CPPSYM(QWebEngineView_setUrl_t, QWebEngineView_setUrl, _ZN14QWebEngineView6setUrlERK4QUrl)
    CPPSYM(QWebEngineView_show_t, QWebEngineView_show, _ZN7QWidget4showEv)
    CPPSYM(QWebEngineView_winId_t, QWebEngineView_winId, _ZNK7QWidget5winIdEv)
    CPPSYM(QWebEngineView_windowHandle_t, QWebEngineView_windowHandle, _ZNK7QWidget12windowHandleEv)
    CPPSYM(QWindow_fromWinId_t, QWindow_fromWinId, _ZN7QWindow9fromWinIdEy)
    CPPSYM(QWindow_setParent_t, QWindow_setParent, _ZN7QWindow9setParentEPS_)

    unsetenv("QT_FONT_DPI");
    unsetenv("QT_SCREEN_SCALE_FACTORS");
    unsetenv("QT_USE_PHYSICAL_DPI");
    setenv("QT_AUTO_SCREEN_SCALE_FACTOR", "0", 1);

    char scale[8] = {};
    std::snprintf(scale, 7, "%.2f", scaleFactor);
    setenv("QT_SCALE_FACTOR", scale, 1);

    QApplication_setAttribute(Qt::AA_X11InitThreads, true);
    QApplication_setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QApplication_setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    static int argc = 0;
    static char* argv[] = { nullptr };

    uint8_t _app[64]; // sizeof(QApplication) == 16
    QApplication* const app = reinterpret_cast<QApplication*>(_app);
    QApplication__init(app, argc, argv, 0);

    uint8_t _qstrurl[32]; // sizeof(QString) == 8
    QString* const qstrurl(reinterpret_cast<QString*>(_qstrurl));

    {
        const size_t url_len = std::strlen(url);
        QChar* const url_qchar = new QChar[url_len + 1];

        for (size_t i = 0; i < url_len; ++i)
            url_qchar[i] = QChar(url[i]);

        url_qchar[url_len] = 0;

        QString__init(qstrurl, url_qchar, url_len);
    }

    uint8_t _qurl[32]; // sizeof(QUrl) == 8
    QUrl* const qurl(reinterpret_cast<QUrl*>(_qurl));
    QUrl__init(qurl, *qstrurl, 1 /* QUrl::StrictMode */);

    uint8_t _webview[128]; // sizeof(QWebEngineView) == 56
    QWebEngineView* const webview = reinterpret_cast<QWebEngineView*>(_webview);
    QWebEngineView__init(webview, nullptr);

    QWebEngineView_move(webview, QPoint(0, kVerticalOffset));
    QWebEngineView_resize(webview, QSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT - kVerticalOffset));
    QWebEngineView_winId(webview);
    QWindow_setParent(QWebEngineView_windowHandle(webview), QWindow_fromWinId(winId));
    QWebEngineView_setUrl(webview, *qurl);
    QWebEngineView_show(webview);

    reloadFn = [=](){
        QWebEngineView_setUrl(webview, *qurl);
    };

    terminateFn = [=](){
        // TODO
    };

    QApplication_exec();

    dlclose(lib);
    return true;
}

// -----------------------------------------------------------------------------------------------------------
// qt6webengine variant (same as qt5 but `QString__init_t` has different arguments)

static bool qt6webengine(const Window winId, const double scaleFactor, const char* const url)
{
    void* lib;
    if ((lib = dlopen("libQt6WebEngineWidgets.so.6", RTLD_NOW|RTLD_GLOBAL)) == nullptr ||
        (lib = dlopen("libQt6WebEngineWidgets.so", RTLD_NOW|RTLD_GLOBAL)) == nullptr)
        return false;

    using QApplication__init_t = void (*)(QApplication*, int&, char**, int);
    using QApplication_exec_t = void (*)();
    using QApplication_setAttribute_t = void (*)(Qt::ApplicationAttribute, bool);
    using QString__init_t = void (*)(void*, const QChar*, long long);
    using QUrl__init_t = void (*)(void*, const QString&, int /* QUrl::ParsingMode */);
    using QWebEngineView__init_t = void (*)(QWebEngineView*, void*);
    using QWebEngineView_move_t = void (*)(QWebEngineView*, const QPoint&);
    using QWebEngineView_resize_t = void (*)(QWebEngineView*, const QSize&);
    using QWebEngineView_setUrl_t = void (*)(QWebEngineView*, const QUrl&);
    using QWebEngineView_show_t = void (*)(QWebEngineView*);
    using QWebEngineView_winId_t = ulonglong (*)(QWebEngineView*);
    using QWebEngineView_windowHandle_t = QWindow* (*)(QWebEngineView*);
    using QWindow_fromWinId_t = QWindow* (*)(ulonglong);
    using QWindow_setParent_t = void (*)(QWindow*, void*);

    CPPSYM(QApplication__init_t, QApplication__init, _ZN12QApplicationC1ERiPPci)
    CPPSYM(QApplication_exec_t, QApplication_exec, _ZN15QGuiApplication4execEv)
    CPPSYM(QApplication_setAttribute_t, QApplication_setAttribute, _ZN16QCoreApplication12setAttributeEN2Qt20ApplicationAttributeEb)
    CPPSYM(QString__init_t, QString__init, _ZN7QStringC2EPK5QCharx)
    CPPSYM(QUrl__init_t, QUrl__init, _ZN4QUrlC1ERK7QStringNS_11ParsingModeE)
    CPPSYM(QWebEngineView__init_t, QWebEngineView__init, _ZN14QWebEngineViewC1EP7QWidget)
    CPPSYM(QWebEngineView_move_t, QWebEngineView_move, _ZN7QWidget4moveERK6QPoint)
    CPPSYM(QWebEngineView_resize_t, QWebEngineView_resize, _ZN7QWidget6resizeERK5QSize)
    CPPSYM(QWebEngineView_setUrl_t, QWebEngineView_setUrl, _ZN14QWebEngineView6setUrlERK4QUrl)
    CPPSYM(QWebEngineView_show_t, QWebEngineView_show, _ZN7QWidget4showEv)
    CPPSYM(QWebEngineView_winId_t, QWebEngineView_winId, _ZNK7QWidget5winIdEv)
    CPPSYM(QWebEngineView_windowHandle_t, QWebEngineView_windowHandle, _ZNK7QWidget12windowHandleEv)
    CPPSYM(QWindow_fromWinId_t, QWindow_fromWinId, _ZN7QWindow9fromWinIdEy)
    CPPSYM(QWindow_setParent_t, QWindow_setParent, _ZN7QWindow9setParentEPS_)

    unsetenv("QT_FONT_DPI");
    unsetenv("QT_SCREEN_SCALE_FACTORS");
    unsetenv("QT_USE_PHYSICAL_DPI");
    setenv("QT_AUTO_SCREEN_SCALE_FACTOR", "0", 1);

    char scale[8] = {};
    std::snprintf(scale, 7, "%.2f", scaleFactor);
    setenv("QT_SCALE_FACTOR", scale, 1);

    QApplication_setAttribute(Qt::AA_X11InitThreads, true);
    QApplication_setAttribute(Qt::AA_EnableHighDpiScaling, true);
    QApplication_setAttribute(Qt::AA_UseHighDpiPixmaps, true);

    static int argc = 0;
    static char* argv[] = { nullptr };

    uint8_t _app[64]; // sizeof(QApplication) == 16
    QApplication* const app = reinterpret_cast<QApplication*>(_app);
    QApplication__init(app, argc, argv, 0);

    uint8_t _qstrurl[32]; // sizeof(QString) == 8
    QString* const qstrurl(reinterpret_cast<QString*>(_qstrurl));

    {
        const size_t url_len = std::strlen(url);
        QChar* const url_qchar = new QChar[url_len + 1];

        for (size_t i = 0; i < url_len; ++i)
            url_qchar[i] = QChar(url[i]);

        url_qchar[url_len] = 0;

        QString__init(qstrurl, url_qchar, url_len);
    }

    uint8_t _qurl[32]; // sizeof(QUrl) == 8
    QUrl* const qurl(reinterpret_cast<QUrl*>(_qurl));
    QUrl__init(qurl, *qstrurl, 1 /* QUrl::StrictMode */);

    uint8_t _webview[128]; // sizeof(QWebEngineView) == 56
    QWebEngineView* const webview = reinterpret_cast<QWebEngineView*>(_webview);
    QWebEngineView__init(webview, nullptr);

    QWebEngineView_move(webview, QPoint(0, kVerticalOffset));
    QWebEngineView_resize(webview, QSize(DISTRHO_UI_DEFAULT_WIDTH, DISTRHO_UI_DEFAULT_HEIGHT - kVerticalOffset));
    QWebEngineView_winId(webview);
    QWindow_setParent(QWebEngineView_windowHandle(webview), QWindow_fromWinId(winId));
    QWebEngineView_setUrl(webview, *qurl);
    QWebEngineView_show(webview);

    reloadFn = [=](){
        QWebEngineView_setUrl(webview, *qurl);
    };

    terminateFn = [=](){
        // TODO
    };

    QApplication_exec();

    dlclose(lib);
    return true;
}
#endif

// -----------------------------------------------------------------------------------------------------------
// startup via ld-linux

static void signalHandler(const int sig)
{
    switch (sig)
    {
    case SIGTERM:
        terminateFn();
        break;
    }
}

static void* threadHandler(void* const ptr)
{
    WebViewRingBuffer* const shmptr = static_cast<WebViewRingBuffer*>(ptr);

    // TODO wait until page is loaded, or something better
    d_sleep(1);

    while (shmptr->valid)
    {
        if (webview_timedwait(&shmptr->client.sem))
            wakeFn(shmptr);
    }

    return nullptr;
}

int dpf_webview_start(const int argc, char* argv[])
{
    d_stdout("started %d %s", argc, argv[1]);

    if (argc != 3)
    {
        d_stderr("WebView entry point, nothing to see here! ;)");
        return 1;
    }

    uselocale(newlocale(LC_NUMERIC_MASK, "C", nullptr));

    Display* const display = XOpenDisplay(nullptr);
    DISTRHO_SAFE_ASSERT_RETURN(display != nullptr, 1);

    const char* const shmname = argv[2];
    const int shmfd = shm_open(shmname, O_RDWR, 0);
    if (shmfd < 0)
    {
        d_stderr("shm_open failed: %s", std::strerror(errno));
        return 1;
    }

    WebViewRingBuffer* const shmptr = static_cast<WebViewRingBuffer*>(mmap(nullptr,
                                                                           sizeof(WebViewRingBuffer),
                                                                           PROT_READ|PROT_WRITE,
                                                                           MAP_SHARED,
                                                                           shmfd, 0));
    if (shmptr == nullptr || shmptr == nullptr)
    {
        d_stderr("mmap failed: %s", std::strerror(errno));
        close(shmfd);
        return 1;
    }

    RingBufferControl<WebViewSharedBuffer> rbctrl;
    rbctrl.setRingBuffer(&shmptr->client, false);

    // fetch initial data
    bool hasInitialData = false;
    Window winId = 0;
    uint width = 0, height = 0;
    double scaleFactor = 0;
    int x = 0, y = 0;
    char* url = nullptr;
    char* initJS = nullptr;

    while (shmptr->valid && webview_timedwait(&shmptr->client.sem))
    {
        if (rbctrl.isDataAvailableForReading())
        {
            DISTRHO_SAFE_ASSERT_RETURN(rbctrl.readUInt() == kWebViewMessageInitData, 1);

            hasInitialData = true;
            winId = rbctrl.readULong();
            width = rbctrl.readUInt();
            height = rbctrl.readUInt();
            scaleFactor = rbctrl.readDouble();
            x = rbctrl.readInt();
            y = rbctrl.readInt();

            const uint urllen = rbctrl.readUInt();
            url = static_cast<char*>(std::malloc(urllen));
            rbctrl.readCustomData(url, urllen);

            if (const uint initjslen = rbctrl.readUInt())
            {
                initJS = static_cast<char*>(std::malloc(initjslen));
                rbctrl.readCustomData(initJS, initjslen);
            }
        }
    }

    pthread_t thread;
    if (hasInitialData && pthread_create(&thread, nullptr, threadHandler, shmptr) == 0)
    {
        struct sigaction sig = {};
        sig.sa_handler = signalHandler;
        sig.sa_flags = SA_RESTART;
        sigemptyset(&sig.sa_mask);
        sigaction(SIGTERM, &sig, nullptr);

        // qt5webengine(winId, scaleFactor, url) ||
        // qt6webengine(winId, scaleFactor, url) ||
        gtk3(display, winId, x, y, width, height, scaleFactor, url, initJS, shmptr);

        shmptr->valid = false;
        pthread_join(thread, nullptr);
    }

    munmap(shmptr, sizeof(WebViewRingBuffer));
    close(shmfd);

    XCloseDisplay(display);

    return 0;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WEB_VIEW_USING_X11_IPC

#ifdef WEB_VIEW_DGL_NAMESPACE
END_NAMESPACE_DGL
#else
END_NAMESPACE_DISTRHO
#endif

#undef MACRO_NAME
#undef MACRO_NAME2

#undef WEB_VIEW_DISTRHO_NAMESPACE
#undef WEB_VIEW_DGL_NAMESPACE
