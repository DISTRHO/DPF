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

#ifndef DISTRHO_IS_STANDALONE
# error Wrong build configuration
#endif

#include "../extra/String.hpp"
#include "../DistrhoStandaloneUtils.hpp"

#ifdef DISTRHO_OS_WINDOWS
# include <windows.h>
#else
# ifndef STATIC_BUILD
#  include <dlfcn.h>
# endif
# include <limits.h>
# include <stdlib.h>
#endif

#ifdef DISTRHO_OS_WINDOWS
# if DISTRHO_IS_STANDALONE || defined(STATIC_BUILD)
static constexpr const HINSTANCE hInstance = nullptr;
# else
static HINSTANCE hInstance = nullptr;

DISTRHO_PLUGIN_EXPORT
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
        hInstance = hInst;
    return 1;
}
# endif
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

const char* getBinaryFilename()
{
    static String filename;

  #ifndef STATIC_BUILD
    if (filename.isNotEmpty())
        return filename;

   #ifdef DISTRHO_OS_WINDOWS
    CHAR filenameBuf[MAX_PATH];
    filenameBuf[0] = '\0';
    GetModuleFileNameA(hInstance, filenameBuf, sizeof(filenameBuf));
    filename = filenameBuf;
   #else
    Dl_info info;
    dladdr((void*)getBinaryFilename, &info);
    char filenameBuf[PATH_MAX];
    filename = realpath(info.dli_fname, filenameBuf);
   #endif
  #endif

    return filename;
}

const char* getPluginFormatName() noexcept
{
#if defined(DISTRHO_PLUGIN_TARGET_AU)
    return "AudioUnit";
#elif defined(DISTRHO_PLUGIN_TARGET_CARLA)
    return "Carla";
#elif defined(DISTRHO_PLUGIN_TARGET_JACK)
   #if defined(DISTRHO_OS_WASM)
    return "Wasm/Standalone";
   #elif defined(HAVE_JACK)
    return "JACK/Standalone";
   #else
    return "Standalone";
   #endif
#elif defined(DISTRHO_PLUGIN_TARGET_LADSPA)
    return "LADSPA";
#elif defined(DISTRHO_PLUGIN_TARGET_DSSI)
    return "DSSI";
#elif defined(DISTRHO_PLUGIN_TARGET_LV2)
    return "LV2";
#elif defined(DISTRHO_PLUGIN_TARGET_VST2)
    return "VST2";
#elif defined(DISTRHO_PLUGIN_TARGET_VST3)
    return "VST3";
#elif defined(DISTRHO_PLUGIN_TARGET_CLAP)
    return "CLAP";
#elif defined(DISTRHO_PLUGIN_TARGET_STATIC) && defined(DISTRHO_PLUGIN_TARGET_STATIC_NAME)
    return DISTRHO_PLUGIN_TARGET_STATIC_NAME;
#else
    return "Unknown";
#endif
}

const char* getResourcePath(const char* const bundlePath) noexcept
{
    DISTRHO_SAFE_ASSERT_RETURN(bundlePath != nullptr, nullptr);

   #if defined(DISTRHO_PLUGIN_TARGET_AU) || defined(DISTRHO_PLUGIN_TARGET_JACK) || defined(DISTRHO_PLUGIN_TARGET_VST2) || defined(DISTRHO_PLUGIN_TARGET_CLAP)
    static String resourcePath;

    if (resourcePath.isEmpty())
    {
        resourcePath = bundlePath;
       #ifdef DISTRHO_OS_MAC
        resourcePath += "/Contents/Resources";
       #else
        resourcePath += DISTRHO_OS_SEP_STR "resources";
       #endif
    }

    return resourcePath.buffer();
   #elif defined(DISTRHO_PLUGIN_TARGET_LV2)
    static String resourcePath;

    if (resourcePath.isEmpty())
    {
        resourcePath = bundlePath;
        resourcePath += DISTRHO_OS_SEP_STR "resources";
    }

    return resourcePath.buffer();
   #elif defined(DISTRHO_PLUGIN_TARGET_VST3)
    static String resourcePath;

    if (resourcePath.isEmpty())
    {
        resourcePath = bundlePath;
        resourcePath += "/Contents/Resources";
    }

    return resourcePath.buffer();
   #endif

    return nullptr;
}

#ifndef DISTRHO_PLUGIN_TARGET_JACK
// all these are null for non-standalone targets
bool isUsingNativeAudio() noexcept { return false; }
bool supportsAudioInput() { return false; }
bool supportsBufferSizeChanges() { return false; }
bool supportsMIDI() { return false; }
bool isAudioInputEnabled() { return false; }
bool isMIDIEnabled() { return false; }
uint getBufferSize() { return 0; }
bool requestAudioInput() { return false; }
bool requestBufferSizeChange(uint) { return false; }
bool requestMIDI() { return false; }
#endif

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
