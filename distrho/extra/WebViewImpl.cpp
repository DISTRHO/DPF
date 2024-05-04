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

#if !defined(DISTRHO_WEBVIEW_HPP_INCLUDED) && !defined(DGL_WEBVIEW_HPP_INCLUDED)
# error bad include
#endif
#if !defined(WEBVIEW_DISTRHO_NAMESPACE) && !defined(WEBVIEW_DGL_NAMESPACE)
# error bad usage
#endif

#define WEBVIEW_USING_CHOC 1

#ifndef WEBVIEW_USING_CHOC
# define WEBVIEW_USING_CHOC 0
#elif WEBVIEW_USING_CHOC && !defined(DISTRHO_OS_WINDOWS)
# undef WEBVIEW_USING_CHOC
# define WEBVIEW_USING_CHOC 0
#endif

#ifdef DISTRHO_OS_MAC
# undef WEBVIEW_USING_MACOS_WEBKIT
# define WEBVIEW_USING_MACOS_WEBKIT 1
#else
# undef WEBVIEW_USING_MACOS_WEBKIT
# define WEBVIEW_USING_MACOS_WEBKIT 0
#endif

#if defined(HAVE_X11) && !(defined(DISTRHO_OS_MAC) || defined(DISTRHO_OS_WINDOWS))
# undef WEBVIEW_USING_X11_IPC
# define WEBVIEW_USING_X11_IPC 1
#else
# undef WEBVIEW_USING_X11_IPC
# define WEBVIEW_USING_X11_IPC 0
#endif

#if WEBVIEW_USING_CHOC
# define WC_ERR_INVALID_CHARS 0
# include "../CHOC/gui/choc_WebView.h"
#elif WEBVIEW_USING_MACOS_WEBKIT
# include <Cocoa/Cocoa.h>
# include <WebKit/WebKit.h>
#elif WEBVIEW_USING_X11_IPC
// #define QT_NO_VERSION_TAGGING
// #include <QtCore/QChar>
// #include <QtCore/QPoint>
// #include <QtCore/QSize>
// #undef signals
# include "ChildProcess.hpp"
# include "String.hpp"
# include <clocale>
# include <cstdio>
# include <dlfcn.h>
# include <functional>
# include <linux/limits.h>
# include <X11/Xlib.h>
#endif

#ifdef WEBVIEW_DGL_NAMESPACE
START_NAMESPACE_DGL
using DISTRHO_NAMESPACE::String;
#else
START_NAMESPACE_DISTRHO
#endif

// -----------------------------------------------------------------------------------------------------------

struct WebViewData {
#if defined(DISTRHO_OS_MAC)
#elif WEBVIEW_USING_CHOC
    choc::ui::WebView* webview;
#elif WEBVIEW_USING_X11_IPC
    ChildProcess p;
    ::Display* display;
    ::Window childWindow;
    ::Window ourWindow;
#endif
};

// -----------------------------------------------------------------------------------------------------------

#if WEBVIEW_USING_X11_IPC
static void getFilenameFromFunctionPtr(char filename[PATH_MAX], const void* const ptr)
{
    Dl_info info = {};
    dladdr(ptr, &info);

    if (info.dli_fname[0] == '.')
    {
        getcwd(filename, PATH_MAX - 1);
        std::strncat(filename, info.dli_fname + 1, PATH_MAX - 1);
    }
    else if (info.dli_fname[0] != '/')
    {
        getcwd(filename, PATH_MAX - 1);
        std::strncat(filename, "/", PATH_MAX - 1);
        std::strncat(filename, info.dli_fname, PATH_MAX - 1);
    }
    else
    {
        std::strncpy(filename, info.dli_fname, PATH_MAX - 1);
    }
}
#endif

