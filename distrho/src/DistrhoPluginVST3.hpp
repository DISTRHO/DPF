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

#ifndef DISTRHO_PLUGIN_VST3_HPP_INCLUDED
#define DISTRHO_PLUGIN_VST3_HPP_INCLUDED

#include "DistrhoPluginChecks.h"
#include "../DistrhoUtils.hpp"

#include <algorithm>
#include <cmath>

#if DISTRHO_PLUGIN_HAS_UI && ! DISTRHO_PLUGIN_HAS_EMBED_UI
# undef DISTRHO_PLUGIN_HAS_UI
# define DISTRHO_PLUGIN_HAS_UI 0
#endif

#if DISTRHO_PLUGIN_HAS_UI && ! defined(HAVE_DGL) && ! DISTRHO_PLUGIN_HAS_EXTERNAL_UI
# undef DISTRHO_PLUGIN_HAS_UI
# define DISTRHO_PLUGIN_HAS_UI 0
#endif

#if DISTRHO_PLUGIN_HAS_UI == 1 && DISTRHO_PLUGIN_WANT_DIRECT_ACCESS == 0
# define DPF_VST3_USES_SEPARATE_CONTROLLER 1
#else
# define DPF_VST3_USES_SEPARATE_CONTROLLER 0
#endif

// --------------------------------------------------------------------------------------------------------------------

#ifdef DISTRHO_PROPER_CPP11_SUPPORT
# include <atomic>
#else
// quick and dirty std::atomic replacement for the things we need
namespace std {
    struct atomic_int {
        volatile int value;
        explicit atomic_int(volatile int v) noexcept : value(v) {}
        int operator++() volatile noexcept { return __atomic_add_fetch(&value, 1, __ATOMIC_RELAXED); }
        int operator--() volatile noexcept { return __atomic_sub_fetch(&value, 1, __ATOMIC_RELAXED); }
        operator int() volatile noexcept { return __atomic_load_n(&value, __ATOMIC_RELAXED); }
    };
};
#endif

// --------------------------------------------------------------------------------------------------------------------

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

enum Vst3InternalParameters {
#if DPF_VST3_USES_SEPARATE_CONTROLLER
    kVst3InternalParameterBufferSize,
    kVst3InternalParameterSampleRate,
#endif
#if DISTRHO_PLUGIN_WANT_LATENCY
    kVst3InternalParameterLatency,
#endif
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    kVst3InternalParameterProgram,
#endif
    kVst3InternalParameterBaseCount,
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    kVst3InternalParameterMidiCC_start = kVst3InternalParameterBaseCount,
    kVst3InternalParameterMidiCC_end = kVst3InternalParameterMidiCC_start + 130*16,
    kVst3InternalParameterCount = kVst3InternalParameterMidiCC_end
#else
    kVst3InternalParameterCount = kVst3InternalParameterBaseCount
#endif
};

#if DPF_VST3_USES_SEPARATE_CONTROLLER || DISTRHO_PLUGIN_WANT_LATENCY || DISTRHO_PLUGIN_WANT_PROGRAMS || DISTRHO_PLUGIN_WANT_MIDI_INPUT
# define DPF_VST3_HAS_INTERNAL_PARAMETERS 1
#else
# define DPF_VST3_HAS_INTERNAL_PARAMETERS 0
#endif

#if DPF_VST3_HAS_INTERNAL_PARAMETERS && DISTRHO_PLUGIN_WANT_MIDI_INPUT && \
    !(DPF_VST3_USES_SEPARATE_CONTROLLER || DISTRHO_PLUGIN_WANT_LATENCY || DISTRHO_PLUGIN_WANT_PROGRAMS)
# define DPF_VST3_PURE_MIDI_INTERNAL_PARAMETERS 1
#else
# define DPF_VST3_PURE_MIDI_INTERNAL_PARAMETERS 0
#endif

// --------------------------------------------------------------------------------------------------------------------

static inline
bool strcmp_utf16(const int16_t* const str16, const char* const str8)
{
    size_t i = 0;
    for (; str8[i] != '\0'; ++i)
    {
        const uint8_t char8 = static_cast<uint8_t>(str8[i]);

        // skip non-ascii chars, unsupported
        if (char8 >= 0x80)
            return false;

        if (str16[i] != char8)
            return false;
    }

    return str16[i] == str8[i];
}

