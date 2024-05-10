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

#if !defined(DISTRHO_FILE_BROWSER_DIALOG_HPP_INCLUDED) && !defined(DGL_FILE_BROWSER_DIALOG_HPP_INCLUDED)
# error bad include
#endif
#if !defined(FILE_BROWSER_DIALOG_DISTRHO_NAMESPACE) && !defined(FILE_BROWSER_DIALOG_DGL_NAMESPACE)
# error bad usage
#endif

#include "ScopedPointer.hpp"
#include "String.hpp"

#ifdef DISTRHO_OS_MAC
# import <Cocoa/Cocoa.h>
#endif

#ifdef DISTRHO_OS_WASM
# include <emscripten/emscripten.h>
# include <sys/stat.h>
#endif

#ifdef DISTRHO_OS_WINDOWS
# include <direct.h>
# include <process.h>
# include <winsock2.h>
# include <windows.h>
# include <commdlg.h>
# include <vector>
#else
# include <unistd.h>
#endif

#ifdef HAVE_DBUS
# include <dbus/dbus.h>
#endif

#ifdef HAVE_X11
# define DBLCLKTME 400
# include "sofd/libsofd.h"
# if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wcast-qual"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wfloat-conversion"
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#  pragma GCC diagnostic ignored "-Wstrict-overflow"
# endif
# include "sofd/libsofd.c"
# if defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#  pragma GCC diagnostic pop
# endif
# undef HAVE_MNTENT
# undef MAX
# undef MIN
#endif

#ifdef FILE_BROWSER_DIALOG_DGL_NAMESPACE
START_NAMESPACE_DGL
using DISTRHO_NAMESPACE::ScopedPointer;
using DISTRHO_NAMESPACE::String;
#else
START_NAMESPACE_DISTRHO
#endif

// --------------------------------------------------------------------------------------------------------------------

// static pointer used for signal null/none action taken
static const char* const kSelectedFileCancelled = "__dpf_cancelled__";

#ifdef HAVE_DBUS
static constexpr bool isHexChar(const char c) noexcept
{
    return c >= '0' && c <= 'f' && (c <= '9' || (c >= 'A' && c <= 'F') || c >= 'a');
}

static constexpr int toHexChar(const char c) noexcept
{
    return c >= '0' && c <= '9' ? c - '0' : (c >= 'A' && c <= 'F' ? c - 'A' : c - 'a') + 10;
}
#endif

// --------------------------------------------------------------------------------------------------------------------

#ifdef DISTRHO_OS_WASM
# define DISTRHO_WASM_NAMESPACE_MACRO_HELPER(NS, SEP, FUNCTION) NS ## SEP ## FUNCTION
# define DISTRHO_WASM_NAMESPACE_MACRO(NS, FUNCTION) DISTRHO_WASM_NAMESPACE_MACRO_HELPER(NS, _, FUNCTION)
# define DISTRHO_WASM_NAMESPACE_HELPER(NS) #NS
# define DISTRHO_WASM_NAMESPACE(NS) DISTRHO_WASM_NAMESPACE_HELPER(NS)
# define fileBrowserSetPathNamespaced DISTRHO_WASM_NAMESPACE_MACRO(FILE_BROWSER_DIALOG_NAMESPACE, fileBrowserSetPath)
# define fileBrowserSetPathFuncName DISTRHO_WASM_NAMESPACE(FILE_BROWSER_DIALOG_NAMESPACE) "_fileBrowserSetPath"

// FIXME use world class name as prefix
static bool openWebBrowserFileDialog(const char* const funcname, void* const handle)
{
    const char* const nameprefix = DISTRHO_WASM_NAMESPACE(FILE_BROWSER_DIALOG_NAMESPACE);

    return EM_ASM_INT({
        var canvasFileObjName = UTF8ToString($0) + "_file_open";
        var canvasFileOpenElem = document.getElementById(canvasFileObjName);

        var jsfuncname = UTF8ToString($1);
        var jsfunc = Module.cwrap(jsfuncname, 'null', ['number', 'string']);

        if (canvasFileOpenElem) {
            document.body.removeChild(canvasFileOpenElem);
        }

        canvasFileOpenElem = document.createElement('input');
        canvasFileOpenElem.type = 'file';
        canvasFileOpenElem.id = canvasFileObjName;
        canvasFileOpenElem.style.display = 'none';
        document.body.appendChild(canvasFileOpenElem);

        canvasFileOpenElem.onchange = function(e) {
            if (!canvasFileOpenElem.files) {
                jsfunc($2, "");
                return;
            }

            // store uploaded files inside a specific dir
            try {
                Module.FS.mkdir('/userfiles');
            } catch (e) {}

            var file = canvasFileOpenElem.files[0];
            var filename = '/userfiles/' + file.name;
            var reader = new FileReader();

            reader.onloadend = function(e) {
                var content = new Uint8Array(reader.result);
                Module.FS.writeFile(filename, content);
                jsfunc($2, filename);
            };

            reader.readAsArrayBuffer(file);
        };

        canvasFileOpenElem.click();
        return 1;
    }, nameprefix, funcname, handle) != 0;
}

