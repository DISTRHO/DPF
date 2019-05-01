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


// -----------------------------------------------------------------------------------------------------------

class MidiKeyboardExampleUI : public UI,
                              public KeyboardWidget::Callback
{
  public:
    /* constructor */
    MidiKeyboardExampleUI()
        : UI(kUIWidth, kUIHeight),
          fKeyboardWidget(getParentWindow())
    {
        const uint keyboardDeltaWidth = kUIWidth - fKeyboardWidget.getWidth();

        fKeyboardWidget.setAbsoluteX(keyboardDeltaWidth / 2);
        fKeyboardWidget.setAbsoluteY(kUIHeight - fKeyboardWidget.getHeight());
        fKeyboardWidget.setCallback(this);

        // Add a min-size constraint to the window, to make sure that it can't become too small
        setGeometryConstraints(kUIWidth, kUIHeight, true, true);
    }

  protected:
    /* --------------------------------------------------------------------------------------------------------
    * DSP/Plugin Callbacks */

    /**
      A parameter has changed on the plugin side.
      This is called by the host to inform the UI about parameter changes.
      This plugin does not have any parameters, so we can leave this blank.
    */
    void parameterChanged(uint32_t, float) override
    {
    }

    /* --------------------------------------------------------------------------------------------------------
    * Widget Callbacks */

    /**
      The OpenGL drawing function.
      Here, we set a custom background color.
    */
    void onDisplay() override
    {
      glClearColor(17.f / 255.f,
                   17.f / 255.f,
                   17.f / 255.f,
                   17.f / 255.f);

      glClear(GL_COLOR_BUFFER_BIT);
    }

    /**
       Called when a note is pressed on the piano.
     */
    void keyboardKeyPressed(const uint keyIndex) override
    {
      const uint C4 = 60;
      sendNote(0, C4 + keyIndex, 127);
    }

    /**
       Called when a note is released on the piano.
     */
    void keyboardKeyReleased(const uint keyIndex) override
    {
      const uint C4 = 60;
      sendNote(0, C4 + keyIndex, 0);
    }

    // -------------------------------------------------------------------------------------------------------

  private:
    static const int kUIWidth = 800;
    static const int kUIHeight = 132;

    KeyboardWidget fKeyboardWidget;

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
