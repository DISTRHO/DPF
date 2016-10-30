/*
 * DPF LV2 *.ttl generator
 */

#include <stdio.h>
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

void ErrorExit(LPTSTR lpszFunction)
{
    // Retrieve the system error message for the last-error code

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );

    // Display the error message and exit the process

    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
    sprintf((LPTSTR)lpDisplayBuf,
        TEXT("%s failed with error %d: %s"),
        lpszFunction, dw, lpMsgBuf);
    MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
    ExitProcess(dw);
}
#endif

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        printf("usage: %s /path/to/plugin-DLL\n", argv[0]);
        return 1;
    }

#ifdef TTL_GENERATOR_WINDOWS
    const HMODULE handle = LoadLibraryA(argv[1]);
#else
    void* const handle = dlopen(argv[1], RTLD_LAZY);
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
    const TTL_Generator_Function ttlFn = (TTL_Generator_Function)GetProcAddress(handle, "lv2_generate_ttl");
#else
    const TTL_Generator_Function ttlFn = (TTL_Generator_Function)dlsym(handle, "lv2_generate_ttl");
#endif

    if (ttlFn != NULL)
    {
        char basename[strlen(argv[1])+1];

#ifdef TTL_GENERATOR_WINDOWS
        char* base2 = strrchr(argv[1], '\\');
#else
        char* base2 = strrchr(argv[1], '/');
#endif
        if (base2 != NULL)
        {
            strcpy(basename, base2+1);
            basename[strrchr(base2, '.')-base2-1] = '\0';
        }
        else if (argv[1][0] == '.' && argv[1][1] == '/')
        {
            strcpy(basename, argv[1]+2);
            basename[strrchr(basename, '.')-basename] = '\0';
        }
        else
        {
            strcpy(basename, argv[1]);
        }

        printf("Generate ttl data for '%s', basename: '%s'\n", argv[1], basename);

        ttlFn(basename);
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
