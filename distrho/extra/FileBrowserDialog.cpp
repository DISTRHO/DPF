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

#ifdef DISTRHO_OS_MAC
# import <Cocoa/Cocoa.h>
#endif
#ifdef DISTRHO_OS_WINDOWS
# include <process.h>
# include <vector>
#endif
#ifdef HAVE_X11
# define DBLCLKTME 400
# include "sofd/libsofd.h"
# include "sofd/libsofd.c"
#endif

START_NAMESPACE_DISTRHO

#ifdef DISTRHO_FILE_BROWSER_DIALOG_EXTRA_NAMESPACE
namespace DISTRHO_FILE_BROWSER_DIALOG_EXTRA_NAMESPACE {
#endif

// --------------------------------------------------------------------------------------------------------------------

// static pointer used for signal null/none action taken
static const char* const kSelectedFileCancelled = "__dpf_cancelled__";

struct FileBrowserData {
    const char* selectedFile;

#ifdef DISTRHO_OS_MAC
    NSOpenPanel* nspanel;
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
    const bool isEmbed;

    FileBrowserData(const bool embed)
        : selectedFile(nullptr),
          threadCancelled(false),
          threadHandle(0),
          fileNameW(32768),
          isEmbed(embed)
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

    void setupAndStart(const char* const startDir,
                       const char* const windowTitle,
                       const uintptr_t winId,
                       const FileBrowserOptions options)
    {
        ofn.hwndOwner = (HWND)winId;

        ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
        if (options.buttons.showHidden == Window::FileBrowserOptions::kButtonVisibleChecked)
            ofn.Flags |= OFN_FORCESHOWHIDDEN;

        ofn.FlagsEx = 0x0;
        if (options.buttons.showPlaces == Window::FileBrowserOptions::kButtonInvisible)
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

        if (GetOpenFileNameW(&ofn))
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
    FileBrowserData(bool)
        : selectedFile(nullptr)
    {
#ifdef DISTRHO_OS_MAC
        nspanel = [[NSOpenPanel openPanel]retain];
#endif
#ifdef HAVE_X11
        x11display = XOpenDisplay(nullptr);
#endif
    }

    ~FileBrowserData()
    {
#ifdef DISTRHO_OS_MAC
        [nspanel release];
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

FileBrowserHandle fileBrowserOpen(const bool isEmbed,
                                  const uintptr_t windowId,
                                  const double scaleFactor,
                                  const char* const startDir,
                                  const char* const windowTitle,
                                  const FileBrowserOptions& options)
{
    ScopedPointer<FileBrowserData> handle;
    handle = new FileBrowserData(isEmbed);

#ifdef DISTRHO_OS_MAC
    NSOpenPanel* const nspanel = handle->nspanel;
    DISTRHO_SAFE_ASSERT_RETURN(nspanel != nullptr, nullptr);

    [nspanel setAllowsMultipleSelection:NO];
    [nspanel setCanChooseDirectories:NO];
    [nspanel setCanChooseFiles:YES];
    [nspanel setDirectoryURL:[NSURL fileURLWithPath:[NSString stringWithUTF8String:startDir]]];

    // TODO file filter using allowedContentTypes: [UTType]

    if (options.buttons.listAllFiles == FileBrowserOptions::kButtonVisibleChecked)
        [nspanel setAllowsOtherFileTypes:YES];
    if (options.buttons.showHidden == FileBrowserOptions::kButtonVisibleChecked)
        [nspanel setShowsHiddenFiles:YES];

    NSString* const titleString = [[NSString alloc]
        initWithBytes:windowTitle
               length:strlen(windowTitle)
             encoding:NSUTF8StringEncoding];
    [nspanel setTitle:titleString];

    dispatch_async(dispatch_get_main_queue(), ^
    {
        [nspanel beginSheetModalForWindow:[(NSView*)windowId) window]
                        completionHandler:^(NSModalResponse result)
        {
            DISTRHO_SAFE_ASSERT_RETURN(handle != nullptr,);

            if (result == NSModalResponseOK && [[nspanel URL] isFileURL])
            {
                NSString* const path = [[nspanel URL] path];
                handle->selectedFile = strdup([path UTF8String]);
            }
            else
            {
                handle->selectedFile = kSelectedFileCancelled;
            }
        }];
    });
#endif

#ifdef DISTRHO_OS_WINDOWS
    handle->setupAndStart(startDir, windowTitle, windowId, options);
#endif

#ifdef HAVE_X11
    Display* const x11display = handle->x11display;
    DISTRHO_SAFE_ASSERT_RETURN(x11display != nullptr, nullptr);

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
    (void)windowId;
    (void)scaleFactor;
    (void)startDir;
    (void)windowTitle;
    (void)options;
}

// --------------------------------------------------------------------------------------------------------------------
// returns true if dialog was closed (with or without a file selection)

bool fileBrowserIdle(const FileBrowserHandle handle)
{
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
    return handle->selectedFile != kSelectedFileCancelled ? handle->selectedFile : nullptr;
}

// --------------------------------------------------------------------------------------------------------------------

#ifdef DISTRHO_FILE_BROWSER_DIALOG_EXTRA_NAMESPACE
}
#endif

END_NAMESPACE_DISTRHO
