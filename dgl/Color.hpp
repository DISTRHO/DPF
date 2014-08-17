/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef DGL_COLOR_HPP_INCLUDED
#define DGL_COLOR_HPP_INCLUDED

#include "Base.hpp"

struct NVGcolor;

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

// TODO: create color from "#333" and "#112233" like strings

/**
  A color made from red, green, blue and alpha floating-point values in [0..1] range.
*/
struct Color {
   /**
      Direct access to the color values.
    */
    union {
        float rgba[4];
        struct { float red, green, blue, alpha; };
    };

   /**
      Create black color.
    */
    Color() noexcept;

   /**
      Create a color from red, green, blue and alpha numeric values.
      Values must be in [0..255] range.
    */
    Color(const int red, const int green, const int blue, const int alpha = 255) noexcept;

   /**
      Create a color from red, green, blue and alpha floating-point values.
      Values must in [0..1] range.
    */
    Color(const float red, const float green, const float blue, const float alpha = 1.0f) noexcept;

   /**
      Create a color by copying another color.
    */
    Color(const Color& color) noexcept;
    Color& operator=(const Color& color) noexcept;

   /**
      Create a color by linearly interpolating two other colors.
    */
    Color(const Color& color1, const Color& color2, const float u) noexcept;

   /**
      Create a color specified by hue, saturation, lightness and alpha.
      HSL values are all in [0..1] range, alpha in [0..255] range.
    */
    static Color HSL(const float hue, const float saturation, const float lightness, const int alpha = 255);

   /**
      Linearly interpolate this color against another.
    */
    void interpolate(const Color& other, const float u) noexcept;

   /**
      Check if this color matches another.
    */
    bool operator==(const Color& color) noexcept;
    bool operator!=(const Color& color) noexcept;

   /**
      @internal
      Needed for NanoVG compatibility.
    */
    Color(const NVGcolor&) noexcept;
    operator NVGcolor() const noexcept;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_COLOR_HPP_INCLUDED
