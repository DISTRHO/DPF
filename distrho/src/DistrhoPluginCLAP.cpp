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

#include "DistrhoPluginInternal.hpp"
#include "extra/ScopedPointer.hpp"

#undef DISTRHO_PLUGIN_HAS_UI
#define DISTRHO_PLUGIN_HAS_UI 0

#if DISTRHO_PLUGIN_HAS_UI
# include "DistrhoUIInternal.hpp"
#endif

#include "clap/entry.h"
#include "clap/plugin-factory.h"
#include "clap/ext/audio-ports.h"
#include "clap/ext/gui.h"
#include "clap/ext/params.h"

START_NAMESPACE_DISTRHO

#if DISTRHO_PLUGIN_HAS_UI

// --------------------------------------------------------------------------------------------------------------------

#if ! DISTRHO_PLUGIN_WANT_STATE
static constexpr const setStateFunc setStateCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_MIDI_INPUT
static constexpr const sendNoteFunc sendNoteCallback = nullptr;
#endif

/**
 * CLAP UI class.
 */
class ClapUI
{
public:
    ClapUI(const intptr_t winId,
           const double sampleRate,
           const char* const bundlePath,
           void* const dspPtr,
           const float scaleFactor)
        : fUI(this, winId, sampleRate,
              editParameterCallback,
              setParameterCallback,
              setStateCallback,
              sendNoteCallback,
              setSizeCallback,
              fileRequestCallback,
              bundlePath, dspPtr, scaleFactor)
    {
    }

    // ----------------------------------------------------------------------------------------------------------------

private:
    // Stub stuff here

    // Plugin UI (after Stub stuff so the UI can call into us during its constructor)
    UIExporter fUI;

    // ----------------------------------------------------------------------------------------------------------------
    // DPF callbacks

    void editParameter(uint32_t, bool) const
    {
    }

    static void editParameterCallback(void* const ptr, const uint32_t rindex, const bool started)
    {
        static_cast<ClapUI*>(ptr)->editParameter(rindex, started);
    }

    void setParameterValue(uint32_t, float)
    {
    }

    static void setParameterCallback(void* const ptr, const uint32_t rindex, const float value)
    {
        static_cast<ClapUI*>(ptr)->setParameterValue(rindex, value);
    }

    void setSize(uint, uint)
    {
    }

    static void setSizeCallback(void* const ptr, const uint width, const uint height)
    {
        static_cast<ClapUI*>(ptr)->setSize(width, height);
    }

   #if DISTRHO_PLUGIN_WANT_STATE
    void setState(const char*, const char*)
    {
    }

    static void setStateCallback(void* const ptr, const char* key, const char* value)
    {
        static_cast<ClapUI*>(ptr)->setState(key, value);
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    void sendNote(const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
    }

    static void sendNoteCallback(void* const ptr, const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        static_cast<ClapUI*>(ptr)->sendNote(channel, note, velocity);
    }
   #endif

    bool fileRequest(const char*)
    {
        return true;
    }

    static bool fileRequestCallback(void* const ptr, const char* const key)
    {
        return static_cast<ClapUI*>(ptr)->fileRequest(key);
    }
};

// --------------------------------------------------------------------------------------------------------------------

#endif // DISTRHO_PLUGIN_HAS_UI

#if ! DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
static constexpr const writeMidiFunc writeMidiCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
static constexpr const requestParameterValueChangeFunc requestParameterValueChangeCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_STATE
static constexpr const updateStateValueFunc updateStateValueCallback = nullptr;
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

    // ----------------------------------------------------------------------------------------------------------------
    // core

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
                        break;
                    case CLAP_EVENT_PARAM_VALUE:
                        DISTRHO_SAFE_ASSERT_UINT2_BREAK(event->size == sizeof(clap_event_param_value),
                                                        event->size, sizeof(clap_event_param_value));
                        setParameterValueFromEvent(static_cast<const clap_event_param_value*>(static_cast<const void*>(event)));
                        break;
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
    // parameters

    uint32_t getParameterCount() const
    {
        return fPlugin.getParameterCount();
    }