static bool downloadWebBrowserFile(const char* const filename)
{
    const char* const nameprefix = DISTRHO_WASM_NAMESPACE(FILE_BROWSER_DIALOG_NAMESPACE);

    return EM_ASM_INT({
        var canvasFileObjName = UTF8ToString($0) + "_file_save";
        var jsfilename = UTF8ToString($1);

        var canvasFileSaveElem = document.getElementById(canvasFileObjName);
        if (canvasFileSaveElem) {
            // only 1 file save allowed at once
            console.warn("One file save operation already in progress, refusing to open another");
            return 0;
        }

        canvasFileSaveElem = document.createElement('a');
        canvasFileSaveElem.download = jsfilename;
        canvasFileSaveElem.id = canvasFileObjName;
        canvasFileSaveElem.style.display = 'none';
        document.body.appendChild(canvasFileSaveElem);

        var content = Module.FS.readFile('/userfiles/' + jsfilename);
        canvasFileSaveElem.href = URL.createObjectURL(new Blob([content]));
        canvasFileSaveElem.click();

        setTimeout(function() {
            URL.revokeObjectURL(canvasFileSaveElem.href);
            document.body.removeChild(canvasFileSaveElem);
        }, 2000);
        return 1;
    }, nameprefix, filename) != 0;
}
#endif

// --------------------------------------------------------------------------------------------------------------------

struct FileBrowserData {
    const char* selectedFile;

#ifdef DISTRHO_OS_MAC
    NSSavePanel* nsBasePanel;
    NSOpenPanel* nsOpenPanel;
#endif
#ifdef HAVE_DBUS
    DBusConnection* dbuscon;
#endif
#ifdef HAVE_X11
    Display* x11display;
#endif

#ifdef DISTRHO_OS_WASM
    char* defaultName;
    bool saving;
#endif

#ifdef DISTRHO_OS_WINDOWS
    OPENFILENAMEW ofn;
    volatile bool threadCancelled;
    uintptr_t threadHandle;
    std::vector<WCHAR> fileNameW;
    std::vector<WCHAR> startDirW;
    std::vector<WCHAR> titleW;
    const bool saving;
    bool isEmbed;

    FileBrowserData(const bool save)
        : selectedFile(nullptr),
          threadCancelled(false),
          threadHandle(0),
          fileNameW(32768),
          saving(save),
          isEmbed(false)
    {
        std::memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.lpstrFile = fileNameW.data();
        ofn.nMaxFile = (DWORD)fileNameW.size();
    }

    ~FileBrowserData()
    {
        if (cancelAndStop())
            free();
    }

    void setupAndStart(const bool embed,
                       const char* const startDir,
                       const char* const windowTitle,
                       const uintptr_t winId,
                       const FileBrowserOptions options)
    {
        isEmbed = embed;

        ofn.hwndOwner = (HWND)winId;

        ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
        if (options.buttons.showHidden == FileBrowserOptions::kButtonVisibleChecked)
            ofn.Flags |= OFN_FORCESHOWHIDDEN;

        ofn.FlagsEx = 0x0;
        if (options.buttons.showPlaces == FileBrowserOptions::kButtonInvisible)
            ofn.FlagsEx |= OFN_EX_NOPLACESBAR;

        startDirW.resize(std::strlen(startDir) + 1);
        if (MultiByteToWideChar(CP_UTF8, 0, startDir, -1, startDirW.data(), static_cast<int>(startDirW.size())))
            ofn.lpstrInitialDir = startDirW.data();

        titleW.resize(std::strlen(windowTitle) + 1);
        if (MultiByteToWideChar(CP_UTF8, 0, windowTitle, -1, titleW.data(), static_cast<int>(titleW.size())))
            ofn.lpstrTitle = titleW.data();

        uint threadId;
        threadCancelled = false;
        threadHandle = _beginthreadex(nullptr, 0, _run, this, 0, &threadId);
    }

