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

#include "DistrhoUI.hpp"

#include "Color.hpp"

#include <cstring>

START_NAMESPACE_DISTRHO

/**
  We need a few classes from DGL.
 */
using DGL_NAMESPACE::Color;
using DGL_NAMESPACE::GraphicsContext;
using DGL_NAMESPACE::Rectangle;

// -----------------------------------------------------------------------------------------------------------

class SendNoteExampleUI : public UI
{
public:
    SendNoteExampleUI()
        : UI(64*12+8, 64+8),
          fLastKey(-1)
    {
        std::memset(fKeyState, 0, sizeof(fKeyState));
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
        (void)index;
        (void)value;
    }

   /* --------------------------------------------------------------------------------------------------------
    * Widget Callbacks */

   /**
      The OpenGL drawing function.
      This UI will draw a row of 12 keys, with on/off states according to pressed status.
    */
    void onDisplay() override
    {
        const GraphicsContext& context(getGraphicsContext());

        for (int key = 0; key < 12; ++key)
        {
            bool pressed = fKeyState[key];
            Rectangle<int> bounds = getKeyBounds(key);

            if (pressed)
                Color(0.8f, 0.5f, 0.3f).setFor(context);
            else
                Color(0.3f, 0.5f, 0.8f).setFor(context);

            bounds.draw(context);
        }
    }

   /**
      Mouse press event.
      This UI will de/activate keys when you click them and reports it as MIDI note events to the plugin.
    */
    bool onMouse(const MouseEvent& ev) override
    {
        // Test for last key release first
        if (fLastKey != -1 && ! ev.press)
        {
            // Send a note event. Velocity=0 means off
            sendNote(0, kNoteOctaveStart+fLastKey, 0);
            // Unset key state
            fKeyState[fLastKey] = false;
            fLastKey = -1;
            repaint();
            return true;
        }

        // Test for left-clicked first.
        if (ev.button != 1)
            return false;

        // Find the key which is pressed, if any
        int whichKey = -1;
        for (int key = 0; key < 12 && whichKey == -1; ++key)
        {
            Rectangle<int> bounds = getKeyBounds(key);

            if (bounds.contains(ev.pos))
                whichKey = key;
        }

        if (whichKey == -1)
            return false;
        if (fKeyState[whichKey] == ev.press)
            return false;

        // Send a note event. Velocity=0 means off
        sendNote(0, kNoteOctaveStart+whichKey, ev.press ? kNoteVelocity : 0);

        // Set pressed state of this key, and update display
        fLastKey = whichKey;
        fKeyState[whichKey] = ev.press;
        repaint();

        return true;
    }

   /**
      Set our UI class as non-copyable and add a leak detector just in case.
    */
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SendNoteExampleUI)

private:
    /**
       Get the bounds of a particular key of the virtual MIDI keyboard.
     */
    Rectangle<int> getKeyBounds(unsigned index) const
    {
        Rectangle<int> bounds;
        int padding = 8;
        bounds.setX(64 * index + padding);
        bounds.setY(padding);
        bounds.setWidth(64 - padding);
        bounds.setHeight(64 - padding);
        return bounds;
    }

    /**
       The pressed state of one octave of a virtual MIDI keyboard.
     */
    bool fKeyState[12];
    int8_t fLastKey;

    enum
    {
        kNoteVelocity = 100, // velocity of sent Note-On events
        kNoteOctaveStart = 60, // starting note of the virtual MIDI keyboard
    };
};

/* ------------------------------------------------------------------------------------------------------------
 * UI entry point, called by DPF to create a new UI instance. */

UI* createUI()
{
    return new SendNoteExampleUI();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
