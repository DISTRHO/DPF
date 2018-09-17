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
  Plugin that demonstrates the latency API in DPF.
 */
class LatencyExamplePlugin : public Plugin
{
public:
    LatencyExamplePlugin()
        : Plugin(1, 0, 0), // 1 parameter
          fLatency(1.0f),
          fLatencyInFrames(0),
          fBuffer(nullptr),
          fBufferPos(0)
    {
        // allocates buffer
        sampleRateChanged(getSampleRate());
    }

    ~LatencyExamplePlugin() override
    {
        delete[] fBuffer;
    }

protected:
   /* --------------------------------------------------------------------------------------------------------
    * Information */

   /**
      Get the plugin label.
      This label is a short restricted name consisting of only _, a-z, A-Z and 0-9 characters.
    */
    const char* getLabel() const override
    {
        return "Latency";
    }

   /**
      Get an extensive comment/description about the plugin.
    */
    const char* getDescription() const override
    {
        return "Plugin that demonstrates the latency API in DPF.";
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
        return d_cconst('d', 'L', 'a', 't');
    }

   /* --------------------------------------------------------------------------------------------------------
    * Init */

   /**
      Initialize the parameter @a index.
      This function will be called once, shortly after the plugin is created.
    */
    void initParameter(uint32_t index, Parameter& parameter) override
    {
        if (index != 0)
            return;

        parameter.hints  = kParameterIsAutomable;
        parameter.name   = "Latency";
        parameter.symbol = "latency";
        parameter.unit   = "s";
        parameter.ranges.def = 1.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 5.0f;
    }

   /* --------------------------------------------------------------------------------------------------------
    * Internal data */

   /**
      Get the current value of a parameter.
      The host may call this function from any context, including realtime processing.
    */
    float getParameterValue(uint32_t index) const override
    {
        if (index != 0)
            return 0.0f;

        return fLatency;
    }

   /**
      Change a parameter value.
      The host may call this function from any context, including realtime processing.
      When a parameter is marked as automable, you must ensure no non-realtime operations are performed.
      @note This function will only be called for parameter inputs.
    */
    void setParameterValue(uint32_t index, float value) override
    {
        if (index != 0)
            return;

        fLatency = value;
        fLatencyInFrames = value*getSampleRate();

        setLatency(fLatencyInFrames);
    }

   /* --------------------------------------------------------------------------------------------------------
    * Audio/MIDI Processing */

   /**
      Run/process function for plugins without MIDI input.
      @note Some parameters might be null if there are no audio inputs or outputs.
    */
    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        const float* const in  = inputs[0];
        /* */ float* const out = outputs[0];

        if (fLatencyInFrames == 0)
        {
            if (out != in)
                std::memcpy(out, in, sizeof(float)*frames);
            return;
        }

        // Put the new audio in the buffer.
        std::memcpy(fBuffer+fBufferPos, in, sizeof(float)*frames);
        fBufferPos += frames;

        // buffer is not filled enough yet
        if (fBufferPos < fLatencyInFrames+frames)
        {
            // silence output
            std::memset(out, 0, sizeof(float)*frames);
        }
        // buffer is ready to copy
        else
        {
            // copy latency buffer to output
            const uint32_t readPos = fBufferPos-fLatencyInFrames-frames;
            std::memcpy(out, fBuffer+readPos, sizeof(float)*frames);

            // move latency buffer back by some frames
            std::memmove(fBuffer, fBuffer+frames, sizeof(float)*fBufferPos);
            fBufferPos -= frames;
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
        if (fBuffer != nullptr)
            delete[] fBuffer;

        const uint32_t maxFrames = newSampleRate*6; // 6 seconds

        fBuffer = new float[maxFrames];
        std::memset(fBuffer, 0, sizeof(float)*maxFrames);

        fLatencyInFrames = fLatency*newSampleRate;
        fBufferPos       = 0;
    }

    // -------------------------------------------------------------------------------------------------------

private:
    // Parameters
    float fLatency;
    uint32_t fLatencyInFrames;

    // Buffer for previous audio, size depends on sample rate
    float* fBuffer;
    uint32_t fBufferPos;

   /**
      Set our plugin class as non-copyable and add a leak detector just in case.
    */
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LatencyExamplePlugin)
};

/* ------------------------------------------------------------------------------------------------------------
 * Plugin entry point, called by DPF to create a new plugin instance. */

Plugin* createPlugin()
{
    return new LatencyExamplePlugin();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
