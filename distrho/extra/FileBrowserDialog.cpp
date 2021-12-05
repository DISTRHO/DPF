/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2021 Filipe Coelho <falktx@falktx.com>
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

#include "FileBrowserDialog.hpp"
#include "ScopedPointer.hpp"
#include "String.hpp"

#ifdef DISTRHO_OS_MAC
# import <Cocoa/Cocoa.h>
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
# include "sofd/libsofd.c"
#endif

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

// static pointer used for signal null/none action taken
static const char* const kSelectedFileCancelled = "__dpf_cancelled__";

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
        if (cancelAndStop() && selectedFile != nullptr && selectedFile != kSelectedFileCancelled)
            std::free(const_cast<char*>(selectedFile));
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
    FileBrowserData(const bool saving)
        : selectedFile(nullptr)
    {
#ifdef DISTRHO_OS_MAC
        if (saving)
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
#ifdef HAVE_DBUS
        if ((dbuscon = dbus_bus_get(DBUS_BUS_SESSION, nullptr)) != nullptr)
            dbus_connection_set_exit_on_disconnect(dbuscon, false);
#endif
#ifdef HAVE_X11
        x11display = XOpenDisplay(nullptr);
#endif

        // maybe unused
        return; (void)saving;
    }

    ~FileBrowserData()
    {
#ifdef DISTRHO_OS_MAC
        [nsBasePanel release];
#endif
#ifdef HAVE_DBUS
        if (dbuscon != nullptr)
            dbus_connection_unref(dbuscon);
#endif
#ifdef HAVE_X11
        if (x11display != nullptr)
            XCloseDisplay(x11display);
#endif

        if (selectedFile != nullptr && selectedFile != kSelectedFileCancelled)
            std::free(const_cast<char*>(selectedFile));
    }
#endif
};

// --------------------------------------------------------------------------------------------------------------------

#ifdef DISTRHO_FILE_BROWSER_DIALOG_EXTRA_NAMESPACE
namespace DISTRHO_FILE_BROWSER_DIALOG_EXTRA_NAMESPACE {
#endif

// --------------------------------------------------------------------------------------------------------------------

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
        {
            startDir = cwd;
            std::free(cwd);
        }
#else
        if (char* const cwd = getcwd(nullptr, 0))
        {
            startDir = cwd;
            std::free(cwd);
        }
#endif
    }

    DISTRHO_SAFE_ASSERT_RETURN(startDir.isNotEmpty(), nullptr);

    if (! startDir.endsWith(DISTRHO_OS_SEP))
        startDir += DISTRHO_OS_SEP_STR;

    String windowTitle(options.title);

    if (windowTitle.isEmpty())
        windowTitle = "FileBrowser";

    ScopedPointer<FileBrowserData> handle(new FileBrowserData(options.saving));

#ifdef DISTRHO_OS_MAC
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

    [nsBasePanel setDirectoryURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:startDir]]];

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
#endif

#ifdef DISTRHO_OS_WINDOWS
    handle->setupAndStart(isEmbed, startDir, windowTitle, windowId, options);
#endif

#ifdef HAVE_DBUS
    // optional, can be null
    DBusConnection* const dbuscon = handle->dbuscon;

    if (dbuscon != nullptr && dbus_bus_name_has_owner(dbuscon, "org.freedesktop.portal.Desktop", nullptr))
    {
        // https://flatpak.github.io/xdg-desktop-portal/portal-docs.html#gdbus-org.freedesktop.portal.FileChooser
        if (DBusMessage* const message = dbus_message_new_method_call("org.freedesktop.portal.Desktop",
                                                                      "/org/freedesktop/portal/desktop",
                                                                      "org.freedesktop.portal.FileChooser",
                                                                      options.saving ? "SaveFile" : "OpenFile"))
        {
            char windowIdStr[32];
            memset(windowIdStr, 0, sizeof(windowIdStr));
# ifdef HAVE_X11
            snprintf(windowIdStr, sizeof(windowIdStr)-1, "x11:%llx", (ulonglong)windowId);
# endif
            const char* windowIdStrPtr = windowIdStr;

            dbus_message_append_args(message,
                                    DBUS_TYPE_STRING, &windowIdStrPtr,
                                    DBUS_TYPE_STRING, &windowTitle,
                                    DBUS_TYPE_INVALID);

            DBusMessageIter iter, array;
            dbus_message_iter_init_append(message, &iter);
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
                                                     &current_folder_val, startDir.length()+1);
                dbus_message_iter_close_container(&variant, &variantArray);
                dbus_message_iter_close_container(&dict, &variant);
                dbus_message_iter_close_container(&array, &dict);
            }

            dbus_message_iter_close_container(&iter, &array);

            dbus_connection_send(dbuscon, message, nullptr);

            dbus_message_unref(message);
            return handle.release();
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
                        handle->selectedFile = strdup(localvalue + 7);

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

#ifdef DISTRHO_FILE_BROWSER_DIALOG_EXTRA_NAMESPACE
}
#endif

END_NAMESPACE_DISTRHO