    bool getParameterInfo(const uint32_t index, clap_param_info_t* const info) const
    {
        const ParameterRanges& ranges(fPlugin.getParameterRanges(index));

        if (fPlugin.getParameterDesignation(index) == kParameterDesignationBypass)
        {
            info->flags = CLAP_PARAM_IS_STEPPED|CLAP_PARAM_IS_BYPASS|CLAP_PARAM_IS_AUTOMATABLE;
            std::strcpy(info->name, "Bypass");
            std::strcpy(info->module, "dpf_bypass");
        }
        else
        {
            const uint32_t hints = fPlugin.getParameterHints(index);
            const uint32_t groupId = fPlugin.getParameterGroupId(index);

            info->flags = 0;
            if (hints & kParameterIsAutomatable)
                info->flags |= CLAP_PARAM_IS_AUTOMATABLE;
            if (hints & (kParameterIsBoolean|kParameterIsInteger))
                info->flags |= CLAP_PARAM_IS_STEPPED;
            if (hints & kParameterIsOutput)
                info->flags |= CLAP_PARAM_IS_READONLY;

            DISTRHO_NAMESPACE::strncpy(info->name, fPlugin.getParameterName(index), CLAP_NAME_SIZE);

            uint wrtn;
            if (groupId != kPortGroupNone)
            {
                const PortGroupWithId& portGroup(fPlugin.getPortGroupById(groupId));
                strncpy(info->module, portGroup.symbol, CLAP_PATH_SIZE / 2);
                info->module[CLAP_PATH_SIZE / 2] = '\0';
                wrtn = std::strlen(info->module);
                info->module[wrtn++] = '/';
            }
            else
            {
                wrtn = 0;
            }

            DISTRHO_NAMESPACE::strncpy(info->module + wrtn, fPlugin.getParameterSymbol(index), CLAP_PATH_SIZE - wrtn);
        }

        info->id = index;
        info->cookie = nullptr;
        info->min_value = ranges.min;
        info->max_value = ranges.max;
        info->default_value = ranges.def;
        return true;
    }

    bool getParameterValue(const clap_id param_id, double* const value) const
    {
        const float plain = fPlugin.getParameterValue(param_id);

        if (fPlugin.isParameterInteger(param_id))
        {
            *value = plain;
            return true;
        }

        *value = fPlugin.getParameterRanges(param_id).getNormalizedValue(static_cast<double>(plain));
        return true;
    }

    bool getParameterStringForValue(const clap_id param_id, const double value, char* const display, const uint32_t size) const
    {
        const ParameterEnumerationValues& enumValues(fPlugin.getParameterEnumValues(param_id));
        const ParameterRanges& ranges(fPlugin.getParameterRanges(param_id));
        const uint32_t hints = fPlugin.getParameterHints(param_id);

        double plain;
        if (hints & kParameterIsInteger)
        {
            plain = value;
        }
        else if (hints & kParameterIsBoolean)
        {
            const float midRange = ranges.min + (ranges.max - ranges.min) * 0.5f;
            plain = value > midRange ? ranges.max : ranges.min;
        }
        else
        {
            plain = ranges.getUnnormalizedValue(value);
        }

        for (uint32_t i=0; i < enumValues.count; ++i)
        {
            if (d_isEqual(static_cast<double>(enumValues.values[i].value), plain))
            {
                DISTRHO_NAMESPACE::strncpy(display, enumValues.values[i].label, size);
                return true;
            }
        }

        if (hints & kParameterIsInteger)
            snprintf_i32(display, plain, size);
        else
            snprintf_f32(display, plain, size);

        return true;
    }

    bool getParameterValueForString(const clap_id param_id, const char* const display, double* const value) const
    {
        const ParameterEnumerationValues& enumValues(fPlugin.getParameterEnumValues(param_id));
        const ParameterRanges& ranges(fPlugin.getParameterRanges(param_id));
        const bool isInteger = fPlugin.isParameterInteger(param_id);

        for (uint32_t i=0; i < enumValues.count; ++i)
        {
            if (std::strcmp(display, enumValues.values[i].label) == 0)
            {
                *value = isInteger 
                       ? enumValues.values[i].value
                       : ranges.getNormalizedValue(enumValues.values[i].value);
                return true;
            }
        }

        double plain;
        if (isInteger)
            plain = std::atoi(display);
        else
            plain = std::atof(display);

        *value = ranges.getNormalizedValue(plain);
        return true;
    }

    void setParameterValueFromEvent(const clap_event_param_value* const param)
    {
        const double plain = fPlugin.isParameterInteger(param->param_id)
                           ? param->value
                           : fPlugin.getParameterRanges(param->param_id).getFixedAndNormalizedValue(param->value);

        fPlugin.setParameterValue(param->param_id, plain);
    }

