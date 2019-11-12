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

START_NAMESPACE_DISTRHO

/**
  We need the rectangle class from DGL.
 */
using DGL_NAMESPACE::Rectangle;

// -----------------------------------------------------------------------------------------------------------

class ExampleUIParameters : public UI
{
public:
    /* constructor */
    ExampleUIParameters()
        : UI(512, 512)
    {
       /**
          Initialize all our parameters to their defaults.
          In this example all default values are false, so we can simply zero them.
        */
        std::memset(fParamGrid, 0, sizeof(bool)*9);

        // TODO explain why this is here
        setGeometryConstraints(128, 128, true);
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
        // update our grid state to match the plugin side
        fParamGrid[index] = (value > 0.5f);

        // trigger repaint
        repaint();
    }

   /**
      A program has been loaded on the plugin side.
      This is called by the host to inform the UI about program changes.
    */
    void programLoaded(uint32_t index) override
    {
        switch (index)
        {
        case 0:
            fParamGrid[0] = false;
            fParamGrid[1] = false;
            fParamGrid[2] = false;
            fParamGrid[3] = false;
            fParamGrid[4] = false;
            fParamGrid[5] = false;
            fParamGrid[6] = false;
            fParamGrid[7] = false;
            fParamGrid[8] = false;
            break;
        case 1:
            fParamGrid[0] = true;
            fParamGrid[1] = true;
            fParamGrid[2] = false;
            fParamGrid[3] = false;
            fParamGrid[4] = true;
            fParamGrid[5] = true;
            fParamGrid[6] = true;
            fParamGrid[7] = false;
            fParamGrid[8] = true;
            break;
        }
        repaint();
    }

   /* --------------------------------------------------------------------------------------------------------
    * Widget Callbacks */

   /**
      The OpenGL drawing function.
      This UI will draw a 3x3 grid, with on/off states according to plugin parameters.
    */
    void onDisplay() override
    {
        const uint width = getWidth();
        const uint height = getHeight();

        Rectangle<int> r;

        r.setWidth(width/3 - 6);
        r.setHeight(height/3 - 6);

        // draw left, center and right columns
        for (int i=0; i<3; ++i)
        {
            r.setX(3 + i*width/3);

            // top
            r.setY(3);

            if (fParamGrid[0+i])
                glColor3f(0.8f, 0.5f, 0.3f);
            else
                glColor3f(0.3f, 0.5f, 0.8f);

            r.draw();

            // middle
            r.setY(3 + height/3);

            if (fParamGrid[3+i])
                glColor3f(0.8f, 0.5f, 0.3f);
            else
                glColor3f(0.3f, 0.5f, 0.8f);

            r.draw();

            // bottom
            r.setY(3 + height*2/3);

            if (fParamGrid[6+i])
                glColor3f(0.8f, 0.5f, 0.3f);
            else
                glColor3f(0.3f, 0.5f, 0.8f);

            r.draw();
        }
    }

   /**
      Mouse press event.
      This UI will de/activate blocks when you click them and reports it as a parameter change to the plugin.
    */
    bool onMouse(const MouseEvent& ev) override
    {
        // Test for left-clicked + pressed first.
        if (ev.button != 1 || ! ev.press)
            return false;

        const uint width = getWidth();
        const uint height = getHeight();

        Rectangle<int> r;

        r.setWidth(width/3 - 6);
        r.setHeight(height/3 - 6);

        // handle left, center and right columns
        for (int i=0; i<3; ++i)
        {
            r.setX(3 + i*width/3);

            // top
            r.setY(3);

            if (r.contains(ev.pos))
            {
                // parameter index that this block applies to
                const uint32_t index = 0+i;

                // invert block state
                fParamGrid[index] = !fParamGrid[index];

                // report change to host (and thus plugin)
                setParameterValue(index, fParamGrid[index] ? 1.0f : 0.0f);

                // trigger repaint
                repaint();
                break;
            }

            // middle
            r.setY(3 + height/3);

            if (r.contains(ev.pos))
            {
                // same as before
                const uint32_t index = 3+i;
                fParamGrid[index] = !fParamGrid[index];
                setParameterValue(index, fParamGrid[index] ? 1.0f : 0.0f);
                repaint();
                break;
            }

            // bottom
            r.setY(3 + height*2/3);

            if (r.contains(ev.pos))
            {
                // same as before
                const uint32_t index = 6+i;
                fParamGrid[index] = !fParamGrid[index];
                setParameterValue(index, fParamGrid[index] ? 1.0f : 0.0f);
                repaint();
                break;
            }
        }

        return true;
    }

    // -------------------------------------------------------------------------------------------------------

private:
   /**
      Our parameters used to display the grid on/off states.
      They match the parameters on the plugin side, but here we define them as booleans.
    */
    bool fParamGrid[9];

   /**
      Set our UI class as non-copyable and add a leak detector just in case.
    */
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExampleUIParameters)
};

/* ------------------------------------------------------------------------------------------------------------
 * UI entry point, called by DPF to create a new UI instance. */

UI* createUI()
{
    return new ExampleUIParameters();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
