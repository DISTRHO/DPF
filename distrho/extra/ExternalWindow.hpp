/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2016 Filipe Coelho <falktx@falktx.com>
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

#ifndef DISTRHO_EXTERNAL_WINDOW_HPP_INCLUDED
#define DISTRHO_EXTERNAL_WINDOW_HPP_INCLUDED

#include "../DistrhoUtils.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// ExternalWindow class

class ExternalWindow
{
public:
    ExternalWindow(const uint w = 1, const uint h = 1)
        : width(w),
          height(h) {}

    uint getWidth() const noexcept
    {
        return width;
    }

    uint getHeight() const noexcept
    {
        return height;
    }

    void setSize(const uint w, const uint h) noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(w > 0 && h > 0,)

        width = w;
        height = h;
    }

private:
    uint width;
    uint height;

    DISTRHO_DECLARE_NON_COPY_CLASS(ExternalWindow)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_EXTERNAL_WINDOW_HPP_INCLUDED
