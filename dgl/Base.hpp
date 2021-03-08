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

#ifndef DGL_BASE_HPP_INCLUDED
#define DGL_BASE_HPP_INCLUDED

#include "../distrho/extra/LeakDetector.hpp"
#include "../distrho/extra/ScopedPointer.hpp"

// --------------------------------------------------------------------------------------------------------------------
// Define namespace

#ifndef DGL_NAMESPACE
# define DGL_NAMESPACE DGL
#endif

#define START_NAMESPACE_DGL namespace DGL_NAMESPACE {
#define END_NAMESPACE_DGL }
#define USE_NAMESPACE_DGL using namespace DGL_NAMESPACE;

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------
// Base DGL enums

/**
   Keyboard modifier flags.
 */
enum Modifier {
    kModifierShift   = 1u << 0u, /**< Shift key */
    kModifierControl = 1u << 1u, /**< Control key */
    kModifierAlt     = 1u << 2u, /**< Alt/Option key */
    kModifierSuper   = 1u << 3u  /**< Mod4/Command/Windows key */
};

/**
   Keyboard key codepoints.

   All keys are identified by a Unicode code point in PuglEventKey::key.  This
   enumeration defines constants for special keys that do not have a standard
   code point, and some convenience constants for control characters.  Note
   that all keys are handled in the same way, this enumeration is just for
   convenience when writing hard-coded key bindings.

   Keys that do not have a standard code point use values in the Private Use
   Area in the Basic Multilingual Plane (`U+E000` to `U+F8FF`).  Applications
   must take care to not interpret these values beyond key detection, the
   mapping used here is arbitrary and specific to DPF.
 */
enum Key {
    // Convenience symbols for ASCII control characters
    kKeyBackspace = 0x08,
    kKeyEscape    = 0x1B,
    kKeyDelete    = 0x7F,

    // Backwards compatibility with old DPF
    kCharBackspace = kKeyBackspace,
    kCharEscape    = kKeyEscape,
    kCharDelete    = kKeyDelete,

    // Unicode Private Use Area
    kKeyF1 = 0xE000,
    kKeyF2,
    kKeyF3,
    kKeyF4,
    kKeyF5,
    kKeyF6,
    kKeyF7,
    kKeyF8,
    kKeyF9,
    kKeyF10,
    kKeyF11,
    kKeyF12,
    kKeyLeft,
    kKeyUp,
    kKeyRight,
    kKeyDown,
    kKeyPageUp,
    kKeyPageDown,
    kKeyHome,
    kKeyEnd,
    kKeyInsert,
    kKeyShift,
    kKeyShiftL = kKeyShift,
    kKeyShiftR,
    kKeyControl,
    kKeyControlL = kKeyControl,
    kKeyControlR,
    kKeyAlt,
    kKeyAltL = kKeyAlt,
    kKeyAltR,
    kKeySuper,
    kKeySuperL = kKeySuper,
    kKeySuperR,
    kKeyMenu,
    kKeyCapsLock,
    kKeyScrollLock,
    kKeyNumLock,
    kKeyPrintScreen,
    kKeyPause
};

// --------------------------------------------------------------------------------------------------------------------
// Base DGL classes

/**
   Graphics context, definition depends on build type.
 */
struct GraphicsContext {};

/**
   Idle callback.
 */
struct IdleCallback
{
    virtual ~IdleCallback() {}
    virtual void idleCallback() = 0;
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#ifndef DONT_SET_USING_DGL_NAMESPACE
  // If your code uses a lot of DGL classes, then this will obviously save you
  // a lot of typing, but can be disabled by setting DONT_SET_USING_DGL_NAMESPACE.
  using namespace DGL_NAMESPACE;
#endif

// --------------------------------------------------------------------------------------------------------------------

#endif // DGL_BASE_HPP_INCLUDED
