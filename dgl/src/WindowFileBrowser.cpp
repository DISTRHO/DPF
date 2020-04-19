/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2020 Filipe Coelho <falktx@falktx.com>
 * Copyright (C) 2019 Jean Pierre Cimalando <jp-dev@inbox.ru>
 * Copyright (C) 2019 Robin Gareus <robin@gareus.org>
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

#include "../StandaloneWindow.hpp"

// static int fib_filter_filename_filter(const char* const name)
// {
//     return 1;
//     (void)name;
// }

// TODO use DGL_NAMESPACE for class names

#ifdef DISTRHO_OS_MAC
@interface FilePanelDelegate : NSObject
{
    void (*fCallback)(NSOpenPanel*, int, void*);
    void* fUserData;
}
-(id)initWithCallback:(void(*)(NSOpenPanel*, int, void*))callback userData:(void*)userData;
-(void)openPanelDidEnd:(NSOpenPanel *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo;
@end

@implementation FilePanelDelegate
-(id)initWithCallback:(void(*)(NSOpenPanel*, int, void*))callback userData:(void *)userData
{
    [super init];
    self->fCallback = callback;
    self->fUserData = userData;
    return self;
}

-(void)openPanelDidEnd:(NSOpenPanel *)sheet returnCode:(int)returnCode contextInfo:(void *)contextInfo
{
    self->fCallback(sheet, returnCode, self->fUserData);
    (void)contextInfo;
}
@end
#endif

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

bool Window::openFileBrowser(const FileBrowserOptions& options)
{
#if defined(DISTRHO_OS_WINDOWS)
    // the old and compatible dialog API
    OPENFILENAMEW ofn;
    memset(&ofn, 0, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = pData->hwnd;

    // set initial directory in UTF-16 coding
    std::vector<WCHAR> startDirW;
    if (options.startDir)
    {
        startDirW.resize(strlen(options.startDir) + 1);
        if (MultiByteToWideChar(CP_UTF8, 0, options.startDir, -1, startDirW.data(), startDirW.size()))
            ofn.lpstrInitialDir = startDirW.data();
    }

    // set title in UTF-16 coding
    std::vector<WCHAR> titleW;
    if (options.title)
    {
        titleW.resize(strlen(options.title) + 1);
        if (MultiByteToWideChar(CP_UTF8, 0, options.title, -1, titleW.data(), titleW.size()))
            ofn.lpstrTitle = titleW.data();
    }

    // prepare a buffer to receive the result
    std::vector<WCHAR> fileNameW(32768); // the Unicode maximum
    ofn.lpstrFile = fileNameW.data();
    ofn.nMaxFile = (DWORD)fileNameW.size();

    // TODO synchronous only, can't do better with WinAPI native dialogs.
    // threading might work, if someone is motivated to risk it.
    if (GetOpenFileNameW(&ofn))
    {
        // back to UTF-8
        std::vector<char> fileNameA(4 * 32768);
        if (WideCharToMultiByte(CP_UTF8, 0, fileNameW.data(), -1, fileNameA.data(), (int)fileNameA.size(), nullptr, nullptr))
        {
            // handle it during the next idle cycle (fake async)
            pData->fSelectedFile = fileNameA.data();
        }
    }

    return true;

#elif defined(DISTRHO_OS_MAC)
    if (pData->fOpenFilePanel) // permit one dialog at most
    {
        [pData->fOpenFilePanel makeKeyAndOrderFront:nil];
        return false;
    }

    NSOpenPanel* panel = [NSOpenPanel openPanel];
    pData->fOpenFilePanel = [panel retain];

    [panel setCanChooseFiles:YES];
    [panel setCanChooseDirectories:NO];
    [panel setAllowsMultipleSelection:NO];

    if (options.startDir)
        [panel setDirectory:[NSString stringWithUTF8String:options.startDir]];

    if (options.title)
    {
        NSString* titleString = [[NSString alloc]
            initWithBytes:options.title
                   length:strlen(options.title)
                 encoding:NSUTF8StringEncoding];
        [panel setTitle:titleString];
    }

    id delegate = pData->fFilePanelDelegate;
    if (!delegate)
    {
        delegate = [[FilePanelDelegate alloc] initWithCallback:&PrivateData::openPanelDidEnd
                                                      userData:pData];
        pData->fFilePanelDelegate = [delegate retain];
    }

    [panel beginSheetForDirectory:nullptr
                             file:nullptr
                   modalForWindow:nullptr
                    modalDelegate:delegate
                   didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:)
                      contextInfo:nullptr];

    return true;

#elif defined(SOFD_HAVE_X11)
    using DISTRHO_NAMESPACE::String;

    // --------------------------------------------------------------------------
    // configure start dir

    // TODO: get abspath if needed
    // TODO: cross-platform

    String startDir(options.startDir);

# ifdef DISTRHO_OS_LINUX
    if (startDir.isEmpty())
    {
        if (char* const dir_name = get_current_dir_name())
        {
            startDir = dir_name;
            std::free(dir_name);
        }
    }
# endif

    DISTRHO_SAFE_ASSERT_RETURN(startDir.isNotEmpty(), false);

    if (! startDir.endsWith('/'))
        startDir += "/";

    DISTRHO_SAFE_ASSERT_RETURN(x_fib_configure(0, startDir) == 0, false);

    // --------------------------------------------------------------------------
    // configure title

    String title(options.title);

    if (title.isEmpty())
    {
        title = pData->getTitle();

        if (title.isEmpty())
            title = "FileBrowser";
    }

    DISTRHO_SAFE_ASSERT_RETURN(x_fib_configure(1, title) == 0, false);

    // --------------------------------------------------------------------------
    // configure filters

    x_fib_cfg_filter_callback(nullptr); //fib_filter_filename_filter);

    // --------------------------------------------------------------------------
    // configure buttons

    x_fib_cfg_buttons(3, options.buttons.listAllFiles-1);
    x_fib_cfg_buttons(1, options.buttons.showHidden-1);
    x_fib_cfg_buttons(2, options.buttons.showPlaces-1);

    // --------------------------------------------------------------------------
    // show

    return (x_fib_show(pData->xDisplay, pData->xWindow, /*options.width*/0, /*options.height*/0) == 0);

#else
    // not implemented
    return false;

    // unused
    (void)options;
#endif
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