    bool cancelAndStop()
    {
        threadCancelled = true;

        if (threadHandle == 0)
            return true;

        // if previous dialog running, carefully close its window
        const HWND owner = isEmbed ? GetParent(ofn.hwndOwner) : ofn.hwndOwner;

        if (owner != nullptr && owner != INVALID_HANDLE_VALUE)
        {
            const HWND window = GetWindow(owner, GW_HWNDFIRST);

            if (window != nullptr && window != INVALID_HANDLE_VALUE)
            {
                SendMessage(window, WM_SYSCOMMAND, SC_CLOSE, 0);
                SendMessage(window, WM_CLOSE, 0, 0);
                WaitForSingleObject((HANDLE)threadHandle, 5000);
            }
        }

        if (threadHandle == 0)
            return true;

        // not good if thread still running, but let's close the handle anyway
        CloseHandle((HANDLE)threadHandle);
        threadHandle = 0;
        return false;
    }

    void run()
    {
        const char* nextFile = nullptr;

        if (saving ? GetSaveFileNameW(&ofn) : GetOpenFileNameW(&ofn))
        {
            if (threadCancelled)
            {
                threadHandle = 0;
                return;
            }

            // back to UTF-8
            std::vector<char> fileNameA(4 * 32768);
            if (WideCharToMultiByte(CP_UTF8, 0, fileNameW.data(), -1,
                                    fileNameA.data(), (int)fileNameA.size(),
                                    nullptr, nullptr))
            {
                nextFile = strdup(fileNameA.data());
            }
        }

        if (threadCancelled)
        {
            threadHandle = 0;
            return;
        }

        if (nextFile == nullptr)
            nextFile = kSelectedFileCancelled;

        selectedFile = nextFile;
        threadHandle = 0;
    }

    static unsigned __stdcall _run(void* const arg)
    {
        // CoInitializeEx(nullptr, COINIT_MULTITHREADED);
        static_cast<FileBrowserData*>(arg)->run();
        // CoUninitialize();
        _endthreadex(0);
        return 0;
    }
#else // DISTRHO_OS_WINDOWS
    FileBrowserData(const bool save)
        : selectedFile(nullptr)
       #ifdef DISTRHO_OS_MAC
        , nsBasePanel(nullptr)
        , nsOpenPanel(nullptr)
       #endif
       #ifdef HAVE_DBUS
        , dbuscon(nullptr)
       #endif
       #ifdef HAVE_X11
        , x11display(nullptr)
       #endif
    {
#ifdef DISTRHO_OS_MAC
        if (save)
        {
            nsOpenPanel = nullptr;
            nsBasePanel = [[NSSavePanel savePanel]retain];
        }
        else
        {
            nsOpenPanel = [[NSOpenPanel openPanel]retain];
            nsBasePanel = nsOpenPanel;
        }
#endif
#ifdef DISTRHO_OS_WASM
        defaultName = nullptr;
        saving = save;
#endif
#ifdef HAVE_DBUS
        if ((dbuscon = dbus_bus_get(DBUS_BUS_SESSION, nullptr)) != nullptr)
            dbus_connection_set_exit_on_disconnect(dbuscon, false);
#endif
#ifdef HAVE_X11
        x11display = XOpenDisplay(nullptr);
#endif

        // maybe unused
        return; (void)save;
    }

    ~FileBrowserData()
    {
#ifdef DISTRHO_OS_MAC
        [nsBasePanel release];
#endif
#ifdef DISTRHO_OS_WASM
        std::free(defaultName);
#endif
#ifdef HAVE_DBUS
        if (dbuscon != nullptr)
            dbus_connection_unref(dbuscon);
#endif
#ifdef HAVE_X11
        if (x11display != nullptr)
            XCloseDisplay(x11display);
#endif

        free();
    }
#endif

    void free()
    {
        if (selectedFile == nullptr)
            return;

        if (selectedFile == kSelectedFileCancelled || std::strcmp(selectedFile, kSelectedFileCancelled) == 0)
        {
            selectedFile = nullptr;
            return;
        }

        std::free(const_cast<char*>(selectedFile));
        selectedFile = nullptr;
    }

