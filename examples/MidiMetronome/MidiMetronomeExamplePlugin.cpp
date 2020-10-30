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
  Plugin that demonstrates MIDI output in sync with jack transport
 */
class MidiMetronomeExamplePlugin : public Plugin
{
public:
  MidiMetronomeExamplePlugin()
      : Plugin(0, 0, 0) {}

protected:
  /* --------------------------------------------------------------------------------------------------------
    * Information */

  /**
      Get the plugin label.
      This label is a short restricted name consisting of only _, a-z, A-Z and 0-9 characters.
    */
  const char *getLabel() const override
  {
    return "MidiMetronome";
  }

  /**
      Get an extensive comment/description about the plugin.
    */
  const char *getDescription() const override
  {
    return "Plugin that demonstrates MIDI output in sync with jack transport.";
  }

  /**
      Get the plugin author/maker.
    */
  const char *getMaker() const override
  {
    return "DISTRHO";
  }

  /**
      Get the plugin homepage.
    */
  const char *getHomePage() const override
  {
    return "https://github.com/DISTRHO/DPF";
  }

  /**
      Get the plugin license name (a single line of text).
      For commercial plugins this should return some short copyright information.
    */
  const char *getLicense() const override
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
    return d_cconst('d', 'H', 'p', 'V');
  }

  /* --------------------------------------------------------------------------------------------------------
    * Init and Internal data, unused in this plugin */

  void initParameter(uint32_t, Parameter &) override {}
  float getParameterValue(uint32_t) const override { return 0.0f; }
  void setParameterValue(uint32_t, float) override {}

  /**
      Run/process function for plugins with MIDI input.
      In this case we just pass-through all MIDI events.
    */
  void run(const float **, float **, uint32_t nframes,
           const MidiEvent *, uint32_t) override
  {

    const TimePosition &pos(getTimePosition());
    uint32_t frames_left = nframes;

    // if jack transport is not rolling, do nothing
    if (!pos.playing)
    {
      return;
    }

    // get bpm from jack transport
    bpm = pos.bbt.beatsPerMinute;
    wave_length = 60 * sr / bpm;

    offset = pos.frame % wave_length;

    // generate midi event to send
    struct MidiEvent midiEvent;
    midiEvent.size = 3;
    midiEvent.data[0] = 144; /* note on, midi channel 1 */
    midiEvent.data[1] = 36;  /* note  C2*/
    midiEvent.data[2] = 100; /* velocity (volume)*/

    while (wave_length - offset <= frames_left)
    {
      frames_left -= wave_length - offset;
      offset = 0;
      writeMidiEvent(midiEvent);
    }
    if (frames_left > 0)
    {
      offset += frames_left;
    }
  }

  // -------------------------------------------------------------------------------------------------------

private:
  long offset = 0;
  uint32_t tone_length = 0;
  double sr = getSampleRate();
  double bpm;
  uint32_t wave_length;

  /**
      Set our plugin class as non-copyable and add a leak detector just in case.
    */
  DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiMetronomeExamplePlugin)
};

/* ------------------------------------------------------------------------------------------------------------
 * Plugin entry point, called by DPF to create a new plugin instance. */

Plugin *createPlugin()
{
  return new MidiMetronomeExamplePlugin();
}

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
