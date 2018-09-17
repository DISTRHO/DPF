/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

// -----------------------------------------------------------------------------------------------------------

class ExampleUIParameters : public UI
{
public:
    /**
      For simplicity this UI will be of constant size.
    */
    static const int kUIWidth  = 512;
    static const int kUIHeight = 512;

    /**
      Get key name from an index.
    */
    static const char* getStateKeyFromIndex(const uint32_t index) noexcept
    {
        switch (index)
        {
        case 0: return "top-left";
        case 1: return "top-center";
        case 2: return "top-right";
        case 3: return "middle-left";
        case 4: return "middle-center";
        case 5: return "middle-right";
        case 6: return "bottom-left";
        case 7: return "bottom-center";
        case 8: return "bottom-right";
        }

        return "unknown";
    }

    /* constructor */
    ExampleUIParameters()
        : UI()
    {
       /**
          Initialize the grid to all off per default.
        */
        std::memset(fParamGrid, 0, sizeof(bool)*9);

        setSize(kUIWidth, kUIHeight);
    }

protected:
   /* --------------------------------------------------------------------------------------------------------
    * DSP/Plugin Callbacks */

   /**
      This plugin has no parameters, so we can safely ignore this.
    */
    void parameterChanged(uint32_t, float) override {}

   /**
      A program has been loaded on the plugin side.
      This is called by the host to inform the UI about program changes.
    */
    void programLoaded(uint32_t index) override
    {
        d_stdout("UI programLoaded %i", index);

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

   /**
      A state has changed on the plugin side.
      This is called by the host to inform the UI about state changes.
    */
    void stateChanged(const char* key, const char* value) override
    {
        const bool valueOnOff = (std::strcmp(value, "true") == 0);

        // check which block changed
        /**/ if (std::strcmp(key, "top-left") == 0)
            fParamGrid[0] = valueOnOff;
        else if (std::strcmp(key, "top-center") == 0)
            fParamGrid[1] = valueOnOff;
        else if (std::strcmp(key, "top-right") == 0)
            fParamGrid[2] = valueOnOff;
        else if (std::strcmp(key, "middle-left") == 0)
            fParamGrid[3] = valueOnOff;
        else if (std::strcmp(key, "middle-center") == 0)
            fParamGrid[4] = valueOnOff;
        else if (std::strcmp(key, "middle-right") == 0)
            fParamGrid[5] = valueOnOff;
        else if (std::strcmp(key, "bottom-left") == 0)
            fParamGrid[6] = valueOnOff;
        else if (std::strcmp(key, "bottom-center") == 0)
            fParamGrid[7] = valueOnOff;
        else if (std::strcmp(key, "bottom-right") == 0)
            fParamGrid[8] = valueOnOff;

        // trigger repaint
        repaint();
    }

   /* --------------------------------------------------------------------------------------------------------
    * Widget Callbacks */

   /**
      The OpenGL drawing function.
      This UI will draw a 3x3 grid, with on/off states according to plugin state.
    */
    void onDisplay() override
    {
        Rectangle<int> r;

        r.setWidth(kUIWidth/3 - 6);
        r.setHeight(kUIHeight/3 - 6);

        // draw left, center and right columns
        for (int i=0; i<3; ++i)
        {
            r.setX(3 + i*kUIWidth/3);

            // top
            r.setY(3);

            if (fParamGrid[0+i])
                glColor3f(0.8f, 0.5f, 0.3f);
            else
                glColor3f(0.3f, 0.5f, 0.8f);

            r.draw();

            // middle
            r.setY(3 + kUIHeight/3);

            if (fParamGrid[3+i])
                glColor3f(0.8f, 0.5f, 0.3f);
            else
                glColor3f(0.3f, 0.5f, 0.8f);

            r.draw();

            // bottom
            r.setY(3 + kUIHeight*2/3);

            if (fParamGrid[6+i])
                glColor3f(0.8f, 0.5f, 0.3f);
            else
                glColor3f(0.3f, 0.5f, 0.8f);

            r.draw();
        }
    }

   /**
      Mouse press event.
      This UI will de/activate blocks when you click them and reports it as a state change to the plugin.
    */
    bool onMouse(const MouseEvent& ev) override
    {
        // Test for left-clicked + pressed first.
        if (ev.button != 1 || ! ev.press)
            return false;

        Rectangle<int> r;

        r.setWidth(kUIWidth/3 - 6);
        r.setHeight(kUIHeight/3 - 6);

        // handle left, center and right columns
        for (int i=0; i<3; ++i)
        {
            r.setX(3 + i*kUIWidth/3);

            // top
            r.setY(3);

            if (r.contains(ev.pos))
            {
                // index that this block applies to
                const uint32_t index = 0+i;

                // invert block state
                fParamGrid[index] = !fParamGrid[index];

                // report change to host (and thus plugin)
                setState(getStateKeyFromIndex(index), fParamGrid[index] ? "true" : "false");

                // trigger repaint
                repaint();
                break;
            }

            // middle
            r.setY(3 + kUIHeight/3);

            if (r.contains(ev.pos))
            {
                // same as before
                const uint32_t index = 3+i;
                fParamGrid[index] = !fParamGrid[index];
                setState(getStateKeyFromIndex(index), fParamGrid[index] ? "true" : "false");
                repaint();
                break;
            }

            // bottom
            r.setY(3 + kUIHeight*2/3);

            if (r.contains(ev.pos))
            {
                // same as before
                const uint32_t index = 6+i;
                fParamGrid[index] = !fParamGrid[index];
                setState(getStateKeyFromIndex(index), fParamGrid[index] ? "true" : "false");
                repaint();
                break;
            }
        }

        return true;
    }

    // -------------------------------------------------------------------------------------------------------

private:
   /**
      Our states used to display the grid.
      The host does not know about these.
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