    DISTRHO_DECLARE_NON_COPYABLE(FileBrowserData)
};

// --------------------------------------------------------------------------------------------------------------------

#ifdef DISTRHO_OS_WASM
extern "C" {
EMSCRIPTEN_KEEPALIVE
void fileBrowserSetPathNamespaced(FileBrowserHandle handle, const char* filename)
{
    handle->free();

    if (filename != nullptr && filename[0] != '\0')
        handle->selectedFile = strdup(filename);
    else
        handle->selectedFile = kSelectedFileCancelled;
}
}
#endif

FileBrowserHandle fileBrowserCreate(const bool isEmbed,
                                    const uintptr_t windowId,
                                    const double scaleFactor,
                                    const FileBrowserOptions& options)
{
    String startDir(options.startDir);

    if (startDir.isEmpty())
    {
#ifdef DISTRHO_OS_WINDOWS
        if (char* const cwd = _getcwd(nullptr, 0))
#else
        if (char* const cwd = getcwd(nullptr, 0))
#endif
        {
            startDir = cwd;
            std::free(cwd);
        }
    }

    DISTRHO_SAFE_ASSERT_RETURN(startDir.isNotEmpty(), nullptr);

    if (! startDir.endsWith(DISTRHO_OS_SEP))
        startDir += DISTRHO_OS_SEP_STR;

    String windowTitle(options.title);

    if (windowTitle.isEmpty())
        windowTitle = "FileBrowser";

    ScopedPointer<FileBrowserData> handle(new FileBrowserData(options.saving));

#ifdef DISTRHO_OS_MAC
# if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_8
    // unsupported
    d_stderr2("fileBrowserCreate is unsupported on macos < 10.8");
    return nullptr;
# else
    NSSavePanel* const nsBasePanel = handle->nsBasePanel;
    DISTRHO_SAFE_ASSERT_RETURN(nsBasePanel != nullptr, nullptr);

    if (! options.saving)
    {
        NSOpenPanel* const nsOpenPanel = handle->nsOpenPanel;
        DISTRHO_SAFE_ASSERT_RETURN(nsOpenPanel != nullptr, nullptr);

        [nsOpenPanel setAllowsMultipleSelection:NO];
        [nsOpenPanel setCanChooseDirectories:NO];
        [nsOpenPanel setCanChooseFiles:YES];
    }

    NSString* const startDirString = [[NSString alloc]
        initWithBytes:startDir
               length:strlen(startDir)
             encoding:NSUTF8StringEncoding];
    [nsBasePanel setDirectoryURL:[NSURL fileURLWithPath:startDirString]];

    // TODO file filter using allowedContentTypes: [UTType]

    if (options.buttons.listAllFiles == FileBrowserOptions::kButtonVisibleChecked)
        [nsBasePanel setAllowsOtherFileTypes:YES];
    if (options.buttons.showHidden == FileBrowserOptions::kButtonVisibleChecked)
        [nsBasePanel setShowsHiddenFiles:YES];

    NSString* const titleString = [[NSString alloc]
        initWithBytes:windowTitle
               length:strlen(windowTitle)
             encoding:NSUTF8StringEncoding];
    [nsBasePanel setTitle:titleString];

    FileBrowserData* const handleptr = handle.get();

    dispatch_async(dispatch_get_main_queue(), ^
    {
        [nsBasePanel beginSheetModalForWindow:[(NSView*)windowId window]
                            completionHandler:^(NSModalResponse result)
        {
            if (result == NSModalResponseOK && [[nsBasePanel URL] isFileURL])
            {
                NSString* const path = [[nsBasePanel URL] path];
                handleptr->selectedFile = strdup([path UTF8String]);
            }
            else
            {
                handleptr->selectedFile = kSelectedFileCancelled;
            }
        }];
    });

    [startDirString release];
    [titleString release];
# endif
#endif

#ifdef DISTRHO_OS_WASM
    if (options.saving)
    {
        const size_t len = options.defaultName != nullptr ? strlen(options.defaultName) : 0;
        DISTRHO_SAFE_ASSERT_RETURN(len != 0, nullptr);

        // store uploaded files inside a specific dir
        mkdir("/userfiles", 0777);

        char* const filename = static_cast<char*>(malloc(len + 12));
        std::strncpy(filename, "/userfiles/", 12);
        std::memcpy(filename + 11, options.defaultName, len + 1);

        handle->defaultName = strdup(options.defaultName);
        handle->selectedFile = filename;
        return handle.release();
    }

    const char* const funcname = fileBrowserSetPathFuncName;
    if (openWebBrowserFileDialog(funcname, handle.get()))
        return handle.release();

    return nullptr;
#endif

#ifdef DISTRHO_OS_WINDOWS
    handle->setupAndStart(isEmbed, startDir, windowTitle, windowId, options);
#endif

#ifdef HAVE_DBUS
    // optional, can be null
    DBusConnection* const dbuscon = handle->dbuscon;

    // https://flatpak.github.io/xdg-desktop-portal/portal-docs.html#gdbus-org.freedesktop.portal.FileChooser
    if (dbuscon != nullptr)
    {
        // if this is the first time we are calling into DBus, check if things are working
        static bool checkAvailable = !dbus_bus_name_has_owner(dbuscon, "org.freedesktop.portal.Desktop", nullptr);

        if (checkAvailable)
        {
            checkAvailable = false;

            if (DBusMessage* const msg = dbus_message_new_method_call("org.freedesktop.portal.Desktop",
                                                                      "/org/freedesktop/portal/desktop",
                                                                      "org.freedesktop.portal.FileChooser",
                                                                      "version"))
            {
                if (DBusMessage* const reply = dbus_connection_send_with_reply_and_block(dbuscon, msg, 250, nullptr))
                    dbus_message_unref(reply);

                dbus_message_unref(msg);
            }
        }

        // Any subsquent calls should have this DBus service active
        if (dbus_bus_name_has_owner(dbuscon, "org.freedesktop.portal.Desktop", nullptr))
        {
            if (DBusMessage* const msg = dbus_message_new_method_call("org.freedesktop.portal.Desktop",
                                                                      "/org/freedesktop/portal/desktop",
                                                                      "org.freedesktop.portal.FileChooser",
                                                                      options.saving ? "SaveFile" : "OpenFile"))
            {
               #ifdef HAVE_X11
                char windowIdStr[32];
                memset(windowIdStr, 0, sizeof(windowIdStr));
                snprintf(windowIdStr, sizeof(windowIdStr)-1, "x11:%llx", (ulonglong)windowId);
                const char* windowIdStrPtr = windowIdStr;
               #endif

                dbus_message_append_args(msg,
                                        #ifdef HAVE_X11
                                         DBUS_TYPE_STRING, &windowIdStrPtr,
                                        #endif
                                         DBUS_TYPE_STRING, &windowTitle,
                                         DBUS_TYPE_INVALID);

                DBusMessageIter iter, array;
                dbus_message_iter_init_append(msg, &iter);
                dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, "{sv}", &array);

                {
                    DBusMessageIter dict, variant, variantArray;
                    const char* const current_folder_key = "current_folder";
                    const char* const current_folder_val = startDir.buffer();

                    dbus_message_iter_open_container(&array, DBUS_TYPE_DICT_ENTRY, nullptr, &dict);
                    dbus_message_iter_append_basic(&dict, DBUS_TYPE_STRING, &current_folder_key);
                    dbus_message_iter_open_container(&dict, DBUS_TYPE_VARIANT, "ay", &variant);
                    dbus_message_iter_open_container(&variant, DBUS_TYPE_ARRAY, "y", &variantArray);
                    dbus_message_iter_append_fixed_array(&variantArray, DBUS_TYPE_BYTE,
                                                         &current_folder_val,
                                                         static_cast<int>(startDir.length() + 1));
                    dbus_message_iter_close_container(&variant, &variantArray);
                    dbus_message_iter_close_container(&dict, &variant);
                    dbus_message_iter_close_container(&array, &dict);
                }

                dbus_message_iter_close_container(&iter, &array);

                dbus_connection_send(dbuscon, msg, nullptr);

                dbus_message_unref(msg);
                return handle.release();
            }
        }
    }
