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

#ifndef DISTRHO_PLUGIN_VST_HPP_INCLUDED
#define DISTRHO_PLUGIN_VST_HPP_INCLUDED

#include "DistrhoPluginChecks.h"
#include "../DistrhoUtils.hpp"

#include <algorithm>
#include <cmath>

#if DISTRHO_PLUGIN_HAS_UI
# include "Base.hpp"
#endif

#if DISTRHO_PLUGIN_HAS_UI == 1 && DISTRHO_PLUGIN_WANT_DIRECT_ACCESS == 0
# define DPF_VST3_USES_SEPARATE_CONTROLLER 1
#else
# define DPF_VST3_USES_SEPARATE_CONTROLLER 0
#endif

// --------------------------------------------------------------------------------------------------------------------

#if defined(DISTRHO_PROPER_CPP11_SUPPORT) || defined(DISTRHO_OS_MAC)
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
    kVst3InternalParameterCount
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
static inline
void snprintf_utf16_t(int16_t* const dst, const T value, const char* const format, const size_t size)
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
void snprintf_f32_utf16(int16_t* const dst, const float value, const size_t size)
{
    return snprintf_utf16_t<float>(dst, value, "%f", size);
}

static inline
void snprintf_f32_utf16(int16_t* const dst, const double value, const size_t size)
{
    return snprintf_utf16_t<double>(dst, value, "%f", size);
}

static inline
void snprintf_i32_utf16(int16_t* const dst, const int32_t value, const size_t size)
{
    return snprintf_utf16_t<int32_t>(dst, value, "%d", size);
}

static inline
void snprintf_u32_utf16(int16_t* const dst, const uint32_t value, const size_t size)
{
    return snprintf_utf16_t<uint32_t>(dst, value, "%u", size);
}

#if DISTRHO_PLUGIN_HAS_UI
// --------------------------------------------------------------------------------------------------------------------
// translate a vstgui-based key character and code to matching values used by DPF

static inline
uint translateVstKeyCode(bool& special, const int16_t keychar, const int16_t keycode) noexcept
{
    using namespace DGL_NAMESPACE;

    // special stuff first
    special = true;
    switch (keycode)
    {
    case 1: return kKeyBackspace;
    // 2 \t (handled below)
    // 3 clear
    // 4 \r (handled below)
    case 6: return kKeyEscape;
    //  7 space (handled below)
    //  8 next
    // 17 select
    // 18 print
    // 19 \n (handled below)
    // 20 snapshot
    case 22: return kKeyDelete;
    // 23 help
    // 57 = (handled below)
    // numpad stuff follows
    // 24 0 (handled below)
    // 25 1 (handled below)
    // 26 2 (handled below)
    // 27 3 (handled below)
    // 28 4 (handled below)
    // 29 5 (handled below)
    // 30 6 (handled below)
    // 31 7 (handled below)
    // 32 8 (handled below)
    // 33 9 (handled below)
    // 34 * (handled below)
    // 35 + (handled below)
    // 36 separator
    // 37 - (handled below)
    // 38 . (handled below)
    // 39 / (handled below)
    // handle rest of special keys
    /* these special keys are missing:
        - kKeySuper
        - kKeyCapsLock
        - kKeyPrintScreen
        */
    case 40: return kKeyF1;
    case 41: return kKeyF2;
    case 42: return kKeyF3;
    case 43: return kKeyF4;
    case 44: return kKeyF5;
    case 45: return kKeyF6;
    case 46: return kKeyF7;
    case 47: return kKeyF8;
    case 48: return kKeyF9;
    case 49: return kKeyF10;
    case 50: return kKeyF11;
    case 51: return kKeyF12;
    case 11: return kKeyLeft;
    case 12: return kKeyUp;
    case 13: return kKeyRight;
    case 14: return kKeyDown;
    case 15: return kKeyPageUp;
    case 16: return kKeyPageDown;
    case 10: return kKeyHome;
    case  9: return kKeyEnd;
    case 21: return kKeyInsert;
    case 54: return kKeyShiftL;
    case 55: return kKeyControlL;
    case 56: return kKeyAltL;
    case 58: return kKeyMenu;
    case 52: return kKeyNumLock;
    case 53: return kKeyScrollLock;
    case  5: return kKeyPause;
    }

    // regular keys next
    special = false;
    switch (keycode)
    {
    case  2: return '\t';
    case  4: return '\r';
    case  7: return ' ';
    case 19: return '\n';
    case 57: return '=';
    case 24: return '0';
    case 25: return '1';
    case 26: return '2';
    case 27: return '3';
    case 28: return '4';
    case 29: return '5';
    case 30: return '6';
    case 31: return '7';
    case 32: return '8';
    case 33: return '9';
    case 34: return '*';
    case 35: return '+';
    case 37: return '-';
    case 38: return '.';
    case 39: return '/';
    }

    // fallback
    return keychar;
}
#endif

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

#endif // DISTRHO_PLUGIN_VST_HPP_INCLUDED