    void flushParameters(const clap_input_events_t* const in, const clap_output_events_t* /* const out */)
    {
        if (const uint32_t len = in->size(in))
        {
            for (uint32_t i=0; i<len; ++i)
            {
                const clap_event_header_t* const event = in->get(in, i);

                if (event->type != CLAP_EVENT_PARAM_VALUE)
                    continue;

                DISTRHO_SAFE_ASSERT_UINT2_BREAK(event->size == sizeof(clap_event_param_value),
                                                event->size, sizeof(clap_event_param_value));
                
                setParameterValueFromEvent(static_cast<const clap_event_param_value*>(static_cast<const void*>(event)));
            }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // gui

   #if DISTRHO_PLUGIN_HAS_UI
    bool createUI(const bool floating)
    {
        fUI = new ClapUI();
        return true;
    }

    void destroyUI()
    {
        fUI = nullptr;
    }

    bool setScale(const double scale)
    {
        fUI.scale = scale;

        if (ClapUI* const ui = fUI.instance)
            ui->notifyScaleFactorChange(scale);

        return true;
    }

    bool getSize(uint32_t* const width, uint32_t* const height) const
    {
        if (ClapUI* const ui = fUI.instance)
        {
            *width = ui->getWidth();
            *height = ui->getHeight();
        }
        else
        {
            // TODO
        }
        return true;
    }

    bool canResize() const
    {
        if (ClapUI* const ui = fUI.instance)
            return ui->canResize();

        return DISTRHO_PLUGIN_IS_UI_USER_RESIZABLE != 0;
    }

    bool getResizeHints(clap_gui_resize_hints_t* const hints) const
    {
        // TODO
        return true;
    }

    bool adjustSize(uint32_t* const width, uint32_t* const height) const
    {
        // TODO
        return true;
    }

    bool setSize(const uint32_t width, const uint32_t height)
    {
        // TODO
        return true;
    }

    bool setParent(const clap_window_t* const window)
    {
        // TODO

        if (ClapUI* const ui = fUI.instance)
        {
            // TODO
        }

        return true;
    }

    bool setTransient(const clap_window_t* const window)
    {
        fUI.transient = window;

        if (ClapUI* const ui = fUI.instance)
            ui->setTransient(window);
    }

    void suggestTitle(const char* const title)
    {
        fUI.title = window;

        if (ClapUI* const ui = fUI.instance)
            ui->setTitle(title);
    }

    bool show()
    {
        if (fUI.instance == nullptr)
            fUI.instance = new ClapUI();

        fUI.instance->show();
        return true;
    }

    bool hide()
    {
        if (ClapUI* const ui = fUI.instance)
            ui->hide();
        return true;
    }
   #endif

    // ----------------------------------------------------------------------------------------------------------------

private:
    // Plugin
    PluginExporter fPlugin;

   #if DISTRHO_PLUGIN_HAS_UI
    // UI
    struct UI {
        double scale;
        uint32_t hostSetWidth, hostSetHeight;
        clap_window_t parent, transient;
        String title;
        ScopedPointer<ClapUI> instance;

        UI()
          : scale(0.0),
            hostSetWidth(0),
            hostSetHeight(0),
            parent(0),
            transient(0),
            title(),
            instance() {}
    } fUI;
   #endif

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
// plugin gui

#if DISTRHO_PLUGIN_HAS_UI
static bool clap_gui_is_api_supported(const clap_plugin_t*, const char* const api, const bool is_floating)
{
    return true;
}

static bool clap_gui_get_preferred_api(const clap_plugin_t*, const char** const api, bool* const is_floating)
{
    return true;
}

static bool clap_gui_create(const clap_plugin_t* const plugin, const char* const api, const bool is_floating)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    return instance->createUI();
}

static void clap_gui_destroy(const clap_plugin_t* const plugin)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    instance->destroyUI();
}

static bool clap_gui_set_scale(const clap_plugin_t* const plugin, const double scale)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    UICLAP* const gui = instance->getUI();
    return gui->setScale(scale);
}

static bool clap_gui_get_size(const clap_plugin_t* const plugin, uint32_t* const width, uint32_t* const height)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    UICLAP* const gui = instance->getUI();
    return gui->getSize(width, height);
}

static bool clap_gui_can_resize(const clap_plugin_t* const plugin)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    UICLAP* const gui = instance->getUI();
    return gui->canResize();
}

static bool clap_gui_get_resize_hints(const clap_plugin_t* const plugin, clap_gui_resize_hints_t* const hints)
{
    return true;
}

