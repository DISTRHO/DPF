/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2021 Filipe Coelho <falktx@falktx.com>
 * Copyright (C) 2020 Takamitsu Endo
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
  1-pole lowpass filter to smooth out parameters and envelopes.
  This filter is guaranteed not to overshoot.
 */
class Smoother {
    float kp;

public:
    float value;

    Smoother()
        : kp(0.0f),
          value(0.0f) {}

    /**
      Set kp from cutoff frequency in Hz.
      For derivation, see the answer of Matt L. on the url below. Equation 3 is used.

      Computation is done on double for accuracy. When using float, kp will be inaccurate
      if the cutoffHz is below around 3.0 to 4.0 Hz.

      Reference:
      - Single-pole IIR low-pass filter - which is the correct formula for the decay coefficient?
        https://dsp.stackexchange.com/questions/54086/single-pole-iir-low-pass-filter-which-is-the-correct-formula-for-the-decay-coe
     */
    void setCutoff(const float sampleRate, const float cutoffHz)
    {
        double omega_c = 2.0 * M_PI * cutoffHz / sampleRate;
        double y = 1.0 - std::cos(omega_c);
        kp = float(-y + std::sqrt((y + 2.0) * y));
    }

    inline float process(const float input)
    {
        return value += kp * (input - value);
    }
};

// -----------------------------------------------------------------------------------------------------------

/**
  Plugin that demonstrates tempo sync in DPF.
  The tempo sync implementation is on the first if branch in run() method.
 */
class ExamplePluginMetronome : public Plugin
{
public:
    ExamplePluginMetronome()
        : Plugin(4, 0, 0), // 4 parameters, 0 programs, 0 states
          sampleRate(getSampleRate()),
          counter(0),
          wasPlaying(false),
          phase(0.0f),
          envelope(1.0f),
          decay(0.0f),
          gain(0.5f),
          semitone(72),
          cent(0),
          decayTime(0.2f)
    {
        sampleRateChanged(sampleRate);
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
      Initialize the audio port @a index.@n
      This function will be called once, shortly after the plugin is created.
    */
    void initAudioPort(bool input, uint32_t index, AudioPort& port) override
    {
        // treat meter audio ports as stereo
        port.groupId = kPortGroupMono;

        // everything else is as default
        Plugin::initAudioPort(input, index, port);
    }

   /**
      Initialize the parameter @a index.
      This function will be called once, shortly after the plugin is created.
    */
    void initParameter(uint32_t index, Parameter& parameter) override
    {
        parameter.hints = kParameterIsAutomatable;

        switch (index)
        {
        case 0:
            parameter.name = "Gain";
            parameter.hints |= kParameterIsLogarithmic;
            parameter.ranges.min = 0.001f;
            parameter.ranges.max = 1.0f;
            parameter.ranges.def = 0.5f;
            break;
        case 1:
            parameter.name = "DecayTime";
            parameter.hints |= kParameterIsLogarithmic;
            parameter.ranges.min = 0.001f;
            parameter.ranges.max = 1.0f;
            parameter.ranges.def = 0.2f;
            break;
        case 2:
            parameter.name = "Semitone";
            parameter.hints |= kParameterIsInteger;
            parameter.ranges.min = 0;
            parameter.ranges.max = 127;
            parameter.ranges.def = 72;
            break;
        case 3:
            parameter.name = "Cent";
            parameter.hints |= kParameterIsInteger;
            parameter.ranges.min = -100;
            parameter.ranges.max = 100;
            parameter.ranges.def = 0;
            break;
        }

        parameter.symbol = parameter.name;
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
        case 0:
            return gain;
        case 1:
            return decayTime;
        case 2:
            return semitone;
        case 3:
            return cent;
        }

        return 0.0f;
    }

   /**
      Change a parameter value.
    */
    void setParameterValue(uint32_t index, float value) override
    {
        switch (index)
        {
        case 0:
            gain = value;
            break;
        case 1:
            decayTime = value;
            break;
        case 2:
            semitone = value;
            break;
        case 3:
            cent = value;
            break;
        }
    }

