/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2025 Filipe Coelho <falktx@falktx.com>
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

// #undef Bool
// #undef CursorShape
// #undef Expose
// #undef FocusIn
// #undef FocusOut
// #undef FontChange
// #undef KeyPress
// #undef KeyRelease
// #undef None
// #undef Status
// #define QT_NO_VERSION_TAGGING
// #include <QGuiApplication>
// #include <QEvent>
// #include <QtCore/QChar>
// #include <QtCore/QPoint>
// #include <QtCore/QSize>
// #undef signals

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
# include "ChildProcess.hpp"
# include "RingBuffer.hpp"
# include "String.hpp"
# include <clocale>
# include <cstdio>
# include <dlfcn.h>
# include <fcntl.h>
# include <pthread.h>
# include <unistd.h>
# include <sys/mman.h>
# include <X11/Xlib.h>
# ifdef DISTRHO_PROPER_CPP11_SUPPORT
#  include <functional>
# endif
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
    MACRO_NAME(WebViewDelegate_, _, WEB_VIEW_NAMESPACE)

@interface WEB_VIEW_DELEGATE_CLASS_NAME : NSObject<WKNavigationDelegate, WKScriptMessageHandler, WKUIDelegate>
@end

@implementation WEB_VIEW_DELEGATE_CLASS_NAME {
@public
    WEB_VIEW_NAMESPACE::WebViewMessageCallback callback;
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

#ifdef WEB_VIEW_DGL_NAMESPACE
using DISTRHO_NAMESPACE::ChildProcess;
using DISTRHO_NAMESPACE::RingBufferControl;
#endif

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
    int shmfd;
    char shmname[128];
    WebViewRingBuffer* shmptr;
    WebViewMessageCallback callback;
    void* callbackPtr;
    ChildProcess p;
    RingBufferControl<WebViewSharedBuffer> rbctrl, rbctrl2;
    ::Display* display;
    ::Window childWindow;
    ::Window ourWindow;
   #endif
    WebViewData();
    DISTRHO_DECLARE_NON_COPYABLE(WebViewData);
};

WebViewData::WebViewData()
   #if WEB_VIEW_USING_CHOC
    : webview(nullptr),
      url()
   #elif WEB_VIEW_USING_MACOS_WEBKIT
    : view(nullptr),
      webview(nullptr),
      urlreq(nullptr),
      delegate(nullptr)
   #elif WEB_VIEW_USING_X11_IPC
    : shmfd(0),
      shmname(),
      shmptr(nullptr),
      callback(nullptr),
      callbackPtr(nullptr),
      p(),
      rbctrl(),
      rbctrl2(),
      display(nullptr),
      childWindow(0),
      ourWindow(0)
    #endif
{
   #if WEB_VIEW_USING_X11_IPC
    std::memset(shmname, 0, sizeof(shmname));
   #endif
}

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

    const size_t urllen = std::strlen(url) + 1;
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
        break;
    }

    std::free(buffer);
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

static bool running = false;
static struct WebFramework {
    virtual ~WebFramework() {}
    virtual void evaluate(const char* js) = 0;
    virtual void reload() = 0;
    virtual void terminate() = 0;
    virtual void wake(WebViewRingBuffer* rb) = 0;
}* webFramework = nullptr;

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

// -----------------------------------------------------------------------------------------------------------

class QApplication { uint8_t _[16 * 2]; };
class QByteArray { public: uint8_t _[24 * 2]; };
class QChar;
class QChildEvent;
class QColor;
class QEvent { uint8_t _[16 * 2]; };
class QIODevice;
class QJsonObject;
class QJsonValue { uint8_t _[128 /* TODO */ * 2]; };
class QMetaMethod;
class QMetaObject { uint8_t _[56 * 2]; };
class QString { uint8_t _[8 * 4]; };
class QTimerEvent;
class QUrl { uint8_t _[8 * 4]; };
class QWebChannel { uint8_t _[128 /* TODO */ * 2]; };
class QWebEnginePage { uint8_t _[128 /* TODO */ * 2]; };
class QWebEngineProfile { uint8_t _[128 /* TODO */ * 2]; };
class QWebEngineScript { uint8_t _[128 /* TODO */ * 2]; };
class QWebEngineScriptCollection;
class QWebEngineSettings;
class QWebEngineUrlRequestJob;
class QWebEngineUrlScheme { uint8_t _[128 /* TODO */ * 2]; };
class QWebEngineUrlSchemeHandler;
class QWebEngineView { uint8_t _[56 * 2]; };
class QWindow;

struct QPoint {
    int _x, _y;
    QPoint(int x, int y) : _x(x), _y(y) {}
};

struct QSize {
    int _w, _h;
    QSize(int w, int h) : _w(w), _h(h) {}
};

// -----------------------------------------------------------------------------------------------------------

#define JOIN(A, B) A ## B

#define CSYM(S, NAME) \
    S NAME = reinterpret_cast<S>(dlsym(nullptr, #NAME)); \
    DISTRHO_SAFE_ASSERT_RETURN(NAME != nullptr, false);

#define CPPSYM(S, NAME, SN) \
    S NAME = reinterpret_cast<S>(dlsym(nullptr, #SN)); \
    DISTRHO_SAFE_ASSERT_RETURN(NAME != nullptr, false);

static int web_wake_idle(void* const ptr)
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
                        d_stderr("client kWebViewMessageEvaluateJS out of memory, abort!");
                        abort();
                    }
                }

                if (rbctrl.readCustomData(buffer, len))
                {
                    d_debug("client kWebViewMessageEvaluateJS -> '%s'", static_cast<char*>(buffer));
                    webFramework->evaluate(static_cast<char*>(buffer));
                    continue;
                }
            }
            break;
        case kWebViewMessageReload:
            d_debug("client kWebViewMessageReload");
            webFramework->reload();
            continue;
        }

        d_stderr("client ringbuffer data race, abort!");
        abort();
    }

    free(buffer);
    return 0;
}

// -----------------------------------------------------------------------------------------------------------
// gtk3 variant

