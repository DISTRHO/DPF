/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2021 Filipe Coelho <falktx@falktx.com>
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

#include <cmath>
#include <cstring>

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------------------------------------------

/**
  Plugin that demonstrates sending notes from the editor in DPF.
 */
class SendNoteExamplePlugin : public Plugin
{
public:
    SendNoteExamplePlugin()
        : Plugin(0, 0, 0)
    {
        std::memset(fNotesPlayed, 0, sizeof(fNotesPlayed));
        std::memset(fOscillatorPhases, 0, sizeof(fOscillatorPhases));
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
        return "SendNote";
    }

   /**
      Get an extensive comment/description about the plugin.
    */
    const char* getDescription() const override
    {
        return "Plugin that demonstrates sending notes from the editor in DPF.";
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
        return d_cconst('d', 'S', 'N', 'o');
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

   /* --------------------------------------------------------------------------------------------------------
    * Audio/MIDI Processing */

   /**
      Run/process function for plugins with MIDI input.
      This synthesizes the MIDI voices with a sum of sine waves.
    */
    void run(const float**, float** outputs, uint32_t frames,
             const MidiEvent* midiEvents, uint32_t midiEventCount) override
    {
        for (uint32_t i = 0; i < midiEventCount; ++i)
        {
            if (midiEvents[i].size <= 3)
            {
                uint8_t status = midiEvents[i].data[0];
                uint8_t note = midiEvents[i].data[1] & 127;
                uint8_t velocity = midiEvents[i].data[2] & 127;

                switch (status & 0xf0)
                {
                case 0x90:
                    if (velocity != 0)
                    {
                        fNotesPlayed[note] = velocity;
                        break;
                    }
                    /* fall through */
                case 0x80:
                    fNotesPlayed[note] = 0;
                    fOscillatorPhases[note] = 0;
                    break;
                }
            }
        }

        float* const output = outputs[0];
        std::memset(output, 0, frames * sizeof(float));

        for (uint32_t noteNumber = 0; noteNumber < 128; ++noteNumber)
        {
            if (fNotesPlayed[noteNumber] == 0)
                continue;

            float notePitch = 8.17579891564 * std::exp(0.0577622650 * noteNumber);

            float phase = fOscillatorPhases[noteNumber];
            float timeStep = notePitch / getSampleRate();
            float k2pi = 2.0 * M_PI;
            float gain = 0.1;

            for (uint32_t i = 0; i < frames; ++i)
            {
                output[i] += gain * std::sin(k2pi * phase);
                phase += timeStep;
                phase -= (int)phase;
            }

            fOscillatorPhases[noteNumber] = phase;
        }
    }

    // -------------------------------------------------------------------------------------------------------

private:
    uint8_t fNotesPlayed[128];
    float fOscillatorPhases[128];

   /**
      Set our plugin class as non-copyable and add a leak detector just in case.
    */
    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SendNoteExamplePlugin)
};

/* ------------------------------------------------------------------------------------------------------------
 * Plugin entry point, called by DPF to create a new plugin instance. */

Plugin* createPlugin()
{
    return new SendNoteExamplePlugin();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
