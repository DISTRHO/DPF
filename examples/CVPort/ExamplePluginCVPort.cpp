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
  Simple plugin to demonstrate how to modify input/output port type in DPF.
  The plugin outputs sample & hold (S&H) value of input signal.
  User can specify hold time via parameter and/or Hold Time CV port.
 */
class ExamplePluginCVPort : public Plugin
{
public:
    ExamplePluginCVPort()
        : Plugin(1, 0, 0) {} // 1 parameters, 0 programs, 0 states

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
The plugin does sample & hold processing.";
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
                // Audio port doesn't need to specify port.hints.
                port.name    = "Audio Input";
                port.symbol  = "audio_in";
                return;
            }
            else if (index == 1) {
                port.hints   = kAudioPortIsCV;
                port.name    = "Hold Time";
                port.symbol  = "hold_time";
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
    
   /**
      Initialize the parameter @a index.
      This function will be called once, shortly after the plugin is created.
    */
    void initParameter(uint32_t index, Parameter& parameter) override
    {
        if (index != 0) return;

        parameter.name       = "Hold Time";
        parameter.symbol     = "hold_time";
        parameter.hints      = kParameterIsAutomable|kParameterIsLogarithmic;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = maxHoldTime;
        parameter.ranges.def = 0.1f;
    }

   /* --------------------------------------------------------------------------------------------------------
    * Internal data */

   /**
      Get the current value of a parameter.
    */
    inline float getParameterValue(uint32_t index) const override
    {
        return index == 0 ? holdTime : 0.0f;
    }

   /**
      Change a parameter value.
    */
    void setParameterValue(uint32_t index, float value) override
    {
        if (index != 0) return;

        holdTime = value;
        counter = uint32_t(holdTime * sampleRate);
    }

   /* --------------------------------------------------------------------------------------------------------
    * Process */

   /**
      Run/process function for plugins without MIDI input.
    */
    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
       /**
        - inputs[0] is input audio port.
        - inputs[1] is hold time CV port.
        - outputs[0] is output CV port.
        */
        for (uint32_t i = 0; i < frames; ++i) {
            if (counter == 0) {
                float cv = inputs[1][i] > 0.0f ? inputs[1][i] : 0.0f;

                float time = holdTime + cv;
                if (time > maxHoldTime)
                    time = maxHoldTime;

                counter = uint32_t(time * sampleRate);

                holdValue = inputs[0][i]; // Refresh hold value.
            } else {
                --counter;
            }
            outputs[0][i] = holdValue;
        }
    }

    // -------------------------------------------------------------------------------------------------------

private:
    const float maxHoldTime = 1.0f;

    float sampleRate = 44100.0f;
    uint32_t counter = 0;  // Hold time in samples. Used to count hold time.
    float holdTime = 0.0f; // Hold time in seconds.
    float holdValue = 0.0f;

   /**
      Set our plugin class as non-copyable and add a leak detector just in case.
    */
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExamplePluginCVPort)
};

/* ------------------------------------------------------------------------------------------------------------
 * Plugin entry point, called by DPF to create a new plugin instance. */

Plugin* createPlugin()
{
    return new ExamplePluginCVPort();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
