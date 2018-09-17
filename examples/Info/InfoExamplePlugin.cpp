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
  Plugin to show how to get some basic information sent to the UI.
 */
class InfoExamplePlugin : public Plugin
{
public:
    InfoExamplePlugin()
        : Plugin(kParameterCount, 0, 0)
    {
        // clear all parameters
        std::memset(fParameters, 0, sizeof(float)*kParameterCount);

        // we can know buffer-size right at the start
        fParameters[kParameterBufferSize] = getBufferSize();
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
        return "Info";
    }

   /**
      Get an extensive comment/description about the plugin.
    */
    const char* getDescription() const override
    {
        return "Plugin to show how to get some basic information sent to the UI.";
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
        return d_cconst('d', 'N', 'f', 'o');
    }

   /* --------------------------------------------------------------------------------------------------------
    * Init */

   /**
      Initialize the parameter @a index.
      This function will be called once, shortly after the plugin is created.
    */
    void initParameter(uint32_t index, Parameter& parameter) override
    {
        parameter.hints      = kParameterIsAutomable|kParameterIsOutput;
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = 0.0f;
        parameter.ranges.max = 16777216.0f;

        switch (index)
        {
        case kParameterBufferSize:
            parameter.name   = "BufferSize";
            parameter.symbol = "buffer_size";
            break;
        case kParameterTimePlaying:
            parameter.hints |= kParameterIsBoolean;
            parameter.name   = "TimePlaying";
            parameter.symbol = "time_playing";
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 1.0f;
            break;
        case kParameterTimeFrame:
            parameter.name   = "TimeFrame";
            parameter.symbol = "time_frame";
            break;
        case kParameterTimeValidBBT:
            parameter.hints |= kParameterIsBoolean;
            parameter.name   = "TimeValidBBT";
            parameter.symbol = "time_validbbt";
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 1.0f;
            break;
        case kParameterTimeBar:
            parameter.name   = "TimeBar";
            parameter.symbol = "time_bar";
            break;
        case kParameterTimeBeat:
            parameter.name   = "TimeBeat";
            parameter.symbol = "time_beat";
            break;
        case kParameterTimeTick:
            parameter.name   = "TimeTick";
            parameter.symbol = "time_tick";
            break;
        case kParameterTimeBarStartTick:
            parameter.name   = "TimeBarStartTick";
            parameter.symbol = "time_barstarttick";
            break;
        case kParameterTimeBeatsPerBar:
            parameter.name   = "TimeBeatsPerBar";
            parameter.symbol = "time_beatsperbar";
            break;
        case kParameterTimeBeatType:
            parameter.name   = "TimeBeatType";
            parameter.symbol = "time_beattype";
            break;
        case kParameterTimeTicksPerBeat:
            parameter.name   = "TimeTicksPerBeat";
            parameter.symbol = "time_ticksperbeat";
            break;
        case kParameterTimeBeatsPerMinute:
            parameter.name   = "TimeBeatsPerMinute";
            parameter.symbol = "time_beatsperminute";
            break;
        }
    }

   /* --------------------------------------------------------------------------------------------------------
    * Internal data */

   /**
      Get the current value of a parameter.
      The host may call this function from any context, including realtime processing.
    */
    float getParameterValue(uint32_t index) const override
    {
        return fParameters[index];

    }

   /**
      Change a parameter value.
      The host may call this function from any context, including realtime processing.
      When a parameter is marked as automable, you must ensure no non-realtime operations are performed.
      @note This function will only be called for parameter inputs.
    */
    void setParameterValue(uint32_t, float) override
    {
        // this is only called for input paramters, which we have none of.
    }

   /* --------------------------------------------------------------------------------------------------------
    * Audio/MIDI Processing */

   /**
      Run/process function for plugins without MIDI input.
      @note Some parameters might be null if there are no audio inputs or outputs.
    */
    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
       /**
          This plugin does nothing, it just demonstrates information usage.
          So here we directly copy inputs over outputs, leaving the audio untouched.
          We need to be careful in case the host re-uses the same buffer for both ins and outs.
        */
        if (outputs[0] != inputs[0])
            std::memcpy(outputs[0], inputs[0], sizeof(float)*frames);

        if (outputs[1] != inputs[1])
            std::memcpy(outputs[1], inputs[1], sizeof(float)*frames);

        // get time position
        const TimePosition& timePos(getTimePosition());

        // set basic values
        fParameters[kParameterTimePlaying]        = timePos.playing ? 1.0f : 0.0f;
        fParameters[kParameterTimeFrame]          = timePos.frame;
        fParameters[kParameterTimeValidBBT]       = timePos.bbt.valid ? 1.0f : 0.0f;

        // set bbt
        if (timePos.bbt.valid)
        {
            fParameters[kParameterTimeBar]            = timePos.bbt.bar;
            fParameters[kParameterTimeBeat]           = timePos.bbt.beat;
            fParameters[kParameterTimeTick]           = timePos.bbt.tick;
            fParameters[kParameterTimeBarStartTick]   = timePos.bbt.barStartTick;
            fParameters[kParameterTimeBeatsPerBar]    = timePos.bbt.beatsPerBar;
            fParameters[kParameterTimeBeatType]       = timePos.bbt.beatType;
            fParameters[kParameterTimeTicksPerBeat]   = timePos.bbt.ticksPerBeat;
            fParameters[kParameterTimeBeatsPerMinute] = timePos.bbt.beatsPerMinute;
        }
        else
        {
            fParameters[kParameterTimeBar]            = 0.0f;
            fParameters[kParameterTimeBeat]           = 0.0f;
            fParameters[kParameterTimeTick]           = 0.0f;
            fParameters[kParameterTimeBarStartTick]   = 0.0f;
            fParameters[kParameterTimeBeatsPerBar]    = 0.0f;
            fParameters[kParameterTimeBeatType]       = 0.0f;
            fParameters[kParameterTimeTicksPerBeat]   = 0.0f;
            fParameters[kParameterTimeBeatsPerMinute] = 0.0f;
        }
    }

   /* --------------------------------------------------------------------------------------------------------
    * Callbacks (optional) */

   /**
      Optional callback to inform the plugin about a buffer size change.@
      This function will only be called when the plugin is deactivated.
      @note This value is only a hint!
            Hosts might call run() with a higher or lower number of frames.
    */
    void bufferSizeChanged(uint32_t newBufferSize) override
    {
        fParameters[kParameterBufferSize] = newBufferSize;
    }

    // -------------------------------------------------------------------------------------------------------

private:
    // Parameters
    float fParameters[kParameterCount];

   /**
      Set our plugin class as non-copyable and add a leak detector just in case.
    */
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InfoExamplePlugin)
};

/* ------------------------------------------------------------------------------------------------------------
 * Plugin entry point, called by DPF to create a new plugin instance. */

Plugin* createPlugin()
{
    return new InfoExamplePlugin();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
