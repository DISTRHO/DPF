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

#ifndef DISTRHO_PLUGIN_HPP_INCLUDED
#define DISTRHO_PLUGIN_HPP_INCLUDED

#include "extra/d_string.hpp"
#include "src/DistrhoPluginChecks.h"

#include <cmath>

#ifdef DISTRHO_PROPER_CPP11_SUPPORT
# include <cstdint>
#else
# include <stdint.h>
#endif

#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

START_NAMESPACE_DISTRHO

/* ------------------------------------------------------------------------------------------------------------
 * Parameter Hints */

/**
   @defgroup ParameterHints Parameter Hints

   Various parameter hints.
   @see Parameter::hints
   @{
 */

/**
   Parameter is automable (real-time safe).
   @see Plugin::d_setParameterValue()
 */
static const uint32_t kParameterIsAutomable = 0x01;

/**
   Parameter value is boolean.
   It's always at either minimum or maximum value.
 */
static const uint32_t kParameterIsBoolean = 0x02;

/**
   Parameter value is integer.
 */
static const uint32_t kParameterIsInteger = 0x04;

/**
   Parameter value is logarithmic.
 */
static const uint32_t kParameterIsLogarithmic = 0x08;

/**
   Parameter is of output type.
   When unset, parameter is assumed to be of input type.

   Parameter inputs are changed by the host and must not be changed by the plugin.
   The only exception being when changing programs, see Plugin::d_setProgram().
   Outputs are changed by the plugin and never modified by the host.
 */
static const uint32_t kParameterIsOutput = 0x10;

/** @} */

/* ------------------------------------------------------------------------------------------------------------
 * DPF Base structs */

/**
   Parameter ranges.
   This is used to set the default, minimum and maximum values of a parameter.

   By default a parameter has 0.0 as minimum, 1.0 as maximum and 0.0 as default.
   When changing this struct values you must ensure maximum > minimum and default is within range.
 */
struct ParameterRanges {
    /**
       Default value.
     */
    float def;

    /**
       Minimum value.
     */
    float min;

    /**
       Maximum value.
     */
    float max;

    /**
       Default constructor.
     */
    ParameterRanges() noexcept
        : def(0.0f),
          min(0.0f),
          max(1.0f) {}

    /**
       Constructor using custom values.
     */
    ParameterRanges(const float df, const float mn, const float mx) noexcept
        : def(df),
          min(mn),
          max(mx) {}

    /**
       Fix the default value within range.
     */
    void fixDefault() noexcept
    {
        fixValue(def);
    }

    /**
       Fix a value within range.
     */
    void fixValue(float& value) const noexcept
    {
        if (value < min)
            value = min;
        else if (value > max)
            value = max;
    }

    /**
       Get a fixed value within range.
     */
    const float& getFixedValue(const float& value) const noexcept
    {
        if (value <= min)
            return min;
        if (value >= max)
            return max;
        return value;
    }

    /**
       Get a value normalized to 0.0<->1.0.
     */
    float getNormalizedValue(const float& value) const noexcept
    {
        const float normValue((value - min) / (max - min));

        if (normValue <= 0.0f)
            return 0.0f;
        if (normValue >= 1.0f)
            return 1.0f;
        return normValue;
    }

    /**
       Get a value normalized to 0.0<->1.0, fixed within range.
     */
    float getFixedAndNormalizedValue(const float& value) const noexcept
    {
        if (value <= min)
            return 0.0f;
        if (value >= max)
            return 1.0f;

        const float normValue((value - min) / (max - min));

        if (normValue <= 0.0f)
            return 0.0f;
        if (normValue >= 1.0f)
            return 1.0f;

        return normValue;
    }

    /**
       Get a proper value previously normalized to 0.0<->1.0.
     */
    float getUnnormalizedValue(const float& value) const noexcept
    {
        if (value <= 0.0f)
            return min;
        if (value >= 1.0f)
            return max;

        return value * (max - min) + min;
    }
};

/**
   Parameter.
 */
struct Parameter {
    /**
       Hints describing this parameter.
       @see ParameterHints
     */
    uint32_t hints;

    /**
       The name of this parameter.
       A parameter name can contain any characters, but hosts might have a hard time with non-ascii ones.
       The name doesn't have to be unique within a plugin instance, but it's recommended.
     */
    d_string name;

    /**
       The symbol of this parameter.
       A parameter symbol is a short restricted name used as a machine and human readable identifier.
       The first character must be one of _, a-z or A-Z and subsequent characters can be from _, a-z, A-Z and 0-9.
       @note: Parameter symbols MUST be unique within a plugin instance.
     */
    d_string symbol;

    /**
       The unit of this parameter.
       This means something like "dB", "kHz" and "ms".
       Can be left blank if units do not apply to this parameter.
     */
    d_string unit;

    /**
       Ranges of this parameter.
       The ranges describe the default, minimum and maximum values.
     */
    ParameterRanges ranges;

    /**
       Default constructor for a null parameter.
     */
    Parameter() noexcept
        : hints(0x0),
          name(),
          symbol(),
          unit(),
          ranges() {}
};

/**
   MIDI event.
 */
struct MidiEvent {
    /**
       Size of internal data.
     */
    static const uint32_t kDataSize = 4;

    /**
       Time offset in frames.
     */
    uint32_t frame;

    /**
       Number of bytes used.
     */
    uint32_t size;

    /**
       MIDI data.
       If size > kDataSize, dataExt is used (otherwise null).
     */
    uint8_t        data[kDataSize];
    const uint8_t* dataExt;
};

/**
   Time position.
   This struct is inspired by the JACK API.
   The @a playing and @a frame values are always valid.
   BBT values are only valid when @a bbt.valid is true.
 */
struct TimePosition {
   /**
      Wherever the host transport is playing/rolling.
    */
    bool playing;

   /**
      Current host transport position in frames.
    */
    uint64_t frame;

   /**
      Bar-Beat-Tick time position.
    */
    struct BarBeatTick {
       /**
          Wherever the host transport is using BBT.
          If false you must not read from this struct.
        */
        bool valid;

       /**
          Current bar.
          Should always be > 0.
          The first bar is bar '1'.
        */
        int32_t bar;

       /**
          Current beat within bar.
          Should always be > 0 and <= @a beatsPerBar.
          The first beat is beat '1'.
        */
        int32_t beat;

       /**
          Current tick within beat.
          Should always be > 0 and <= @a ticksPerBeat.
          The first tick is tick '0'.
        */
        int32_t tick;

       /**
          Number of ticks that have elapsed between frame 0 and the first beat of the current measure.
        */
        double barStartTick;

       /**
          Time signature "numerator".
        */
        float beatsPerBar;

       /**
          Time signature "denominator".
        */
        float beatType;

       /**
          Number of ticks within a bar.
          Usually a moderately large integer with many denominators, such as 1920.0.
        */
        double ticksPerBeat;

       /**
          Number of beats per minute.
        */
        double beatsPerMinute;

       /**
          Default constructor for a null BBT time position.
        */
        BarBeatTick() noexcept
            : valid(false),
              bar(0),
              beat(0),
              tick(0),
              barStartTick(0.0),
              beatsPerBar(0.0f),
              beatType(0.0f),
              ticksPerBeat(0.0),
              beatsPerMinute(0.0) {}
    } bbt;

    /**
       Default constructor for a time position.
     */
    TimePosition() noexcept
        : playing(false),
          frame(0),
          bbt() {}
};

/* ------------------------------------------------------------------------------------------------------------
 * DPF Plugin */

/**
   TODO.
 */
class Plugin
{
public:
    /**
       TODO.
     */
    Plugin(const uint32_t parameterCount, const uint32_t programCount, const uint32_t stateCount);

    /**
       Destructor.
     */
    virtual ~Plugin();

    // -------------------------------------------------------------------
    // Host state

    /**
       TODO.
     */
    uint32_t       d_getBufferSize() const noexcept;

    /**
       TODO.
     */
    double         d_getSampleRate() const noexcept;

    /**
       TODO.
     */
#if DISTRHO_PLUGIN_WANT_TIMEPOS
    /**
       TODO.
     */
    const TimePos& d_getTimePos()    const noexcept;
#endif

#if DISTRHO_PLUGIN_WANT_LATENCY
    /**
       TODO.
     */
    void           d_setLatency(uint32_t frames) noexcept;
#endif

protected:
    // -------------------------------------------------------------------
    // Information

    /**
       TODO.
     */
    virtual const char* d_getName()     const noexcept { return DISTRHO_PLUGIN_NAME; }

    /**
       TODO.
     */
    virtual const char* d_getLabel()    const noexcept = 0;

    /**
       TODO.
     */
    virtual const char* d_getMaker()    const noexcept = 0;

    /**
       TODO.
     */
    virtual const char* d_getLicense()  const noexcept = 0;

    /**
       TODO.
     */
    virtual uint32_t    d_getVersion()  const noexcept = 0;

    /**
       TODO.
     */
    virtual long        d_getUniqueId() const noexcept = 0;

    // -------------------------------------------------------------------
    // Init

    /**
       TODO.
     */
    virtual void d_initParameter(uint32_t index, Parameter& parameter) = 0;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    /**
       TODO.
     */
    virtual void d_initProgramName(uint32_t index, d_string& programName) = 0;
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    /**
       TODO.
     */
    virtual void d_initStateKey(uint32_t index, d_string& stateKey) = 0;
#endif

    // -------------------------------------------------------------------
    // Internal data

    /**
       TODO.
     */
    virtual float d_getParameterValue(uint32_t index) const = 0;

    /**
       TODO.
     */
    virtual void  d_setParameterValue(uint32_t index, float value) = 0;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    /**
       TODO.
     */
    virtual void  d_setProgram(uint32_t index) = 0;
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    /**
       TODO.
     */
    virtual void  d_setState(const char* key, const char* value) = 0;
#endif

    // -------------------------------------------------------------------
    // Process

    /**
       TODO.
     */
    virtual void d_activate() {}

    /**
       TODO.
     */
    virtual void d_deactivate() {}

#if DISTRHO_PLUGIN_IS_SYNTH
    /**
       TODO.
     */
    virtual void d_run(const float** inputs, float** outputs, uint32_t frames, const MidiEvent* midiEvents, uint32_t midiEventCount) = 0;
#else
    /**
       TODO.
     */
    virtual void d_run(const float** inputs, float** outputs, uint32_t frames) = 0;
#endif

    // -------------------------------------------------------------------
    // Callbacks (optional)

    /**
       TODO.
     */
    virtual void d_bufferSizeChanged(uint32_t newBufferSize);

    /**
       TODO.
     */
    virtual void d_sampleRateChanged(double newSampleRate);

    // -------------------------------------------------------------------

private:
    struct PrivateData;
    PrivateData* const pData;
    friend class PluginExporter;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Plugin)
};

/* ------------------------------------------------------------------------------------------------------------
 * Create plugin, entry point */

/**
   TODO.
 */
extern Plugin* createPlugin();

/* ------------------------------------------------------------------------------------------------------------ */

END_NAMESPACE_DISTRHO

#endif // DISTRHO_PLUGIN_HPP_INCLUDED
