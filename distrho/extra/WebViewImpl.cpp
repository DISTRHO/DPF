/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2026 Filipe Coelho <falktx@falktx.com>
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

// #define WEB_VIEW_INCLUDE_GTK3_EXPLICITLY
// #define WEB_VIEW_INCLUDE_QTx_EXPLICITLY

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

#ifdef WEB_VIEW_INCLUDE_GTK3_EXPLICITLY
#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <webkit2/webkit2.h>
#endif

#ifdef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
enum {
    X_Expose = Expose,
    X_False = False,
    X_FocusIn = FocusIn,
    X_FocusOut = FocusOut,
    X_KeyPress = KeyPress,
    X_KeyRelease = KeyRelease,
    X_None = None,
    X_True = True,
};
typedef Status X_Status;
#undef Always
#undef Bool
#undef CursorShape
#undef Expose
#undef False
#undef FocusIn
#undef FocusOut
#undef FontChange
#undef GrayScale
#undef KeyPress
#undef KeyRelease
#undef None
#undef Status
#undef True
#undef Unsorted
#undef index
#undef signals
#include <QtWebEngineWidgets>
#define Expose X_Expose
#define False X_False
#define FocusIn X_FocusIn
#define FocusOut X_FocusOut
#define KeyPress X_KeyPress
#define KeyRelease X_KeyRelease
#define None 0L
#define Status X_Status
#define True X_True
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
#include "String.hpp"
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

struct WebViewInitOptions {
    Window winId;
    uint width;
    uint height;
    double scaleFactor;
    int x;
    int y;
    uint backgroundColor;
    bool developerToolsEnabled;
    char* url;
    char* initialJS;
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
    WebView* const webview = webview_choc_create(url, options);
    if (webview == nullptr)
        return nullptr;

    const HWND hwnd = static_cast<HWND>(webview_choc_handle(webview));
    ShowWindow(hwnd, SW_HIDE);

    LONG_PTR flags = GetWindowLongPtr(hwnd, -16);
    flags = (flags & ~WS_POPUP) | WS_CHILD;
    SetWindowLongPtr(hwnd, -16, flags);

