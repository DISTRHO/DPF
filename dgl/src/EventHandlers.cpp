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

#include "../EventHandlers.hpp"
#include "../SubWidget.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

struct ButtonEventHandler::PrivateData {
    ButtonEventHandler* const self;
    SubWidget* const widget;
    ButtonEventHandler::Callback* callback;

    int button;
    int state;
    bool checkable;
    bool checked;

    Point<double> oldMotionPos;

    PrivateData(ButtonEventHandler* const s, SubWidget* const w)
        : self(s),
          widget(w),
          callback(nullptr),
          button(-1),
          state(kButtonStateDefault),
          checkable(false),
          checked(false),
          oldMotionPos(0, 0) {}

    bool mouseEvent(const Widget::MouseEvent& ev)
    {
        // button was released, handle it now
        if (button != -1 && ! ev.press)
        {
            DISTRHO_SAFE_ASSERT(state & kButtonStateActive);

            // release button
            const int button2 = button;
            button = -1;

            const int state2 = state;
            state &= ~kButtonStateActive;

            // cursor was moved outside the button bounds, ignore click
            if (! widget->contains(ev.pos))
            {
                self->stateChanged(static_cast<State>(state2), static_cast<State>(state));
                widget->repaint();
                return true;
            }

            // still on bounds, register click
            self->stateChanged(static_cast<State>(state2), static_cast<State>(state));
            widget->repaint();

            if (checkable)
                checked = !checked;

            if (callback != nullptr)
                callback->buttonClicked(widget, button2);

            return true;
        }

        // button was pressed, wait for release
        if (ev.press && widget->contains(ev.pos))
        {
            const int state2 = state;
            button = static_cast<int>(ev.button);
            state |= kButtonStateActive;
            self->stateChanged(static_cast<State>(state2), static_cast<State>(state));
            widget->repaint();
            return true;
        }

        return false;
    }

    bool motionEvent(const Widget::MotionEvent& ev)
    {
        // keep pressed
        if (button != -1)
        {
            oldMotionPos = ev.pos;
            return true;
        }

        bool ret = false;

        if (widget->contains(ev.pos))
        {
            // check if entering hover
            if ((state & kButtonStateHover) == 0x0)
            {
                const int state2 = state;
                state |= kButtonStateHover;
                ret = widget->contains(oldMotionPos);
                self->stateChanged(static_cast<State>(state2), static_cast<State>(state));
                widget->repaint();
            }
        }
        else
        {
            // check if exiting hover
            if (state & kButtonStateHover)
            {
                const int state2 = state;
                state &= ~kButtonStateHover;
                ret = widget->contains(oldMotionPos);
                self->stateChanged(static_cast<State>(state2), static_cast<State>(state));
                widget->repaint();
            }
        }

        oldMotionPos = ev.pos;
        return ret;
    }

    void setActive(const bool active2, const bool sendCallback) noexcept
    {
        const bool active = state & kButtonStateActive;
        if (active == active2)
            return;

        state |= kButtonStateActive;
        widget->repaint();

        if (sendCallback && callback != nullptr)
            callback->buttonClicked(widget, -1);
    }

    void setChecked(const bool checked2, const bool sendCallback) noexcept
    {
        if (checked == checked2)
            return;

        checked = checked2;
        widget->repaint();

        if (sendCallback && callback != nullptr)
            callback->buttonClicked(widget, -1);
    }

    DISTRHO_DECLARE_NON_COPYABLE(PrivateData)
};

// --------------------------------------------------------------------------------------------------------------------

ButtonEventHandler::ButtonEventHandler(SubWidget* const self)
    : pData(new PrivateData(this, self)) {}

ButtonEventHandler::~ButtonEventHandler()
{
    delete pData;
}

bool ButtonEventHandler::isActive() noexcept
{
    return pData->state & kButtonStateActive;
}

void ButtonEventHandler::setActive(const bool active, const bool sendCallback) noexcept
{
    pData->setActive(active, sendCallback);
}

bool ButtonEventHandler::isChecked() const noexcept
{
    return pData->checked;
}

void ButtonEventHandler::setChecked(const bool checked, const bool sendCallback) noexcept
{
    pData->setChecked(checked, sendCallback);
}

bool ButtonEventHandler::isCheckable() const noexcept
{
    return pData->checkable;
}

void ButtonEventHandler::setCheckable(const bool checkable) noexcept
{
    if (pData->checkable == checkable)
        return;

    pData->checkable = checkable;
}

void ButtonEventHandler::setCallback(Callback* const callback) noexcept
{
    pData->callback = callback;
}

ButtonEventHandler::State ButtonEventHandler::getState() const noexcept
{
    return static_cast<State>(pData->state);
}

void ButtonEventHandler::stateChanged(State, State)
{
}

bool ButtonEventHandler::mouseEvent(const Widget::MouseEvent& ev)
{
    return pData->mouseEvent(ev);
}

bool ButtonEventHandler::motionEvent(const Widget::MotionEvent& ev)
{
    return pData->motionEvent(ev);
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL
