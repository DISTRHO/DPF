/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2022 Filipe Coelho <falktx@falktx.com>
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

#include "DistrhoPluginInfo.h"
#include "DistrhoPluginInternal.hpp"
#include "extra/ScopedPointer.hpp"

#include "clap/entry.h"
#include "clap/plugin-factory.h"
#include "clap/ext/audio-ports.h"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

#if ! DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
static constexpr const writeMidiFunc writeMidiCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
static constexpr const requestParameterValueChangeFunc requestParameterValueChangeCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_STATE
static const updateStateValueFunc updateStateValueCallback = nullptr;
#endif

// --------------------------------------------------------------------------------------------------------------------

/**
 * CLAP plugin class.
 */
class PluginCLAP
{
public:
    PluginCLAP(const clap_host_t* const host)
        : fPlugin(this,
                  writeMidiCallback,
                  requestParameterValueChangeCallback,
                  updateStateValueCallback),
          fHost(host),
          fOutputEvents(nullptr)
    {
    }

    bool init()
    {
        if (!clap_version_is_compatible(fHost->clap_version))
            return false;

        // TODO check host features
        return true;
    }

    void activate(const double sampleRate, const uint32_t maxFramesCount)
    {
        fPlugin.setSampleRate(sampleRate, true);
        fPlugin.setBufferSize(maxFramesCount, true);
        fPlugin.activate();
    }

    void deactivate()
    {
        fPlugin.deactivate();
    }

