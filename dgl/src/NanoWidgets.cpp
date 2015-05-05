/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2015 Filipe Coelho <falktx@falktx.com>
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

#include "../NanoWidgets.hpp"

#define BLENDISH_IMPLEMENTATION
#include "nanovg/nanovg.h"
#include "oui-blendish/blendish.h"
#include "oui-blendish/blendish_resources.h"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

static void registerBlendishResourcesIfNeeded(NVGcontext* const context)
{
    if (nvgFindFont(context, "__dpf_blendish__") >= 0)
        return;

    using namespace blendish_resources;

    bndSetFont(nvgCreateFontMem(context, "__dpf_blendish__", (const uchar*)dejavusans_ttf, dejavusans_ttf_size, 0));
    bndSetIconImage(nvgCreateImageMem(context, 0, (const uchar*)blender_icons16_png, blender_icons16_png_size));
}

// -----------------------------------------------------------------------

BlendishButton::BlendishButton(Window& parent, const char* text, int iconId) noexcept
    : NanoWidget(parent),
      fCurButton(-1),
      fCurState(0),
      fIconId(iconId),
      fText(text),
      fCallback(nullptr),
      leakDetector_BlendishButton()
{
    registerBlendishResourcesIfNeeded(getContext());
    _updateBounds();
}

BlendishButton::BlendishButton(NanoWidget* widget, const char* text, int iconId) noexcept
    : NanoWidget(widget),
      fCurButton(-1),
      fCurState(0),
      fIconId(iconId),
      fText(text),
      fCallback(nullptr),
      leakDetector_BlendishButton()
{
    registerBlendishResourcesIfNeeded(getContext());
    _updateBounds();
}

int BlendishButton::getIconId() const noexcept
{
    return fIconId;
}

void BlendishButton::setIconId(int iconId) noexcept
{
    if (fIconId == iconId)
        return;

    fIconId = iconId;
    _updateBounds();
    repaint();
}

const char* BlendishButton::getText() const noexcept
{
    return fText;
}

void BlendishButton::setText(const char* text) noexcept
{
    if (fText == text)
        return;

    fText = text;
    _updateBounds();
    repaint();
}

void BlendishButton::setCallback(Callback* callback) noexcept
{
    fCallback = callback;
}

void BlendishButton::onNanoDisplay()
{
    bndToolButton(getContext(),
                  getAbsoluteX(), getAbsoluteY(), getWidth(), getHeight(),
                  0, static_cast<BNDwidgetState>(fCurState), fIconId, fText);
}

bool BlendishButton::onMouse(const MouseEvent& ev)
{
    // button was released, handle it now
    if (fCurButton != -1 && ! ev.press)
    {
        DISTRHO_SAFE_ASSERT(fCurState == 2);

        // release button
        const int button = fCurButton;
        fCurButton = -1;

        // cursor was moved outside the button bounds, ignore click
        if (! contains(ev.pos))
        {
            fCurState = 0;
            repaint();
            return true;
        }

        // still on bounds, register click
        fCurState = 1;
        repaint();

        if (fCallback != nullptr)
            fCallback->blendishButtonClicked(this, button);

        return true;
    }

    // button was pressed, wait for release
    if (ev.press && contains(ev.pos))
    {
        fCurButton = ev.button;
        fCurState  = 2;
        repaint();
        return true;
    }

    return false;
}

bool BlendishButton::onMotion(const MotionEvent& ev)
{
    // keep pressed
    if (fCurButton != -1)
        return true;

    if (contains(ev.pos))
    {
        // check if entering hover
        if (fCurState == 0)
        {
            fCurState = 1;
            repaint();
            return true;
        }
    }
    else
    {
        // check if exiting hover
        if (fCurState == 1)
        {
            fCurState = 0;
            repaint();
            return true;
        }
    }

    return false;
}

void BlendishButton::_updateBounds()
{
    const float width  = bndLabelWidth (getContext(), fIconId, fText);
    const float height = bndLabelHeight(getContext(), fIconId, fText, width);

    setSize(width, height);
}

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#include "oui-blendish/blendish_resources.cpp"

// -----------------------------------------------------------------------