   /* --------------------------------------------------------------------------------------------------------
    * Process */

   /**
      Activate this plugin.
      We use this to reset our filter states.
    */
   void activate() override
   {
        deltaPhaseSmoother.value = 0.0f;
        envelopeSmoother.value = 0.0f;
        gainSmoother.value = gain;
   }

   /**
      Run/process function for plugins without MIDI input.
      `inputs` is commented out because this plugin has no inputs.
    */
    void run(const float** /* inputs */, float** outputs, uint32_t frames) override
    {
        const TimePosition& timePos(getTimePosition());
        float* const output = outputs[0];

        if (timePos.playing && timePos.bbt.valid)
        {
            // Better to use double when manipulating time.
            double secondsPerBeat = 60.0 / timePos.bbt.beatsPerMinute;
            double framesPerBeat  = sampleRate * secondsPerBeat;
            double beatFraction   = timePos.bbt.tick / timePos.bbt.ticksPerBeat;

            // If beatFraction is zero, next beat is exactly at the start of currenct cycle.
            // Otherwise, reset counter to the frames to the next beat.
            counter = d_isZero(beatFraction)
                    ? 0
                    : static_cast<uint32_t>(framesPerBeat * (1.0 - beatFraction));

            // Compute deltaPhase in normalized frequency.
            // semitone is midi note number, which is A4 (440Hz at standard tuning) at 69.
            // Frequency goes up to 1 octave higher at the start of bar.
            float frequency = 440.0f * std::pow(2.0f, (100.0f * (semitone - 69.0f) + cent) / 1200.0f);
            float deltaPhase = frequency / sampleRate;
            float octave = timePos.bbt.beat == 1 ? 2.0f : 1.0f;

            // Envelope reaches 1e-5 at decayTime after triggering.
            decay = std::pow(1e-5, 1.0 / (decayTime * sampleRate));

            // Reset phase and frequency at the start of transpose.
            if (!wasPlaying)
            {
                phase = 0.0f;

                deltaPhaseSmoother.value = deltaPhase;
                envelopeSmoother.value = 0.0f;
                gainSmoother.value = 0.0f;
            }

            for (uint32_t i = 0; i < frames; ++i)
            {
                if (counter <= 0)
                {
                    envelope = 1.0f;
                    counter = static_cast<uint32_t>(framesPerBeat + 0.5);
                    octave = (!wasPlaying || timePos.bbt.beat == static_cast<int32_t>(timePos.bbt.beatsPerBar)) ? 2.0f
                                                                                                                : 1.0f;
                }
                --counter;

                envelope *= decay;

                phase += octave * deltaPhaseSmoother.process(deltaPhase);
                phase -= std::floor(phase);

                output[i] = gainSmoother.process(gain)
                          * envelopeSmoother.process(envelope)
                          * std::sin(float(2.0 * M_PI) * phase);
            }
        }
        else
        {
            // Stop metronome if not playing or timePos.bbt is invalid.
            std::memset(output, 0, sizeof(float)*frames);
        }

        wasPlaying = timePos.playing;
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

        // Cutoff value was tuned manually.
        deltaPhaseSmoother.setCutoff(sampleRate, 100.0f);
        gainSmoother.setCutoff(sampleRate, 500.0f);
        envelopeSmoother.setCutoff(sampleRate, 250.0f);
    }

    // -------------------------------------------------------------------------------------------------------

private:
    float sampleRate;
    uint32_t counter; // Stores number of frames to the next beat.
    bool wasPlaying;  // Used to reset phase and frequency at the start of transpose.
    float phase;      // Sine wave phase. Normalized in [0, 1).
    float envelope;   // Current value of gain envelope.
    float decay;      // Coefficient to decay envelope in a frame.

    Smoother deltaPhaseSmoother;
    Smoother envelopeSmoother;
    Smoother gainSmoother;

    // Parameters.
    float gain;
    float semitone;
    float cent;
    float decayTime;

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
