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

#include "tests.hpp"

#define DPF_TEST_COLOR_CPP
#include "dgl/src/Color.cpp"

// --------------------------------------------------------------------------------------------------------------------

int main()
{
    USE_NAMESPACE_DGL;

    // constructor with no arguments, must give solid black
    {
        Color c;
        DISTRHO_ASSERT_EQUAL(c.red, 0.0f, "red value is 0");
        DISTRHO_ASSERT_EQUAL(c.green, 0.0f, "green value is 0");
        DISTRHO_ASSERT_EQUAL(c.blue, 0.0f, "blue value is 0");
        DISTRHO_ASSERT_EQUAL(c.alpha, 1.0f, "alpha value is 1");
    }

    // constructor gives correct floating-point values (arguments are r, g, b, a; in order)
    {
        Color c(0.1f, 0.2f, 0.3f, 0.4f);
        DISTRHO_ASSERT_EQUAL(c.red, 0.1f, "red value is 0.1");
        DISTRHO_ASSERT_EQUAL(c.green, 0.2f, "green value is 0.2");
        DISTRHO_ASSERT_EQUAL(c.blue, 0.3f, "blue value is 0.3");
        DISTRHO_ASSERT_EQUAL(c.alpha, 0.4f, "alpha value is 0.4");
    }

    // constructor gives correct integer values normalized to float (arguments are r, g, b, a; in order)
    {
        Color c(51, 102, 153);
        DISTRHO_ASSERT_SAFE_EQUAL(c.red, 0.2f, "red value is 0.2 (integer 51)");
        DISTRHO_ASSERT_SAFE_EQUAL(c.green, 0.4f, "green value is 0.4 (integer 102)");
        DISTRHO_ASSERT_SAFE_EQUAL(c.blue, 0.6f, "blue value is 0.6 (integer 153)");
        DISTRHO_ASSERT_SAFE_EQUAL(c.alpha, 1.0f, "alpha value is 1");

        Color white(255, 255, 255);
        DISTRHO_ASSERT_EQUAL(white.red, 1.0f, "white's red value is 1");
        DISTRHO_ASSERT_EQUAL(white.green, 1.0f, "white's green value is 1");
        DISTRHO_ASSERT_EQUAL(white.blue, 1.0f, "white's blue value is 1");
        DISTRHO_ASSERT_EQUAL(white.alpha, 1.0f, "white alpha value is 1");
    }

    // copy colors around
    {
        Color black;
        Color halfTransparentWhite(1.0f, 1.0f, 1.0f, 0.5f);

        // constructor copy
        Color test(halfTransparentWhite);
        DISTRHO_ASSERT_EQUAL(test.red, 1.0f, "copied white's red value is 1.0");
        DISTRHO_ASSERT_EQUAL(test.green, 1.0f, "copied white's green value is 1");
        DISTRHO_ASSERT_EQUAL(test.blue, 1.0f, "copied white's blue value is 1");
        DISTRHO_ASSERT_EQUAL(test.alpha, 0.5f, "copied white's alpha value is 0.5");

        // assign operator
        test = black;
        DISTRHO_ASSERT_EQUAL(test.red, 0.0f, "assigned black's red value is 0");
        DISTRHO_ASSERT_EQUAL(test.green, 0.0f, "assigned black's green value is 0");
        DISTRHO_ASSERT_EQUAL(test.blue, 0.0f, "assigned black's blue value is 0");
        DISTRHO_ASSERT_EQUAL(test.alpha, 1.0f, "assigned black's alpha value is 1");
    }

    // simple color comparisons
    {
        Color black1, black2;
        Color white(1.0f, 1.0f, 1.0f);
        Color halfTransparentWhite(1.0f, 1.0f, 1.0f, 0.5f);

        // logic operators
        DISTRHO_ASSERT_EQUAL(black1, black1, "color equals itself");
        DISTRHO_ASSERT_EQUAL(black1, black2, "black equals black");
        DISTRHO_ASSERT_NOT_EQUAL(black1, white, "black is not white");
        DISTRHO_ASSERT_NOT_EQUAL(black1, halfTransparentWhite, "black is not half-transparent white");
        DISTRHO_ASSERT_NOT_EQUAL(white, halfTransparentWhite, "white is not half-transparent white");

        // class functions (truthful)
        DISTRHO_ASSERT_EQUAL(black1.isEqual(black1), true, "color equals itself");
        DISTRHO_ASSERT_EQUAL(black1.isEqual(black2), true, "black equals black");
        DISTRHO_ASSERT_EQUAL(black1.isNotEqual(white), true, "black is not white");
        DISTRHO_ASSERT_EQUAL(white.isNotEqual(halfTransparentWhite), true, "white is not half-transparent white");

        // class functions (inverted)
        DISTRHO_ASSERT_EQUAL(black1.isNotEqual(black1), false, "color equals itself");
        DISTRHO_ASSERT_EQUAL(black1.isNotEqual(black2), false, "black equals black");
        DISTRHO_ASSERT_EQUAL(black1.isEqual(white), false, "black is not white");
        DISTRHO_ASSERT_EQUAL(white.isEqual(halfTransparentWhite), false, "white is not half-transparent white");

        // class functions ignoring alpha
        DISTRHO_ASSERT_EQUAL(black1.isEqual(black1, false), true, "color equals itself");
        DISTRHO_ASSERT_EQUAL(black1.isEqual(black2, false), true, "black equals black");
        DISTRHO_ASSERT_EQUAL(black1.isNotEqual(white, false), true, "black is not white");
        DISTRHO_ASSERT_EQUAL(white.isEqual(halfTransparentWhite, false), true,
                             "white is half-transparent white if we ignore alpha");
    }

    // TODO advanced color comparisons
    {
    }

    // TODO fromHSL
    {
    }

    // create colors from html strings
    {
        Color c000 = Color::fromHTML("#000");
        DISTRHO_ASSERT_EQUAL(c000.red, 0.0f, "#000 red value is 0");
        DISTRHO_ASSERT_EQUAL(c000.green, 0.0f, "#000 green value is 0");
        DISTRHO_ASSERT_EQUAL(c000.blue, 0.0f, "#000 blue value is 0");
        DISTRHO_ASSERT_EQUAL(c000.alpha, 1.0f, "#000 alpha value is 1");

        Color c000000 = Color::fromHTML("#000000");
        DISTRHO_ASSERT_EQUAL(c000000.red, 0.0f, "#000000 red value is 0");
        DISTRHO_ASSERT_EQUAL(c000000.green, 0.0f, "#000000 green value is 0");
        DISTRHO_ASSERT_EQUAL(c000000.blue, 0.0f, "#000000 blue value is 0");
        DISTRHO_ASSERT_EQUAL(c000000.alpha, 1.0f, "#000000 alpha value is 1");

        Color cfff = Color::fromHTML("#fff");
        DISTRHO_ASSERT_EQUAL(cfff.red, 1.0f, "#fff red value is 1");
        DISTRHO_ASSERT_EQUAL(cfff.green, 1.0f, "#fff green value is 1");
        DISTRHO_ASSERT_EQUAL(cfff.blue, 1.0f, "#fff blue value is 1");
        DISTRHO_ASSERT_EQUAL(cfff.alpha, 1.0f, "#fff alpha value is 1");

        Color cffffff = Color::fromHTML("#ffffff");
        DISTRHO_ASSERT_EQUAL(cffffff.red, 1.0f, "#ffffff red value is 1");
        DISTRHO_ASSERT_EQUAL(cffffff.green, 1.0f, "#ffffff green value is 1");
        DISTRHO_ASSERT_EQUAL(cffffff.blue, 1.0f, "#ffffff blue value is 1");
        DISTRHO_ASSERT_EQUAL(cffffff.alpha, 1.0f, "#ffffff alpha value is 1");

        Color cf00 = Color::fromHTML("#f00");
        DISTRHO_ASSERT_EQUAL(cf00.red, 1.0f, "#f00 red value is 1");
        DISTRHO_ASSERT_EQUAL(cf00.green, 0.0f, "#f00 green value is 0");
        DISTRHO_ASSERT_EQUAL(cf00.blue, 0.0f, "#f00 blue value is 0");

        Color cff0000 = Color::fromHTML("#ff0000");
        DISTRHO_ASSERT_EQUAL(cff0000.red, 1.0f, "#ff0000 red value is 1");
        DISTRHO_ASSERT_EQUAL(cff0000.green, 0.0f, "#ff0000 green value is 0");
        DISTRHO_ASSERT_EQUAL(cff0000.blue, 0.0f, "#ff0000 blue value is 0");

        Color c0f0 = Color::fromHTML("#0f0");
        DISTRHO_ASSERT_EQUAL(c0f0.red, 0.0f, "#0f0 red value is 0");
        DISTRHO_ASSERT_EQUAL(c0f0.green, 1.0f, "#0f0 green value is 1");
        DISTRHO_ASSERT_EQUAL(c0f0.blue, 0.0f, "#0f0 blue value is 0");

        Color c00ff00 = Color::fromHTML("#00ff00");
        DISTRHO_ASSERT_EQUAL(c00ff00.red, 0.0f, "#00ff00 red value is 0");
        DISTRHO_ASSERT_EQUAL(c00ff00.green, 1.0f, "#00ff00 green value is 1");
        DISTRHO_ASSERT_EQUAL(c00ff00.blue, 0.0f, "#00ff00 blue value is 0");

        Color c00f = Color::fromHTML("#00f");
        DISTRHO_ASSERT_EQUAL(c00f.red, 0.0f, "#00f red value is 0");
        DISTRHO_ASSERT_EQUAL(c00f.green, 0.0f, "#00f green value is 0");
        DISTRHO_ASSERT_EQUAL(c00f.blue, 1.0f, "#00f blue value is 1");

        Color c0000ff = Color::fromHTML("#0000ff");
        DISTRHO_ASSERT_EQUAL(c0000ff.red, 0.0f, "#0000ff red value is 0");
        DISTRHO_ASSERT_EQUAL(c0000ff.green, 0.0f, "#0000ff green value is 0");
        DISTRHO_ASSERT_EQUAL(c0000ff.blue, 1.0f, "#0000ff blue value is 1");

        // half point, round to 1 decimal point due to precision loss
        Color grey = Color::fromHTML("#7b7b7b");
        DISTRHO_ASSERT_SAFE_EQUAL(std::round(grey.red*10)/10, 0.5f, "grey's rounded red value is 0.5");
        DISTRHO_ASSERT_SAFE_EQUAL(std::round(grey.green*10)/10, 0.5f, "grey's rounded green value is 0.5");
        DISTRHO_ASSERT_SAFE_EQUAL(std::round(grey.blue*10)/10, 0.5f, "grey's rounded blue value is 0.5");
    }

    // check bounds
    {
        Color negativeInteger(-1, -1, -1, -1.0f);
        DISTRHO_ASSERT_EQUAL(negativeInteger.red, 0.0f, "red value is 0");
        DISTRHO_ASSERT_EQUAL(negativeInteger.green, 0.0f, "green value is 0");
        DISTRHO_ASSERT_EQUAL(negativeInteger.blue, 0.0f, "blue value is 0");
        DISTRHO_ASSERT_EQUAL(negativeInteger.alpha, 0.0f, "alpha value is 0");

        Color negativeFloat(-1.0f, -1.0f, -1.0f, -1.0f);
        DISTRHO_ASSERT_EQUAL(negativeFloat.red, 0.0f, "red value is 0");
        DISTRHO_ASSERT_EQUAL(negativeFloat.green, 0.0f, "green value is 0");
        DISTRHO_ASSERT_EQUAL(negativeFloat.blue, 0.0f, "blue value is 0");
        DISTRHO_ASSERT_EQUAL(negativeFloat.alpha, 0.0f, "alpha value is 0");

        Color overflowInteger(0xfff, 0xfff, 0xfff, 0xfff);
        DISTRHO_ASSERT_EQUAL(overflowInteger.red, 1.0f, "red value is 1");
        DISTRHO_ASSERT_EQUAL(overflowInteger.green, 1.0f, "green value is 1");
        DISTRHO_ASSERT_EQUAL(overflowInteger.blue, 1.0f, "blue value is 1");
        DISTRHO_ASSERT_EQUAL(overflowInteger.alpha, 1.0f, "alpha value is 1");

        Color overflowFloat(2.0f, 2.0f, 2.0f, 2.0f);
        DISTRHO_ASSERT_EQUAL(overflowFloat.red, 1.0f, "red value is 1");
        DISTRHO_ASSERT_EQUAL(overflowFloat.green, 1.0f, "green value is 1");
        DISTRHO_ASSERT_EQUAL(overflowFloat.blue, 1.0f, "blue value is 1");
        DISTRHO_ASSERT_EQUAL(overflowFloat.alpha, 1.0f, "alpha value is 1");
    }

    // TODO interpolation
    {
    }

    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
