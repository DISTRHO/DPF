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

// A few utils declared in DistrhoUI.cpp but defined here because they use Obj-C

#include "DistrhoDetails.hpp"
#include "src/DistrhoPluginChecks.h"
#include "src/DistrhoDefines.h"

#define Point CocoaPoint
#define Size CocoaSize
#import <Cocoa/Cocoa.h>
#undef Point
#undef Size

#if DISTRHO_UI_FILE_BROWSER
# define DISTRHO_FILE_BROWSER_DIALOG_HPP_INCLUDED
# define FILE_BROWSER_DIALOG_NAMESPACE DISTRHO_NAMESPACE
# define FILE_BROWSER_DIALOG_DISTRHO_NAMESPACE
START_NAMESPACE_DISTRHO
# include "extra/FileBrowserDialogImpl.hpp"
END_NAMESPACE_DISTRHO
# include "extra/FileBrowserDialogImpl.cpp"
#endif

#if DISTRHO_UI_WEB_VIEW
# define DISTRHO_WEB_VIEW_HPP_INCLUDED
# define WEB_VIEW_NAMESPACE DISTRHO_NAMESPACE
# define WEB_VIEW_DISTRHO_NAMESPACE
START_NAMESPACE_DISTRHO
# include "extra/WebViewImpl.hpp"
END_NAMESPACE_DISTRHO
# include "extra/WebViewImpl.cpp"
#endif

#include <algorithm>
#include <cmath>

START_NAMESPACE_DISTRHO
double getDesktopScaleFactor(const uintptr_t parentWindowHandle)
{
    // allow custom scale for testing
    if (const char* const scale = getenv("DPF_SCALE_FACTOR"))
        return std::max(1.0, std::atof(scale));

    if (NSView* const parentView = (NSView*)parentWindowHandle)
        if (NSWindow* const parentWindow = [parentView window])
           return [parentWindow screen].backingScaleFactor;

    return [NSScreen mainScreen].backingScaleFactor;
}
END_NAMESPACE_DISTRHO