    bool process(const clap_process_t* const process)
    {
       #if DISTRHO_PLUGIN_WANT_TIMEPOS
        if (const clap_event_transport_t* const transport = process->transport)
        {
            fTimePosition.playing = transport->flags & CLAP_TRANSPORT_IS_PLAYING;

            fTimePosition.frame = process->steady_time >= 0 ? process->steady_time : 0;

            if (transport->flags & CLAP_TRANSPORT_HAS_TEMPO)
                fTimePosition.bbt.beatsPerMinute = transport->tempo;
            else
                fTimePosition.bbt.beatsPerMinute = 120.0;

            // ticksPerBeat is not possible with CLAP
            fTimePosition.bbt.ticksPerBeat = 1920.0;

            // TODO verify if this works or makes any sense
            if ((transport->flags & (CLAP_TRANSPORT_HAS_BEATS_TIMELINE|CLAP_TRANSPORT_HAS_TIME_SIGNATURE)) == (CLAP_TRANSPORT_HAS_BEATS_TIMELINE|CLAP_TRANSPORT_HAS_TIME_SIGNATURE))
            {
                const double ppqPos    = std::abs(transport->song_pos_beats);
                const int    ppqPerBar = transport->tsig_num * 4 / transport->tsig_denom;
                const double barBeats  = (std::fmod(ppqPos, ppqPerBar) / ppqPerBar) * transport->tsig_num;
                const double rest      =  std::fmod(barBeats, 1.0);

                fTimePosition.bbt.valid       = true;
                fTimePosition.bbt.bar         = static_cast<int32_t>(ppqPos) / ppqPerBar + 1;
                fTimePosition.bbt.beat        = static_cast<int32_t>(barBeats - rest + 0.5) + 1;
                fTimePosition.bbt.tick        = rest * fTimePosition.bbt.ticksPerBeat;
                fTimePosition.bbt.beatsPerBar = transport->tsig_num;
                fTimePosition.bbt.beatType    = transport->tsig_denom;

                if (transport->song_pos_beats < 0.0)
                {
                    --fTimePosition.bbt.bar;
                    fTimePosition.bbt.beat = transport->tsig_num - fTimePosition.bbt.beat + 1;
                    fTimePosition.bbt.tick = fTimePosition.bbt.ticksPerBeat - fTimePosition.bbt.tick - 1;
                }
            }
            else
            {
                fTimePosition.bbt.valid       = false;
                fTimePosition.bbt.bar         = 1;
                fTimePosition.bbt.beat        = 1;
                fTimePosition.bbt.tick        = 0.0;
                fTimePosition.bbt.beatsPerBar = 4.0f;
                fTimePosition.bbt.beatType    = 4.0f;
            }

            fTimePosition.bbt.barStartTick = fTimePosition.bbt.ticksPerBeat*
                                             fTimePosition.bbt.beatsPerBar*
                                             (fTimePosition.bbt.bar-1);
        }
        else
        {
            fTimePosition.playing = false;
            fTimePosition.frame = 0;
            fTimePosition.bbt.valid          = false;
            fTimePosition.bbt.beatsPerMinute = 120.0;
            fTimePosition.bbt.bar            = 1;
            fTimePosition.bbt.beat           = 1;
            fTimePosition.bbt.tick           = 0.0;
            fTimePosition.bbt.beatsPerBar    = 4.f;
            fTimePosition.bbt.beatType       = 4.f;
            fTimePosition.bbt.barStartTick   = 0;
        }

        fPlugin.setTimePosition(fTimePosition);
       #endif

        if (const clap_input_events_t* const inputEvents = process->in_events)
        {
            if (const uint32_t len = inputEvents->size(inputEvents))
            {
                for (uint32_t i=0; i<len; ++i)
                {
                    const clap_event_header_t* const event = inputEvents->get(inputEvents, i);

                    // event->time
                    switch (event->type)
                    {
                    case CLAP_EVENT_NOTE_ON:
                    case CLAP_EVENT_NOTE_OFF:
                    case CLAP_EVENT_NOTE_CHOKE:
                    case CLAP_EVENT_NOTE_END:
                    case CLAP_EVENT_NOTE_EXPRESSION:
                    case CLAP_EVENT_PARAM_VALUE:
                    case CLAP_EVENT_PARAM_MOD:
                    case CLAP_EVENT_PARAM_GESTURE_BEGIN:
                    case CLAP_EVENT_PARAM_GESTURE_END:
                    case CLAP_EVENT_TRANSPORT:
                    case CLAP_EVENT_MIDI:
                    case CLAP_EVENT_MIDI_SYSEX:
                    case CLAP_EVENT_MIDI2:
                        break;
                    }
                }
            }
        }

        if (const uint32_t frames = process->frames_count)
        {
            // TODO multi-port bus stuff
            DISTRHO_SAFE_ASSERT_UINT_RETURN(process->audio_inputs_count == 0 || process->audio_inputs_count == 1,
                                            process->audio_inputs_count, false);
            DISTRHO_SAFE_ASSERT_UINT_RETURN(process->audio_outputs_count == 0 || process->audio_outputs_count == 1,
                                            process->audio_outputs_count, false);

            const float** inputs = process->audio_inputs != nullptr
                                 ? const_cast<const float**>(process->audio_inputs[0].data32)
                                 : nullptr;
            /**/ float** outputs = process->audio_outputs != nullptr
                                 ? process->audio_outputs[0].data32
                                 : nullptr;

            fOutputEvents = process->out_events;

           #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
            fPlugin.run(inputs, outputs, frames, fMidiEvents, midiEventCount);
           #else
            fPlugin.run(inputs, outputs, frames);
           #endif

            fOutputEvents = nullptr;
        }

        return true;
    }

    // ----------------------------------------------------------------------------------------------------------------

private:
    // Plugin
    PluginExporter fPlugin;

    // CLAP stuff
    const clap_host_t* const fHost;
    const clap_output_events_t* fOutputEvents;
   #if DISTRHO_PLUGIN_WANT_TIMEPOS
    TimePosition fTimePosition;
   #endif

    // ----------------------------------------------------------------------------------------------------------------
    // DPF callbacks

   #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    bool writeMidi(const MidiEvent&)
    {
        return true;
    }