    SetParent(hwnd, reinterpret_cast<HWND>(windowId));
    SetWindowPos(hwnd,
                 nullptr,
                 options.offset.x,
                 options.offset.y,
                 initialWidth + options.offset.x,
                 initialHeight + options.offset.y,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    ShowWindow(hwnd, SW_SHOW);

    WebViewData* const whandle = new WebViewData;
    whandle->webview = webview;
    whandle->url = url;

    return whandle;
#elif WEB_VIEW_USING_MACOS_WEBKIT
    NSView* const view = reinterpret_cast<NSView*>(windowId);

    WKPreferences* const prefs = [[WKPreferences alloc] init];
    [prefs setValue:@YES forKey:@"DOMPasteAllowed"];
    [prefs setValue:@YES forKey:@"javaScriptCanAccessClipboard"];

    if (options.developerToolsEnabled)
        [prefs setValue:@YES forKey:@"developerExtrasEnabled"];

    WKWebViewConfiguration* const config = [[WKWebViewConfiguration alloc] init];
    config.allowsAirPlayForMediaPlayback = false;
    config.limitsNavigationsToAppBoundDomains = false;
    config.mediaTypesRequiringUserActionForPlayback = WKAudiovisualMediaTypeNone;
    config.preferences = prefs;
    config.websiteDataStore = [WKWebsiteDataStore nonPersistentDataStore];

    const CGRect rect = CGRectMake(options.offset.x / scaleFactor,
                                   options.offset.y / scaleFactor,
                                   initialWidth / scaleFactor,
                                   initialHeight / scaleFactor);

    WKWebView* const webview = [[WKWebView alloc] initWithFrame:rect
                                                  configuration:config];
    [webview setHidden:YES];
    [view addSubview:webview];

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

    if (options.backgroundColor != 0xffffffff)
    {
        const double colorf[] = {
            static_cast<double>((options.backgroundColor >> 24) & 0xff) / 0xff,
            static_cast<double>((options.backgroundColor >> 16) & 0xff) / 0xff,
            static_cast<double>((options.backgroundColor >> 8) & 0xff) / 0xff,
            static_cast<double>(options.backgroundColor & 0xff) / 0xff,
        };
        NSColor* const color = [NSColor colorWithSRGBRed:colorf[0]
                                                   green:colorf[1]
                                                    blue:colorf[2]
                                                   alpha:colorf[3]];
        [webview setUnderPageBackgroundColor:color];

        [webview setValue:@NO forKey:@"drawsBackground"];
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

#if 0
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
#endif

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
    handle->rbctrl.writeUInt(options.backgroundColor) &&
    handle->rbctrl.writeBool(options.developerToolsEnabled) &&
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
    SetWindowPos(hwnd, nullptr, 0, 0, width, height, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);
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

#ifndef WEB_VIEW_INCLUDE_GTK3_EXPLICITLY
struct GtkContainer;
struct GtkPlug;
struct GdkRGBA;
struct GtkWidget;
struct GtkWindow;
struct JSCValue;
struct WebKitJavascriptResult;
struct WebKitSettings;
struct WebKitUserContentManager;
struct WebKitUserScript;
struct WebKitWebContext;
struct WebKitWebView;
typedef int gboolean;
typedef int (*GSourceFunc)(void*);

#define G_CALLBACK(p) reinterpret_cast<void*>(p)
#define GTK_CONTAINER(p) reinterpret_cast<GtkContainer*>(p)
#define GTK_PLUG(p) reinterpret_cast<GtkPlug*>(p)
#define GTK_WINDOW(p) reinterpret_cast<GtkWindow*>(p)
#define WEBKIT_WEB_VIEW(p) reinterpret_cast<WebKitWebView*>(p)

#define G_CONNECT_DEFAULT 0
#define WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER 0
#define WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER 2
#define WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES 0
#define WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START 0
#endif

// --------------------------------------------------------------------------------------------------------------------

#define SIZEOF_QApplication 16
#define SIZEOF_QByteArray 24
#define SIZEOF_QColor 16
#define SIZEOF_QContextMenuEvent 64
#define SIZEOF_QEvent 24
#define SIZEOF_QJsonValue 24
#define SIZEOF_QMetaObject 56
#define SIZEOF_QObject 16
#define SIZEOF_QPoint 8
#define SIZEOF_QSize 8
#define SIZEOF_QString 24
#define SIZEOF_QTimerEvent 24
#define SIZEOF_QUrl 8
#define SIZEOF_QWebChannel 16
#define SIZEOF_QWebEnginePage 24
#define SIZEOF_QWebEngineScript 8
#define SIZEOF_QWebEngineUrlScheme 8
#define SIZEOF_QWebEngineView 56

#ifdef WEB_VIEW_INCLUDE_QTx_EXPLICITLY

#define QApplication_exec() QApplication::exec()
#define QApplication_postEvent(a, b, c) QApplication::postEvent(a, b, c)
#define QApplication_quit() QApplication::quit()
#define QApplication_setAttribute(a, b) QApplication::setAttribute(a, b)
#define QJsonObject_value(obj, a) static_cast<const QJsonObject*>(obj)->value(a)
#define QJsonValue_toString(obj) static_cast<const QJsonValue*>(obj)->toString()
#define QMenu_exec(obj, a, b) static_cast<QMenu*>(obj)->exec(a, b)
#define QObject_installEventFilter(obj, a) static_cast<QObject*>(obj)->installEventFilter(a)
#define QObject_killTimer(obj, a) static_cast<QObject*>(obj)->killTimer(a)
#define QObject_startTimer(obj, a, b) static_cast<QObject*>(obj)->startTimer(a, b)
#define QString_toUtf8(obj)  static_cast<QString*>(obj)->toUtf8()
#define QWebChannel_registerObject(obj, a, b) static_cast<QWebChannel*>(obj)->registerObject(a, b)
#define QWebEnginePage_action(obj, a) static_cast<QWebEnginePage*>(obj)->action(a)
#define QWebEnginePage_runJavaScript(obj, a, b, c) static_cast<QWebEnginePage*>(obj)->runJavaScript(a, b, c)
#define QWebEnginePage_runJavaScript_compat(obj, a) static_cast<QWebEnginePage*>(obj)->runJavaScript(a)
#define QWebEnginePage_setBackgroundColor(obj, a) static_cast<QWebEnginePage*>(obj)->setBackgroundColor(a)
#define QWebEnginePage_setDevToolsPage(obj, a) static_cast<QWebEnginePage*>(obj)->setDevToolsPage(a)
#define QWebEnginePage_setWebChannel(obj, a, b) static_cast<QWebEnginePage*>(obj)->setWebChannel(a, b)
#define QWebEnginePage_triggerAction(obj, a) static_cast<QWebEnginePage*>(obj)->triggerAction(a)
#define QWebEnginePage_webChannel(obj) static_cast<QWebEnginePage*>(obj)->webChannel()
#define QWebEngineProfile_defaultProfile() QWebEngineProfile::defaultProfile()
#define QWebEngineProfile_installUrlSchemeHandler(obj, a, b) static_cast<QWebEngineProfile*>(obj)->installUrlSchemeHandler(a, b)
#define QWebEngineProfile_scripts(obj) static_cast<QWebEngineProfile*>(obj)->scripts()
#define QWebEngineProfile_setHttpCacheType(obj, a) static_cast<QWebEngineProfile*>(obj)->setHttpCacheType(a)
#define QWebEngineProfile_settings(obj) static_cast<QWebEngineProfile*>(obj)->settings()
#define QWebEngineScriptCollection_insert(obj, a) static_cast<QWebEngineScriptCollection*>(obj)->insert(a)
#define QWebEngineScript_setInjectionPoint(obj, a) static_cast<QWebEngineScript*>(obj)->setInjectionPoint(a)
#define QWebEngineScript_setRunsOnSubFrames(obj, a) static_cast<QWebEngineScript*>(obj)->setRunsOnSubFrames(a)
#define QWebEngineScript_setSourceCode(obj, a) static_cast<QWebEngineScript*>(obj)->setSourceCode(a)
#define QWebEngineScript_setWorldId(obj, a) static_cast<QWebEngineScript*>(obj)->setWorldId(a)
#define QWebEngineSettings_setAttribute(obj, a, b) static_cast<QWebEngineSettings*>(obj)->setAttribute(a, b)
#define QWebEngineUrlRequestJob_reply(obj, a, b) static_cast<QWebEngineUrlRequestJob*>(obj)->reply(a)
#define QWebEngineUrlScheme_registerScheme(a) QWebEngineUrlScheme::registerScheme(a)
#define QWebEngineUrlScheme_setFlags(obj, a) static_cast<QWebEngineUrlScheme*>(obj)->setFlags(a)
#define QWebEngineUrlScheme_setSyntax(obj, a) static_cast<QWebEngineUrlScheme*>(obj)->setSyntax(a)
#define QWebEngineView_createStandardContextMenu(obj) static_cast<QWebEngineView*>(obj)->createStandardContextMenu()
#define QWebEngineView_move(obj, a) static_cast<QWebEngineView*>(obj)->move(a)
#define QWebEngineView_page(obj) static_cast<QWebEngineView*>(obj)->page()
#define QWebEngineView_resize(obj, a) static_cast<QWebEngineView*>(obj)->resize(a)
#define QWebEngineView_setPage(obj, a) static_cast<QWebEngineView*>(obj)->setPage(a)
#define QWebEngineView_setUrl(obj, a) static_cast<QWebEngineView*>(obj)->setUrl(a)
#define QWebEngineView_show(obj) static_cast<QWebEngineView*>(obj)->show()
#define QWebEngineView_winId(obj) static_cast<QWebEngineView*>(obj)->winId()
#define QWebEngineView_windowHandle(obj) static_cast<QWebEngineView*>(obj)->windowHandle()
#define QWidget_addAction(obj, a) static_cast<QWidget*>(obj)->addAction(a)
#define QWidget_removeAction(obj, a) static_cast<QWidget*>(obj)->removeAction(a)
#define QWidget_setAttribute(obj, a, b) static_cast<QWidget*>(obj)->setAttribute(a, b)
#define QWidget_setContextMenuPolicy(obj, a) static_cast<QWidget*>(obj)->setContextMenuPolicy(a)
#define QWidget_show(obj) static_cast<QWidget*>(obj)->show()
#define QWindow_fromWinId(a) QWindow::fromWinId(a)
#define QWindow_setParent(obj, a) static_cast<QWindow*>(obj)->setParent(a)

#else

struct QAction;
struct QApplication { uint8_t _[SIZEOF_QApplication]; };
struct QByteArray { uint8_t _[SIZEOF_QByteArray]; };
struct QChar;
struct QChildEvent;
struct QColor {
    int _spec;
    uint16_t _a, _r, _g, _b, _pad;
    QColor(int r, int g, int b, int a)
        : _spec(1),
          _a(a * 0x0101),
          _r(r * 0x0101),
          _g(g * 0x0101),
          _b(b * 0x0101),
          _pad(0)
    {}
};
struct QContextMenuEvent;
struct QEvent {
    uint8_t _[SIZEOF_QEvent];
    enum Type {
        Close = 19,
        DragEnter = 60,
        ContextMenu = 82,
        User = 1000,
    };
};
struct QContextMenuEvent { uint8_t _[SIZEOF_QContextMenuEvent]; };
struct QIODevice;
struct QJsonObject;
struct QJsonValue { uint8_t _[SIZEOF_QJsonValue]; };
struct QMenu;
struct QMetaMethod;
struct QMetaObject { uint8_t _[SIZEOF_QMetaObject]; };
struct QString { uint8_t _[SIZEOF_QString]; };
struct QTimerEvent { uint8_t _[SIZEOF_QTimerEvent]; };
struct QUrl {
    uint8_t _[SIZEOF_QUrl];
    enum ParsingMode {
        StrictMode = 1,
    };
};
struct QWebChannel { uint8_t _[SIZEOF_QWebChannel]; };
struct QWebEnginePage {
    uint8_t _[SIZEOF_QWebEnginePage];
    enum WebAction {
        Back = 0,
        Forward,
        Stop,
        Reload,
        ReloadAndBypassCache = 10,
        OpenLinkInThisWindow = 12,
        OpenLinkInNewWindow,
        OpenLinkInNewTab,
        DownloadLinkToDisk = 16,
        DownloadImageToDisk = 19,
        DownloadMediaToDisk = 25,
        InspectElement,
        SavePage = 30,
        OpenLinkInNewBackgroundTab,
        ViewSource,
    };
};
struct QWebEngineProfile {
    enum HttpCacheType {
        NoCache = 2,
    };
};
struct QWebEngineScript {
    uint8_t _[SIZEOF_QWebEngineUrlScheme];
    enum InjectionPoint {
        DocumentCreation = 2,
    };
    enum ScriptWorldId {
        MainWorld = 0,
    };
};
struct QWebEngineScriptCollection;
struct QWebEngineSettings {
    enum WebAttribute {
        JavascriptCanAccessClipboard = 3,
        LocalContentCanAccessRemoteUrls = 6,
        PlaybackRequiresUserGesture = 26,
        JavascriptCanPaste = 28,
    };
};
struct QWebEngineUrlRequestJob;
struct QWebEngineUrlScheme {
    uint8_t _[SIZEOF_QWebEngineUrlScheme];
    enum Flags {
        SecureScheme = 0x1,
        LocalScheme = 0x2,
        LocalAccessAllowed = 0x4,
        ServiceWorkersAllowed = 0x8,
        ViewSourceAllowed = 0x20,
        ContentSecurityPolicyIgnored = 0x40,
    };
    struct Syntax {
        enum {
            Path = 3,
        };
    };
};
struct QWebEngineUrlSchemeHandler;
struct QWebEngineView { uint8_t _[SIZEOF_QWebEngineView]; };
struct QWidget;
struct QWindow;

namespace Qt {
    enum ApplicationAttribute {
        AA_X11InitThreads = 10,
        AA_UseHighDpiPixmaps = 13,
        AA_EnableHighDpiScaling = 20,
    };
    enum ContextMenuPolicy {
        DefaultContextMenu = 1,
        CustomContextMenu = 3,
    };
    enum EventPriority {
        HighEventPriority = 1,
    };
    enum TimerType {
        CoarseTimer = 1,
    };
    enum WidgetAttribute {
        WA_DeleteOnClose = 55,
    };
};

struct QPoint {
    int _x, _y;
    QPoint(int x, int y) : _x(x), _y(y) {}
};

struct QSize {
    int _w, _h;
    QSize(int w, int h) : _w(w), _h(h) {}
};

typedef int QFlags;
#endif

// -----------------------------------------------------------------------------------------------------------

#define JOIN(A, B) A ## B

#define CSYM(NAME) \
    JOIN(NAME, _t) NAME = reinterpret_cast<JOIN(NAME, _t)>(dlsym(nullptr, #NAME)); \
    DISTRHO_SAFE_ASSERT_RETURN(NAME != nullptr, false);

#define CPPSYM(NAME, SYMBOL) \
    JOIN(NAME, _t) NAME = reinterpret_cast<JOIN(NAME, _t)>(dlsym(nullptr, #SYMBOL)); \
    DISTRHO_SAFE_ASSERT_RETURN(NAME != nullptr, false);

// -----------------------------------------------------------------------------------------------------------

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

    CSYM(g_free)
    CSYM(jsc_value_to_string)
    CSYM(webkit_javascript_result_get_js_value)

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

static bool gtk3(Display* const display, const WebViewInitOptions& options, WebViewRingBuffer* const shmptr)
{
   #ifndef WEB_VIEW_INCLUDE_GTK3_EXPLICITLY
    void* lib;
    if ((lib = dlopen("libwebkit2gtk-4.0.so.37", RTLD_NOW|RTLD_GLOBAL)) == nullptr &&
        (lib = dlopen("libwebkit2gtk-4.1.so.0", RTLD_NOW|RTLD_GLOBAL)) == nullptr &&
        (lib = dlopen("libwebkit2gtk-4.0.so", RTLD_NOW|RTLD_GLOBAL)) == nullptr &&
        (lib = dlopen("libwebkit2gtk-4.1.so", RTLD_NOW|RTLD_GLOBAL)) == nullptr)
    {
        d_stdout("WebView gtk3 platform not available: %s", dlerror());
        return false;
    }

    typedef void (*g_main_context_invoke_t)(void*, GSourceFunc, void*);
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
    typedef void (*webkit_settings_set_allow_universal_access_from_file_urls_t)(WebKitSettings*, int);
    typedef void (*webkit_settings_set_enable_back_forward_navigation_gestures_t)(WebKitSettings*, int);
    typedef void (*webkit_settings_set_hardware_acceleration_policy_t)(WebKitSettings*, int);
    typedef void (*webkit_settings_set_javascript_can_access_clipboard_t)(WebKitSettings*, gboolean);
    typedef void (*webkit_settings_set_media_playback_allows_inline_t)(WebKitSettings*, gboolean);
    typedef void (*webkit_settings_set_media_playback_requires_user_gesture_t)(WebKitSettings*, gboolean);
    typedef void (*webkit_user_content_manager_add_script_t)(WebKitUserContentManager*, WebKitUserScript*);
    typedef gboolean (*webkit_user_content_manager_register_script_message_handler_t)(WebKitUserContentManager*, const char*);
    typedef WebKitUserScript* (*webkit_user_script_new_t)(const char*, int, int, const char* const*, const char* const*);
    typedef void* (*webkit_web_view_evaluate_javascript_t)(WebKitWebView*, const char*, ssize_t, const char*, const char*, void*, void*, void*);
    typedef void (*webkit_web_context_set_cache_model_t)(WebKitWebContext*, int);
    typedef WebKitWebContext* (*webkit_web_view_get_context_t)(WebKitWebView*);
    typedef WebKitUserContentManager* (*webkit_web_view_get_user_content_manager_t)(WebKitWebView*);
    typedef void (*webkit_web_view_load_uri_t)(WebKitWebView*, const char*);
    typedef GtkWidget* (*webkit_web_view_new_with_settings_t)(WebKitSettings*);
    typedef void* (*webkit_web_view_run_javascript_t)(WebKitWebView*, const char*, void*, void*, void*);
    typedef void (*webkit_web_view_set_background_color_t)(WebKitWebView*, const GdkRGBA*);

    CSYM(g_main_context_invoke)
    CSYM(g_signal_connect_data)
    CSYM(gdk_set_allowed_backends)
    CSYM(gtk_container_add)
    CSYM(gtk_init_check)
    CSYM(gtk_main)
    CSYM(gtk_main_quit)
    CSYM(gtk_plug_get_id)
    CSYM(gtk_plug_new)
    CSYM(gtk_widget_show_all)
    CSYM(gtk_window_move)
    CSYM(gtk_window_set_default_size)
    CSYM(webkit_settings_new)
    CSYM(webkit_settings_set_enable_developer_extras)
    CSYM(webkit_settings_set_enable_write_console_messages_to_stdout)
    CSYM(webkit_settings_set_allow_universal_access_from_file_urls)
    CSYM(webkit_settings_set_enable_back_forward_navigation_gestures)
    CSYM(webkit_settings_set_hardware_acceleration_policy)
    CSYM(webkit_settings_set_javascript_can_access_clipboard)
    CSYM(webkit_settings_set_media_playback_allows_inline)
    CSYM(webkit_settings_set_media_playback_requires_user_gesture)
    CSYM(webkit_user_content_manager_add_script)
    CSYM(webkit_user_content_manager_register_script_message_handler)
    CSYM(webkit_user_script_new)
    CSYM(webkit_web_context_set_cache_model)
    CSYM(webkit_web_view_get_context)
    CSYM(webkit_web_view_get_user_content_manager)
    CSYM(webkit_web_view_load_uri)
    CSYM(webkit_web_view_new_with_settings)
    CSYM(webkit_web_view_set_background_color)

    // special case for legacy API handling
    webkit_web_view_evaluate_javascript_t webkit_web_view_evaluate_javascript = reinterpret_cast<webkit_web_view_evaluate_javascript_t>(dlsym(nullptr, "webkit_web_view_evaluate_javascript"));
    webkit_web_view_run_javascript_t webkit_web_view_run_javascript = reinterpret_cast<webkit_web_view_run_javascript_t>(dlsym(nullptr, "webkit_web_view_run_javascript"));
    DISTRHO_SAFE_ASSERT_RETURN(webkit_web_view_evaluate_javascript != nullptr || webkit_web_view_run_javascript != nullptr, false);
   #endif

    // get desktop scale factor, gtk dpi scaling needs to be based on this one
    XrmInitialize();

    double desktopScaleFactor = 1.0;
    if (char* const rms = XResourceManagerString(display))
    {
        if (const XrmDatabase db = XrmGetStringDatabase(rms))
        {
            char* type = nullptr;
            XrmValue value = {};

            if (XrmGetResource(db, "Xft.dpi", "Xft.Dpi", &type, &value)
                && type != nullptr
                && std::strcmp(type, "String") == 0
                && value.addr != nullptr)
            {
                char*        end    = nullptr;
                const double xftDpi = std::strtod(value.addr, &end);
                if (xftDpi > 0.0 && xftDpi < HUGE_VAL)
                    desktopScaleFactor = xftDpi / 96.0;
            }

            XrmDestroyDatabase(db);
        }
    }

    // gtk3 does not support fractional scaling, so we round to next integer if fmod >= 0.75
    double scaleFactor = options.scaleFactor;

    const int gdkScale = std::fmod(scaleFactor, 1.0) >= 0.75
                       ? static_cast<int>(scaleFactor + 0.5)
                       : static_cast<int>(scaleFactor);

    if (gdkScale != 1)
    {
        char scale[8] = {};
        std::snprintf(scale, 7, "%d", gdkScale);
        setenv("GDK_SCALE", scale, 1);

        if (gdkScale > scaleFactor)
        {
            std::snprintf(scale, 7, "%.2f", 0.5 + ((0.25 - gdkScale + scaleFactor) / 0.25 * 0.0835));
            setenv("GDK_DPI_SCALE", scale, 1);
        }
        else
        {
            std::snprintf(scale, 7, "%.2f", 1.0 / scaleFactor);
            setenv("GDK_DPI_SCALE", scale, 1);
        }
    }
    else
    {
        setenv("GDK_SCALE", "1", 1);

        char scale[8] = {};
        std::snprintf(scale, 7, "%.2f", (1.0 / desktopScaleFactor) * scaleFactor);
        setenv("GDK_DPI_SCALE", scale, 1);
    }

    scaleFactor /= gdkScale;

    gdk_set_allowed_backends("x11");

    if (! gtk_init_check(nullptr, nullptr))
    {
        d_stderr("WebView gtk_init_check failed");
        return false;
    }

    GtkWidget* const window = gtk_plug_new(options.winId);
    DISTRHO_SAFE_ASSERT_RETURN(window != nullptr, false);

    gtk_window_set_default_size(GTK_WINDOW(window), options.width / gdkScale, options.height / gdkScale);
    gtk_window_move(GTK_WINDOW(window), options.x / gdkScale, options.y / gdkScale);

    WebKitSettings* const settings = webkit_settings_new();
    DISTRHO_SAFE_ASSERT_RETURN(settings != nullptr, false);

    webkit_settings_set_allow_universal_access_from_file_urls(settings, true);
    webkit_settings_set_enable_back_forward_navigation_gestures(settings, false);
    webkit_settings_set_hardware_acceleration_policy(settings, WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER);
    webkit_settings_set_javascript_can_access_clipboard(settings, true);
    webkit_settings_set_media_playback_allows_inline(settings, true);
    webkit_settings_set_media_playback_requires_user_gesture(settings, false);

    webkit_settings_set_enable_developer_extras(settings, options.developerToolsEnabled);
    webkit_settings_set_enable_write_console_messages_to_stdout(settings, options.developerToolsEnabled);

    GtkWidget* const webview = webkit_web_view_new_with_settings(settings);
    DISTRHO_SAFE_ASSERT_RETURN(webview != nullptr, false);

    WebKitWebContext* const context = webkit_web_view_get_context(WEBKIT_WEB_VIEW(webview));
    DISTRHO_SAFE_ASSERT_RETURN(context != nullptr, false);

    webkit_web_context_set_cache_model(context, WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER);

    if (options.backgroundColor != 0xffffffff)
    {
        const double color[] = {
            static_cast<double>((options.backgroundColor >> 24) & 0xff) / 0xff,
            static_cast<double>((options.backgroundColor >> 16) & 0xff) / 0xff,
            static_cast<double>((options.backgroundColor >> 8) & 0xff) / 0xff,
            static_cast<double>(options.backgroundColor & 0xff) / 0xff,
        };
        webkit_web_view_set_background_color(WEBKIT_WEB_VIEW(webview), reinterpret_cast<const GdkRGBA*>(color));
    }

    if (WebKitUserContentManager* const manager = webkit_web_view_get_user_content_manager(WEBKIT_WEB_VIEW(webview)))
    {
        g_signal_connect_data(manager,
                              "script-message-received::external",
                              G_CALLBACK(gtk3_js_cb),
                              shmptr,
                              nullptr,
                              G_CONNECT_DEFAULT);
        webkit_user_content_manager_register_script_message_handler(manager, "external");

        WebKitUserScript* const mscript = webkit_user_script_new(
            "function postMessage(m){window.webkit.messageHandlers.external.postMessage(m)}",
            WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
            WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
            nullptr,
            nullptr);
        webkit_user_content_manager_add_script(manager, mscript);

        if (options.initialJS != nullptr)
        {
            WebKitUserScript* const script = webkit_user_script_new(options.initialJS,
                                                                    WEBKIT_USER_CONTENT_INJECT_ALL_FRAMES,
                                                                    WEBKIT_USER_SCRIPT_INJECT_AT_DOCUMENT_START,
                                                                    nullptr,
                                                                    nullptr);
            webkit_user_content_manager_add_script(manager, script);
        }
    }

    webkit_web_view_load_uri(WEBKIT_WEB_VIEW(webview), options.url);

    gtk_container_add(GTK_CONTAINER(window), webview);

    gtk_widget_show_all(window);

    Window wid = gtk_plug_get_id(GTK_PLUG(window));
    XMapWindow(display, wid);
    XFlush(display);

    struct Gtk3WebFramework : WebFramework {
        const char* const _url;
        WebViewRingBuffer* const _shmptr;
        GtkWidget* const _webview;
       #ifndef WEB_VIEW_INCLUDE_GTK3_EXPLICITLY
        const webkit_web_view_evaluate_javascript_t webkit_web_view_evaluate_javascript;
        const webkit_web_view_run_javascript_t webkit_web_view_run_javascript;
        const webkit_web_view_load_uri_t webkit_web_view_load_uri;
        const gtk_main_quit_t gtk_main_quit;
        const g_main_context_invoke_t g_main_context_invoke;
       #endif

        Gtk3WebFramework(const char* const url,
                         WebViewRingBuffer* const shmptr,
                         GtkWidget* const webview
                      #ifndef WEB_VIEW_INCLUDE_GTK3_EXPLICITLY
                       , const webkit_web_view_evaluate_javascript_t _webkit_web_view_evaluate_javascript,
                         const webkit_web_view_run_javascript_t _webkit_web_view_run_javascript,
                         const webkit_web_view_load_uri_t _webkit_web_view_load_uri,
                         const gtk_main_quit_t _gtk_main_quit,
                         const g_main_context_invoke_t _g_main_context_invoke
                      #endif
                         )
            : _url(url),
              _shmptr(shmptr),
              _webview(webview)
           #ifndef WEB_VIEW_INCLUDE_GTK3_EXPLICITLY
            , webkit_web_view_evaluate_javascript(_webkit_web_view_evaluate_javascript),
              webkit_web_view_run_javascript(_webkit_web_view_run_javascript),
              webkit_web_view_load_uri(_webkit_web_view_load_uri),
              gtk_main_quit(_gtk_main_quit),
              g_main_context_invoke(_g_main_context_invoke)
           #endif
        {
        }

        void evaluate(const char* const js) override
        {
           #ifndef WEB_VIEW_INCLUDE_GTK3_EXPLICITLY
            if (webkit_web_view_evaluate_javascript == nullptr)
                webkit_web_view_run_javascript(WEBKIT_WEB_VIEW(_webview), js, nullptr, nullptr, nullptr);
            else
           #endif
                webkit_web_view_evaluate_javascript(
                    WEBKIT_WEB_VIEW(_webview), js, -1, nullptr, nullptr, nullptr, nullptr, nullptr);
        }

        void reload() override
        {
            webkit_web_view_load_uri(WEBKIT_WEB_VIEW(_webview), _url);
        }

        void terminate() override
        {
            if (running)
            {
                running = false;
                webview_wake(&_shmptr->client.sem);
                gtk_main_quit();
            }
        }

        void wake(WebViewRingBuffer* const rb) override
        {
            g_main_context_invoke(NULL, web_wake_idle, rb);
        }
    };

    Gtk3WebFramework webFrameworkObj(options.url,
                                     shmptr,
                                     webview
                                  #ifndef WEB_VIEW_INCLUDE_GTK3_EXPLICITLY
                                   , webkit_web_view_evaluate_javascript,
                                     webkit_web_view_run_javascript,
                                     webkit_web_view_load_uri,
                                     gtk_main_quit,
                                     g_main_context_invoke
                                  #endif
                                     );

    webFramework = &webFrameworkObj;

    // notify server we started ok
    webview_wake(&shmptr->server.sem);

    d_stdout("WebView gtk3 main loop started");

    gtk_main();

    d_stdout("WebView gtk3 main loop quit");

   #ifndef WEB_VIEW_INCLUDE_GTK3_EXPLICITLY
    dlclose(lib);
   #endif
    return true;
}

// -----------------------------------------------------------------------------------------------------------
// qt common code

#ifndef WEB_VIEW_INCLUDE_QTx_EXPLICITLY

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
    uint8_t _[SIZEOF_QObject - sizeof(void*)];
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
#endif

#if __cplusplus >= 201103L
static_assert(sizeof(QApplication) == SIZEOF_QApplication, "wrong size");
static_assert(sizeof(QByteArray) <= SIZEOF_QByteArray, "wrong size");
static_assert(sizeof(QColor) == SIZEOF_QColor, "wrong size");
static_assert(sizeof(QContextMenuEvent) <= SIZEOF_QContextMenuEvent, "wrong size");
static_assert(sizeof(QEvent) <= SIZEOF_QEvent, "wrong size");
static_assert(sizeof(QJsonValue) == SIZEOF_QJsonValue, "wrong size");
static_assert(sizeof(QMetaObject) <= SIZEOF_QMetaObject, "wrong size");
static_assert(sizeof(QObject) == SIZEOF_QObject, "wrong size");
static_assert(sizeof(QPoint) == SIZEOF_QPoint, "wrong size");
static_assert(sizeof(QSize) == SIZEOF_QSize, "wrong size");
static_assert(sizeof(QString) <= SIZEOF_QString, "wrong size");
static_assert(sizeof(QTimerEvent) <= SIZEOF_QTimerEvent, "wrong size");
static_assert(sizeof(QUrl) == SIZEOF_QUrl, "wrong size");
static_assert(sizeof(QWebChannel) == SIZEOF_QWebChannel, "wrong size");
static_assert(sizeof(QWebEnginePage) == SIZEOF_QWebEnginePage, "wrong size");
static_assert(sizeof(QWebEngineScript) == SIZEOF_QWebEngineScript, "wrong size");
static_assert(sizeof(QWebEngineUrlScheme) == SIZEOF_QWebEngineUrlScheme, "wrong size");
static_assert(sizeof(QWebEngineView) <= SIZEOF_QWebEngineView, "wrong size");
#endif

// --------------------------------------------------------------------------------------------------------------------

// QObject subclass for dealing with a few Qt details:
// 1. opening context menu
// 2. opening developer tools
// 3. receiving js events for IPC
class WebViewEventFilter : public QWebChannelAbstractTransport
{
    WebViewRingBuffer* const _rb;
    QWebEngineView* const _view;
    Display* const _display;
    const bool _developerToolsEnabled;

    QWebEngineView* _devToolsView;

    QPoint _menuPos;
    QString _qstrkey;
   #ifndef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
    bool isQt5;
   #endif
    int _timerId;
    Window _winId;

public:
    WebViewEventFilter(WebViewRingBuffer* const rb,
                       QWebEngineView* const view,
                       Display* const display,
                       const bool developerToolsEnabled)
        : QWebChannelAbstractTransport(reinterpret_cast<QObject*>(view)),
          _rb(rb),
          _view(view),
          _display(display),
          _developerToolsEnabled(developerToolsEnabled),
          _devToolsView(nullptr),
          _menuPos(0, 0),
         #ifndef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
          isQt5(false),
         #endif
          _timerId(-1),
          _winId(0)
    {
       #ifdef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
        _qstrkey = "m";
       #else
        void (*QString__init)(QString*, const QChar*, int) =
            reinterpret_cast<typeof(QString__init)>(dlsym(nullptr, "_ZN7QStringC2EPK5QCharx"));

        if (QString__init == nullptr)
        {
            isQt5 = true;
            QString__init = reinterpret_cast<typeof(QString__init)>(dlsym(nullptr, "_ZN7QStringC2EPK5QChari"));
        }

        constexpr const ushort key_qchar[] = { 'm', 0 };
        QString__init(&_qstrkey, reinterpret_cast<const QChar*>(key_qchar), 1);
       #endif
    }

    void setWinId(const Window winId)
    {
        _winId = winId;
    }

protected:
    void customEvent(QEvent*) override
    {
        web_wake_idle(_rb);
    }

    bool eventFilter(QObject* const watched, QEvent* const event) override
    {
       #ifdef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
        const QEvent::Type type = event->type();
       #else
        static int (*QObject_startTimer)(QObject*, int, Qt::TimerType) =
            reinterpret_cast<typeof(QObject_startTimer)>(dlsym(nullptr, "_ZN7QObject10startTimerEiN2Qt9TimerTypeE"));

        const QEvent::Type type = static_cast<QEvent::Type>(*reinterpret_cast<uint16_t*>(event->_ + (isQt5 ? 16 : 8)));
       #endif

        if (type == QEvent::ContextMenu && watched == reinterpret_cast<QObject*>(_view))
        {
            // HACK forcing webview window position, needed for sub-menus
            if (_winId != 0)
            {
                XWindowAttributes attrs;
                XGetWindowAttributes(_display, _winId, &attrs);
                XMoveWindow(_display, _winId, attrs.x + 1, attrs.y);
                XMoveWindow(_display, _winId, attrs.x, attrs.y);
                XFlush(_display);
            }

            // show context menu "soon"
            // if timer is too quick, createStandardContextMenu() will refer to old objects
            if (_timerId == -1)
            {
               #ifdef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
                _menuPos = static_cast<QContextMenuEvent*>(event)->globalPos();
               #else
                _menuPos = *reinterpret_cast<QPoint*>(event->_ + (isQt5 ? 40 : 48));
               #endif
                _timerId = QObject_startTimer(this, 10, Qt::CoarseTimer);
            }

            return true;
        }

        if (type == QEvent::DragEnter) {
            if (_winId != 0)
            {
                XWindowAttributes attrs;
                XGetWindowAttributes(_display, _winId, &attrs);
                XMoveWindow(_display, _winId, attrs.x + 1, attrs.y);
                XMoveWindow(_display, _winId, attrs.x, attrs.y);
                XFlush(_display);
            }
        }

        if (type == QEvent::Close && watched != nullptr && watched == reinterpret_cast<QObject*>(_devToolsView))
            _devToolsView = nullptr;

        return QObject::eventFilter(watched, event);
    }

    void timerEvent(QTimerEvent* const event) override
    {
       #ifdef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
        const int timerId = event->timerId();
       #else
        static QAction* (*QAction_setVisible)(QAction*, bool) =
            reinterpret_cast<typeof(QAction_setVisible)>(dlsym(nullptr, "_ZN7QAction10setVisibleEb"));

        static QAction* (*QMenu_exec)(QMenu*, const QPoint&, QAction*) =
            reinterpret_cast<typeof(QMenu_exec)>(dlsym(nullptr, "_ZN5QMenu4execERK6QPointP7QAction"));

        static void (*QObject_installEventFilter)(QObject*, QObject*) =
            reinterpret_cast<typeof(QObject_installEventFilter)>(dlsym(nullptr, "_ZN7QObject18installEventFilterEPS_"));

        static void (*QObject_killTimer)(QObject*, int) =
            reinterpret_cast<typeof(QObject_killTimer)>(dlsym(nullptr, "_ZN7QObject9killTimerEi"));

        static QAction* (*QWebEnginePage_action)(QWebEnginePage*, QWebEnginePage::WebAction) =
            reinterpret_cast<typeof(QWebEnginePage_action)>(dlsym(nullptr, "_ZNK14QWebEnginePage6actionENS_9WebActionE"));

        static QMenu* (*QWebEnginePage_createStandardContextMenu)(QWebEnginePage*) =
            reinterpret_cast<typeof(QWebEnginePage_createStandardContextMenu)>(dlsym(nullptr, "_ZN14QWebEnginePage25createStandardContextMenuEv"));

        static void (*QWebEnginePage_setDevToolsPage)(QWebEnginePage*, QWebEnginePage*) =
            reinterpret_cast<typeof(QWebEnginePage_setDevToolsPage)>(dlsym(nullptr, "_ZN14QWebEnginePage15setDevToolsPageEPS_"));

        static void (*QWebEnginePage_triggerAction)(QWebEnginePage*, QWebEnginePage::WebAction) =
            reinterpret_cast<typeof(QWebEnginePage_triggerAction)>(dlsym(nullptr, "_ZN14QWebEnginePage13triggerActionENS_9WebActionEb"));

        static QMenu* (*QWebEngineView_createStandardContextMenu)(QWebEngineView*) =
            reinterpret_cast<typeof(QWebEngineView_createStandardContextMenu)>(dlsym(nullptr, "_ZN14QWebEngineView25createStandardContextMenuEv"));

        static void (*QWebEngineView__init)(QWebEngineView*, QObject*) =
            reinterpret_cast<typeof(QWebEngineView__init)>(dlsym(nullptr, "_ZN14QWebEngineViewC1EP7QWidget"));

        static QWebEnginePage* (*QWebEngineView_page)(QWebEngineView*) =
            reinterpret_cast<typeof(QWebEngineView_page)>(dlsym(nullptr, "_ZNK14QWebEngineView4pageEv"));

        static void (*QWidget_addAction)(QWidget*, QAction*) =
            reinterpret_cast<typeof(QWidget_addAction)>(dlsym(nullptr, "_ZN7QWidget9addActionEP7QAction"));

        static void (*QWidget_removeAction)(QWidget*, QAction*) =
            reinterpret_cast<typeof(QWidget_removeAction)>(dlsym(nullptr, "_ZN7QWidget12removeActionEP7QAction"));

        static void (*QWidget_setAttribute)(QWidget*, Qt::WidgetAttribute, bool) =
            reinterpret_cast<typeof(QWidget_setAttribute)>(dlsym(nullptr, "_ZN7QWidget12setAttributeEN2Qt15WidgetAttributeEb"));

        static void (*QWidget_show)(QWidget*) =
            reinterpret_cast<typeof(QWidget_show)>(dlsym(nullptr, "_ZN7QWidget4showEv"));

        const int timerId = *reinterpret_cast<int*>(event->_ + (isQt5 ? 20 : 16));
       #endif

        if (_timerId != timerId)
            return QObject::timerEvent(event);

        QObject_killTimer(this, _timerId);
        _timerId = -1;

        QWebEnginePage* const page = QWebEngineView_page(_view);

      #ifdef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
       #if QT_VERSION_MAJOR == 5
        QMenu* const menu = page->createStandardContextMenu();
       #else
        QMenu* const menu = _view->createStandardContextMenu();
       #endif
      #else
        QMenu* const menu = QWebEngineView_createStandardContextMenu != nullptr
                          ? QWebEngineView_createStandardContextMenu(_view)
                          : QWebEnginePage_createStandardContextMenu(page);
      #endif
        DISTRHO_SAFE_ASSERT_RETURN(menu != nullptr,);

        static constexpr const QWebEnginePage::WebAction unwantedActions[] = {
            QWebEnginePage::Back,
            QWebEnginePage::Forward,
            QWebEnginePage::Stop,
            QWebEnginePage::OpenLinkInThisWindow,
            QWebEnginePage::OpenLinkInNewWindow,
            QWebEnginePage::OpenLinkInNewTab,
            QWebEnginePage::OpenLinkInNewBackgroundTab,
            QWebEnginePage::DownloadLinkToDisk,
            QWebEnginePage::DownloadImageToDisk,
            QWebEnginePage::DownloadMediaToDisk,
            QWebEnginePage::SavePage,
            QWebEnginePage::ViewSource,
        };

        for (uint i = 0; i < ARRAY_SIZE(unwantedActions); ++i)
            if (QAction* const action = QWebEnginePage_action(page, unwantedActions[i]))
                QWidget_removeAction(reinterpret_cast<QWidget*>(menu), action);

        QAction* inspectAction = nullptr;
        if (_developerToolsEnabled)
        {
            inspectAction = QWebEnginePage_action(page, QWebEnginePage::InspectElement);

            if (inspectAction != nullptr && _devToolsView == nullptr)
                QWidget_addAction(reinterpret_cast<QWidget*>(menu), inspectAction);
        }
        else
        {
            if (QAction* const action = QWebEnginePage_action(page, QWebEnginePage::Reload))
                QWidget_removeAction(reinterpret_cast<QWidget*>(menu), action);
            if (QAction* const action = QWebEnginePage_action(page, QWebEnginePage::ReloadAndBypassCache))
                QWidget_removeAction(reinterpret_cast<QWidget*>(menu), action);
            if (QAction* const action = QWebEnginePage_action(page, QWebEnginePage::InspectElement))
                QWidget_removeAction(reinterpret_cast<QWidget*>(menu), action);
        }

        QAction* const chosen = QMenu_exec(menu, _menuPos, nullptr);

        if (chosen == nullptr || chosen != inspectAction)
            return;

        if (_devToolsView == nullptr)
        {
           #ifdef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
            _devToolsView = new QWebEngineView(static_cast<QWidget*>(nullptr));
           #else
            _devToolsView = new QWebEngineView;
            QWebEngineView__init(_devToolsView, nullptr);
           #endif
            QWidget_setAttribute(reinterpret_cast<QWidget*>(_devToolsView), Qt::WA_DeleteOnClose, true);

            QWebEnginePage_setDevToolsPage(page, QWebEngineView_page(_devToolsView));
            QWebEnginePage_triggerAction(page, QWebEnginePage::InspectElement);

            QObject_installEventFilter(reinterpret_cast<QObject*>(_devToolsView), this);
        }

        QWidget_show(reinterpret_cast<QWidget*>(_devToolsView));
    }

    void sendMessage(const QJsonObject& obj) override
    {
       #ifndef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
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
       #endif

        const QJsonValue json = QJsonObject_value(&obj, _qstrkey);
        QString qstrvalue = QJsonValue_toString(&json);
        QByteArray data = QString_toUtf8(&qstrvalue);

       #ifdef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
        const char* const value = data.data();
       #else
        const uint8_t* const dptr = static_cast<const uint8_t*>(*reinterpret_cast<const void**>(data._));
        const intptr_t offset = isQt5 ? *reinterpret_cast<const intptr_t*>(dptr + 16) : 16;
        const char* const value = reinterpret_cast<const char*>(dptr + offset);
       #endif

        d_debug("js call received with data '%s'", value);

        const size_t len = std::strlen(value) + 1;
        RingBufferControl<WebViewSharedBuffer> rbctrl2;
        rbctrl2.setRingBuffer(&_rb->server, false);
        rbctrl2.writeUInt(kWebViewMessageCallback) &&
        rbctrl2.writeUInt(len) &&
        rbctrl2.writeCustomData(value, len);
        rbctrl2.commitWrite();

       #ifndef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
        // QByteArray and QString destructors are inlined and can't be called from here, call their next closest thing
        QByteArray_clear(&data);
        QString_setRawData(&qstrvalue, nullptr, 0);
        QJsonValue__deinit(&json);
       #endif
    }
};

// --------------------------------------------------------------------------------------------------------------------
// qt5webengine variant

static bool qtwebengine(const int qtVersion,
                        Display* const display,
                        const WebViewInitOptions& options,
                        WebViewRingBuffer* const shmptr)
{
   #ifndef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
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
    typedef void (*QApplication_postEvent_t)(QObject*, QEvent*, Qt::EventPriority);
    typedef void (*QApplication_quit_t)();
    typedef void (*QApplication_setAttribute_t)(Qt::ApplicationAttribute, bool);
    typedef QJsonValue (*QJsonObject_value_t)(const QJsonObject*, const QString &);
    typedef QString (*QJsonValue_toString_t)(const QJsonValue*);
    typedef void (*QObject_installEventFilter_t)(QObject*, QObject*);
    typedef void (*QUrl__init_t)(QUrl*, const QString&, QUrl::ParsingMode);
    typedef void (*QWebChannel__init_t)(QWebChannel*, QObject*);
    typedef void (*QWebChannel_registerObject_t)(QWebChannel*, const QString&, QObject*);
    typedef void (*QWebEnginePage__init_t)(QWebEnginePage*, QWebEngineProfile*, QObject*);
    typedef void (*QWebEnginePage_setBackgroundColor_t)(QWebEnginePage*, const QColor&);
    typedef QWebChannel* (*QWebEnginePage_webChannel_t)(QWebEnginePage*);
    typedef QWebEngineProfile* (*QWebEngineProfile_defaultProfile_t)();
    typedef void (*QWebEngineProfile_installUrlSchemeHandler_t)(QWebEngineProfile*, const QByteArray&, QWebEngineUrlSchemeHandler*);
    typedef void (*QWebEngineProfile_setHttpCacheType_t)(QWebEngineProfile*, QWebEngineProfile::HttpCacheType);
    typedef QWebEngineSettings* (*QWebEngineProfile_settings_t)(QWebEngineProfile*);
    typedef QWebEngineScriptCollection* (*QWebEngineProfile_scripts_t)(QWebEngineProfile*);
    typedef void (*QWebEngineScript__init_t)(QWebEngineScript*);
    typedef void (*QWebEngineScript_setInjectionPoint_t)(QWebEngineScript*, QWebEngineScript::InjectionPoint);
    typedef void (*QWebEngineScript_setRunsOnSubFrames_t)(QWebEngineScript*, bool);
    typedef void (*QWebEngineScript_setSourceCode_t)(QWebEngineScript*, const QString &);
    typedef void (*QWebEngineScript_setWorldId_t)(QWebEngineScript*, uint32_t);
    typedef void (*QWebEngineScriptCollection_insert_t)(QWebEngineScriptCollection*, QWebEngineScript&);
    typedef void (*QWebEngineSettings_setAttribute_t)(QWebEngineSettings*, QWebEngineSettings::WebAttribute, bool);
    // typedef void (*QWebEngineUrlRequestJob_reply_t)(QWebEngineUrlRequestJob*, const QByteArray&, QIODevice*);
    typedef void (*QWebEngineUrlScheme__init_t)(QWebEngineUrlScheme*, const QByteArray&);
    typedef void (*QWebEngineUrlScheme_registerScheme_t)(QWebEngineUrlScheme&);
    typedef void (*QWebEngineUrlScheme_setFlags_t)(QWebEngineUrlScheme*, QWebEngineUrlScheme::Flags);
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
    typedef void (*QWidget_setContextMenuPolicy_t)(QWidget*, Qt::ContextMenuPolicy);
    typedef QWindow* (*QWindow_fromWinId_t)(ulonglong);
    typedef void (*QWindow_setParent_t)(QWindow*, void*);

    CPPSYM(QApplication__init, _ZN12QApplicationC1ERiPPci)
    CPPSYM(QApplication_exec, _ZN15QGuiApplication4execEv)
    CPPSYM(QApplication_postEvent, _ZN16QCoreApplication9postEventEP7QObjectP6QEventi)
    CPPSYM(QApplication_quit, _ZN16QCoreApplication4quitEv)
    CPPSYM(QApplication_setAttribute, _ZN16QCoreApplication12setAttributeEN2Qt20ApplicationAttributeEb)
    CPPSYM(QJsonObject_value, _ZNK11QJsonObject5valueERK7QString)
    CPPSYM(QJsonValue_toString, _ZNK10QJsonValue8toStringEv)
    CPPSYM(QObject_installEventFilter, _ZN7QObject18installEventFilterEPS_)
    CPPSYM(QUrl__init, _ZN4QUrlC1ERK7QStringNS_11ParsingModeE)
    CPPSYM(QWebChannel__init, _ZN11QWebChannelC1EP7QObject)
    CPPSYM(QWebChannel_registerObject, _ZN11QWebChannel14registerObjectERK7QStringP7QObject)
    CPPSYM(QWebEnginePage__init, _ZN14QWebEnginePageC1EP17QWebEngineProfileP7QObject)
    CPPSYM(QWebEnginePage_setBackgroundColor, _ZN14QWebEnginePage18setBackgroundColorERK6QColor)
    CPPSYM(QWebEnginePage_webChannel, _ZNK14QWebEnginePage10webChannelEv)
    CPPSYM(QWebEngineProfile_defaultProfile, _ZN17QWebEngineProfile14defaultProfileEv)
    CPPSYM(QWebEngineProfile_installUrlSchemeHandler, _ZN17QWebEngineProfile23installUrlSchemeHandlerERK10QByteArrayP26QWebEngineUrlSchemeHandler)
    CPPSYM(QWebEngineProfile_setHttpCacheType, _ZN17QWebEngineProfile16setHttpCacheTypeENS_13HttpCacheTypeE)
    CPPSYM(QWebEngineProfile_settings, _ZNK17QWebEngineProfile8settingsEv)
    CPPSYM(QWebEngineProfile_scripts, _ZNK17QWebEngineProfile7scriptsEv)
    CPPSYM(QWebEngineScript__init, _ZN16QWebEngineScriptC1Ev)
    CPPSYM(QWebEngineScript_setInjectionPoint, _ZN16QWebEngineScript17setInjectionPointENS_14InjectionPointE)
    CPPSYM(QWebEngineScript_setRunsOnSubFrames, _ZN16QWebEngineScript18setRunsOnSubFramesEb)
    CPPSYM(QWebEngineScript_setSourceCode, _ZN16QWebEngineScript13setSourceCodeERK7QString)
    CPPSYM(QWebEngineScript_setWorldId, _ZN16QWebEngineScript10setWorldIdEj)
    CPPSYM(QWebEngineScriptCollection_insert, _ZN26QWebEngineScriptCollection6insertERK16QWebEngineScript)
    CPPSYM(QWebEngineSettings_setAttribute, _ZN18QWebEngineSettings12setAttributeENS_12WebAttributeEb)
    // CPPSYM(QWebEngineUrlRequestJob_reply, _ZN23QWebEngineUrlRequestJob5replyERK10QByteArrayP9QIODevice)
    CPPSYM(QWebEngineUrlScheme__init, _ZN19QWebEngineUrlSchemeC1ERK10QByteArray)
    CPPSYM(QWebEngineUrlScheme_registerScheme, _ZN19QWebEngineUrlScheme14registerSchemeERKS_)
    CPPSYM(QWebEngineUrlScheme_setFlags, _ZN19QWebEngineUrlScheme8setFlagsE6QFlagsINS_4FlagEE)
    CPPSYM(QWebEngineUrlScheme_setSyntax, _ZN19QWebEngineUrlScheme9setSyntaxENS_6SyntaxE)
    CPPSYM(QWebEngineUrlSchemeHandler__init, _ZN26QWebEngineUrlSchemeHandlerC1EP7QObject)
    CPPSYM(QWebEngineView__init, _ZN14QWebEngineViewC1EP7QWidget)
    CPPSYM(QWebEngineView_move, _ZN7QWidget4moveERK6QPoint)
    CPPSYM(QWebEngineView_resize, _ZN7QWidget6resizeERK5QSize)
    CPPSYM(QWebEngineView_setPage, _ZN14QWebEngineView7setPageEP14QWebEnginePage)
    CPPSYM(QWebEngineView_setUrl, _ZN14QWebEngineView6setUrlERK4QUrl)
    CPPSYM(QWebEngineView_show, _ZN7QWidget4showEv)
    CPPSYM(QWebEngineView_winId, _ZNK7QWidget5winIdEv)
    CPPSYM(QWebEngineView_windowHandle, _ZNK7QWidget12windowHandleEv)
    CPPSYM(QWidget_setContextMenuPolicy, _ZN7QWidget20setContextMenuPolicyEN2Qt17ContextMenuPolicyE)
    CPPSYM(QWindow_fromWinId, _ZN7QWindow9fromWinIdEy)
    CPPSYM(QWindow_setParent, _ZN7QWindow9setParentEPS_)
   #endif

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
    std::snprintf(scale, 7, "%.2f", options.scaleFactor);
    setenv("QT_SCALE_FACTOR", scale, 1);

   #ifdef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
    QByteArray urlSchemeName("dpf", 3);
   #else
    QByteArray urlSchemeName;
    QByteArray__init(&urlSchemeName, "dpf", 3);
   #endif

    const QWebEngineUrlScheme::Flags urlSchemeFlags = static_cast<QWebEngineUrlScheme::Flags>(0
        | QWebEngineUrlScheme::SecureScheme
        | QWebEngineUrlScheme::LocalScheme
        | QWebEngineUrlScheme::LocalAccessAllowed
        | QWebEngineUrlScheme::ServiceWorkersAllowed
        | QWebEngineUrlScheme::ContentSecurityPolicyIgnored
        | (options.developerToolsEnabled ? QWebEngineUrlScheme::ViewSourceAllowed : 0)
        );
   #ifdef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
    QWebEngineUrlScheme urlScheme(urlSchemeName);
   #else
    QWebEngineUrlScheme urlScheme;
    QWebEngineUrlScheme__init(&urlScheme, urlSchemeName);
   #endif
    QWebEngineUrlScheme_setSyntax(&urlScheme, QWebEngineUrlScheme::Syntax::Path);
    QWebEngineUrlScheme_setFlags(&urlScheme, urlSchemeFlags);
    QWebEngineUrlScheme_registerScheme(urlScheme);

   #if !defined(QT_VERSION_MAJOR) || (QT_VERSION_MAJOR < 6)
    if (qtVersion == 5)
    {
        QApplication_setAttribute(Qt::AA_X11InitThreads, true);
        QApplication_setAttribute(Qt::AA_UseHighDpiPixmaps, true);
        QApplication_setAttribute(Qt::AA_EnableHighDpiScaling, true);
    }
   #endif

    static int argc = 1;
    static char argv0[] = "dpf-webview";
    static char* argv[] = { argv0, nullptr };

   #ifdef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
    QApplication app(argc, argv, 0);
   #else
    QApplication app;
    QApplication__init(&app, argc, argv, 0);
   #endif

    QString qstrchannel, qstrmcode, qstrurl;
    {
        static constexpr const char* channel_src = "external";
       #ifdef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
        qstrchannel = channel_src;
       #else
        const size_t channel_len = std::strlen(channel_src);
        ushort* const channel_qchar = new ushort[channel_len + 1];

        for (size_t i = 0; i < channel_len; ++i)
            channel_qchar[i] = channel_src[i];

        channel_qchar[channel_len] = 0;

        QString__init(&qstrchannel, reinterpret_cast<QChar*>(channel_qchar), channel_len);

        delete[] channel_qchar;
       #endif
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
       #ifdef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
        qstrmcode = mcode_src;
       #else
        const size_t mcode_len = std::strlen(mcode_src);
        ushort* const mcode_qchar = new ushort[mcode_len + 1];

        for (size_t i = 0; i < mcode_len; ++i)
            mcode_qchar[i] = mcode_src[i];

        mcode_qchar[mcode_len] = 0;

        QString__init(&qstrmcode, reinterpret_cast<QChar*>(mcode_qchar), mcode_len);

        delete[] mcode_qchar;
       #endif
    }
    {
       #ifdef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
        qstrurl = options.url;
       #else
        const size_t url_len = std::strlen(options.url);
        ushort* const url_qchar = new ushort[url_len + 1];

        for (size_t i = 0; i < url_len; ++i)
            url_qchar[i] = options.url[i];

        url_qchar[url_len] = 0;

        QString__init(&qstrurl, reinterpret_cast<QChar*>(url_qchar), url_len);

        delete[] url_qchar;
       #endif
    }

   #ifdef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
    QUrl qurl(qstrurl, QUrl::StrictMode);
   #else
    QUrl qurl;
    QUrl__init(&qurl, qstrurl, QUrl::StrictMode);
   #endif

    QWebEngineProfile* const profile = QWebEngineProfile_defaultProfile();
    QWebEngineProfile_setHttpCacheType(profile, QWebEngineProfile::NoCache);

    QWebEngineScriptCollection* const scripts = QWebEngineProfile_scripts(profile);
    QWebEngineSettings* const settings = QWebEngineProfile_settings(profile);

    {
        QWebEngineScript mscript;
       #ifndef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
        QWebEngineScript__init(&mscript);
       #endif
        QWebEngineScript_setInjectionPoint(&mscript, QWebEngineScript::DocumentCreation);
        QWebEngineScript_setRunsOnSubFrames(&mscript, true);
        QWebEngineScript_setSourceCode(&mscript, qstrmcode);
        QWebEngineScript_setWorldId(&mscript, QWebEngineScript::MainWorld);
        QWebEngineScriptCollection_insert(scripts, mscript);
    }

    if (options.initialJS != nullptr)
    {
        QString qstrcode;
        {
           #ifdef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
            qstrcode = options.initialJS;
           #else
            const size_t code_len = std::strlen(options.initialJS);
            ushort* const code_qchar = new ushort[code_len + 1];

            for (size_t i = 0; i < code_len; ++i)
                code_qchar[i] = options.initialJS[i];

            code_qchar[code_len] = 0;

            QString__init(&qstrcode, reinterpret_cast<QChar*>(code_qchar), code_len);
           #endif
        }

        QWebEngineScript script;
       #ifndef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
        QWebEngineScript__init(&script);
       #endif
        QWebEngineScript_setInjectionPoint(&script, QWebEngineScript::DocumentCreation);
        QWebEngineScript_setRunsOnSubFrames(&script, true);
        QWebEngineScript_setSourceCode(&script, qstrcode);
        QWebEngineScript_setWorldId(&script, QWebEngineScript::MainWorld);
        QWebEngineScriptCollection_insert(scripts, script);
    }

    QWebEngineSettings_setAttribute(settings, QWebEngineSettings::JavascriptCanAccessClipboard, true);
    QWebEngineSettings_setAttribute(settings, QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    QWebEngineSettings_setAttribute(settings, QWebEngineSettings::PlaybackRequiresUserGesture, false);
    QWebEngineSettings_setAttribute(settings, QWebEngineSettings::JavascriptCanPaste, true);

   #ifdef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
    QWebEngineView webview(static_cast<QWidget*>(nullptr));
   #else
    QWebEngineView webview;
    QWebEngineView__init(&webview, nullptr);
   #endif

   #ifdef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
    QWebEnginePage page(profile, &webview);
   #else
    QWebEnginePage page;
    QWebEnginePage__init(&page, profile, reinterpret_cast<QObject*>(&webview));
   #endif

    if (options.backgroundColor != 0xffffffff)
    {
        const QColor color(
            (options.backgroundColor >> 24) & 0xff,
            (options.backgroundColor >> 16) & 0xff,
            (options.backgroundColor >> 8) & 0xff,
            options.backgroundColor & 0xff
        );
        QWebEnginePage_setBackgroundColor(&page, color);
    }

    WebViewEventFilter eventFilter(shmptr, &webview, display, options.developerToolsEnabled);
    QObject_installEventFilter(reinterpret_cast<QObject*>(&webview), &eventFilter);
    QWidget_setContextMenuPolicy(reinterpret_cast<QWidget*>(&webview), Qt::CustomContextMenu);

   #ifdef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
    QWebChannel channel(&webview);
   #else
    QWebChannel channel;
    QWebChannel__init(&channel, reinterpret_cast<QObject*>(&webview));
   #endif
    QWebChannel_registerObject(&channel, qstrchannel, &eventFilter);
    QWebEnginePage_setWebChannel(&page, &channel, 0);

    QWebEngineView_move(&webview, QPoint(options.x, options.y));
    QWebEngineView_resize(&webview, QSize(static_cast<int>(options.width / options.scaleFactor),
                                          static_cast<int>(options.height / options.scaleFactor)));
    QWebEngineView_winId(&webview);
    QWindow_setParent(QWebEngineView_windowHandle(&webview), QWindow_fromWinId(options.winId));

    QWebEngineView_setPage(&webview, &page);
    QWebEngineView_setUrl(&webview, qurl);

    const uintptr_t webviewWinId = QWebEngineView_winId(&webview);
    eventFilter.setWinId(webviewWinId);

    // FIXME Qt6 seems to need some forcing..
    if (qtVersion >= 6)
    {
        XReparentWindow(display, webviewWinId, options.winId, options.x, options.y);
        XFlush(display);
    }

    QWebEngineView_show(&webview);

    struct QtWebFramework : WebFramework {
        const int _qtVersion;
        WebViewRingBuffer* const _shmptr;
        const QUrl& _qurl;
        QWebEnginePage& _page;
        QWebEngineView& _webview;
        WebViewEventFilter& _eventFilter;
       #ifndef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
        const QString__init_t QString__init;
        const QWebEnginePage_runJavaScript_compat_t QWebEnginePage_runJavaScript_compat;
        const QWebEnginePage_runJavaScript_t QWebEnginePage_runJavaScript;
        const QWebEngineView_setUrl_t QWebEngineView_setUrl;
        const QApplication_quit_t QApplication_quit;
        const QApplication_postEvent_t QApplication_postEvent;
       #endif

        QtWebFramework(const int qtVersion,
                       WebViewRingBuffer* const shmptr,
                       const QUrl& qurl,
                       QWebEnginePage& page,
                       QWebEngineView& webview,
                       WebViewEventFilter& eventFilter
                    #ifndef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
                     , const QString__init_t _QString__init,
                       const QWebEnginePage_runJavaScript_compat_t _QWebEnginePage_runJavaScript_compat,
                       const QWebEnginePage_runJavaScript_t _QWebEnginePage_runJavaScript,
                       const QWebEngineView_setUrl_t _QWebEngineView_setUrl,
                       const QApplication_quit_t _QApplication_quit,
                       const QApplication_postEvent_t _QApplication_postEvent
                    #endif
                       )
            : _qtVersion(qtVersion),
              _shmptr(shmptr),
              _qurl(qurl),
              _page(page),
              _webview(webview),
              _eventFilter(eventFilter)
           #ifndef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
            , QString__init(_QString__init),
              QWebEnginePage_runJavaScript_compat(_QWebEnginePage_runJavaScript_compat),
              QWebEnginePage_runJavaScript(_QWebEnginePage_runJavaScript),
              QWebEngineView_setUrl(_QWebEngineView_setUrl),
              QApplication_quit(_QApplication_quit),
              QApplication_postEvent(_QApplication_postEvent)
           #endif
        {
        }

        void evaluate(const char* const js) override
        {
            QString qstrjs;
            {
               #ifdef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
                qstrjs = js;
               #else
                const size_t js_len = std::strlen(js);
                ushort* const js_qchar = new ushort[js_len + 1];

                for (size_t i = 0; i < js_len; ++i)
                    js_qchar[i] = js[i];

                js_qchar[js_len] = 0;

                QString__init(&qstrjs, reinterpret_cast<const QChar*>(js_qchar), js_len);
               #endif
            }

            if (_qtVersion == 5)
                QWebEnginePage_runJavaScript_compat(&_page, qstrjs);
            else
                QWebEnginePage_runJavaScript(&_page,
                                             qstrjs,
                                             0,
                                            #ifdef DISTRHO_PROPER_CPP11_SUPPORT
                                             {}
                                            #else
                                             0
                                            #endif
                                             );
        }

        void reload() override
        {
            QWebEngineView_setUrl(&_webview, _qurl);
        }

        void terminate() override
        {
            if (running)
            {
                running = false;
                webview_wake(&_shmptr->client.sem);
                QApplication_quit();
            }
        }

        void wake(WebViewRingBuffer*) override
        {
            // NOTE event pointer is deleted by Qt
           #ifdef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
            QEvent* const qevent = new QEvent(QEvent::User);
           #else
            static void (*QEvent__init)(QEvent*, QEvent::Type)
                = reinterpret_cast<typeof(QEvent__init)>(dlsym(nullptr, "_ZN6QEventC1ENS_4TypeE"));
            QEvent* const qevent = new QEvent;
            QEvent__init(qevent, QEvent::User);
           #endif
            QApplication_postEvent(&_eventFilter, qevent, Qt::HighEventPriority);
        }
    };

    QtWebFramework webFrameworkObj(qtVersion,
                                   shmptr,
                                   qurl,
                                   page,
                                   webview,
                                   eventFilter
                                #ifndef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
                                 , QString__init,
                                   QWebEnginePage_runJavaScript_compat,
                                   QWebEnginePage_runJavaScript,
                                   QWebEngineView_setUrl,
                                   QApplication_quit,
                                   QApplication_postEvent
                                #endif
                                   );

    webFramework = &webFrameworkObj;

    // notify server we started ok
    webview_wake(&shmptr->server.sem);

    d_stdout("WebView Qt%d main loop started", qtVersion);

    QApplication_exec();

    d_stdout("WebView Qt%d main loop quit", qtVersion);

   #ifndef WEB_VIEW_INCLUDE_QTx_EXPLICITLY
    dlclose(lib);
   #endif
    return true;
}

// -----------------------------------------------------------------------------------------------------------
// startup via ld-linux

static void signalHandler(const int sig)
{
    switch (sig)
    {
    case SIGTERM:
        if (webFramework != nullptr)
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
    WebViewInitOptions options;
    options.url = nullptr;
    options.initialJS = nullptr;

    while (shmptr->valid && webview_timedwait(&shmptr->client.sem))
    {
        if (rbctrl.isDataAvailableForReading())
        {
            DISTRHO_SAFE_ASSERT_RETURN(rbctrl.readUInt() == kWebViewMessageInitData, 1);

            hasInitialData = running = true;
            options.winId = rbctrl.readULong();
            options.width = rbctrl.readUInt();
            options.height = rbctrl.readUInt();
            options.scaleFactor = rbctrl.readDouble();
            options.x = rbctrl.readInt();
            options.y = rbctrl.readInt();
            options.backgroundColor = rbctrl.readUInt();
            options.developerToolsEnabled = rbctrl.readBool();

            const uint urllen = rbctrl.readUInt();
            options.url = static_cast<char*>(std::malloc(urllen));
            rbctrl.readCustomData(options.url, urllen);

            if (const uint initjslen = rbctrl.readUInt())
            {
                options.initialJS = static_cast<char*>(std::malloc(initjslen));
                rbctrl.readCustomData(options.initialJS, initjslen);
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

       #if defined(WEB_VIEW_INCLUDE_GTK3_EXPLICITLY)
        if (! gtk3(display, options, shmptr))
       #elif defined(WEB_VIEW_INCLUDE_QTx_EXPLICITLY)
        if (! qtwebengine(QT_VERSION_MAJOR, display, options, shmptr))
       #else
        if (! qtwebengine(5, display, options, shmptr) &&
            ! qtwebengine(6, display, options, shmptr) &&
            ! gtk3(display, options, shmptr))
       #endif
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

    std::free(options.url);
    std::free(options.initialJS);

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
