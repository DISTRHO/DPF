/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2019 Filipe Coelho <falktx@falktx.com>
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

// -----------------------------------------------------------------------
// Define namespace

#ifndef DGL_NAMESPACE
# define DGL_NAMESPACE DGL
#endif

#define START_NAMESPACE_DGL namespace DGL_NAMESPACE {
#define END_NAMESPACE_DGL }
#define USE_NAMESPACE_DGL using namespace DGL_NAMESPACE;

START_NAMESPACE_DGL

// -----------------------------------------------------------------------
// Base DGL enums

/**
   Convenience symbols for ASCII control characters.
 */
enum Char {
    kCharBackspace = 0x08,
    kCharEscape    = 0x1B,
    kCharDelete    = 0x7F
};

/**
   Keyboard modifier flags.
 */
enum Modifier {
    kModifierShift   = 1 << 0, /**< Shift key */
    kModifierControl = 1 << 1, /**< Control key */
    kModifierAlt     = 1 << 2, /**< Alt/Option key */
    kModifierSuper   = 1 << 3  /**< Mod4/Command/Windows key */
};

/**
   Special (non-Unicode) keyboard keys.
 */
enum Key {
    kKeyF1 = 1,
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
    kKeyControl,
    kKeyAlt,
    kKeySuper
};

/**
   Layout-independent keycodes.
   These keycodes are relative to an US QWERTY keyboard.
   Therefore, the keycode for the letter 'A' on an AZERTY keyboard will be equal to kKeyCodeQ.
 */
enum KeyCode {
    /* Zero, does not correspond to any key. */
    kKeyCodeNone = 0,

    kKeyCodeA = 4,
    kKeyCodeB = 5,
    kKeyCodeC = 6,
    kKeyCodeD = 7,
    kKeyCodeE = 8,
    kKeyCodeF = 9,
    kKeyCodeG = 10,
    kKeyCodeH = 11,
    kKeyCodeI = 12,
    kKeyCodeJ = 13,
    kKeyCodeK = 14,
    kKeyCodeL = 15,
    kKeyCodeM = 16,
    kKeyCodeN = 17,
    kKeyCodeO = 18,
    kKeyCodeP = 19,
    kKeyCodeQ = 20,
    kKeyCodeR = 21,
    kKeyCodeS = 22,
    kKeyCodeT = 23,
    kKeyCodeU = 24,
    kKeyCodeV = 25,
    kKeyCodeW = 26,
    kKeyCodeX = 27,
    kKeyCodeY = 28,
    kKeyCodeZ = 29,
    kKeyCode1 = 30,
    kKeyCode2 = 31,
    kKeyCode3 = 32,
    kKeyCode4 = 33,
    kKeyCode5 = 34,
    kKeyCode6 = 35,
    kKeyCode7 = 36,
    kKeyCode8 = 37,
    kKeyCode9 = 38,
    kKeyCode0 = 39,
    kKeyCodeEscape = 41,
    kKeyCodeDelete = 42,
    kKeyCodeTab = 43,
    kKeyCodeSpace = 44,
    kKeyCodeMinus = 45,
    kKeyCodeEquals = 46,
    kKeyCodeLeftBracket = 47,
    kKeyCodeRightBracket = 48,
    kKeyCodeBackslash = 49,
    kKeyCodeSemicolon = 51,
    kKeyCodeQuote = 52,
    kKeyCodeGrave = 53,
    kKeyCodeComma = 54,
    kKeyCodePeriod = 55,
    kKeyCodeSlash = 56,
    kKeyCodeCapsLock = 57,
    kKeyCodeF1 = 58,
    kKeyCodeF2 = 59,
    kKeyCodeF3 = 60,
    kKeyCodeF4 = 61,
    kKeyCodeF5 = 62,
    kKeyCodeF6 = 63,
    kKeyCodeF7 = 64,
    kKeyCodeF8 = 65,
    kKeyCodeF9 = 66,
    kKeyCodeF10 = 67,
    kKeyCodeF11 = 68,
    kKeyCodeF12 = 69,
    kKeyCodePrintScreen = 70,
    kKeyCodeScrollLock = 71,
    kKeyCodePause = 72,
    kKeyCodeInsert = 73,
    kKeyCodeHome = 74,
    kKeyCodePageUp = 75,
    kKeyCodeDeleteForward = 76,
    kKeyCodeEnd = 77,
    kKeyCodePageDown = 78,
    kKeyCodeRight = 79,
    kKeyCodeLeft = 80,
    kKeyCodeDown = 81,
    kKeyCodeUp = 82,
    kKeyCodeKP_NumLock = 83,
    kKeyCodeKP_Divide = 84,
    kKeyCodeKP_Multiply = 85,
    kKeyCodeKP_Subtract = 86,
    kKeyCodeKP_Add = 87,
    kKeyCodeKP_Enter = 88,
    kKeyCodeKP_1 = 89,
    kKeyCodeKP_2 = 90,
    kKeyCodeKP_3 = 91,
    kKeyCodeKP_4 = 92,
    kKeyCodeKP_5 = 93,
    kKeyCodeKP_6 = 94,
    kKeyCodeKP_7 = 95,
    kKeyCodeKP_8 = 96,
    kKeyCodeKP_9 = 97,
    kKeyCodeKP_0 = 98,
    kKeyCodePoint = 99,
    kKeyCodeNonUSBackslash = 100,
    kKeyCodeKP_Equals = 103,
    kKeyCodeF13 = 104,
    kKeyCodeF14 = 105,
    kKeyCodeF15 = 106,
    kKeyCodeF16 = 107,
    kKeyCodeF17 = 108,
    kKeyCodeF18 = 109,
    kKeyCodeF19 = 110,
    kKeyCodeF20 = 111,
    kKeyCodeF21 = 112,
    kKeyCodeF22 = 113,
    kKeyCodeF23 = 114,
    kKeyCodeF24 = 115,
    kKeyCodeHelp = 117,
    kKeyCodeMenu = 118,
    kKeyCodeMute = 127,
    kKeyCodeSysReq = 154,
    kKeyCodeReturn = 158,
    kKeyCodeKP_Clear = 216,
    kKeyCodeKP_Decimal = 220,
    kKeyCodeLeftControl = 224,
    kKeyCodeLeftShift = 225,
    kKeyCodeLeftAlt = 226,
    kKeyCodeLeftGUI = 227,
    kKeyCodeRightControl = 228,
    kKeyCodeRightShift = 229,
    kKeyCodeRightAlt = 230,
    kKeyCodeRightGUI = 231
};

// -----------------------------------------------------------------------
// Base DGL classes

/**
   Graphics context, definition depends on build type.
 */
struct GraphicsContext;

/**
   Idle callback.
 */
class IdleCallback
{
public:
    virtual ~IdleCallback() {}
    virtual void idleCallback() = 0;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#ifndef DONT_SET_USING_DGL_NAMESPACE
  // If your code uses a lot of DGL classes, then this will obviously save you
  // a lot of typing, but can be disabled by setting DONT_SET_USING_DGL_NAMESPACE.
  using namespace DGL_NAMESPACE;
#endif

// -----------------------------------------------------------------------

#endif // DGL_BASE_HPP_INCLUDED