static int gtk3_js_cb(WebKitUserContentManager*, WebKitJavascriptResult* const result, void* const arg)
{
    WebViewRingBuffer* const shmptr = static_cast<WebViewRingBuffer*>(arg);

    typedef void (*g_free_t)(void*);
    typedef char* (*jsc_value_to_string_t)(JSCValue*);
    typedef JSCValue* (*webkit_javascript_result_get_js_value_t)(WebKitJavascriptResult*);

    CSYM(g_free_t, g_free)
    CSYM(jsc_value_to_string_t, jsc_value_to_string)
    CSYM(webkit_javascript_result_get_js_value_t, webkit_javascript_result_get_js_value)

    JSCValue* const value = webkit_javascript_result_get_js_value(result);
    DISTRHO_SAFE_ASSERT_RETURN(value != nullptr, false);

    char* const string = jsc_value_to_string(value);
    DISTRHO_SAFE_ASSERT_RETURN(string != nullptr, false);

    d_debug("js call received with data '%s'", string);

    const size_t len = std::strlen(string) + 1;
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
    if ((lib = dlopen("libwebkit2gtk-4.0.so.37", RTLD_NOW|RTLD_GLOBAL)) == nullptr &&
        (lib = dlopen("libwebkit2gtk-4.1.so.0", RTLD_NOW|RTLD_GLOBAL)) == nullptr &&
        (lib = dlopen("libwebkit2gtk-4.0.so", RTLD_NOW|RTLD_GLOBAL)) == nullptr &&
        (lib = dlopen("libwebkit2gtk-4.1.so", RTLD_NOW|RTLD_GLOBAL)) == nullptr)
    {
        d_stdout("WebView gtk3 platform not available: %s", dlerror());
        return false;
    }

    typedef void (*g_main_context_invoke_t)(void*, void*, void*);
    typedef ulong (*g_signal_connect_data_t)(void*, const char*, void*, void*, void*, int);
    typedef void (*gdk_set_allowed_backends_t)(const char*);
    typedef void (*gtk_container_add_t)(GtkContainer*, GtkWidget*);
    typedef gboolean (*gtk_init_check_t)(int*, char***);
    typedef void (*gtk_main_t)();
    typedef void (*gtk_main_quit_t)();
    typedef Window (*gtk_plug_get_id_t)(GtkPlug*);
    typedef GtkWidget* (*gtk_plug_new_t)(Window);
    typedef void (*gtk_widget_show_all_t)(GtkWidget*);
    typedef void (*gtk_window_move_t)(GtkWindow*, int, int);
    typedef void (*gtk_window_set_default_size_t)(GtkWindow*, int, int);
    typedef WebKitSettings* (*webkit_settings_new_t)();
    typedef void (*webkit_settings_set_enable_developer_extras_t)(WebKitSettings*, gboolean);
    typedef void (*webkit_settings_set_enable_write_console_messages_to_stdout_t)(WebKitSettings*, gboolean);
    typedef void (*webkit_settings_set_hardware_acceleration_policy_t)(WebKitSettings*, int);
    typedef void (*webkit_settings_set_javascript_can_access_clipboard_t)(WebKitSettings*, gboolean);
    typedef void (*webkit_user_content_manager_add_script_t)(WebKitUserContentManager*, WebKitUserScript*);
    typedef gboolean (*webkit_user_content_manager_register_script_message_handler_t)(WebKitUserContentManager*, const char*);
    typedef WebKitUserScript* (*webkit_user_script_new_t)(const char*, int, int, const char* const*, const char* const*);
    typedef void* (*webkit_web_view_evaluate_javascript_t)(WebKitWebView*, const char*, ssize_t, const char*, const char*, void*, void*, void*);
    typedef WebKitUserContentManager* (*webkit_web_view_get_user_content_manager_t)(WebKitWebView*);
    typedef void (*webkit_web_view_load_uri_t)(WebKitWebView*, const char*);
    typedef GtkWidget* (*webkit_web_view_new_with_settings_t)(WebKitSettings*);
    typedef void* (*webkit_web_view_run_javascript_t)(WebKitWebView*, const char*, void*, void*, void*);
    typedef void (*webkit_web_view_set_background_color_t)(WebKitWebView*, const double*);

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

    if (! gtk_init_check(nullptr, nullptr))
    {
        d_stderr("WebView gtk_init_check failed");
        return false;
    }

    GtkWidget* const window = gtk_plug_new(winId);
    DISTRHO_SAFE_ASSERT_RETURN(window != nullptr, false);

    gtk_window_set_default_size(GTK_WINDOW(window), width, height);
    gtk_window_move(GTK_WINDOW(window), x, y);

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

    struct Gtk3WebFramework : WebFramework {
        const char* const _url;
        WebViewRingBuffer* const _shmptr;
        GtkWidget* const _webview;
        const webkit_web_view_evaluate_javascript_t _webkit_web_view_evaluate_javascript;
        const webkit_web_view_run_javascript_t _webkit_web_view_run_javascript;
        const webkit_web_view_load_uri_t _webkit_web_view_load_uri;
        const gtk_main_quit_t _gtk_main_quit;
        const g_main_context_invoke_t _g_main_context_invoke;

        Gtk3WebFramework(const char* const url,
                         WebViewRingBuffer* const shmptr,
                         GtkWidget* const webview,
                         const webkit_web_view_evaluate_javascript_t webkit_web_view_evaluate_javascript,
                         const webkit_web_view_run_javascript_t webkit_web_view_run_javascript,
                         const webkit_web_view_load_uri_t webkit_web_view_load_uri,
                         const gtk_main_quit_t gtk_main_quit,
                         const g_main_context_invoke_t g_main_context_invoke)
            : _url(url),
              _shmptr(shmptr),
              _webview(webview),
              _webkit_web_view_evaluate_javascript(webkit_web_view_evaluate_javascript),
              _webkit_web_view_run_javascript(webkit_web_view_run_javascript),
              _webkit_web_view_load_uri(webkit_web_view_load_uri),
              _gtk_main_quit(gtk_main_quit),
              _g_main_context_invoke(g_main_context_invoke) {}

        void evaluate(const char* const js) override
        {
            if (_webkit_web_view_evaluate_javascript != nullptr)
                _webkit_web_view_evaluate_javascript(WEBKIT_WEB_VIEW(_webview), js, -1,
                                                     nullptr, nullptr, nullptr, nullptr, nullptr);
            else
                _webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(_webview), js, nullptr, nullptr, nullptr);
        }

        void reload() override
        {
            _webkit_web_view_load_uri(WEBKIT_WEB_VIEW(_webview), _url);
        }

        void terminate() override
        {
            if (running)
            {
                running = false;
                webview_wake(&_shmptr->client.sem);
                _gtk_main_quit();
            }
        }

        void wake(WebViewRingBuffer* const rb) override
        {
            _g_main_context_invoke(NULL, G_CALLBACK(web_wake_idle), rb);
        }
    };

    Gtk3WebFramework webFrameworkObj(url,
                                     shmptr,
                                     webview,
                                     webkit_web_view_evaluate_javascript,
                                     webkit_web_view_run_javascript,
                                     webkit_web_view_load_uri,
                                     gtk_main_quit,
                                     g_main_context_invoke);

    webFramework = &webFrameworkObj;

    // notify server we started ok
    webview_wake(&shmptr->server.sem);

    d_stdout("WebView gtk3 main loop started");

    gtk_main();

    d_stdout("WebView gtk3 main loop quit");

    dlclose(lib);
    return true;
}