#endif

#ifdef HAVE_X11
    Display* const x11display = handle->x11display;
    DISTRHO_SAFE_ASSERT_RETURN(x11display != nullptr, nullptr);

    // unsupported at the moment
    if (options.saving)
        return nullptr;

    DISTRHO_SAFE_ASSERT_RETURN(x_fib_configure(0, startDir) == 0, nullptr);
    DISTRHO_SAFE_ASSERT_RETURN(x_fib_configure(1, windowTitle) == 0, nullptr);

    const int button1 = options.buttons.showHidden == FileBrowserOptions::kButtonVisibleChecked ? 1
                      : options.buttons.showHidden == FileBrowserOptions::kButtonVisibleUnchecked ? 0 : -1;
    const int button2 = options.buttons.showPlaces == FileBrowserOptions::kButtonVisibleChecked ? 1
                      : options.buttons.showPlaces == FileBrowserOptions::kButtonVisibleUnchecked ? 0 : -1;
    const int button3 = options.buttons.listAllFiles == FileBrowserOptions::kButtonVisibleChecked ? 1
                      : options.buttons.listAllFiles == FileBrowserOptions::kButtonVisibleUnchecked ? 0 : -1;

    x_fib_cfg_buttons(1, button1);
    x_fib_cfg_buttons(2, button2);
    x_fib_cfg_buttons(3, button3);

    if (x_fib_show(x11display, windowId, 0, 0, scaleFactor + 0.5) != 0)
        return nullptr;
