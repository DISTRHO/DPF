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

#define DPF_TEST_POINT_CPP
#include "dgl/src/Geometry.cpp"

// --------------------------------------------------------------------------------------------------------------------

template <typename T>
static int runTestsPerType()
{
    USE_NAMESPACE_DGL;

    // basic usage
    {
        Point<T> p;
        DISTRHO_ASSERT_EQUAL(p.getX(), 0, "point start X value is 0");
        DISTRHO_ASSERT_EQUAL(p.getY(), 0, "point start Y value is 0");
        DISTRHO_ASSERT_EQUAL(p.isZero(), true, "point start is zero");
        DISTRHO_ASSERT_EQUAL(p.isNotZero(), false, "point start is for sure zero");

        p.setX(5);
        DISTRHO_ASSERT_EQUAL(p.getX(), 5, "point X value changed to 5");
        DISTRHO_ASSERT_EQUAL(p.getY(), 0, "point start Y value remains 0");
        DISTRHO_ASSERT_EQUAL(p.isZero(), false, "point after custom X is not zero");
        DISTRHO_ASSERT_EQUAL(p.isNotZero(), true, "point after custom X is for sure not zero");

        p.setY(7);
        DISTRHO_ASSERT_EQUAL(p.getX(), 5, "point X value remains 5");
        DISTRHO_ASSERT_EQUAL(p.getY(), 7, "point Y value changed to 7");
        DISTRHO_ASSERT_EQUAL(p.isZero(), false, "point after custom X and Y is not zero");
        DISTRHO_ASSERT_EQUAL(p.isNotZero(), true, "point after custom X and Y is for sure not zero");

        // TODO everything else
    }

    return 0;
}

int main()
{
    if (const int ret = runTestsPerType<double>())
        return ret;

    if (const int ret = runTestsPerType<float>())
        return ret;

    if (const int ret = runTestsPerType<int>())
        return ret;

    if (const int ret = runTestsPerType<uint>())
        return ret;

    if (const int ret = runTestsPerType<short>())
        return ret;

    if (const int ret = runTestsPerType<ushort>())
        return ret;

    if (const int ret = runTestsPerType<long>())
        return ret;

    if (const int ret = runTestsPerType<ulong>())
        return ret;

    if (const int ret = runTestsPerType<long long>())
        return ret;

    if (const int ret = runTestsPerType<unsigned long long>())
        return ret;

    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
