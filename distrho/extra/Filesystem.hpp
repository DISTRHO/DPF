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

#ifndef DISTRHO_FILESYSTEM_UTILS_HPP_INCLUDED
#define DISTRHO_FILESYSTEM_UTILS_HPP_INCLUDED

#include "String.hpp"

#include <cstdio>

#ifdef DISTRHO_OS_WINDOWS
# include <stringapiset.h>
#else
# include <cerrno>
#endif

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
// filesystem related calls

/*
 * Wrapper around `fopen` call, needed on Windows because its C standard functions use ASCII instead of UTF-8.
 */
static inline
FILE* d_fopen(const char* const pathname, const char* const mode)
{
   #ifdef DISTRHO_OS_WINDOWS
    WCHAR lpathname[MAX_PATH];
    WCHAR lmode[4];
    if (MultiByteToWideChar(CP_UTF8, 0, pathname, -1, lpathname, ARRAY_SIZE(lpathname)) != 0 &&
        MultiByteToWideChar(CP_UTF8, 0, mode, -1, lmode, ARRAY_SIZE(lmode)) != 0)
        return _wfopen(lpathname, lmode);
   #endif

    return fopen(pathname, mode);
}

// --------------------------------------------------------------------------------------------------------------------
// filesystem related classes

/**
   Handy class to help write files in a safe way, which does:
    - open pathname + ".tmp" instead of opening a file directly (so partial writes are safe)
    - on close, flush data to disk and rename file to remove ".tmp"

   To use it, create a local variable (on the stack) and call ok() or manually check @a fd variable.
   @code
    if (const SafeFileWriter file("/path/to/file.txt"); file.ok())
        file.write("Success!");
   @endcode
 */
struct SafeFileWriter
{
    String filename;
    FILE* const fd;

    /**
       Constructor, opening @a pathname + ".tmp" for writing.
    */
    SafeFileWriter(const char* const pathname, const char* const mode = "w")
        : filename(pathname),
          fd(d_fopen(filename + ".tmp", mode))
    {
       #ifndef DISTRHO_OS_WINDOWS
        if (fd == nullptr)
            d_stderr2("failed to open '%s' for writing: %s", pathname, std::strerror(errno));
       #endif
    }

    /**
       Destructor, will flush file data contents, close and rename file.
    */
    ~SafeFileWriter()
    {
        if (fd == nullptr)
            return;

        std::fflush(fd);
        std::fclose(fd);
        std::rename(filename + ".tmp", filename);
    }

    /** Check if the file was opened successfully. */
    inline bool ok() const noexcept
    {
        return fd != nullptr;
    }

    /** Wrapper around `fwrite`, purely for convenience. */
    inline size_t write(const void* const ptr, const size_t size, const size_t nmemb = 1) const
    {
        return std::fwrite(ptr, size, nmemb, fd);
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_FILESYSTEM_UTILS_HPP_INCLUDED