WebViewHandle addWebView(const uintptr_t parentWindowId,
                         const int x,
                         const int y,
                         const uint width,
                         const uint height,
                         const double scaleFactor)
{
#if defined(DISTRHO_OS_MAC)
#elif WEBVIEW_USING_CHOC
    std::unique_ptr<choc::ui::WebView> webview = std::make_unique<choc::ui::WebView>();
    DISTRHO_SAFE_ASSERT_RETURN(webview->loadedOK(), nullptr);

    const HWND handle = static_cast<HWND>(webview->getViewHandle());
    DISTRHO_SAFE_ASSERT_RETURN(handle != nullptr, nullptr);

    webview->navigate("https://mastodon.falktx.com/");

    LONG_PTR flags = GetWindowLongPtr(handle, -16);
    flags = (flags & ~WS_POPUP) | WS_CHILD;
    SetWindowLongPtr(handle, -16, flags);

    SetParent(handle, reinterpret_cast<HWND>(parentWindowId));
    SetWindowPos(handle, nullptr,
                 x * scaleFactor,
                 y * scaleFactor,
                 (width - x) * scaleFactor,
                 (height - y) * scaleFactor,
                 SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    ShowWindow(handle, SW_SHOW);

    WebViewData* const wvdata = new WebViewData();
    wvdata->webview = webview.release();
    return wvdata;
#elif WEBVIEW_USING_X11_IPC
    char ldlinux[PATH_MAX] = {};
    getFilenameFromFunctionPtr(ldlinux, dlsym(nullptr, "_rtld_global"));

    char filename[PATH_MAX] = {};
    getFilenameFromFunctionPtr(filename, reinterpret_cast<const void*>(addWebView));

    d_stdout("ld-linux is '%s'", ldlinux);
    d_stdout("filename is '%s'", filename);

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
        envp[e++] = ("DPF_WEBVIEW_SCALE_FACTOR=" + String(scaleFactor)).getAndReleaseBuffer();
        envp[e++] = ("DPF_WEBVIEW_WIN_ID=" +String(parentWindowId)).getAndReleaseBuffer();

        for (uint i = e; i < envsize + 5; ++i)
            envp[e++] = nullptr;
    }

    WebViewData* const handle = new WebViewData();
    handle->display = display;
    handle->childWindow = 0;
    handle->ourWindow = parentWindowId;

    const char* const args[] = { ldlinux, filename, "dpf-ld-linux-webview", nullptr };
    handle->p.start(args, envp);

    for (uint i = 0; envp[i] != nullptr; ++i)
        std::free(envp[i]);
    delete[] envp;

    return handle;
#endif

    return nullptr;
}

void destroyWebView(const WebViewHandle handle)
{
#if defined(DISTRHO_OS_MAC)
#elif WEBVIEW_USING_CHOC
    delete handle->webview;
    delete handle;
#elif WEBVIEW_USING_X11_IPC
    XCloseDisplay(handle->display);
    delete handle;
#endif
}

void reloadWebView(const WebViewHandle handle, uint)
{
#if defined(DISTRHO_OS_MAC)
#elif WEBVIEW_USING_CHOC
#elif WEBVIEW_USING_X11_IPC
    handle->p.signal(SIGUSR1);
#endif
}