// -----------------------------------------------------------------------------------------------------------
// qt common code

#define TRACE d_stdout("%04d: %s", __LINE__, __PRETTY_FUNCTION__);

class QObject
{
public:
    QObject(QObject* parent = nullptr)
    {
        static void (*m)(QObject*, QObject*) = reinterpret_cast<typeof(m)>(dlsym(
            nullptr, "_ZN7QObjectC1EPS_"));
        m(this, parent);
    }

    virtual const QMetaObject* metaObject() const
    {
        static const QMetaObject* (*m)(const QObject*) = reinterpret_cast<typeof(m)>(dlsym(
            nullptr, "_ZNK7QObject10metaObjectEv"));
        return m(this);
    }

    virtual void* qt_metacast(const char*) { return 0; }
    virtual int qt_metacall(void* /* QMetaObject::Call */, int, void**) { return 0; }
    virtual ~QObject() {}

    virtual bool event(QEvent* e)
    {
        static bool (*m)(QObject*, QEvent*) = reinterpret_cast<typeof(m)>(dlsym(
            nullptr, "_ZN7QObject5eventEP6QEvent"));
        return m(this, e);
    }

    virtual bool eventFilter(QObject* watched, QEvent* event)
    {
        static bool (*m)(QObject*, QObject*, QEvent*) = reinterpret_cast<typeof(m)>(dlsym(
            nullptr, "_ZN7QObject11eventFilterEPS_P6QEvent"));
        return m(this, watched, event);
    }

    virtual void timerEvent(QTimerEvent* event)
    {
        static void (*m)(QObject*, QTimerEvent*) = reinterpret_cast<typeof(m)>(dlsym(
            nullptr, "_ZN7QObject10timerEventEP11QTimerEvent"));
        m(this, event);
    }

    virtual void childEvent(QChildEvent* event)
    {
        static void (*m)(QObject*, QChildEvent*) = reinterpret_cast<typeof(m)>(dlsym(
            nullptr, "_ZN7QObject10childEventEP11QChildEvent"));
        m(this, event);
    }

    virtual void customEvent(QEvent* event)
    {
        static void (*m)(QObject*, QEvent*) = reinterpret_cast<typeof(m)>(dlsym(
            nullptr, "_ZN7QObject11customEventEP6QEvent"));
        m(this, event);
    }

    virtual void connectNotify(const QMetaMethod& signal)
    {
        static void (*m)(QObject*, const QMetaMethod&) = reinterpret_cast<typeof(m)>(dlsym(
            nullptr, "_ZN7QObject13connectNotifyERK11QMetaMethod"));
        m(this, signal);
    }

    virtual void disconnectNotify(const QMetaMethod& signal)
    {
        static void (*m)(QObject*, const QMetaMethod&) = reinterpret_cast<typeof(m)>(dlsym(
            nullptr, "_ZN7QObject16disconnectNotifyERK11QMetaMethod"));
        m(this, signal);
    }

private:
    uint8_t _[8 * 2];
};

class QWebChannelAbstractTransport : public QObject
{
protected:
    const QMetaObject* metaObject() const override
    {
        static const QMetaObject* (*m)(const QObject*) = reinterpret_cast<typeof(m)>(dlsym(
            nullptr, "_ZNK28QWebChannelAbstractTransport10metaObjectEv"));
        return m(this);
    }

    void* qt_metacast(const char*) override { return 0; }
    int qt_metacall(void* /* QMetaObject::Call */, int, void**) override { return 0; }
    ~QWebChannelAbstractTransport() override {}

public:
    QWebChannelAbstractTransport(QObject* parent = nullptr)
        : QObject(parent)
    {
        static void (*m)(QObject*, QObject*) = reinterpret_cast<typeof(m)>(dlsym(
            nullptr, "_ZN28QWebChannelAbstractTransportC1EP7QObject"));
        m(this, parent);
    }

    virtual void sendMessage(const QJsonObject&) = 0;
};

// -----------------------------------------------------------------------------------------------------------

// QObject subclass for receiving events on main thread
class EventFilterQObject : public QWebChannelAbstractTransport
{
    QString qstrkey;
    WebViewRingBuffer* const _rb;
    bool isQt5;

public:
    EventFilterQObject(WebViewRingBuffer* const rb)
        : QWebChannelAbstractTransport(),
          _rb(rb),
          isQt5(false)
    {
        void (*QString__init)(QString*, const QChar*, int) =
            reinterpret_cast<typeof(QString__init)>(dlsym(nullptr, "_ZN7QStringC2EPK5QCharx"));

        if (QString__init == nullptr)
        {
            isQt5 = true;
            QString__init = reinterpret_cast<typeof(QString__init)>(dlsym(nullptr, "_ZN7QStringC2EPK5QChari"));
        }

        const ushort key_qchar[] = { 'm', 0 };
        QString__init(&qstrkey, reinterpret_cast<const QChar*>(key_qchar), 1);
    }

    void customEvent(QEvent*) override
    {
        web_wake_idle(_rb);
    }

