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

#include "DistrhoUI.hpp"
#include "Window.hpp"
#include "KeyboardWidget.hpp"

START_NAMESPACE_DISTRHO

/**
  We need the rectangle class from DGL.
 */
using DGL::Rectangle;

// -----------------------------------------------------------------------------------------------------------

class MidiKeyboardExampleUI : public UI
{
  public:
    /* constructor */
    MidiKeyboardExampleUI()
        : UI(512, 512)
    {
        //getParentWindow().focus();
        // Add a min-size constraint to the window, to make sure that it can't become too small
        setGeometryConstraints(getWidth(), getHeight(), true, true);
    }

  protected:
    /* --------------------------------------------------------------------------------------------------------
    * DSP/Plugin Callbacks */

    /**
      A parameter has changed on the plugin side.
      This is called by the host to inform the UI about parameter changes.
    */
    void parameterChanged(uint32_t index, float value) override
    {
    }

    /* --------------------------------------------------------------------------------------------------------
    * Widget Callbacks */

    /**
      The OpenGL drawing function.
    */
    void onNanoDisplay() override
    {
        const uint width = getWidth();
        const uint height = getHeight();

        //KeyboardRenderer().draw(getContext());
    }

    /**
      Mouse press event.
    */
    bool onMouse(const MouseEvent &ev) override
    {
        return false;
    }

    bool onKeyboard(const KeyboardEvent &ev) override
    {
        fprintf(stderr, "%u\n", ev.key);
        repaint();
        return false;
    }

    // -------------------------------------------------------------------------------------------------------

  private:
    /**
      Set our UI class as non-copyable and add a leak detector just in case.
    */
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiKeyboardExampleUI)
};

/* ------------------------------------------------------------------------------------------------------------
 * UI entry point, called by DPF to create a new UI instance. */

UI *createUI()
{
    return new MidiKeyboardExampleUI();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
