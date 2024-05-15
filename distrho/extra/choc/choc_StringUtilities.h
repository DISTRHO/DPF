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

#ifndef CHOC_STRING_UTILS_HEADER_INCLUDED
#define CHOC_STRING_UTILS_HEADER_INCLUDED

#include <cctype>
#include <string>
#include <vector>
#include <cmath>
#include <chrono>
#include <memory>
#include <algorithm>
#include <cwctype>

START_NAMESPACE_DISTRHO

//==============================================================================
/// Returns a hex string for the given value.
/// If the minimum number of digits is non-zero, it will be zero-padded to fill this length;
template <typename IntegerType>
std::string createHexString (IntegerType value);


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

template <typename IntegerType>
std::string createHexString (IntegerType v)
{
    static_assert (std::is_integral<IntegerType>::value, "Need to pass integers into this method");
    auto value = static_cast<typename std::make_unsigned<IntegerType>::type> (v);

    char hex[40];
    const auto end = hex + sizeof (hex) - 1;
    auto d = end;
    *d = 0;

    for (;;)
    {
        *--d = "0123456789abcdef"[static_cast<uint32_t> (value) & 15u];
        value = static_cast<decltype (value)> (value >> 4);

        if (value == 0)
            return std::string (d, end);
    }
}

END_NAMESPACE_DISTRHO

#endif
