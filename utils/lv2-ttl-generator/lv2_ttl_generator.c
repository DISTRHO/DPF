/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2016 Filipe Coelho <falktx@falktx.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
 #include <windows.h>
 #define TTL_GENERATOR_WINDOWS
#else
 #include <dlfcn.h>
#endif

#ifndef nullptr
 #define nullptr (0)
#endif

typedef void (*TTL_Generator_Function)(const char* basename);

static int isPathSeparator(char c);
static char* makeNormalPath(const char* path);

// TODO support Unicode paths on the Windows platform

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        printf("usage: %s /path/to/plugin-DLL\n", argv[0]);
        return 1;
    }

    const char* path = argv[1];

#ifdef TTL_GENERATOR_WINDOWS
    const HMODULE handle = LoadLibraryA(path);
#else
    void* const handle = dlopen(path, RTLD_LAZY);
#endif

    if (! handle)
    {
#ifdef TTL_GENERATOR_WINDOWS
        printf("Failed to open plugin DLL\n");
#else
        printf("Failed to open plugin DLL, error was:\n%s\n", dlerror());
#endif
        return 2;
    }

#ifdef TTL_GENERATOR_WINDOWS
# if defined(__GNUC__) && (__GNUC__ >= 9)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wcast-function-type"
# endif
    const TTL_Generator_Function ttlFn = (TTL_Generator_Function)GetProcAddress(handle, "lv2_generate_ttl");
# if defined(__GNUC__) && (__GNUC__ >= 9)
#  pragma GCC diagnostic pop
# endif
#else
    const TTL_Generator_Function ttlFn = (TTL_Generator_Function)dlsym(handle, "lv2_generate_ttl");
#endif

    if (ttlFn != NULL)
    {
        // convert the paths to a normalized form, such that path separators are
        // replaced with '/', and duplicate separators are removed
        char* normalPath = makeNormalPath(path);

        // get rid of any "./" prefixes
        path = normalPath;
        while (path[0] == '.' && path[1] == '/')
            path += 2;

        // extract the file name part
        char* basename = strrchr(path, '/');
        if (basename != NULL)
            basename += 1;
        else
            basename = (char*)path;

        // remove the file extension
        char* dotPos = strrchr(basename, '.');
        if (dotPos)
            *dotPos = '\0';

        printf("Generate ttl data for '%s', basename: '%s'\n", path, basename);

        ttlFn(basename);

        free(normalPath);
    }
    else
        printf("Failed to find 'lv2_generate_ttl' function\n");

#ifdef TTL_GENERATOR_WINDOWS
    FreeLibrary(handle);
#else
    dlclose(handle);
#endif

    return 0;
}

static int isPathSeparator(char c)
{
#ifdef TTL_GENERATOR_WINDOWS
    return c == '/' || c == '\\';
#else
    return c == '/';
#endif
}

static char* makeNormalPath(const char* path)
{
    size_t i, j;
    size_t len = strlen(path);
    char* result = (char*)malloc(len + 1);
    int isSep, wasSep = 0;
    for (i = 0, j = 0; i < len; ++i)
    {
        isSep = isPathSeparator(path[i]);
        if (!isSep)
            result[j++] = path[i];
        else if (!wasSep)
            result[j++] = '/';
        wasSep = isSep;
    }
    result[j] = '\0';
    return result;
}
