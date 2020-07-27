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
  Plugin that demonstrates tempo sync in DPF.
 */
class ExamplePluginMetronome : public Plugin
{
public:
    ExamplePluginMetronome()
        : Plugin(0, 0, 0), // 0 parameters, 0 programs, 0 states
          sampleRate(44100.0f),
          counter(0) {} 

protected:
   /* --------------------------------------------------------------------------------------------------------
    * Information */

   /**
      Get the plugin label.
      A plugin label follows the same rules as Parameter::symbol, with the exception that it can start with numbers.
    */
    const char* getLabel() const override
    {
        return "Metronome";
    }

   /**
      Get an extensive comment/description about the plugin.
    */
    const char* getDescription() const override
    {
        return "Simple metronome plugin which outputs impulse at the start of every beat.";
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
        return d_cconst('d', 'M', 'e', 't');
    }

   /* --------------------------------------------------------------------------------------------------------
    * Init */

   /**
      Initialize the parameter @a index.
      This function will be called once, shortly after the plugin is created.
    */
    void initParameter(uint32_t /* index */, Parameter& /* parameter */) override
    {
    }
    
   /* --------------------------------------------------------------------------------------------------------
    * Internal data */

   /**
      Get the current value of a parameter.
    */
    float getParameterValue(uint32_t /* index */) const override
    {
        return 0.0f;
    }

   /**
      Change a parameter value.
    */
    void setParameterValue(uint32_t /* index */, float /* value */) override
    {
    }

   /* --------------------------------------------------------------------------------------------------------
    * Process */

   /**
      Run/process function for plugins without MIDI input.
      `inputs` is commented out because this plugin has no inputs.
    */
    void run(const float** /* inputs */, float** outputs, uint32_t frames) override
    {
        const TimePosition& timePos(getTimePosition());

        if (timePos.playing && timePos.bbt.valid) {
            // Better to use double when manipulating time.
            double secondsPerBeat = 60.0 / timePos.bbt.beatsPerMinute;
            double framesPerBeat  = sampleRate * secondsPerBeat;
            double beatFraction   = timePos.bbt.tick / timePos.bbt.ticksPerBeat;
            
            // If beatFraction == 0.0, next beat is exactly at the start of currenct cycle.
            // Otherwise, reset counter to the frames to the next beat.
            counter = beatFraction == 0.0
                ? 0
                : static_cast<uint32_t>(framesPerBeat * (1.0 - beatFraction));

            for (uint32_t i = 0; i < frames; ++i) {
                if (counter <= 0) {
                    outputs[0][i] = 1.0f;
                    counter = uint32_t(framesPerBeat);
                } else {
                    outputs[0][i] = 0.0f;
                }

                --counter;
            }
        } else {
            // Stop metronome if not playing or timePos.bbt is invalid.
            for (uint32_t i = 0; i < frames; ++i) outputs[0][i] = 0.0f;
        }
    }
    
   /* --------------------------------------------------------------------------------------------------------
    * Callbacks (optional) */

   /**
      Optional callback to inform the plugin about a sample rate change.
      This function will only be called when the plugin is deactivated.
    */
    void sampleRateChanged(double newSampleRate) override
    {
        sampleRate = newSampleRate;
    }

    // -------------------------------------------------------------------------------------------------------

private:
    float sampleRate;
    uint32_t counter; // Stores number of frames to the next beat.

   /**
      Set our plugin class as non-copyable and add a leak detector just in case.
    */
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExamplePluginMetronome)
};

/* ------------------------------------------------------------------------------------------------------------
 * Plugin entry point, called by DPF to create a new plugin instance. */

Plugin* createPlugin()
{
    return new ExamplePluginMetronome();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