    static bool writeMidiCallback(void* const ptr, const MidiEvent& midiEvent)
    {
        return ((PluginStub*)ptr)->writeMidi(midiEvent);
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
    bool requestParameterValueChange(uint32_t, float)
    {
        return true;
    }

    static bool requestParameterValueChangeCallback(void* const ptr, const uint32_t index, const float value)
    {
        return ((PluginStub*)ptr)->requestParameterValueChange(index, value);
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_STATE
    bool updateState(const char*, const char*)
    {
        return true;
    }

    static bool updateStateValueCallback(void* const ptr, const char* const key, const char* const value)
    {
        return ((PluginStub*)ptr)->updateState(key, value);
    }
   #endif
};

// --------------------------------------------------------------------------------------------------------------------

static ScopedPointer<PluginExporter> sPlugin;

// --------------------------------------------------------------------------------------------------------------------
// plugin audio ports

static uint32_t clap_plugin_audio_ports_count(const clap_plugin_t*, const bool is_input)
{
    return (is_input ? DISTRHO_PLUGIN_NUM_INPUTS : DISTRHO_PLUGIN_NUM_OUTPUTS) != 0 ? 1 : 0;
}

static bool clap_plugin_audio_ports_get(const clap_plugin_t*,
                                        const uint32_t index,
                                        const bool is_input,
                                        clap_audio_port_info_t* const info)
{
    const uint32_t maxPortCount = is_input ? DISTRHO_PLUGIN_NUM_INPUTS : DISTRHO_PLUGIN_NUM_OUTPUTS;
    DISTRHO_SAFE_ASSERT_UINT2_RETURN(index < maxPortCount, index, maxPortCount, false);

    // TODO use groups
    AudioPortWithBusId& audioPort(sPlugin->getAudioPort(is_input, index));

    info->id = index;
    std::strncpy(info->name, audioPort.name, CLAP_NAME_SIZE-1);

    // TODO bus stuff
    info->flags = CLAP_AUDIO_PORT_IS_MAIN;
    info->channel_count = maxPortCount;

    // TODO CV
    // info->port_type = audioPort.hints & kAudioPortIsCV ? CLAP_PORT_CV : nullptr;
    info->port_type = nullptr;

    info->in_place_pair = DISTRHO_PLUGIN_NUM_INPUTS == DISTRHO_PLUGIN_NUM_OUTPUTS ? index : CLAP_INVALID_ID;

    return true;
}

static const clap_plugin_audio_ports_t clap_plugin_audio_ports = {
    clap_plugin_audio_ports_count,
    clap_plugin_audio_ports_get
};

// --------------------------------------------------------------------------------------------------------------------
// plugin

static bool clap_plugin_init(const clap_plugin_t* const plugin)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    return instance->init();
}

static void clap_plugin_destroy(const clap_plugin_t* const plugin)
{
    delete static_cast<PluginCLAP*>(plugin->plugin_data);
    std::free(const_cast<clap_plugin_t*>(plugin));
}

static bool clap_plugin_activate(const clap_plugin_t* const plugin,
                                 const double sample_rate,
                                 uint32_t,
                                 const uint32_t max_frames_count)
{
    d_nextBufferSize = max_frames_count;
    d_nextSampleRate = sample_rate;

    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    instance->activate(sample_rate, max_frames_count);
    return true;
}

static void clap_plugin_deactivate(const clap_plugin_t* const plugin)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    instance->deactivate();
}

static bool clap_plugin_start_processing(const clap_plugin_t*)
{
    // nothing to do
    return true;
}

static void clap_plugin_stop_processing(const clap_plugin_t*)
{
    // nothing to do
}

static void clap_plugin_reset(const clap_plugin_t*)
{
    // nothing to do
}

static clap_process_status clap_plugin_process(const clap_plugin_t* const plugin, const clap_process_t* const process)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    return instance->process(process) ? CLAP_PROCESS_CONTINUE : CLAP_PROCESS_ERROR;
}

static const void* clap_plugin_get_extension(const clap_plugin_t*, const char* const id)
{
    if (std::strcmp(id, CLAP_EXT_AUDIO_PORTS) == 0)
        return &clap_plugin_audio_ports;
    return nullptr;
}

static void clap_plugin_on_main_thread(const clap_plugin_t*)
{
    // nothing to do
}

// --------------------------------------------------------------------------------------------------------------------
// plugin factory

