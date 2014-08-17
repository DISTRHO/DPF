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

#include "../Color.hpp"

#include "nanovg/nanovg.h"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

Color::Color() noexcept
    : red(1.0f), green(1.0f), blue(1.0f), alpha(1.0f) {}

Color::Color(const int r, const int g, const int b, const int a) noexcept
    : red(static_cast<float>(r)/255.0f), green(static_cast<float>(g)/255.0f), blue(static_cast<float>(b)/255.0f), alpha(static_cast<float>(a)/255.0f) {}

Color::Color(const float r, const float g, const float b, const float a) noexcept
    : red(r), green(g), blue(b), alpha(a) {}

Color::Color(const Color& color) noexcept
    : red(color.red), green(color.green), blue(color.blue), alpha(color.alpha) {}

Color::Color(const Color& color1, const Color& color2, const float u) noexcept
    : red(color1.red), green(color1.green), blue(color1.blue), alpha(color1.alpha)
{
    interpolate(color2, u);
}

void Color::interpolate(const Color& other, const float u) noexcept
{
    const float u2 = (u < 0.0f) ? 0.0f : ((u > 1.0f) ? 1.0f : u);
    const float oneMinusU = 1.0f - u;

    red   = red   * oneMinusU + other.red   * u2;
    green = green * oneMinusU + other.green * u2;
    blue  = blue  * oneMinusU + other.blue  * u2;
    alpha = alpha * oneMinusU + other.alpha * u2;
}

Color Color::HSL(const float hue, const float saturation, const float lightness, const int alpha)
{
    return nvgHSLA(hue, saturation, lightness, alpha);
}

Color::Color(const NVGcolor& c) noexcept
    : red(c.r), green(c.g), blue(c.b), alpha(c.a) {}

Color::operator NVGcolor() const noexcept
{
    NVGcolor nc;
    nc.r = red;
    nc.g = green;
    nc.b = blue;
    nc.a = alpha;
    return nc;
}

Color& Color::operator=(const Color& color) noexcept
{
    red   = color.red;
    green = color.green;
    blue  = color.blue;
    alpha = color.alpha;
    return *this;
}

bool Color::operator==(const Color& color) noexcept
{
    return (red == color.red && green == color.green && blue == color.blue && alpha == color.alpha);
}

bool Color::operator!=(const Color& color) noexcept
{
    return (red != color.red || green != color.green || blue != color.blue || alpha != color.alpha);
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL
