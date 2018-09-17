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

#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

/**
  Plugin to demonstrate parameter outputs using meters.
 */
class ExamplePluginMeters : public Plugin
{
public:
    ExamplePluginMeters()
        : Plugin(3, 0, 0), // 3 parameters, 0 programs, 0 states
          fColor(0.0f),
          fOutLeft(0.0f),
          fOutRight(0.0f),
          fNeedsReset(true)
    {
    }

protected:
   /* --------------------------------------------------------------------------------------------------------
    * Information */

   /**
      Get the plugin label.
      A plugin label follows the same rules as Parameter::symbol, with the exception that it can start with numbers.
    */
    const char* getLabel() const override
    {
        return "meters";
    }

   /**
      Get an extensive comment/description about the plugin.
    */
    const char* getDescription() const override
    {
        return "Plugin to demonstrate parameter outputs using meters.";
    }

   /**
      Get the plugin author/maker.
    */
    const char* getMaker() const override
    {
        return "DISTRHO";
    }

   /**
      Get the plugin homepage.
    */
    const char* getHomePage() const override
    {
        return "https://github.com/DISTRHO/plugin-examples";
    }

   /**
      Get the plugin license name (a single line of text).
      For commercial plugins this should return some short copyright information.
    */
    const char* getLicense() const override
    {
        return "ISC";
    }

   /**
      Get the plugin version, in hexadecimal.
    */
    uint32_t getVersion() const override
    {
        return d_version(1, 0, 0);
    }

   /**
      Get the plugin unique Id.
      This value is used by LADSPA, DSSI and VST plugin formats.
    */
    int64_t getUniqueId() const override
    {
        return d_cconst('d', 'M', 't', 'r');
    }

   /* --------------------------------------------------------------------------------------------------------
    * Init */

   /**
      Initialize the parameter @a index.
      This function will be called once, shortly after the plugin is created.
    */
    void initParameter(uint32_t index, Parameter& parameter) override
    {
       /**
          All parameters in this plugin have the same ranges.
        */
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 1.0f;
        parameter.ranges.def = 0.0f;

       /**
          Set parameter data.
        */
        switch (index)
        {
        case 0:
            parameter.hints  = kParameterIsAutomable|kParameterIsInteger;
            parameter.name   = "color";
            parameter.symbol = "color";
            parameter.enumValues.count = 2;
            parameter.enumValues.restrictedMode = true;
            {
                ParameterEnumerationValue* const values = new ParameterEnumerationValue[2];
                parameter.enumValues.values = values;

                values[0].label = "Green";
                values[0].value = METER_COLOR_GREEN;
                values[1].label = "Blue";
                values[1].value = METER_COLOR_BLUE;
            }
            break;
        case 1:
            parameter.hints  = kParameterIsAutomable|kParameterIsOutput;
            parameter.name   = "out-left";
            parameter.symbol = "out_left";
            break;
        case 2:
            parameter.hints  = kParameterIsAutomable|kParameterIsOutput;
            parameter.name   = "out-right";
            parameter.symbol = "out_right";
            break;
        }
    }

   /**
      Set a state key and default value.
      This function will be called once, shortly after the plugin is created.
    */
    void initState(uint32_t, String&, String&) override
    {
        // we are using states but don't want them saved in the host
    }

   /* --------------------------------------------------------------------------------------------------------
    * Internal data */

   /**
      Get the current value of a parameter.
    */
    float getParameterValue(uint32_t index) const override
    {
        switch (index)
        {
        case 0: return fColor;
        case 1: return fOutLeft;
        case 2: return fOutRight;
        }

        return 0.0f;
    }

   /**
      Change a parameter value.
    */
    void setParameterValue(uint32_t index, float value) override
    {
        // this is only called for input paramters, and we only have one of those.
        if (index != 0) return;

        fColor = value;
    }

   /**
      Change an internal state.
    */
    void setState(const char* key, const char*) override
    {
        if (std::strcmp(key, "reset") != 0)
            return;

        fNeedsReset = true;
    }

   /* --------------------------------------------------------------------------------------------------------
    * Process */

   /**
      Run/process function for plugins without MIDI input.
    */
    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        float tmp;
        float tmpLeft  = 0.0f;
        float tmpRight = 0.0f;

        for (uint32_t i=0; i<frames; ++i)
        {
            // left
            tmp = std::abs(inputs[0][i]);

            if (tmp > tmpLeft)
                tmpLeft = tmp;

            // right
            tmp = std::abs(inputs[1][i]);

            if (tmp > tmpRight)
                tmpRight = tmp;
        }

        if (tmpLeft > 1.0f)
            tmpLeft = 1.0f;
        if (tmpRight > 1.0f)
            tmpRight = 1.0f;

        if (fNeedsReset)
        {
            fOutLeft  = tmpLeft;
            fOutRight = tmpRight;
            fNeedsReset = false;
        }
        else
        {
            if (tmpLeft > fOutLeft)
                fOutLeft = tmpLeft;
            if (tmpRight > fOutRight)
                fOutRight = tmpRight;
        }

        // copy inputs over outputs if needed
        if (outputs[0] != inputs[0])
            std::memcpy(outputs[0], inputs[0], sizeof(float)*frames);

        if (outputs[1] != inputs[1])
            std::memcpy(outputs[1], inputs[1], sizeof(float)*frames);
    }

    // -------------------------------------------------------------------------------------------------------

private:
   /**
      Parameters.
    */
    float fColor, fOutLeft, fOutRight;

   /**
      Boolean used to reset meter values.
      The UI will send a "reset" message which sets this as true.
    */
    volatile bool fNeedsReset;

   /**
      Set our plugin class as non-copyable and add a leak detector just in case.
    */
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExamplePluginMeters)
};

/* ------------------------------------------------------------------------------------------------------------
 * Plugin entry point, called by DPF to create a new plugin instance. */

Plugin* createPlugin()
{
    return new ExamplePluginMeters();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