#endif

    return handle.release();

    // might be unused
    (void)isEmbed;
    (void)windowId;
    (void)scaleFactor;
}

// --------------------------------------------------------------------------------------------------------------------
// returns true if dialog was closed (with or without a file selection)

bool fileBrowserIdle(const FileBrowserHandle handle)
{
#ifdef HAVE_DBUS
    if (DBusConnection* dbuscon = handle->dbuscon)
    {
        while (dbus_connection_dispatch(dbuscon) == DBUS_DISPATCH_DATA_REMAINS) {}
        dbus_connection_read_write_dispatch(dbuscon, 0);

        if (DBusMessage* const message = dbus_connection_pop_message(dbuscon))
        {
            const char* const interface = dbus_message_get_interface(message);
            const char* const member = dbus_message_get_member(message);

            if (interface != nullptr && std::strcmp(interface, "org.freedesktop.portal.Request") == 0
                && member != nullptr && std::strcmp(member, "Response") == 0)
            {
                do {
                    DBusMessageIter iter;
                    dbus_message_iter_init(message, &iter);

                    // starts with uint32 for return/exit code
                    DISTRHO_SAFE_ASSERT_BREAK(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_UINT32);

                    uint32_t ret = 1;
                    dbus_message_iter_get_basic(&iter, &ret);

                    if (ret != 0)
                        break;

                    // next must be array
                    dbus_message_iter_next(&iter);
                    DISTRHO_SAFE_ASSERT_BREAK(dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY);

                    // open dict array
                    DBusMessageIter dictArray;
                    dbus_message_iter_recurse(&iter, &dictArray);
                    DISTRHO_SAFE_ASSERT_BREAK(dbus_message_iter_get_arg_type(&dictArray) == DBUS_TYPE_DICT_ENTRY);

                    // open containing dict
                    DBusMessageIter dict;
                    dbus_message_iter_recurse(&dictArray, &dict);

                    // look for dict with string "uris"
                    DISTRHO_SAFE_ASSERT_BREAK(dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_STRING);

                    const char* key = nullptr;
                    dbus_message_iter_get_basic(&dict, &key);
                    DISTRHO_SAFE_ASSERT_BREAK(key != nullptr);

                    // keep going until we find it
                    while (std::strcmp(key, "uris") != 0)
                    {
                        key = nullptr;
                        dbus_message_iter_next(&dictArray);
                        DISTRHO_SAFE_ASSERT_BREAK(dbus_message_iter_get_arg_type(&dictArray) == DBUS_TYPE_DICT_ENTRY);

                        dbus_message_iter_recurse(&dictArray, &dict);
                        DISTRHO_SAFE_ASSERT_BREAK(dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_STRING);

                        dbus_message_iter_get_basic(&dict, &key);
                        DISTRHO_SAFE_ASSERT_BREAK(key != nullptr);
                    }

                    if (key == nullptr)
                        break;

                    // then comes variant
                    dbus_message_iter_next(&dict);
                    DISTRHO_SAFE_ASSERT_BREAK(dbus_message_iter_get_arg_type(&dict) == DBUS_TYPE_VARIANT);

                    DBusMessageIter variant;
                    dbus_message_iter_recurse(&dict, &variant);
                    DISTRHO_SAFE_ASSERT_BREAK(dbus_message_iter_get_arg_type(&variant) == DBUS_TYPE_ARRAY);

                    // open variant array (variant type is string)
                    DBusMessageIter variantArray;
                    dbus_message_iter_recurse(&variant, &variantArray);
                    DISTRHO_SAFE_ASSERT_BREAK(dbus_message_iter_get_arg_type(&variantArray) == DBUS_TYPE_STRING);

                    const char* value = nullptr;
                    dbus_message_iter_get_basic(&variantArray, &value);

                    // and finally we have our dear value, just make sure it is local
                    DISTRHO_SAFE_ASSERT_BREAK(value != nullptr);

                    if (const char* const localvalue = std::strstr(value, "file:///"))
                    {
                        if (char* const decodedvalue = strdup(localvalue + 7))
                        {
                            for (char* s = decodedvalue; (s = std::strchr(s, '%')) != nullptr; ++s)
                            {
                                if (! isHexChar(s[1]) || ! isHexChar(s[2]))
                                    continue;

                                const int decodedNum = toHexChar(s[1]) * 0x10 + toHexChar(s[2]);

                                char replacementChar;
                                switch (decodedNum)
                                {
                                case 0x20: replacementChar = ' '; break;
                                case 0x22: replacementChar = '\"'; break;
                                case 0x23: replacementChar = '#'; break;
                                case 0x25: replacementChar = '%'; break;
                                case 0x3c: replacementChar = '<'; break;
                                case 0x3e: replacementChar = '>'; break;
                                case 0x5b: replacementChar = '['; break;
                                case 0x5c: replacementChar = '\\'; break;
                                case 0x5d: replacementChar = ']'; break;
                                case 0x5e: replacementChar = '^'; break;
                                case 0x60: replacementChar = '`'; break;
                                case 0x7b: replacementChar = '{'; break;
                                case 0x7c: replacementChar = '|'; break;
                                case 0x7d: replacementChar = '}'; break;
                                case 0x7e: replacementChar = '~'; break;
                                default: continue;
                                }

                                s[0] = replacementChar;
                                std::memmove(s + 1, s + 3, std::strlen(s) - 2);
                            }

                            handle->selectedFile = decodedvalue;
                        }
                    }

                } while(false);

                if (handle->selectedFile == nullptr)
                    handle->selectedFile = kSelectedFileCancelled;
            }
        }
    }
#endif

#ifdef HAVE_X11
    Display* const x11display = handle->x11display;

    if (x11display == nullptr)
        return false;

    XEvent event;
    while (XPending(x11display) > 0)
    {
        XNextEvent(x11display, &event);

        if (x_fib_handle_events(x11display, &event) == 0)
            continue;

        if (x_fib_status() > 0)
            handle->selectedFile = x_fib_filename();
        else
            handle->selectedFile = kSelectedFileCancelled;

        x_fib_close(x11display);
        XCloseDisplay(x11display);
        handle->x11display = nullptr;
        break;
    }
#endif

    return handle->selectedFile != nullptr;
}