// --------------------------------------------------------------------------------------------------------------------

static inline
size_t strlen_utf16(const int16_t* const str)
{
    size_t i = 0;

    while (str[i] != 0)
        ++i;

    return i;
}

// --------------------------------------------------------------------------------------------------------------------

static inline
void strncpy(char* const dst, const char* const src, const size_t length)
{
    DISTRHO_SAFE_ASSERT_RETURN(length > 0,);

    if (const size_t len = std::min(std::strlen(src), length-1U))
    {
        std::memcpy(dst, src, len);
        dst[len] = '\0';
    }
    else
    {
        dst[0] = '\0';
    }
}

static inline
void strncpy_utf8(char* const dst, const int16_t* const src, const size_t length)
{
    DISTRHO_SAFE_ASSERT_RETURN(length > 0,);

    if (const size_t len = std::min(strlen_utf16(src), length-1U))
    {
        for (size_t i=0; i<len; ++i)
        {
            // skip non-ascii chars, unsupported
            if (src[i] >= 0x80)
                continue;

            dst[i] = src[i];
        }
        dst[len] = 0;
    }
    else
    {
        dst[0] = 0;
    }
}

static inline
void strncpy_utf16(int16_t* const dst, const char* const src, const size_t length)
{
    DISTRHO_SAFE_ASSERT_RETURN(length > 0,);

    if (const size_t len = std::min(std::strlen(src), length-1U))
    {
        for (size_t i=0; i<len; ++i)
        {
            // skip non-ascii chars, unsupported
            if ((uint8_t)src[i] >= 0x80)
                continue;

            dst[i] = src[i];
        }
        dst[len] = 0;
    }
    else
    {
        dst[0] = 0;
    }
}

// --------------------------------------------------------------------------------------------------------------------

template<typename T>
static void snprintf_t(char* const dst, const T value, const char* const format, const size_t size)
{
    DISTRHO_SAFE_ASSERT_RETURN(size > 0,);
    std::snprintf(dst, size-1, format, value);
    dst[size-1] = '\0';
}

template<typename T>
static void snprintf_utf16_t(int16_t* const dst, const T value, const char* const format, const size_t size)
{
    DISTRHO_SAFE_ASSERT_RETURN(size > 0,);

    char* const tmpbuf = (char*)std::malloc(size);
    DISTRHO_SAFE_ASSERT_RETURN(tmpbuf != nullptr,);

    std::snprintf(tmpbuf, size-1, format, value);
    tmpbuf[size-1] = '\0';

    strncpy_utf16(dst, tmpbuf, size);
    std::free(tmpbuf);
}

static inline
void snprintf_u32(char* const dst, const uint32_t value, const size_t size)
{
    return snprintf_t<uint32_t>(dst, value, "%u", size);
}

static inline
void snprintf_f32_utf16(int16_t* const dst, const float value, const size_t size)
{
    return snprintf_utf16_t<float>(dst, value, "%f", size);
}

static inline
void snprintf_i32_utf16(int16_t* const dst, const int value, const size_t size)
{
    return snprintf_utf16_t<int>(dst, value, "%d", size);
}

// --------------------------------------------------------------------------------------------------------------------
// handy way to create a utf16 string from a utf8 one on the current function scope, used for message strings

struct ScopedUTF16String {
    int16_t* str;
    ScopedUTF16String(const char* const s) noexcept
        : str(nullptr)
    {
        const size_t len = std::strlen(s);
        str = static_cast<int16_t*>(std::malloc(sizeof(int16_t) * (len + 1)));
        DISTRHO_SAFE_ASSERT_RETURN(str != nullptr,);
        strncpy_utf16(str, s, len + 1);
    }

    ~ScopedUTF16String() noexcept
    {
        std::free(str);
    }

    operator const int16_t*() const noexcept
    {
        return str;
    }
};

// --------------------------------------------------------------------------------------------------------------------
// handy way to create a utf8 string from a utf16 one on the current function scope (limited to 128 chars)

struct ScopedUTF8String {
    char str[128];

    ScopedUTF8String(const int16_t* const s) noexcept
    {
        strncpy_utf8(str, s, 128);
    }

    operator const char*() const noexcept
    {
        return str;
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

#endif // DISTRHO_PLUGIN_VST3_HPP_INCLUDED
