/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2018 Filipe Coelho <falktx@falktx.com>
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

START_NAMESPACE_DISTRHO

/**
  We need the Color class from DGL.
 */
using DGL::Color;

/**
  Smooth meters a bit.
 */
static const float kSmoothMultiplier = 3.0f;

// -----------------------------------------------------------------------------------------------------------

class ExampleUIMeters : public UI
{
public:
    ExampleUIMeters()
        : UI(128, 512),
          // default color is green
          fColor(93, 231, 61),
          // which is value 0
          fColorValue(0),
          // init meter values to 0
          fOutLeft(0.0f),
          fOutRight(0.0f)
    {
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
        switch (index)
        {
        case 0: // color
            updateColor(std::round(value));
            break;

        case 1: // out-left
            value = (fOutLeft * kSmoothMultiplier + value) / (kSmoothMultiplier + 1.0f);

            /**/ if (value < 0.001f) value = 0.0f;
            else if (value > 0.999f) value = 1.0f;

            if (fOutLeft != value)
            {
                fOutLeft = value;
                repaint();
            }
            break;

        case 2: // out-right
            value = (fOutRight * kSmoothMultiplier + value) / (kSmoothMultiplier + 1.0f);

            /**/ if (value < 0.001f) value = 0.0f;
            else if (value > 0.999f) value = 1.0f;

            if (fOutRight != value)
            {
                fOutRight = value;
                repaint();
            }
            break;
        }
    }

   /**
      A state has changed on the plugin side.
      This is called by the host to inform the UI about state changes.
    */
    void stateChanged(const char*, const char*)
    {
        // nothing here
    }

   /* --------------------------------------------------------------------------------------------------------
    * Widget Callbacks */

   /**
      The NanoVG drawing function.
    */
    void onNanoDisplay() override
    {
        static const Color kColorBlack(0, 0, 0);
        static const Color kColorRed(255, 0, 0);
        static const Color kColorYellow(255, 255, 0);

        // get meter values
        const float outLeft(fOutLeft);
        const float outRight(fOutRight);

        // tell DSP side to reset meter values
        setState("reset", "");

        // useful vars
        const float halfWidth        = static_cast<float>(getWidth())/2;
        const float redYellowHeight  = static_cast<float>(getHeight())*0.2f;
        const float yellowBaseHeight = static_cast<float>(getHeight())*0.4f;
        const float baseBaseHeight   = static_cast<float>(getHeight())*0.6f;

        // create gradients
        Paint fGradient1 = linearGradient(0.0f, 0.0f,            0.0f, redYellowHeight,  kColorRed,    kColorYellow);
        Paint fGradient2 = linearGradient(0.0f, redYellowHeight, 0.0f, yellowBaseHeight, kColorYellow, fColor);

        // paint left meter
        beginPath();
        rect(0.0f, 0.0f, halfWidth-1.0f, redYellowHeight);
        fillPaint(fGradient1);
        fill();
        closePath();

        beginPath();
        rect(0.0f, redYellowHeight-0.5f, halfWidth-1.0f, yellowBaseHeight);
        fillPaint(fGradient2);
        fill();
        closePath();

        beginPath();
        rect(0.0f, redYellowHeight+yellowBaseHeight-1.5f, halfWidth-1.0f, baseBaseHeight);
        fillColor(fColor);
        fill();
        closePath();

        // paint left black matching output level
        beginPath();
        rect(0.0f, 0.0f, halfWidth-1.0f, (1.0f-outLeft)*getHeight());
        fillColor(kColorBlack);
        fill();
        closePath();

        // paint right meter
        beginPath();
        rect(halfWidth+1.0f, 0.0f, halfWidth-2.0f, redYellowHeight);
        fillPaint(fGradient1);
        fill();
        closePath();

        beginPath();
        rect(halfWidth+1.0f, redYellowHeight-0.5f, halfWidth-2.0f, yellowBaseHeight);
        fillPaint(fGradient2);
        fill();
        closePath();

        beginPath();
        rect(halfWidth+1.0f, redYellowHeight+yellowBaseHeight-1.5f, halfWidth-2.0f, baseBaseHeight);
        fillColor(fColor);
        fill();
        closePath();

        // paint right black matching output level
        beginPath();
        rect(halfWidth+1.0f, 0.0f, halfWidth-2.0f, (1.0f-outRight)*getHeight());
        fillColor(kColorBlack);
        fill();
        closePath();
    }

   /**
      Mouse press event.
      This UI will change color when clicked.
    */
    bool onMouse(const MouseEvent& ev) override
    {
        // Test for left-clicked + pressed first.
        if (ev.button != 1 || ! ev.press)
            return false;

        const int newColor(fColorValue == 0 ? 1 : 0);
        updateColor(newColor);
        setParameterValue(0, newColor);

        return true;
    }

    // -------------------------------------------------------------------------------------------------------

private:
   /**
      Color and its matching parameter value.
    */
    Color fColor;
    int   fColorValue;

   /**
      Meter values.
      These are the parameter outputs from the DSP side.
    */
    float fOutLeft, fOutRight;

   /**
      Update color if needed.
    */
    void updateColor(const int color)
    {
        if (fColorValue == color)
            return;

        fColorValue = color;

        switch (color)
        {
        case METER_COLOR_GREEN:
            fColor = Color(93, 231, 61);
            break;
        case METER_COLOR_BLUE:
            fColor = Color(82, 238, 248);
            break;
        }

        repaint();
    }

   /**
      Set our UI class as non-copyable and add a leak detector just in case.
    */
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExampleUIMeters)
};

/* ------------------------------------------------------------------------------------------------------------
 * UI entry point, called by DPF to create a new UI instance. */

UI* createUI()
{
    return new ExampleUIMeters();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
