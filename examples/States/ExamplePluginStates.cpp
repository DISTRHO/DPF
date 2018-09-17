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

#include "DistrhoPlugin.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

/**
  Simple plugin to demonstrate state usage (including UI).
  The plugin will be treated as an effect, but it will not change the host audio.
 */
class ExamplePluginStates : public Plugin
{
public:
    ExamplePluginStates()
        : Plugin(0, 2, 9) // 0 parameters, 2 programs, 9 states
    {
       /**
          Initialize all our parameters to their defaults.
          In this example all default values are false, so we can simply zero them.
        */
        std::memset(fParamGrid, 0, sizeof(bool)*9);
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
        return "states";
    }

   /**
      Get an extensive comment/description about the plugin.
    */
    const char* getDescription() const override
    {
        return "Simple plugin to demonstrate state usage (including UI).\n\
The plugin will be treated as an effect, but it will not change the host audio.";
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
        return d_cconst('d', 'S', 't', 's');
    }

   /* --------------------------------------------------------------------------------------------------------
    * Init */

   /**
      This plugin has no parameters..
    */
    void  initParameter(uint32_t, Parameter&) override {}

   /**
      Set the name of the program @a index.
      This function will be called once, shortly after the plugin is created.
    */
    void initProgramName(uint32_t index, String& programName) override
    {
        switch (index)
        {
        case 0:
            programName = "Default";
            break;
        case 1:
            programName = "Custom";
            break;
        }
    }

   /**
      Set the state key and default value of @a index.
      This function will be called once, shortly after the plugin is created.
    */
    void initState(uint32_t index, String& stateKey, String& defaultStateValue) override
    {
        switch (index)
        {
        case 0:
            stateKey = "top-left";
            break;
        case 1:
            stateKey = "top-center";
            break;
        case 2:
            stateKey = "top-right";
            break;
        case 3:
            stateKey = "middle-left";
            break;
        case 4:
            stateKey = "middle-center";
            break;
        case 5:
            stateKey = "middle-right";
            break;
        case 6:
            stateKey = "bottom-left";
            break;
        case 7:
            stateKey = "bottom-center";
            break;
        case 8:
            stateKey = "bottom-right";
            break;
        }

        defaultStateValue = "false";
    }

   /* --------------------------------------------------------------------------------------------------------
    * Internal data */

   /**
      This plugin has no parameters..
    */
    void  setParameterValue(uint32_t, float) override {}
    float getParameterValue(uint32_t) const  override { return 0.0f; }

   /**
      Load a program.
      The host may call this function from any context, including realtime processing.
    */
    void loadProgram(uint32_t index) override
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
    }

   /**
      Get the value of an internal state.
      The host may call this function from any non-realtime context.
    */
    String getState(const char* key) const override
    {
        static const String sTrue ("true");
        static const String sFalse("false");

        // check which block changed
        /**/ if (std::strcmp(key, "top-left") == 0)
            return fParamGrid[0] ? sTrue : sFalse;
        else if (std::strcmp(key, "top-center") == 0)
            return fParamGrid[1] ? sTrue : sFalse;
        else if (std::strcmp(key, "top-right") == 0)
            return fParamGrid[2] ? sTrue : sFalse;
        else if (std::strcmp(key, "middle-left") == 0)
            return fParamGrid[3] ? sTrue : sFalse;
        else if (std::strcmp(key, "middle-center") == 0)
            return fParamGrid[4] ? sTrue : sFalse;
        else if (std::strcmp(key, "middle-right") == 0)
            return fParamGrid[5] ? sTrue : sFalse;
        else if (std::strcmp(key, "bottom-left") == 0)
            return fParamGrid[6] ? sTrue : sFalse;
        else if (std::strcmp(key, "bottom-center") == 0)
            return fParamGrid[7] ? sTrue : sFalse;
        else if (std::strcmp(key, "bottom-right") == 0)
            return fParamGrid[8] ? sTrue : sFalse;

        return sFalse;
    }

   /**
      Change an internal state.
    */
    void setState(const char* key, const char* value) override
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
    }

   /* --------------------------------------------------------------------------------------------------------
    * Process */

   /**
      Run/process function for plugins without MIDI input.
    */
    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
       /**
          This plugin does nothing, it just demonstrates parameter usage.
          So here we directly copy inputs over outputs, leaving the audio untouched.
          We need to be careful in case the host re-uses the same buffer for both ins and outs.
        */
        if (outputs[0] != inputs[0])
            std::memcpy(outputs[0], inputs[0], sizeof(float)*frames);

        if (outputs[1] != inputs[1])
            std::memcpy(outputs[1], inputs[1], sizeof(float)*frames);
    }

    // -------------------------------------------------------------------------------------------------------

private:
   /**
      Our parameters used to display the grid on/off states.
    */
    bool fParamGrid[9];


   /**
      Set our plugin class as non-copyable and add a leak detector just in case.
    */
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExamplePluginStates)
};

/* ------------------------------------------------------------------------------------------------------------
 * Plugin entry point, called by DPF to create a new plugin instance. */

Plugin* createPlugin()
{
    return new ExamplePluginStates();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
