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
  Simple plugin to demonstrate parameter usage (including UI).
  The plugin will be treated as an effect, but it will not change the host audio.
 */
class ExamplePluginParameters : public Plugin
{
public:
    ExamplePluginParameters()
        : Plugin(0, 0, 0) {} // 0 parameters, 0 programs, 0 states

protected:
   /* --------------------------------------------------------------------------------------------------------
    * Information */

   /**
      Get the plugin label.
      A plugin label follows the same rules as Parameter::symbol, with the exception that it can start with numbers.
    */
    const char* getLabel() const override
    {
        return "CVPort";
    }

   /**
      Get an extensive comment/description about the plugin.
    */
    const char* getDescription() const override
    {
        return "Simple plugin with CVPort.\n\
The plugin will be treated as an effect, signal will bypassed.";
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
        return "https://github.com/DISTRHO/DPF";
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
        return d_cconst('d', 'C', 'V', 'P');
    }

   /* --------------------------------------------------------------------------------------------------------
    * Init */

   /**
      Initialize the audio port @a index.@n
      This function will be called once, shortly after the plugin is created.
    */
    void initAudioPort(bool input, uint32_t index, AudioPort& port) override
    {
       /**
          Note that index is independent for input and output.
          In other words, input port index starts from 0 and output port index also starts from 0.
        */
        if (input)
        {
            if (index == 0) {
                port.hints   = kAudioPortIsCV;
                port.name    = "CV Input";
                port.symbol  = "cv_in";
                return;
            }
            // Add more condition here when increasing DISTRHO_PLUGIN_NUM_INPUTS.
        }
        else
        {
            if (index == 0) {
                port.hints   = kAudioPortIsCV;
                port.name    = "CV Output";
                port.symbol  = "cv_out";
                return;
            }
            // Add more condition here when increasing DISTRHO_PLUGIN_NUM_OUTPUTS.
        }
        
       /**
          It shouldn't reach here, but just in case if index is greater than 0.
        */
        Plugin::initAudioPort(input, index, port);
    }

   /* --------------------------------------------------------------------------------------------------------
    * Init and Internal data, unused in this plugin */

    void  initParameter(uint32_t, Parameter&) override {}
    float getParameterValue(uint32_t) const   override { return 0.0f;}
    void  setParameterValue(uint32_t, float)  override {}

   /* --------------------------------------------------------------------------------------------------------
    * Process */

   /**
      Run/process function for plugins without MIDI input.
    */
    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
       /**
          This plugin does nothing, it just demonstrates CVPort usage.
          So here we directly copy inputs over outputs, leaving the audio untouched.
          We need to be careful in case the host re-uses the same buffer for both ins and outs.
        */
        if (outputs[0] != inputs[0])
            std::memcpy(outputs[0], inputs[0], sizeof(float)*frames);
    }

    // -------------------------------------------------------------------------------------------------------

private:
   /**
      Set our plugin class as non-copyable and add a leak detector just in case.
    */
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExamplePluginParameters)
};

/* ------------------------------------------------------------------------------------------------------------
 * Plugin entry point, called by DPF to create a new plugin instance. */

Plugin* createPlugin()
{
    return new ExamplePluginParameters();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
