//
//    ██████ ██   ██  ██████   ██████
//   ██      ██   ██ ██    ██ ██            ** Classy Header-Only Classes **
//   ██      ███████ ██    ██ ██
//   ██      ██   ██ ██    ██ ██           https://github.com/Tracktion/choc
//    ██████ ██   ██  ██████   ██████
//
//   CHOC is (C)2022 Tracktion Corporation, and is offered under the terms of the ISC license:
//
//   Permission to use, copy, modify, and/or distribute this software for any purpose with or
//   without fee is hereby granted, provided that the above copyright notice and this permission
//   notice appear in all copies. THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
//   WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
//   AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
//   CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
//   WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
//   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

#ifndef CHOC_DESKTOPWINDOW_HEADER_INCLUDED
#define CHOC_DESKTOPWINDOW_HEADER_INCLUDED

#include "choc_Platform.h"

//==============================================================================
//        _        _           _  _
//     __| |  ___ | |_   __ _ (_)| | ___
//    / _` | / _ \| __| / _` || || |/ __|
//   | (_| ||  __/| |_ | (_| || || |\__ \ _  _  _
//    \__,_| \___| \__| \__,_||_||_||___/(_)(_)(_)
//
//   Code beyond this point is implementation detail...
//
//==============================================================================

#undef  WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#undef  NOMINMAX
#define NOMINMAX
#define Rectangle Rectangle_renamed_to_avoid_name_collisions
#include <windows.h>
#undef Rectangle

START_NAMESPACE_DISTRHO

struct HWNDHolder
{
    HWNDHolder() = default;
    HWNDHolder (HWND h) : hwnd (h) {}
    HWNDHolder (const HWNDHolder&) = delete;
    HWNDHolder& operator= (const HWNDHolder&) = delete;
    HWNDHolder (HWNDHolder&& other) : hwnd (other.hwnd) { other.hwnd = {}; }
    HWNDHolder& operator= (HWNDHolder&& other)  { reset(); hwnd = other.hwnd; other.hwnd = {}; return *this; }
    ~HWNDHolder() { reset(); }

    operator HWND() const  { return hwnd; }
    operator void*() const  { return (void*) hwnd; }

    void reset() { if (IsWindow (hwnd)) DestroyWindow (hwnd); hwnd = {}; }

    HWND hwnd = {};
};

struct WindowClass
{
    WindowClass (std::wstring name, WNDPROC wndProc)
    {
        name += std::to_wstring (static_cast<uint32_t> (GetTickCount()));

        moduleHandle = GetModuleHandle (nullptr);
        auto icon = (HICON) LoadImage (moduleHandle, IDI_APPLICATION, IMAGE_ICON,
                                       GetSystemMetrics (SM_CXSMICON),
                                       GetSystemMetrics (SM_CYSMICON),
                                       LR_DEFAULTCOLOR);

        WNDCLASSEXW wc;
        ZeroMemory (&wc, sizeof(wc));
        wc.cbSize = sizeof(wc);
        wc.style = CS_OWNDC;
        wc.hInstance = moduleHandle;
        wc.lpszClassName = name.c_str();
        wc.hIcon = icon;
        wc.hIconSm = icon;
        wc.lpfnWndProc = wndProc;

        classAtom = (LPCWSTR) (uintptr_t) RegisterClassExW (&wc);
        DISTRHO_SAFE_ASSERT (classAtom != 0);
    }

    ~WindowClass()
    {
        UnregisterClassW (classAtom, moduleHandle);
    }

    HWNDHolder createWindow (DWORD style, int w, int h, void* userData)
    {
        if (auto hwnd = CreateWindowW (classAtom, L"", style, CW_USEDEFAULT, CW_USEDEFAULT,
                                       w, h, nullptr, nullptr, moduleHandle, nullptr))
        {
            SetWindowLongPtr (hwnd, GWLP_USERDATA, (LONG_PTR) userData);
            return hwnd;
        }

        return {};
    }

    auto getClassName() const    { return classAtom; }

    HINSTANCE moduleHandle = {};
    LPCWSTR classAtom = {};
};

static std::string createUTF8FromUTF16 (const std::wstring& utf16)
{
    if (! utf16.empty())
    {
        auto numWideChars = static_cast<int> (utf16.size());
        auto resultSize = WideCharToMultiByte (CP_UTF8, WC_ERR_INVALID_CHARS, utf16.data(), numWideChars, nullptr, 0, nullptr, nullptr);

        if (resultSize > 0)
        {
            std::string result;
            result.resize (static_cast<size_t> (resultSize));

            if (WideCharToMultiByte (CP_UTF8, WC_ERR_INVALID_CHARS, utf16.data(), numWideChars, result.data(), resultSize, nullptr, nullptr) > 0)
                return result;
        }
    }

    return {};
}

static std::wstring createUTF16StringFromUTF8 (std::string_view utf8)
{
    if (! utf8.empty())
    {
        auto numUTF8Bytes = static_cast<int> (utf8.size());
        auto resultSize = MultiByteToWideChar (CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), numUTF8Bytes, nullptr, 0);

        if (resultSize > 0)
        {
            std::wstring result;
            result.resize (static_cast<size_t> (resultSize));

            if (MultiByteToWideChar (CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), numUTF8Bytes, result.data(), resultSize) > 0)
                return result;
        }
    }

    return {};
}

END_NAMESPACE_DISTRHO

#endif // CHOC_DESKTOPWINDOW_HEADER_INCLUDED