void resizeWebView(const WebViewHandle handle, int x, int y, uint width, uint height)
{
#if defined(DISTRHO_OS_MAC)
#elif WEBVIEW_USING_CHOC
    const HWND hwnd = static_cast<HWND>(handle->webview->getViewHandle());
    SetWindowPos(hwnd, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
#elif WEBVIEW_USING_X11_IPC
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

    XMoveResizeWindow(handle->display, handle->childWindow, x, y, width, height);
    XFlush(handle->display);
#endif
}

#if WEBVIEW_USING_X11_IPC

// -----------------------------------------------------------------------------------------------------------

static std::function<void()> reloadFn;

// -----------------------------------------------------------------------------------------------------------

struct GtkContainer;
struct GtkPlug;
struct GtkWidget;
struct GtkWindow;
struct WebKitSettings;
struct WebKitWebView;

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

static bool gtk3(Display* const display,
                 const Window winId,
                 const uint x,
                 const uint y,
                 const uint width,
                 const uint height,
                 double scaleFactor,
                 const char* const url)
{
    void* lib;
    if ((lib = dlopen("libwebkit2gtk-4.0.so.37", RTLD_NOW|RTLD_GLOBAL)) == nullptr ||
        (lib = dlopen("libwebkit2gtk-4.0.so", RTLD_NOW|RTLD_GLOBAL)) == nullptr)
        return false;

    using gdk_set_allowed_backends_t = void (*)(const char*);
    using gtk_container_add_t = void (*)(GtkContainer*, GtkWidget*);
    using gtk_init_check_t = bool (*)(int*, char***);
    using gtk_main_t = void (*)();
    using gtk_plug_get_id_t = Window (*)(GtkPlug*);
    using gtk_plug_new_t = GtkWidget* (*)(Window);
    using gtk_widget_show_all_t = void (*)(GtkWidget*);
    using gtk_window_move_t = void (*)(GtkWindow*, int, int);
    using gtk_window_set_default_size_t = void (*)(GtkWindow*, int, int);
    using webkit_settings_new_t = WebKitSettings* (*)();
    using webkit_settings_set_hardware_acceleration_policy_t = void (*)(WebKitSettings*, int);
    using webkit_settings_set_javascript_can_access_clipboard_t = void (*)(WebKitSettings*, bool);
    using webkit_web_view_load_uri_t = void (*)(WebKitWebView*, const char*);
    using webkit_web_view_new_with_settings_t = GtkWidget* (*)(WebKitSettings*);

    CSYM(gdk_set_allowed_backends_t, gdk_set_allowed_backends)
    CSYM(gtk_container_add_t, gtk_container_add)
    CSYM(gtk_init_check_t, gtk_init_check)
    CSYM(gtk_main_t, gtk_main)
    CSYM(gtk_plug_get_id_t, gtk_plug_get_id)
    CSYM(gtk_plug_new_t, gtk_plug_new)
    CSYM(gtk_widget_show_all_t, gtk_widget_show_all)
    CSYM(gtk_window_move_t, gtk_window_move)
    CSYM(gtk_window_set_default_size_t, gtk_window_set_default_size)
    CSYM(webkit_settings_new_t, webkit_settings_new)
    CSYM(webkit_settings_set_hardware_acceleration_policy_t, webkit_settings_set_hardware_acceleration_policy)
    CSYM(webkit_settings_set_javascript_can_access_clipboard_t, webkit_settings_set_javascript_can_access_clipboard)
    CSYM(webkit_web_view_load_uri_t, webkit_web_view_load_uri)
    CSYM(webkit_web_view_new_with_settings_t, webkit_web_view_new_with_settings)

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

    webkit_settings_set_javascript_can_access_clipboard(settings, true);
    webkit_settings_set_hardware_acceleration_policy(settings, 2 /* WEBKIT_HARDWARE_ACCELERATION_POLICY_NEVER */);

    GtkWidget* const webview = webkit_web_view_new_with_settings(settings);
    DISTRHO_SAFE_ASSERT_RETURN(webview != nullptr, false);

    webkit_web_view_load_uri(WEBKIT_WEB_VIEW (webview), url);

    gtk_container_add(GTK_CONTAINER(window), webview);

    gtk_widget_show_all(window);

    Window wid = gtk_plug_get_id(GTK_PLUG(window));
    XMapWindow(display, wid);
    XFlush(display);

    reloadFn = [=](){
        webkit_web_view_load_uri(WEBKIT_WEB_VIEW (webview), url);
    };

    gtk_main();

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

    QApplication_exec();

    dlclose(lib);
    return true;
}
#endif

// -----------------------------------------------------------------------------------------------------------
// startup via ld-linux

static void signalHandler(const int sig)
{
    DISTRHO_SAFE_ASSERT_RETURN(sig == SIGUSR1,);

    reloadFn();
}

int dpf_webview_start(int /* argc */, char** /* argv[] */)
{
    uselocale(newlocale(LC_NUMERIC_MASK, "C", nullptr));

    const char* const envScaleFactor = std::getenv("DPF_WEBVIEW_SCALE_FACTOR");
    DISTRHO_SAFE_ASSERT_RETURN(envScaleFactor != nullptr, 1);

    const char* const envWinId = std::getenv("DPF_WEBVIEW_WIN_ID");
    DISTRHO_SAFE_ASSERT_RETURN(envWinId != nullptr, 1);

    const Window winId = std::strtoul(envWinId, nullptr, 10);
    DISTRHO_SAFE_ASSERT_RETURN(winId != 0, 1);

    const double scaleFactor = std::atof(envScaleFactor);
    DISTRHO_SAFE_ASSERT_RETURN(scaleFactor > 0.0, 1);

    Display* const display = XOpenDisplay(nullptr);
    DISTRHO_SAFE_ASSERT_RETURN(display != nullptr, 1);

//     const char* url = "file:///home/falktx/";
    const char* url = "https://mastodon.falktx.com/";

    struct sigaction sig = {};
    sig.sa_handler = signalHandler;
    sig.sa_flags = SA_RESTART;
    sigemptyset(&sig.sa_mask);
    sigaction(SIGUSR1, &sig, nullptr);

//     qt5webengine(winId, scaleFactor, url) ||
//     qt6webengine(winId, scaleFactor, url) ||
    gtk3(display, winId, 0, 0, 600, 400, scaleFactor, url);

    XCloseDisplay(display);

    return 0;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WEBVIEW_USING_X11_IPC

#ifdef WEBVIEW_DGL_NAMESPACE
END_NAMESPACE_DGL
#else
END_NAMESPACE_DISTRHO
#endif

#undef WEBVIEW_DISTRHO_NAMESPACE
#undef WEBVIEW_DGL_NAMESPACE
#undef WEBVIEW_NAMESPACE