// --------------------------------------------------------------------------------------------------------------------
// close sofd file dialog

void fileBrowserClose(const FileBrowserHandle handle)
{
#ifdef DISTRHO_OS_WASM
    if (handle->saving && fileBrowserGetPath(handle) != nullptr)
        downloadWebBrowserFile(handle->defaultName);
#endif

#ifdef HAVE_X11
    if (Display* const x11display = handle->x11display)
        x_fib_close(x11display);
#endif

    delete handle;
}

// --------------------------------------------------------------------------------------------------------------------
// get path chosen via sofd file dialog

const char* fileBrowserGetPath(const FileBrowserHandle handle)
{
    if (const char* const selectedFile = handle->selectedFile)
        if (selectedFile != kSelectedFileCancelled && std::strcmp(selectedFile, kSelectedFileCancelled) != 0)
            return selectedFile;

    return nullptr;
}

// --------------------------------------------------------------------------------------------------------------------

#ifdef FILE_BROWSER_DIALOG_DGL_NAMESPACE
END_NAMESPACE_DGL
#else
END_NAMESPACE_DISTRHO
#endif

#undef FILE_BROWSER_DIALOG_DISTRHO_NAMESPACE
#undef FILE_BROWSER_DIALOG_DGL_NAMESPACE
#undef FILE_BROWSER_DIALOG_NAMESPACE

#undef fileBrowserSetPathNamespaced
#undef fileBrowserSetPathFuncName