static bool clap_gui_adjust_size(const clap_plugin_t* const plugin, uint32_t* const width, uint32_t* const height)
{
    return true;
}

static bool clap_gui_set_size(const clap_plugin_t* const plugin, const uint32_t width, const uint32_t height)
{
    return true;
}

static bool clap_gui_set_parent(const clap_plugin_t* const plugin, const clap_window_t* const window)
{
    return true;
}

static bool clap_gui_set_transient(const clap_plugin_t* const plugin, const clap_window_t* const window)
{
    return true;
}

static void clap_gui_suggest_title(const clap_plugin_t* const plugin, const char* const title)
{
}

static bool clap_gui_show(const clap_plugin_t* const plugin)
{
    return true;
}

static bool clap_gui_hide(const clap_plugin_t* const plugin)
{
    return true;
}

static const clap_plugin_gui_t clap_plugin_gui = {
    clap_gui_is_api_supported,
    clap_gui_get_preferred_api,
    clap_gui_create,
    clap_gui_destroy,
    clap_gui_set_scale,
    clap_gui_get_size,
    clap_gui_can_resize,
    clap_gui_get_resize_hints,
    clap_gui_adjust_size,
    clap_gui_set_size,
    clap_gui_set_parent,
    clap_gui_set_transient,
    clap_gui_suggest_title,
    clap_gui_show,
    clap_gui_hide
};
#endif // DISTRHO_PLUGIN_HAS_UI

// --------------------------------------------------------------------------------------------------------------------
// plugin audio ports

static uint32_t clap_plugin_audio_ports_count(const clap_plugin_t*, const bool is_input)
{
    return (is_input ? DISTRHO_PLUGIN_NUM_INPUTS : DISTRHO_PLUGIN_NUM_OUTPUTS) != 0 ? 1 : 0;
}

static bool clap_plugin_audio_ports_get(const clap_plugin_t* /* const plugin */,
                                        const uint32_t index,
                                        const bool is_input,
                                        clap_audio_port_info_t* const info)
{
    const uint32_t maxPortCount = is_input ? DISTRHO_PLUGIN_NUM_INPUTS : DISTRHO_PLUGIN_NUM_OUTPUTS;
    DISTRHO_SAFE_ASSERT_UINT2_RETURN(index < maxPortCount, index, maxPortCount, false);

    // PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);

    // TODO use groups
    AudioPortWithBusId& audioPort(sPlugin->getAudioPort(is_input, index));

    info->id = index;
    DISTRHO_NAMESPACE::strncpy(info->name, audioPort.name, CLAP_NAME_SIZE);

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
// plugin parameters

static uint32_t clap_plugin_params_count(const clap_plugin_t* const plugin)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    return instance->getParameterCount();
}

static bool clap_plugin_params_get_info(const clap_plugin_t* const plugin, const uint32_t index, clap_param_info_t* const info)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    return instance->getParameterInfo(index, info);
}

static bool clap_plugin_params_get_value(const clap_plugin_t* const plugin, const clap_id param_id, double* const value)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    return instance->getParameterValue(param_id, value);
}

static bool clap_plugin_params_value_to_text(const clap_plugin_t* plugin, const clap_id param_id, const double value, char* const display, const uint32_t size)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    return instance->getParameterStringForValue(param_id, value, display, size);
}

static bool clap_plugin_params_text_to_value(const clap_plugin_t* plugin, const clap_id param_id, const char* const display, double* const value)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    return instance->getParameterValueForString(param_id, display, value);
}

static void clap_plugin_params_flush(const clap_plugin_t* plugin, const clap_input_events_t* in, const clap_output_events_t* out)
{
    PluginCLAP* const instance = static_cast<PluginCLAP*>(plugin->plugin_data);
    return instance->flushParameters(in, out);
}

static const clap_plugin_params_t clap_plugin_params = {
    clap_plugin_params_count,
    clap_plugin_params_get_info,
    clap_plugin_params_get_value,
    clap_plugin_params_value_to_text,
    clap_plugin_params_text_to_value,
    clap_plugin_params_flush
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
    if (std::strcmp(id, CLAP_EXT_PARAMS) == 0)
        return &clap_plugin_params;
   #if DISTRHO_PLUGIN_HAS_UI
    if (std::strcmp(id, CLAP_EXT_GUI) == 0)
        return &clap_plugin_gui;
   #endif
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
