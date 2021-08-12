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

#ifndef DISTRHO_LIBRARY_UTILS_HPP_INCLUDED
#define DISTRHO_LIBRARY_UTILS_HPP_INCLUDED

#include "../DistrhoUtils.hpp"

#ifdef DISTRHO_OS_WINDOWS
# ifndef NOMINMAX
#  define NOMINMAX
# endif
# include <winsock2.h>
# include <windows.h>
typedef HMODULE lib_t;
#else
# include <dlfcn.h>
typedef void* lib_t;
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// library related calls

/*
 * Open 'filename' library (must not be null).
 * May return null, in which case "lib_error" has the error.
 */
static inline
lib_t lib_open(const char* const filename) noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', nullptr);

    try {
#ifdef DISTRHO_OS_WINDOWS
        return ::LoadLibraryA(filename);
#else
        return ::dlopen(filename, RTLD_NOW|RTLD_LOCAL);
#endif
    } DISTRHO_SAFE_EXCEPTION_RETURN("lib_open", nullptr);
}

/*
 * Close a previously opened library (must not be null).
 * If false is returned, "lib_error" has the error.
 */
static inline
bool lib_close(const lib_t lib) noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(lib != nullptr, false);

    try {
#ifdef DISTRHO_OS_WINDOWS
        return ::FreeLibrary(lib);
#else
        return (::dlclose(lib) == 0);
#endif
    } DISTRHO_SAFE_EXCEPTION_RETURN("lib_close", false);
}

/*
 * Get a library symbol (must not be null).
 * Returns null if the symbol is not found.
 */
template<typename Func>
static inline
Func lib_symbol(const lib_t lib, const char* const symbol) noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(lib != nullptr, nullptr);
    DISTRHO_SAFE_ASSERT_RETURN(symbol != nullptr && symbol[0] != '\0', nullptr);

    try {
#ifdef DISTRHO_OS_WINDOWS
# if defined(__GNUC__) && (__GNUC__ >= 9)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wcast-function-type"
# endif
        return (Func)::GetProcAddress(lib, symbol);
# if defined(__GNUC__) && (__GNUC__ >= 9)
#  pragma GCC diagnostic pop
# endif
#else
        return (Func)(uintptr_t)::dlsym(lib, symbol);
#endif
    } DISTRHO_SAFE_EXCEPTION_RETURN("lib_symbol", nullptr);
}

/*
 * Return the last operation error ('filename' must not be null).
 * May return null.
 */
static inline
const char* lib_error(const char* const filename) noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(filename != nullptr && filename[0] != '\0', nullptr);

#ifdef DISTRHO_OS_WINDOWS
    static char libError[2048+1];
    std::memset(libError, 0, sizeof(libError));

    try {
        const DWORD winErrorCode  = ::GetLastError();
        const int   winErrorFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS;
        LPVOID      winErrorString;

        ::FormatMessage(winErrorFlags, nullptr, winErrorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&winErrorString, 0, nullptr);

        std::snprintf(libError, 2048, "%s: error code %li: %s", filename, winErrorCode, (const char*)winErrorString);
        ::LocalFree(winErrorString);
    } DISTRHO_SAFE_EXCEPTION("lib_error");

    return (libError[0] != '\0') ? libError : nullptr;
#else
    return ::dlerror();
#endif
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_LIBRARY_UTILS_HPP_INCLUDED
