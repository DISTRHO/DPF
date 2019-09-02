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

#ifndef DISTRHO_VSTGUI_HPP_INCLUDED
#define DISTRHO_VSTGUI_HPP_INCLUDED

#include "Base.hpp"
#include "../distrho/extra/String.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

class VstGuiWidget
{
public:
    VstGuiWidget(const uint w = 1, const uint h = 1, const char* const t = "")
        : width(w),
          height(h),
          title(t),
          transientWinId(0),
          visible(false) {}

    virtual ~VstGuiWidget()
    {
    }

    uint getWidth() const noexcept
    {
        return width;
    }

    uint getHeight() const noexcept
    {
        return height;
    }

    const char* getTitle() const noexcept
    {
        return title;
    }

    uintptr_t getTransientWinId() const noexcept
    {
        return transientWinId;
    }

    bool isVisible() const noexcept
    {
        return visible;
    }

    bool isRunning() noexcept
    {
        return visible;
    }

    virtual void idle() {}

    virtual void setSize(uint w, uint h)
    {
        width = w;
        height = h;
    }

    virtual void setTitle(const char* const t)
    {
        title = t;
    }

    virtual void setTransientWinId(const uintptr_t winId)
    {
        transientWinId = winId;
    }

    virtual void setVisible(const bool yesNo)
    {
        visible = yesNo;
    }

    void terminateAndWaitForProcess()
    {
    }

private:
    uint width;
    uint height;
    DISTRHO_NAMESPACE::String title;
    uintptr_t transientWinId;
    bool visible;
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DISTRHO_VSTGUI_HPP_INCLUDED