static uint32_t clap_get_plugin_count(const clap_plugin_factory_t*)
{
    return 1;
}

static const clap_plugin_descriptor_t* clap_get_plugin_descriptor(const clap_plugin_factory_t*, const uint32_t index)
{
    DISTRHO_SAFE_ASSERT_UINT_RETURN(index == 0, index, nullptr);

    static const char* features[] = {
       #ifdef DISTRHO_PLUGIN_CLAP_FEATURES
        DISTRHO_PLUGIN_CLAP_FEATURES,
       #elif DISTRHO_PLUGIN_IS_SYNTH
        "instrument",
       #endif
        nullptr
    };

    static const clap_plugin_descriptor_t descriptor = {
        CLAP_VERSION,
        sPlugin->getLabel(),
        sPlugin->getName(),
        sPlugin->getMaker(),
        // TODO url
        "",
        // TODO manual url
        "",
        // TODO support url
        "",
        // TODO version string
        "",
        sPlugin->getDescription(),
        features
    };

    return &descriptor;
}

static const clap_plugin_t* clap_create_plugin(const clap_plugin_factory_t* const factory,
                                               const clap_host_t* const host,
                                               const char*)
{
    clap_plugin_t* const pluginptr = static_cast<clap_plugin_t*>(std::malloc(sizeof(clap_plugin_t)));
    DISTRHO_SAFE_ASSERT_RETURN(pluginptr != nullptr, nullptr);

    // default early values
    if (d_nextBufferSize == 0)
        d_nextBufferSize = 1024;
    if (d_nextSampleRate <= 0.0)
        d_nextSampleRate = 44100.0;

    d_nextCanRequestParameterValueChanges = true;

    const clap_plugin_t plugin = {
        clap_get_plugin_descriptor(factory, 0),
        new PluginCLAP(host),
        clap_plugin_init,
        clap_plugin_destroy,
        clap_plugin_activate,
        clap_plugin_deactivate,
        clap_plugin_start_processing,
        clap_plugin_stop_processing,
        clap_plugin_reset,
        clap_plugin_process,
        clap_plugin_get_extension,
        clap_plugin_on_main_thread
    };

    std::memcpy(pluginptr, &plugin, sizeof(clap_plugin_t));
    return pluginptr;
}

static const clap_plugin_factory_t clap_plugin_factory = {
    clap_get_plugin_count,
    clap_get_plugin_descriptor,
    clap_create_plugin
};

// --------------------------------------------------------------------------------------------------------------------
// plugin entry

static bool clap_plugin_entry_init(const char* const plugin_path)
{
    static String bundlePath;
    bundlePath = plugin_path;
    d_nextBundlePath = bundlePath.buffer();

    // init dummy plugin
    if (sPlugin == nullptr)
    {
        // set valid but dummy values
        d_nextBufferSize = 512;
        d_nextSampleRate = 44100.0;
        d_nextPluginIsDummy = true;
        d_nextCanRequestParameterValueChanges = true;

        // Create dummy plugin to get data from
        sPlugin = new PluginExporter(nullptr, nullptr, nullptr, nullptr);

        // unset
        d_nextBufferSize = 0;
        d_nextSampleRate = 0.0;
        d_nextPluginIsDummy = false;
        d_nextCanRequestParameterValueChanges = false;
    }

    return true;
}

static void clap_plugin_entry_deinit(void)
{
    sPlugin = nullptr;
}

static const void* clap_plugin_entry_get_factory(const char* const factory_id)
{
    if (std::strcmp(factory_id, CLAP_PLUGIN_FACTORY_ID) == 0)
        return &clap_plugin_factory;
    return nullptr;
}

static const clap_plugin_entry_t clap_plugin_entry = {
    CLAP_VERSION,
    clap_plugin_entry_init,
    clap_plugin_entry_deinit,
    clap_plugin_entry_get_factory
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

DISTRHO_PLUGIN_EXPORT
const clap_plugin_entry_t clap_entry = DISTRHO_NAMESPACE::clap_plugin_entry;

// --------------------------------------------------------------------------------------------------------------------