    void sendMessage(const QJsonObject& obj) override
    {
        static void (*QByteArray_clear)(QByteArray*) =
            reinterpret_cast<typeof(QByteArray_clear)>(dlsym(nullptr, "_ZN10QByteArray5clearEv"));

        static QJsonValue (*QJsonObject_value)(const QJsonObject*, const QString&) =
            reinterpret_cast<typeof(QJsonObject_value)>(dlsym(nullptr, "_ZNK11QJsonObject5valueERK7QString"));

        static void (*QJsonValue__deinit)(const QJsonValue*) =
            reinterpret_cast<typeof(QJsonValue__deinit)>(dlsym(nullptr, "_ZN10QJsonValueD1Ev"));
        static QString (*QJsonValue_toString)(const QJsonValue*) =
            reinterpret_cast<typeof(QJsonValue_toString)>(dlsym(nullptr, "_ZNK10QJsonValue8toStringEv"));

        static QString& (*QString_setRawData)(QString*, const QChar*, int) =
            reinterpret_cast<typeof(QString_setRawData)>(dlsym(nullptr, "_ZN7QString10setRawDataEPK5QCharx")) ?:
            reinterpret_cast<typeof(QString_setRawData)>(dlsym(nullptr, "_ZN7QString10setRawDataEPK5QChari"));
        static QByteArray (*QString_toUtf8)(const QString*) =
            reinterpret_cast<typeof(QString_toUtf8)>(dlsym(nullptr, "_ZNK7QString6toUtf8Ev")) ?:
            reinterpret_cast<typeof(QString_toUtf8)>(dlsym(nullptr, "_ZN7QString13toUtf8_helperERKS_"));

        const QJsonValue json = QJsonObject_value(&obj, qstrkey);
        QString qstrvalue = QJsonValue_toString(&json);
        QByteArray data = QString_toUtf8(&qstrvalue);

        const uint8_t* const dptr = static_cast<const uint8_t*>(*reinterpret_cast<const void**>(data._));
        const intptr_t offset = isQt5 ? *reinterpret_cast<const intptr_t*>(dptr + 16) : 16;
        const char* const value = reinterpret_cast<const char*>(dptr + offset);

        d_debug("js call received with data '%s'", value);

        const size_t len = std::strlen(value) + 1;
        RingBufferControl<WebViewSharedBuffer> rbctrl2;
        rbctrl2.setRingBuffer(&_rb->server, false);
        rbctrl2.writeUInt(kWebViewMessageCallback) &&
        rbctrl2.writeUInt(len) &&
        rbctrl2.writeCustomData(value, len);
        rbctrl2.commitWrite();

        // QByteArray and QString destructors are inlined and can't be called from here, call their next closest thing
        QByteArray_clear(&data);
        QString_setRawData(&qstrvalue, nullptr, 0);

        QJsonValue__deinit(&json);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// qt5webengine variant

static bool qtwebengine(const int qtVersion,
                        Display* const display,
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
    switch (qtVersion)
    {
    case 5:
        if ((lib = dlopen("libQt5WebEngineWidgets.so.5", RTLD_NOW|RTLD_GLOBAL)) == nullptr &&
            (lib = dlopen("libQt5WebEngineWidgets.so", RTLD_NOW|RTLD_GLOBAL)) == nullptr)
        {
            d_stdout("WebView Qt5 platform not available: %s", dlerror());
            return false;
        }
        break;
    case 6:
        if ((lib = dlopen("libQt6WebEngineWidgets.so.6", RTLD_NOW|RTLD_GLOBAL)) == nullptr &&
            (lib = dlopen("libQt6WebEngineWidgets.so", RTLD_NOW|RTLD_GLOBAL)) == nullptr)
        {
            d_stdout("WebView Qt6 platform not available: %s", dlerror());
            return false;
        }
        break;
    default:
        return false;
    }

    // Qt >= 6 uses int
    void (*QByteArray__init)(QByteArray*, const char*, int) =
        reinterpret_cast<typeof(QByteArray__init)>(dlsym(nullptr, "_ZN10QByteArrayC1EPKcx")) ?:
        reinterpret_cast<typeof(QByteArray__init)>(dlsym(nullptr, "_ZN10QByteArrayC1EPKci"));
    DISTRHO_SAFE_ASSERT_RETURN(QByteArray__init != nullptr, false);

    typedef void (*QString__init_t)(QString*, const QChar*, int);
    const QString__init_t QString__init =
        reinterpret_cast<QString__init_t>(dlsym(nullptr, "_ZN7QStringC2EPK5QCharx")) ?:
        reinterpret_cast<QString__init_t>(dlsym(nullptr, "_ZN7QStringC2EPK5QChari"));
    DISTRHO_SAFE_ASSERT_RETURN(QString__init != nullptr, false);

    void (*QWebEnginePage_setWebChannel)(QWebEnginePage*, QWebChannel*, uint) =
        reinterpret_cast<typeof(QWebEnginePage_setWebChannel)>(dlsym(
            nullptr, "_ZN14QWebEnginePage13setWebChannelEP11QWebChannelj")) ?:
        reinterpret_cast<typeof(QWebEnginePage_setWebChannel)>(dlsym(
            nullptr, "_ZN14QWebEnginePage13setWebChannelEP11QWebChannel"));
    DISTRHO_SAFE_ASSERT_RETURN(QWebEnginePage_setWebChannel != nullptr, false);

    // Qt >= 6 has new function signature with lambdas
#ifdef DISTRHO_PROPER_CPP11_SUPPORT
    typedef void (*QWebEnginePage_runJavaScript_t)(QWebEnginePage*, const QString&, uint, const std::function<void()>&);
#else
    typedef void (*QWebEnginePage_runJavaScript_t)(QWebEnginePage*, const QString&, uint, const uintptr_t&);
#endif
    typedef void (*QWebEnginePage_runJavaScript_compat_t)(QWebEnginePage*, const QString&);

    QWebEnginePage_runJavaScript_t QWebEnginePage_runJavaScript;
    QWebEnginePage_runJavaScript_compat_t QWebEnginePage_runJavaScript_compat;

    if (qtVersion == 5) {
        QWebEnginePage_runJavaScript = nullptr;
        QWebEnginePage_runJavaScript_compat = reinterpret_cast<QWebEnginePage_runJavaScript_compat_t>(dlsym(
            nullptr, "_ZN14QWebEnginePage13runJavaScriptERK7QString"));
        DISTRHO_SAFE_ASSERT_RETURN(QWebEnginePage_runJavaScript_compat != nullptr, false);
    } else {
        QWebEnginePage_runJavaScript_compat = nullptr;
        QWebEnginePage_runJavaScript = reinterpret_cast<QWebEnginePage_runJavaScript_t>(dlsym(
            nullptr, "_ZN14QWebEnginePage13runJavaScriptERK7QStringjRKSt8functionIFvRK8QVariantEE"));
        DISTRHO_SAFE_ASSERT_RETURN(QWebEnginePage_runJavaScript != nullptr, false);
    }

    typedef void (*QApplication__init_t)(QApplication*, int&, char**, int);
    typedef void (*QApplication_exec_t)();
    typedef void (*QApplication_postEvent_t)(QObject*, QEvent*, int);
    typedef void (*QApplication_quit_t)();
    typedef void (*QApplication_setAttribute_t)(int, bool);
    typedef void (*QEvent__init_t)(QEvent*, int /* QEvent::Type */);
    typedef QJsonValue (*QJsonObject_value_t)(const QJsonObject*, const QString &);
    typedef QString (*QJsonValue_toString_t)(const QJsonValue*);
    typedef void (*QUrl__init_t)(QUrl*, const QString&, int /* QUrl::ParsingMode */);
    typedef void (*QWebChannel__init_t)(QWebChannel*, QObject*);
    typedef void (*QWebChannel_registerObject_t)(QWebChannel*, const QString&, QObject*);
    typedef void (*QWebEnginePage__init_t)(QWebEnginePage*, QWebEngineProfile*, QObject*);
    typedef void (*QWebEnginePage_setBackgroundColor_t)(QWebEnginePage*, const QColor&);
    typedef QWebChannel* (*QWebEnginePage_webChannel_t)(QWebEnginePage*);
    typedef QWebEngineProfile* (*QWebEngineProfile_defaultProfile_t)();
    typedef void (*QWebEngineProfile_installUrlSchemeHandler_t)(QWebEngineProfile*, const QByteArray&, QWebEngineUrlSchemeHandler*);
    typedef QWebEngineSettings* (*QWebEngineProfile_settings_t)(QWebEngineProfile*);
    typedef QWebEngineScriptCollection* (*QWebEngineProfile_scripts_t)(QWebEngineProfile*);
    typedef void (*QWebEngineScript__init_t)(QWebEngineScript*);
    typedef void (*QWebEngineScript_setInjectionPoint_t)(QWebEngineScript*, int /* QWebEngineScript::InjectionPoint */);
    typedef void (*QWebEngineScript_setRunsOnSubFrames_t)(QWebEngineScript*, bool);
    typedef void (*QWebEngineScript_setSourceCode_t)(QWebEngineScript*, const QString &);
    typedef void (*QWebEngineScript_setWorldId_t)(QWebEngineScript*, uint32_t);
    typedef void (*QWebEngineScriptCollection_insert_t)(QWebEngineScriptCollection*, QWebEngineScript&);
    typedef void (*QWebEngineSettings_setAttribute_t)(QWebEngineSettings*, int /* QWebEngineSettings::WebAttribute */, bool);
    // typedef void (*QWebEngineUrlRequestJob_reply_t)(QWebEngineUrlRequestJob*, const QByteArray&, QIODevice*);
    typedef void (*QWebEngineUrlScheme__init_t)(QWebEngineUrlScheme*, const QByteArray&);
    typedef void (*QWebEngineUrlScheme_registerScheme_t)(QWebEngineUrlScheme&);
    typedef void (*QWebEngineUrlScheme_setFlags_t)(QWebEngineUrlScheme*, int /* QWebEngineUrlScheme::Flags */);
    typedef void (*QWebEngineUrlScheme_setSyntax_t)(QWebEngineUrlScheme*, int /* QWebEngineUrlScheme::Syntax */);
    typedef void (*QWebEngineUrlSchemeHandler__init_t)(QObject*, QObject*);
    typedef void (*QWebEngineView__init_t)(QWebEngineView*, QObject*);
    typedef void (*QWebEngineView_move_t)(QWebEngineView*, const QPoint&);
    typedef void (*QWebEngineView_resize_t)(QWebEngineView*, const QSize&);
    typedef void (*QWebEngineView_setPage_t)(QWebEngineView*, QWebEnginePage*);
    typedef void (*QWebEngineView_setUrl_t)(QWebEngineView*, const QUrl&);
    typedef void (*QWebEngineView_show_t)(QWebEngineView*);
    typedef ulonglong (*QWebEngineView_winId_t)(QWebEngineView*);
    typedef QWindow* (*QWebEngineView_windowHandle_t)(QWebEngineView*);
    typedef QWindow* (*QWindow_fromWinId_t)(ulonglong);
    typedef void (*QWindow_setParent_t)(QWindow*, void*);

    CPPSYM(QApplication__init_t, QApplication__init, _ZN12QApplicationC1ERiPPci)
    CPPSYM(QApplication_exec_t, QApplication_exec, _ZN15QGuiApplication4execEv)
    CPPSYM(QApplication_postEvent_t, QApplication_postEvent, _ZN16QCoreApplication9postEventEP7QObjectP6QEventi)
    CPPSYM(QApplication_quit_t, QApplication_quit, _ZN16QCoreApplication4quitEv)
    CPPSYM(QApplication_setAttribute_t, QApplication_setAttribute, _ZN16QCoreApplication12setAttributeEN2Qt20ApplicationAttributeEb)
    CPPSYM(QEvent__init_t, QEvent__init, _ZN6QEventC1ENS_4TypeE)
    CPPSYM(QJsonObject_value_t, QJsonObject_value, _ZNK11QJsonObject5valueERK7QString)
    CPPSYM(QJsonValue_toString_t, QJsonValue_toString, _ZNK10QJsonValue8toStringEv)
    CPPSYM(QUrl__init_t, QUrl__init, _ZN4QUrlC1ERK7QStringNS_11ParsingModeE)
    CPPSYM(QWebChannel__init_t, QWebChannel__init, _ZN11QWebChannelC1EP7QObject)
    CPPSYM(QWebChannel_registerObject_t, QWebChannel_registerObject, _ZN11QWebChannel14registerObjectERK7QStringP7QObject)
    CPPSYM(QWebEnginePage__init_t, QWebEnginePage__init, _ZN14QWebEnginePageC1EP17QWebEngineProfileP7QObject)
    CPPSYM(QWebEnginePage_setBackgroundColor_t, QWebEnginePage_setBackgroundColor, _ZN14QWebEnginePage18setBackgroundColorERK6QColor)
    CPPSYM(QWebEnginePage_webChannel_t, QWebEnginePage_webChannel, _ZNK14QWebEnginePage10webChannelEv)
    CPPSYM(QWebEngineProfile_defaultProfile_t, QWebEngineProfile_defaultProfile, _ZN17QWebEngineProfile14defaultProfileEv)
    CPPSYM(QWebEngineProfile_installUrlSchemeHandler_t, QWebEngineProfile_installUrlSchemeHandler, _ZN17QWebEngineProfile23installUrlSchemeHandlerERK10QByteArrayP26QWebEngineUrlSchemeHandler)
    CPPSYM(QWebEngineProfile_settings_t, QWebEngineProfile_settings, _ZNK17QWebEngineProfile8settingsEv)
    CPPSYM(QWebEngineProfile_scripts_t, QWebEngineProfile_scripts, _ZNK17QWebEngineProfile7scriptsEv)
    CPPSYM(QWebEngineScript__init_t, QWebEngineScript__init, _ZN16QWebEngineScriptC1Ev)
    CPPSYM(QWebEngineScript_setInjectionPoint_t, QWebEngineScript_setInjectionPoint, _ZN16QWebEngineScript17setInjectionPointENS_14InjectionPointE)
    CPPSYM(QWebEngineScript_setRunsOnSubFrames_t, QWebEngineScript_setRunsOnSubFrames, _ZN16QWebEngineScript18setRunsOnSubFramesEb)
    CPPSYM(QWebEngineScript_setSourceCode_t, QWebEngineScript_setSourceCode, _ZN16QWebEngineScript13setSourceCodeERK7QString)
    CPPSYM(QWebEngineScript_setWorldId_t, QWebEngineScript_setWorldId, _ZN16QWebEngineScript10setWorldIdEj)
    CPPSYM(QWebEngineScriptCollection_insert_t, QWebEngineScriptCollection_insert, _ZN26QWebEngineScriptCollection6insertERK16QWebEngineScript)
    CPPSYM(QWebEngineSettings_setAttribute_t, QWebEngineSettings_setAttribute, _ZN18QWebEngineSettings12setAttributeENS_12WebAttributeEb)
    // CPPSYM(QWebEngineUrlRequestJob_reply_t, QWebEngineUrlRequestJob_reply, _ZN23QWebEngineUrlRequestJob5replyERK10QByteArrayP9QIODevice)
    CPPSYM(QWebEngineUrlScheme__init_t, QWebEngineUrlScheme__init, _ZN19QWebEngineUrlSchemeC1ERK10QByteArray)
    CPPSYM(QWebEngineUrlScheme_registerScheme_t, QWebEngineUrlScheme_registerScheme, _ZN19QWebEngineUrlScheme14registerSchemeERKS_)
    CPPSYM(QWebEngineUrlScheme_setFlags_t, QWebEngineUrlScheme_setFlags, _ZN19QWebEngineUrlScheme8setFlagsE6QFlagsINS_4FlagEE)
    CPPSYM(QWebEngineUrlScheme_setSyntax_t, QWebEngineUrlScheme_setSyntax, _ZN19QWebEngineUrlScheme9setSyntaxENS_6SyntaxE)
    CPPSYM(QWebEngineUrlSchemeHandler__init_t, QWebEngineUrlSchemeHandler__init, _ZN26QWebEngineUrlSchemeHandlerC1EP7QObject)
    CPPSYM(QWebEngineView__init_t, QWebEngineView__init, _ZN14QWebEngineViewC1EP7QWidget)
    CPPSYM(QWebEngineView_move_t, QWebEngineView_move, _ZN7QWidget4moveERK6QPoint)
    CPPSYM(QWebEngineView_resize_t, QWebEngineView_resize, _ZN7QWidget6resizeERK5QSize)
    CPPSYM(QWebEngineView_setPage_t, QWebEngineView_setPage, _ZN14QWebEngineView7setPageEP14QWebEnginePage)
    CPPSYM(QWebEngineView_setUrl_t, QWebEngineView_setUrl, _ZN14QWebEngineView6setUrlERK4QUrl)
    CPPSYM(QWebEngineView_show_t, QWebEngineView_show, _ZN7QWidget4showEv)
    CPPSYM(QWebEngineView_winId_t, QWebEngineView_winId, _ZNK7QWidget5winIdEv)
    CPPSYM(QWebEngineView_windowHandle_t, QWebEngineView_windowHandle, _ZNK7QWidget12windowHandleEv)
    CPPSYM(QWindow_fromWinId_t, QWindow_fromWinId, _ZN7QWindow9fromWinIdEy)
    CPPSYM(QWindow_setParent_t, QWindow_setParent, _ZN7QWindow9setParentEPS_)

    unsetenv("QT_FONT_DPI");
    unsetenv("QT_SCREEN_SCALE_FACTORS");
    unsetenv("QT_USE_PHYSICAL_DPI");
    setenv("QT_QPA_PLATFORM", "xcb", 1);

    if (qtVersion == 5)
    {
        setenv("QT_AUTO_SCREEN_SCALE_FACTOR", "0", 1);
    }
    else
    {
        setenv("QT_ENABLE_HIGHDPI_SCALING", "0", 1);
    }

    char scale[8] = {};
    std::snprintf(scale, 7, "%.2f", scaleFactor);
    setenv("QT_SCALE_FACTOR", scale, 1);

    QByteArray urlSchemeName;
    QByteArray__init(&urlSchemeName, "dpf", 3);

    constexpr const int urlSchemeFlags = 0
        | 0x1 /* QWebEngineUrlScheme::SecureScheme */
        | 0x2 /* QWebEngineUrlScheme::LocalScheme */
        | 0x4 /* QWebEngineUrlScheme::LocalAccessAllowed */
        | 0x8 /* QWebEngineUrlScheme::ServiceWorkersAllowed */
        | 0x40 /* QWebEngineUrlScheme::ContentSecurityPolicyIgnored */
    ;
    QWebEngineUrlScheme urlScheme;
    QWebEngineUrlScheme__init(&urlScheme, urlSchemeName);
    QWebEngineUrlScheme_setSyntax(&urlScheme, 3 /* QWebEngineUrlScheme::Syntax::Path */);
    QWebEngineUrlScheme_setFlags(&urlScheme, urlSchemeFlags);
    QWebEngineUrlScheme_registerScheme(urlScheme);

    if (qtVersion == 5)
    {
        QApplication_setAttribute(10 /* Qt::AA_X11InitThreads */, true);
        QApplication_setAttribute(13 /* Qt::AA_UseHighDpiPixmaps */, true);
        QApplication_setAttribute(20 /* Qt::AA_EnableHighDpiScaling */, true);
    }

    static int argc = 1;
    static char argv0[] = "dpf-webview";
    static char* argv[] = { argv0, nullptr };

    QApplication app;
    QApplication__init(&app, argc, argv, 0);

    EventFilterQObject eventFilter(shmptr);

    QString qstrchannel, qstrmcode, qstrurl;
    {
        static constexpr const char* channel_src = "external";
        const size_t channel_len = std::strlen(channel_src);
        ushort* const channel_qchar = new ushort[channel_len + 1];

        for (size_t i = 0; i < channel_len; ++i)
            channel_qchar[i] = channel_src[i];

        channel_qchar[channel_len] = 0;

        QString__init(&qstrchannel, reinterpret_cast<QChar*>(channel_qchar), channel_len);

        delete[] channel_qchar;
    }
    {
        static constexpr const char* mcode_src = "\
        function postMessage(m){qt.webChannelTransport.send(JSON.stringify({\
            \"type\":6, \
            \"id\": \"WebSender\",\
            \"__QObject*__\": true,\
            \"object\": \"external\", \
            \"method\": \"sendMessage\",\
            \"args\":[{\"m\":m}], \
        }));}";
        const size_t mcode_len = std::strlen(mcode_src);
        ushort* const mcode_qchar = new ushort[mcode_len + 1];

        for (size_t i = 0; i < mcode_len; ++i)
            mcode_qchar[i] = mcode_src[i];

        mcode_qchar[mcode_len] = 0;

        QString__init(&qstrmcode, reinterpret_cast<QChar*>(mcode_qchar), mcode_len);

        delete[] mcode_qchar;
    }
    {
        const size_t url_len = std::strlen(url);
        ushort* const url_qchar = new ushort[url_len + 1];

        for (size_t i = 0; i < url_len; ++i)
            url_qchar[i] = url[i];

        url_qchar[url_len] = 0;

        QString__init(&qstrurl, reinterpret_cast<QChar*>(url_qchar), url_len);

        delete[] url_qchar;
    }

    QUrl qurl;
    QUrl__init(&qurl, qstrurl, 1 /* QUrl::StrictMode */);

    QWebEngineProfile* const profile = QWebEngineProfile_defaultProfile();
    QWebEngineScriptCollection* const scripts = QWebEngineProfile_scripts(profile);
    QWebEngineSettings* const settings = QWebEngineProfile_settings(profile);

    {
        QWebEngineScript mscript;
        QWebEngineScript__init(&mscript);
        QWebEngineScript_setInjectionPoint(&mscript, 2 /* QWebEngineScript::DocumentCreation */);
        QWebEngineScript_setRunsOnSubFrames(&mscript, true);
        QWebEngineScript_setSourceCode(&mscript, qstrmcode);
        QWebEngineScript_setWorldId(&mscript, 0 /* QWebEngineScript::MainWorld */);
        QWebEngineScriptCollection_insert(scripts, mscript);
    }

    if (initialJS != nullptr)
    {
        QString qstrcode;
        {
            const size_t code_len = std::strlen(initialJS);
            ushort* const code_qchar = new ushort[code_len + 1];

            for (size_t i = 0; i < code_len; ++i)
                code_qchar[i] = initialJS[i];

            code_qchar[code_len] = 0;

            QString__init(&qstrcode, reinterpret_cast<QChar*>(code_qchar), code_len);
        }

        QWebEngineScript script;
        QWebEngineScript__init(&script);
        QWebEngineScript_setInjectionPoint(&script, 2 /* QWebEngineScript::DocumentCreation */);
        QWebEngineScript_setRunsOnSubFrames(&script, true);
        QWebEngineScript_setSourceCode(&script, qstrcode);
        QWebEngineScript_setWorldId(&script, 0 /* QWebEngineScript::MainWorld */);
        QWebEngineScriptCollection_insert(scripts, script);
    }

    QWebEngineSettings_setAttribute(settings, 3 /* QWebEngineSettings::JavascriptCanAccessClipboard */, true);
    QWebEngineSettings_setAttribute(settings, 6 /* QWebEngineSettings::LocalContentCanAccessRemoteUrls */, true);
    QWebEngineSettings_setAttribute(settings, 9 /* QWebEngineSettings::LocalContentCanAccessFileUrls */, true);
    QWebEngineSettings_setAttribute(settings, 28 /* QWebEngineSettings::JavascriptCanPaste */, true);

    QWebEngineView webview;
    QWebEngineView__init(&webview, nullptr);

    QWebEnginePage page;
    QWebEnginePage__init(&page, profile, reinterpret_cast<QObject*>(&webview));
    // QWebEnginePage_setBackgroundColor(&page, QColor{0,0,0,0});

    QWebChannel channel;
    QWebChannel__init(&channel, reinterpret_cast<QObject*>(&webview));
    QWebChannel_registerObject(&channel, qstrchannel, &eventFilter);
    QWebEnginePage_setWebChannel(&page, &channel, 0);

    QWebEngineView_move(&webview, QPoint(x, y));
    QWebEngineView_resize(&webview, QSize(static_cast<int>(width / scaleFactor), static_cast<int>(height / scaleFactor)));
    QWebEngineView_winId(&webview);
    QWindow_setParent(QWebEngineView_windowHandle(&webview), QWindow_fromWinId(winId));

    QWebEngineView_setPage(&webview, &page);
    QWebEngineView_setUrl(&webview, qurl);

    // FIXME Qt6 seems to need some forcing..
    if (qtVersion >= 6)
    {
        XReparentWindow(display, QWebEngineView_winId(&webview), winId, x, y);
        XFlush(display);
    }

    QWebEngineView_show(&webview);

    struct QtWebFramework : WebFramework {
        const int _qtVersion;
        WebViewRingBuffer* const _shmptr;
        const QUrl& _qurl;
        QWebEnginePage& _page;
        QWebEngineView& _webview;
        EventFilterQObject& _eventFilter;
        const QString__init_t _QString__init;
        const QWebEnginePage_runJavaScript_compat_t _QWebEnginePage_runJavaScript_compat;
        const QWebEnginePage_runJavaScript_t _QWebEnginePage_runJavaScript;
        const QWebEngineView_setUrl_t _QWebEngineView_setUrl;
        const QApplication_quit_t _QApplication_quit;
        const QEvent__init_t _QEvent__init;
        const QApplication_postEvent_t _QApplication_postEvent;

        QtWebFramework(const int qtVersion,
                       WebViewRingBuffer* const shmptr,
                       const QUrl& qurl,
                       QWebEnginePage& page,
                       QWebEngineView& webview,
                       EventFilterQObject& eventFilter,
                       const QString__init_t QString__init,
                       const QWebEnginePage_runJavaScript_compat_t QWebEnginePage_runJavaScript_compat,
                       const QWebEnginePage_runJavaScript_t QWebEnginePage_runJavaScript,
                       const QWebEngineView_setUrl_t QWebEngineView_setUrl,
                       const QApplication_quit_t QApplication_quit,
                       const QEvent__init_t QEvent__init,
                       const QApplication_postEvent_t QApplication_postEvent)
            : _qtVersion(qtVersion),
              _shmptr(shmptr),
              _qurl(qurl),
              _page(page),
              _webview(webview),
              _eventFilter(eventFilter),
              _QString__init(QString__init),
              _QWebEnginePage_runJavaScript_compat(QWebEnginePage_runJavaScript_compat),
              _QWebEnginePage_runJavaScript(QWebEnginePage_runJavaScript),
              _QWebEngineView_setUrl(QWebEngineView_setUrl),
              _QApplication_quit(QApplication_quit),
              _QEvent__init(QEvent__init),
              _QApplication_postEvent(QApplication_postEvent) {}

        void evaluate(const char* const js) override
        {
            QString qstrjs;
            {
                const size_t js_len = std::strlen(js);
                ushort* const js_qchar = new ushort[js_len + 1];

                for (size_t i = 0; i < js_len; ++i)
                    js_qchar[i] = js[i];

                js_qchar[js_len] = 0;

                _QString__init(&qstrjs, reinterpret_cast<const QChar*>(js_qchar), js_len);
            }

            if (_qtVersion == 5)
                _QWebEnginePage_runJavaScript_compat(&_page, qstrjs);
            else
                _QWebEnginePage_runJavaScript(&_page, qstrjs, 0,
                                             #ifdef DISTRHO_PROPER_CPP11_SUPPORT
                                              {}
                                             #else
                                              0
                                             #endif
                                              );
        }

        void reload() override
        {
            _QWebEngineView_setUrl(&_webview, _qurl);
        }

        void terminate() override
        {
            if (running)
            {
                running = false;
                webview_wake(&_shmptr->client.sem);
                _QApplication_quit();
            }
        }

        void wake(WebViewRingBuffer*) override
        {
            // NOTE event pointer is deleted by Qt
            QEvent* const qevent = new QEvent;
            _QEvent__init(qevent, 1000 /* QEvent::User */);
            _QApplication_postEvent(&_eventFilter, qevent, 1 /* Qt::HighEventPriority */);
        }
    };

    QtWebFramework webFrameworkObj(qtVersion,
                                   shmptr,
                                   qurl,
                                   page,
                                   webview,
                                   eventFilter,
                                   QString__init,
                                   QWebEnginePage_runJavaScript_compat,
                                   QWebEnginePage_runJavaScript,
                                   QWebEngineView_setUrl,
                                   QApplication_quit,
                                   QEvent__init,
                                   QApplication_postEvent);

    webFramework = &webFrameworkObj;

    // notify server we started ok
    webview_wake(&shmptr->server.sem);

    d_stdout("WebView Qt%d main loop started", qtVersion);

    QApplication_exec();

    d_stdout("WebView Qt%d main loop quit", qtVersion);

    dlclose(lib);
    return true;
}

// -----------------------------------------------------------------------------------------------------------
// startup via ld-linux

static void signalHandler(const int sig)
{
    switch (sig)
    {
    case SIGTERM:
        webFramework->terminate();
        break;
    }
}

static void* threadHandler(void* const ptr)
{
    WebViewRingBuffer* const shmptr = static_cast<WebViewRingBuffer*>(ptr);

    while (running && shmptr->valid)
    {
        if (webview_timedwait(&shmptr->client.sem) && running)
            webFramework->wake(shmptr);
    }

    return nullptr;
}

int dpf_webview_start(const int argc, char* argv[])
{
    if (argc != 3)
    {
        d_stderr("WebView entry point, nothing to see here! ;)");
        return 1;
    }

    d_stdout("starting... %d '%s' '%s'", argc, argv[1], argv[2]);

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

            hasInitialData = running = true;
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
        d_stdout("WebView IPC in place, starting engine...");

        struct sigaction sig = {};
        sig.sa_handler = signalHandler;
        sig.sa_flags = SA_RESTART;
        sigemptyset(&sig.sa_mask);
        sigaction(SIGTERM, &sig, nullptr);

        if (! qtwebengine(5, display, winId, x, y, width, height, scaleFactor, url, initJS, shmptr) &&
            ! qtwebengine(6, display, winId, x, y, width, height, scaleFactor, url, initJS, shmptr) &&
            ! gtk3(display, winId, x, y, width, height, scaleFactor, url, initJS, shmptr))
        {
            d_stderr("Failed to find usable WebView platform");
        }

        shmptr->valid = running = false;
        pthread_join(thread, nullptr);
    }
    else
    {
        d_stderr("Failed to setup WebView IPC");
    }

    std::free(initJS);
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
#undef WEB_VIEW_NAMESPACE
