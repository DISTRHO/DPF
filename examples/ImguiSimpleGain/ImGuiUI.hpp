/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2021 Jean Pierre Cimalando <jp-dev@inbox.ru>
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

#pragma once

#include "TopLevelWidget.hpp"
#include "Color.hpp"

#ifndef DGL_OPENGL
# error ImGUI is only available in OpenGL mode
#endif

START_NAMESPACE_DGL

/**
   ImGui user interface class.
*/
class ImGuiUI : public TopLevelWidget,
                public IdleCallback {
public:
    ImGuiUI(Window& windowToMapTo);
    ~ImGuiUI() override;
    void setBackgroundColor(Color color);
    void setRepaintInterval(int intervalMs);

protected:
    virtual void onImGuiDisplay() = 0;

    virtual void onDisplay() override;
    virtual bool onKeyboard(const KeyboardEvent& event) override;
    virtual bool onSpecial(const SpecialEvent& event) override;
    virtual bool onMouse(const MouseEvent& event) override;
    virtual bool onMotion(const MotionEvent& event) override;
    virtual bool onScroll(const ScrollEvent& event) override;
    virtual void onResize(const ResizeEvent& event) override;
    virtual void idleCallback() override;

private:
    struct Impl;
    Impl* fImpl;
};

END_NAMESPACE_DGL
