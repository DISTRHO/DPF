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

#include "DistrhoPluginInternal.hpp"
#include "../extra/ScopedPointer.hpp"

#include "travesty/audio_processor.h"
#include "travesty/component.h"
#include "travesty/edit_controller.h"
#include "travesty/factory.h"

#include <atomic>
#include <vector>

// TESTING awful idea dont reuse
#include "../extra/Thread.hpp"

#if DISTRHO_PLUGIN_HAS_UI && ! DISTRHO_PLUGIN_HAS_EMBED_UI
# undef DISTRHO_PLUGIN_HAS_UI
# define DISTRHO_PLUGIN_HAS_UI 0
#endif

#if DISTRHO_PLUGIN_HAS_UI && ! defined(HAVE_DGL) && ! DISTRHO_PLUGIN_HAS_EXTERNAL_UI
# undef DISTRHO_PLUGIN_HAS_UI
# define DISTRHO_PLUGIN_HAS_UI 0
#endif

#undef DISTRHO_PLUGIN_HAS_UI
#define DISTRHO_PLUGIN_HAS_UI 1

#if DISTRHO_PLUGIN_HAS_UI
# include "DistrhoUIInternal.hpp"
# include "../extra/RingBuffer.hpp"
#endif

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

#if ! DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
static const writeMidiFunc writeMidiCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
static const requestParameterValueChangeFunc requestParameterValueChangeCallback = nullptr;
#endif

// --------------------------------------------------------------------------------------------------------------------
// custom v3_tuid compatible type

typedef uint32_t dpf_tuid[4];
static_assert(sizeof(v3_tuid) == sizeof(dpf_tuid), "uid size mismatch");

// --------------------------------------------------------------------------------------------------------------------
// custom, constant uids related to DPF

static constexpr const uint32_t dpf_id_entry = d_cconst('D', 'P', 'F', ' ');
static constexpr const uint32_t dpf_id_clas  = d_cconst('c', 'l', 'a', 's');
static constexpr const uint32_t dpf_id_comp  = d_cconst('c', 'o', 'm', 'p');
static constexpr const uint32_t dpf_id_ctrl  = d_cconst('c', 't', 'r', 'l');
static constexpr const uint32_t dpf_id_proc  = d_cconst('p', 'r', 'o', 'c');
static constexpr const uint32_t dpf_id_view  = d_cconst('v', 'i', 'e', 'w');

// --------------------------------------------------------------------------------------------------------------------
// plugin specific uids (values are filled in during plugin init)

static dpf_tuid dpf_tuid_class = { dpf_id_entry, dpf_id_clas, 0, 0 };
static dpf_tuid dpf_tuid_component = { dpf_id_entry, dpf_id_comp, 0, 0 };
static dpf_tuid dpf_tuid_controller = { dpf_id_entry, dpf_id_ctrl, 0, 0 };
static dpf_tuid dpf_tuid_processor = { dpf_id_entry, dpf_id_proc, 0, 0 };
static dpf_tuid dpf_tuid_view = { dpf_id_entry, dpf_id_view, 0, 0 };

// --------------------------------------------------------------------------------------------------------------------
// Utility functions

const char* tuid2str(const v3_tuid iid)
{
    if (v3_tuid_match(iid, v3_funknown_iid))
        return "{v3_funknown}";
    if (v3_tuid_match(iid, v3_plugin_base_iid))
        return "{v3_plugin_base}";
    if (v3_tuid_match(iid, v3_plugin_factory_iid))
        return "{v3_plugin_factory}";
    if (v3_tuid_match(iid, v3_plugin_factory_2_iid))
        return "{v3_plugin_factory_2}";
    if (v3_tuid_match(iid, v3_plugin_factory_3_iid))
        return "{v3_plugin_factory_3}";
    if (v3_tuid_match(iid, v3_component_iid))
        return "{v3_component}";
    if (v3_tuid_match(iid, v3_bstream_iid))
        return "{v3_bstream}";
    if (v3_tuid_match(iid, v3_event_list_iid))
        return "{v3_event_list}";
    if (v3_tuid_match(iid, v3_param_value_queue_iid))
        return "{v3_param_value_queue}";
    if (v3_tuid_match(iid, v3_param_changes_iid))
        return "{v3_param_changes}";
    if (v3_tuid_match(iid, v3_process_context_requirements_iid))
        return "{v3_process_context_requirements}";
    if (v3_tuid_match(iid, v3_audio_processor_iid))
        return "{v3_audio_processor}";
    if (v3_tuid_match(iid, v3_component_handler_iid))
        return "{v3_component_handler}";
    if (v3_tuid_match(iid, v3_edit_controller_iid))
        return "{v3_edit_controller}";
    if (v3_tuid_match(iid, v3_plugin_view_iid))
        return "{v3_plugin_view}";
    if (v3_tuid_match(iid, v3_plugin_frame_iid))
        return "{v3_plugin_frame}";
    if (v3_tuid_match(iid, v3_plugin_view_content_scale_steinberg_iid))
        return "{v3_plugin_view_content_scale_steinberg}";
    if (v3_tuid_match(iid, v3_plugin_view_parameter_finder_iid))
        return "{v3_plugin_view_parameter_finder}";
    if (std::memcmp(iid, dpf_tuid_class, sizeof(dpf_tuid)) == 0)
        return "{dpf_tuid_class}";
    if (std::memcmp(iid, dpf_tuid_component, sizeof(dpf_tuid)) == 0)
        return "{dpf_tuid_component}";
    if (std::memcmp(iid, dpf_tuid_controller, sizeof(dpf_tuid)) == 0)
        return "{dpf_tuid_controller}";
    if (std::memcmp(iid, dpf_tuid_processor, sizeof(dpf_tuid)) == 0)
        return "{dpf_tuid_processor}";
    if (std::memcmp(iid, dpf_tuid_view, sizeof(dpf_tuid)) == 0)
        return "{dpf_tuid_view}";

    static char buf[46];
    std::snprintf(buf, sizeof(buf), "{0x%08X,0x%08X,0x%08X,0x%08X}",
                  (uint32_t)d_cconst(iid[ 0], iid[ 1], iid[ 2], iid[ 3]),
                  (uint32_t)d_cconst(iid[ 4], iid[ 5], iid[ 6], iid[ 7]),
                  (uint32_t)d_cconst(iid[ 8], iid[ 9], iid[10], iid[11]),
                  (uint32_t)d_cconst(iid[12], iid[13], iid[14], iid[15]));
    return buf;
}

void strncpy(char* const dst, const char* const src, const size_t size)
{
    DISTRHO_SAFE_ASSERT_RETURN(size > 0,);

    if (const size_t len = std::min(std::strlen(src), size-1U))
    {
        std::memcpy(dst, src, len);
        dst[len] = '\0';
    }
    else
    {
        dst[0] = '\0';
    }
}

void strncpy_16from8(int16_t* const dst, const char* const src, const size_t size)
{
    DISTRHO_SAFE_ASSERT_RETURN(size > 0,);

    if (const size_t len = std::min(std::strlen(src), size-1U))
    {
        for (size_t i=0; i<len; ++i)
        {
            // skip non-ascii chars
            if ((uint8_t)src[i] >= 0x80)
                continue;

            dst[i] = src[i];
        }
        dst[len] = 0;
    }
    else
    {
        dst[0] = 0;
    }
}

// --------------------------------------------------------------------------------------------------------------------

struct ParameterAndNotesHelper
{
    float* parameterValues;
#if DISTRHO_PLUGIN_HAS_UI
    bool* parameterChecks;
# if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    SmallStackBuffer notesRingBuffer;
# endif
#endif

    ParameterAndNotesHelper()
        : parameterValues(nullptr)
#if DISTRHO_PLUGIN_HAS_UI
        , parameterChecks(nullptr)
# if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        , notesRingBuffer(StackBuffer_INIT)
# endif
#endif
    {
#if DISTRHO_PLUGIN_HAS_UI && DISTRHO_PLUGIN_WANT_MIDI_INPUT && ! defined(DISTRHO_PROPER_CPP11_SUPPORT)
        std::memset(&notesRingBuffer, 0, sizeof(notesRingBuffer));
#endif
    }

    virtual ~ParameterAndNotesHelper()
    {
        if (parameterValues != nullptr)
        {
            delete[] parameterValues;
            parameterValues = nullptr;
        }
#if DISTRHO_PLUGIN_HAS_UI
        if (parameterChecks != nullptr)
        {
            delete[] parameterChecks;
            parameterChecks = nullptr;
        }
#endif
    }

#if DISTRHO_PLUGIN_WANT_STATE
    virtual void setStateFromUI(const char* const newKey, const char* const newValue) = 0;
#endif
};

// --------------------------------------------------------------------------------------------------------------------

class PluginVst3
{
    /* buses: we provide 1 for the main audio (if there is any) plus 1 for each sidechain or cv port.
     * Main audio comes first, if available.
     * Then sidechain, also if available.
     * And finally each CV port individually.
     *
     * MIDI will have a single bus, nothing special there.
     */
    struct BusInfo {
        uint8_t audio = 0;     // either 0 or 1
        uint8_t sidechain = 0; // either 0 or 1
        uint32_t numMainAudio = 0;
        uint32_t numSidechain = 0;
        uint32_t numCV = 0;
    } inputBuses, outputBuses;

public:
    PluginVst3()
        : fPlugin(this, writeMidiCallback, requestParameterValueChangeCallback)
    {
#if DISTRHO_PLUGIN_NUM_INPUTS > 0
        for (uint32_t i=0; i<DISTRHO_PLUGIN_NUM_INPUTS; ++i)
        {
            const uint32_t hints = fPlugin.getAudioPortHints(true, i);

            if (hints & kAudioPortIsCV)
                ++inputBuses.numCV;
            else
                ++inputBuses.numMainAudio;

            if (hints & kAudioPortIsSidechain)
                ++inputBuses.numSidechain;
        }

        if (inputBuses.numMainAudio != 0)
            inputBuses.audio = 1;
        if (inputBuses.numSidechain != 0)
            inputBuses.sidechain = 1;

        uint32_t cvInputBusId = 0;
        for (uint32_t i=0; i<DISTRHO_PLUGIN_NUM_INPUTS; ++i)
        {
            AudioPortWithBusId& port(fPlugin.getAudioPort(true, i));

            if (port.hints & kAudioPortIsCV)
                port.busId = inputBuses.audio + inputBuses.sidechain + cvInputBusId++;
            else if (port.hints & kAudioPortIsSidechain)
                port.busId = 1;
            else
                port.busId = 0;
        }
#endif
#if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        for (uint32_t i=0; i<DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
        {
            const uint32_t hints = fPlugin.getAudioPortHints(false, i);

            if (hints & kAudioPortIsCV)
                ++outputBuses.numCV;
            else
                ++outputBuses.numMainAudio;

            if (hints & kAudioPortIsSidechain)
                ++outputBuses.numSidechain;
        }

        if (outputBuses.numMainAudio != 0)
            outputBuses.audio = 1;
        if (outputBuses.numSidechain != 0)
            outputBuses.sidechain = 1;

        uint32_t cvOutputBusId = 0;
        for (uint32_t i=0; i<DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
        {
            AudioPortWithBusId& port(fPlugin.getAudioPort(false, i));

            if (port.hints & kAudioPortIsCV)
                port.busId = outputBuses.audio + outputBuses.sidechain + cvOutputBusId++;
            else if (port.hints & kAudioPortIsSidechain)
                port.busId = 1;
            else
                port.busId = 0;
        }
#endif
    }

    void* getInstancePointer() const noexcept
    {
        return fPlugin.getInstancePointer();
    }

    double getSampleRate() const noexcept
    {
        return fPlugin.getSampleRate();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_component interface calls

#if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
    int32_t getAudioBusCount(const bool isInput) const noexcept
    {
        if (isInput)
            return inputBuses.audio + inputBuses.sidechain + inputBuses.numCV;
        else
            return outputBuses.audio + outputBuses.sidechain + outputBuses.numCV;
    };

    v3_result getBusAudioBusInfo(const bool isInput, const uint32_t index, v3_bus_info* info) const
    {
        int32_t channel_count;
        v3_bus_types bus_type;
        v3_bus_flags flags;
        v3_str_128 bus_name;

        if (isInput)
        {
#if DISTRHO_PLUGIN_NUM_INPUTS > 0
            switch (index)
            {
            case 0:
                if (inputBuses.audio)
                {
                    channel_count = inputBuses.numMainAudio;
                    bus_type = V3_MAIN;
                    flags = V3_DEFAULT_ACTIVE;
                    break;
                }
            // fall-through
            case 1:
                if (inputBuses.sidechain)
                {
                    channel_count = inputBuses.numSidechain;
                    bus_type = V3_AUX;
                    flags = v3_bus_flags(0);
                    break;
                }
            // fall-through
            default:
                channel_count = 1;
                bus_type = V3_AUX;
                flags = V3_IS_CONTROL_VOLTAGE;
                break;
            }

            if (bus_type == V3_MAIN)
            {
                strncpy_16from8(info->bus_name, "Audio Input", 128);
            }
            else
            {
                for (uint32_t i=0; i<DISTRHO_PLUGIN_NUM_INPUTS; ++i)
                {
                    const AudioPortWithBusId& port(fPlugin.getAudioPort(true, i));

                    // TODO find port group name
                    if (port.busId == index)
                    {
                        strncpy_16from8(info->bus_name, port.name, 128);
                        break;
                    }
                }
            }
#else
            return V3_INVALID_ARG;
#endif
        }
        else
        {
#if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
            switch (index)
            {
            case 0:
                if (outputBuses.audio)
                {
                    channel_count = outputBuses.numMainAudio;
                    bus_type = V3_MAIN;
                    flags = V3_DEFAULT_ACTIVE;
                    break;
                }
            // fall-through
            case 1:
                if (outputBuses.sidechain)
                {
                    channel_count = outputBuses.numSidechain;
                    bus_type = V3_AUX;
                    flags = v3_bus_flags(0);
                    break;
                }
            // fall-through
            default:
                channel_count = 1;
                bus_type = V3_AUX;
                flags = V3_IS_CONTROL_VOLTAGE;
                break;
            }

            if (bus_type == V3_MAIN)
            {
                strncpy_16from8(info->bus_name, "Audio Output", 128);
            }
            else
            {
                for (uint32_t i=0; i<DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
                {
                    const AudioPortWithBusId& port(fPlugin.getAudioPort(false, i));

                    // TODO find port group name
                    if (port.busId == index)
                    {
                        strncpy_16from8(info->bus_name, port.name, 128);
                        break;
                    }
                }
            }
#else
            return V3_INVALID_ARG;
#endif
        }

        std::memset(info, 0, sizeof(v3_bus_info));
        info->media_type = V3_AUDIO;
        info->direction = isInput ? V3_INPUT : V3_OUTPUT;
        info->channel_count = channel_count;
        std::memcpy(info->bus_name, bus_name, sizeof(bus_name));
        info->bus_type = bus_type;
        info->flags = flags;
        return V3_OK;
    }
#endif

    void setActive(const bool active)
    {
        if (active)
            fPlugin.activate();
        else
            fPlugin.deactivateIfNeeded();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_edit_controller interface calls

    uint32_t getParameterCount() const noexcept
    {
        return fPlugin.getParameterCount();
    }

    void getParameterInfo(const uint32_t index, v3_param_info* const info) const noexcept
    {
        // set up flags
        int32_t flags = 0;

        const auto desig = fPlugin.getParameterDesignation(index);
        const auto hints = fPlugin.getParameterHints(index);

        const ParameterRanges& ranges(fPlugin.getParameterRanges(index));

        switch (desig)
        {
        case kParameterDesignationNull:
            break;
        case kParameterDesignationBypass:
            flags |= V3_PARAM_IS_BYPASS;
            break;
        }

        if (hints & kParameterIsAutomable)
            flags |= V3_PARAM_CAN_AUTOMATE;
        if (hints & kParameterIsOutput)
            flags |= V3_PARAM_READ_ONLY;
        // TODO V3_PARAM_IS_LIST

        // set up step_count
        int32_t step_count = 0;

        if (hints & kParameterIsBoolean)
            step_count = 1;
        if ((hints & kParameterIsInteger) && ranges.max - ranges.min > 1)
            step_count = ranges.max - ranges.min - 1;

        std::memset(info, 0, sizeof(v3_param_info));
        info->param_id = index;
        info->flags = flags;
        info->step_count = step_count;
        info->default_normalised_value = ranges.getNormalizedValue(ranges.def);
        // int32_t unit_id;
        strncpy_16from8(info->title,       fPlugin.getParameterName(index), 128);
        strncpy_16from8(info->short_title, fPlugin.getParameterShortName(index), 128);
        strncpy_16from8(info->units,       fPlugin.getParameterUnit(index), 128);
    }

    double getNormalizedParameterValue(const uint32_t index)
    {
        const ParameterRanges& ranges(fPlugin.getParameterRanges(index));
        return ranges.getNormalizedValue(fPlugin.getParameterValue(index));
    }

    void setNormalizedParameterValue(const uint32_t index, const double value)
    {
        const uint32_t hints = fPlugin.getParameterHints(index);
        const ParameterRanges& ranges(fPlugin.getParameterRanges(index));

        float realValue = ranges.getUnnormalizedValue(value);

        if (hints & kParameterIsBoolean)
        {
            const float midRange = ranges.min + (ranges.max - ranges.min) / 2.0f;
            realValue = realValue > midRange ? ranges.max : ranges.min;
        }

        if (hints & kParameterIsInteger)
        {
            realValue = std::round(realValue);
        }

        fPlugin.setParameterValue(index, realValue);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_audio_processor interface calls

#if DISTRHO_PLUGIN_WANT_LATENCY
    uint32_t getLatencySamples() const noexcept
    {
        return fPlugin.getLatency();
    }
#endif

    void setupProcessing(v3_process_setup* const setup)
    {
        DISTRHO_SAFE_ASSERT_RETURN(setup->symbolic_sample_size == V3_SAMPLE_32,);

        const bool active = fPlugin.isActive();
        fPlugin.deactivateIfNeeded();

        // TODO process_mode can be V3_REALTIME, V3_PREFETCH, V3_OFFLINE

        fPlugin.setSampleRate(setup->sample_rate, true);
        fPlugin.setBufferSize(setup->max_block_size, true);

        if (active)
            fPlugin.activate();
    }

    void setProcessing(const bool processing)
    {
        if (processing)
        {
            if (! fPlugin.isActive())
                fPlugin.activate();
        }
        else
        {
            fPlugin.deactivate();
        }
    }

    void process(v3_process_data* const data)
    {
        DISTRHO_SAFE_ASSERT_RETURN(data->symbolic_sample_size == V3_SAMPLE_32,);

        // TODO full thing
    }

    // ----------------------------------------------------------------------------------------------------------------

private:
    // Plugin
    PluginExporter fPlugin;

    // VST3 stuff
    // TODO

#if DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
    bool requestParameterValueChange(uint32_t, float)
    {
        // TODO
        return true;
    }

    static bool requestParameterValueChangeCallback(void* const ptr, const uint32_t index, const float value)
    {
        return ((PluginVst3*)ptr)->requestParameterValueChange(index, value);
    }
#endif

#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    bool writeMidi(const MidiEvent& midiEvent)
    {
        // TODO
        return true;
    }

    static bool writeMidiCallback(void* ptr, const MidiEvent& midiEvent)
    {
        return ((PluginVst3*)ptr)->writeMidi(midiEvent);
    }
#endif

};

// --------------------------------------------------------------------------------------------------------------------

#if DISTRHO_PLUGIN_HAS_UI

#if ! DISTRHO_PLUGIN_WANT_MIDI_INPUT
static const sendNoteFunc sendNoteCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_STATE
static const setStateFunc setStateCallback = nullptr;
#endif

class UIVst3 : public Thread
{
public:
    UIVst3(ScopedPointer<PluginVst3>& v, v3_plugin_frame* const f, const intptr_t winId, const float scaleFactor)
        : vst3(v),
          frame(f),
          fUI(this, winId, v->getSampleRate(),
              editParameterCallback,
              setParameterCallback,
              setStateCallback,
              sendNoteCallback,
              setSizeCallback,
              nullptr, // TODO file request
              nullptr,
              v->getInstancePointer(),
              scaleFactor)
# if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        , fNotesRingBuffer()
# endif
    {
# if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        fNotesRingBuffer.setRingBuffer(&uiHelper->notesRingBuffer, false);
# endif
        // TESTING awful idea dont reuse
        startThread();
    }

    ~UIVst3() override
    {
        stopThread(5000);
    }

    // -------------------------------------------------------------------

    // TESTING awful idea dont reuse
    void run() override
    {
        while (! shouldThreadExit())
        {
            idle();
            d_msleep(50);
        }
    }

    void idle()
    {
        /*
        for (uint32_t i=0, count = fPlugin->getParameterCount(); i < count; ++i)
        {
            if (fUiHelper->parameterChecks[i])
            {
                fUiHelper->parameterChecks[i] = false;
                fUI.parameterChanged(i, fUiHelper->parameterValues[i]);
            }
        }
        */

        fUI.plugin_idle();
    }

    int16_t getWidth() const
    {
        return fUI.getWidth();
    }

    int16_t getHeight() const
    {
        return fUI.getHeight();
    }

    double getScaleFactor() const
    {
        return fUI.getScaleFactor();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_plugin_view interface calls

    void setFrame(v3_plugin_frame* const f) noexcept
    {
        frame = f;
    }

    // ----------------------------------------------------------------------------------------------------------------

protected:
    void editParameter(const uint32_t /*index*/, const bool /*started*/) const
    {
        // hostCallback(started ? audioMasterBeginEdit : audioMasterEndEdit, index);
    }

    void setParameterValue(const uint32_t /*index*/, const float /*realValue*/)
    {
        // const ParameterRanges& ranges(fPlugin->getParameterRanges(index));
        // const float perValue(ranges.getNormalizedValue(realValue));

        // fPlugin->setParameterValue(index, realValue);
        // hostCallback(audioMasterAutomate, index, 0, nullptr, perValue);
    }

    void setSize(uint width, uint height)
    {
# ifdef DISTRHO_OS_MAC
        const double scaleFactor = fUI.getScaleFactor();
        width /= scaleFactor;
        height /= scaleFactor;
# endif
        if (frame == nullptr)
            return;

        v3_view_rect rect = {};
        rect.right = width;
        rect.bottom = height;
        (void)rect;
        // frame->resize_view(nullptr, uivst3, &rect);
    }

# if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    void sendNote(const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        uint8_t midiData[3];
        midiData[0] = (velocity != 0 ? 0x90 : 0x80) | channel;
        midiData[1] = note;
        midiData[2] = velocity;
        fNotesRingBuffer.writeCustomData(midiData, 3);
        fNotesRingBuffer.commitWrite();
    }
# endif

# if DISTRHO_PLUGIN_WANT_STATE
    void setState(const char* const key, const char* const value)
    {
        // fUiHelper->setStateFromUI(key, value);
    }
# endif

private:
    // VST3 stuff
    ScopedPointer<PluginVst3>& vst3;
    v3_plugin_frame* frame;

    // Plugin UI
    UIExporter fUI;
# if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    RingBufferControl<SmallStackBuffer> fNotesRingBuffer;
# endif

    // -------------------------------------------------------------------
    // Callbacks

    #define handlePtr ((UIVst3*)ptr)

    static void editParameterCallback(void* ptr, uint32_t index, bool started)
    {
        handlePtr->editParameter(index, started);
    }

    static void setParameterCallback(void* ptr, uint32_t rindex, float value)
    {
        handlePtr->setParameterValue(rindex, value);
    }

    static void setSizeCallback(void* ptr, uint width, uint height)
    {
        handlePtr->setSize(width, height);
    }

# if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    static void sendNoteCallback(void* ptr, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        handlePtr->sendNote(channel, note, velocity);
    }
# endif

# if DISTRHO_PLUGIN_WANT_STATE
    static void setStateCallback(void* ptr, const char* key, const char* value)
    {
        handlePtr->setState(key, value);
    }
# endif

    #undef handlePtr
};

#endif //  DISTRHO_PLUGIN_HAS_UI

// --------------------------------------------------------------------------------------------------------------------
// Dummy plugin to get data from

static ScopedPointer<PluginExporter> gPluginInfo;

// --------------------------------------------------------------------------------------------------------------------
// dpf_plugin_view

#if DISTRHO_PLUGIN_HAS_UI
struct v3_plugin_view_cpp : v3_funknown {
    v3_plugin_view view;
};

struct dpf_plugin_view : v3_plugin_view_cpp {
    ScopedPointer<PluginVst3>& vst3;
    ScopedPointer<UIVst3> uivst3;
    double lastScaleFactor = 0.0;
    v3_plugin_frame* hostframe = nullptr;

    dpf_plugin_view(ScopedPointer<PluginVst3>& v)
        : vst3(v)
    {
        static const uint8_t* kSupportedFactories[] = {
            v3_funknown_iid,
            v3_plugin_view_iid
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            d_stdout("dpf_plugin_view::query_interface         => %s | %p %s %p", __PRETTY_FUNCTION__ + 41, self, tuid2str(iid), iface);
            *iface = NULL;
            DISTRHO_SAFE_ASSERT_RETURN(self != nullptr, V3_NO_INTERFACE);

            for (const uint8_t* factory_iid : kSupportedFactories)
            {
                if (v3_tuid_match(factory_iid, iid))
                {
                    *iface = self;
                    return V3_OK;
                }
            }

            return V3_NO_INTERFACE;
        };

        // we only support 1 plugin per binary, so don't have to care here
        ref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_plugin_view::ref                     => %s | %p", __PRETTY_FUNCTION__ + 41, self);
            return 1;
        };

        unref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_plugin_view::unref                   => %s | %p", __PRETTY_FUNCTION__ + 41, self);
            return 0;
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_plugin_view

        view.is_platform_type_supported = []V3_API(void* self, const char* platform_type) -> v3_result
        {
            d_stdout("dpf_plugin_view::is_platform_type_supported => %s | %p %s", __PRETTY_FUNCTION__ + 41, self, platform_type);
            const char* const supported[] = {
#ifdef _WIN32
                V3_VIEW_PLATFORM_TYPE_HWND,
#elif defined(__APPLE__)
                V3_VIEW_PLATFORM_TYPE_NSVIEW,
#else
                V3_VIEW_PLATFORM_TYPE_X11,
#endif
            };

            for (size_t i=0; i<sizeof(supported)/sizeof(supported[0]); ++i)
            {
                if (std::strcmp(supported[i], platform_type))
                    return V3_OK;
            }

            return V3_NOT_IMPLEMENTED;
        };

        view.attached = []V3_API(void* self, void* parent, const char* platform_type) -> v3_result
        {
            d_stdout("dpf_plugin_view::attached                   => %s | %p %p %s", __PRETTY_FUNCTION__ + 41, self, parent, platform_type);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);
            DISTRHO_SAFE_ASSERT_RETURN(view->uivst3 == nullptr, V3_INVALID_ARG);
            view->uivst3 = new UIVst3(view->vst3, view->hostframe, (uintptr_t)parent, view->lastScaleFactor);
            return V3_OK;
        };

        view.removed = []V3_API(void* self) -> v3_result
        {
            d_stdout("dpf_plugin_view::removed                    => %s | %p", __PRETTY_FUNCTION__ + 41, self);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);
            DISTRHO_SAFE_ASSERT_RETURN(view->uivst3 != nullptr, V3_INVALID_ARG);
            view->uivst3 = nullptr;
            return V3_OK;
        };

        view.on_wheel = []V3_API(void* self, float distance) -> v3_result
        {
            d_stdout("dpf_plugin_view::on_wheel                   => %s | %p %f", __PRETTY_FUNCTION__ + 41, self, distance);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);
            DISTRHO_SAFE_ASSERT_RETURN(view->uivst3 != nullptr, V3_NOT_INITIALISED);
            return V3_NOT_IMPLEMENTED;
        };

        view.on_key_down = []V3_API(void* self, int16_t key_char, int16_t key_code, int16_t modifiers) -> v3_result
        {
            d_stdout("dpf_plugin_view::on_key_down                => %s | %p %i %i %i", __PRETTY_FUNCTION__ + 41, self, key_char, key_code, modifiers);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);
            DISTRHO_SAFE_ASSERT_RETURN(view->uivst3 != nullptr, V3_NOT_INITIALISED);
            return V3_NOT_IMPLEMENTED;
        };

        view.on_key_up = []V3_API(void* self, int16_t key_char, int16_t key_code, int16_t modifiers) -> v3_result
        {
            d_stdout("dpf_plugin_view::on_key_up                  => %s | %p %i %i %i", __PRETTY_FUNCTION__ + 41, self, key_char, key_code, modifiers);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);
            DISTRHO_SAFE_ASSERT_RETURN(view->uivst3 != nullptr, V3_NOT_INITIALISED);
            return V3_NOT_IMPLEMENTED;
        };

        view.get_size = []V3_API(void* self, v3_view_rect* rect) -> v3_result
        {
            d_stdout("dpf_plugin_view::get_size                   => %s | %p", __PRETTY_FUNCTION__ + 41, self);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);

            std::memset(rect, 0, sizeof(v3_view_rect));

            if (view->uivst3 != nullptr)
            {
                rect->right  = view->uivst3->getWidth();
                rect->bottom = view->uivst3->getHeight();
# ifdef DISTRHO_OS_MAC
                const double scaleFactor = view->uivst3->getScaleFactor();
                rect->right /= scaleFactor;
                rect->bottom /= scaleFactor;
# endif
            }
            else
            {
                UIExporter tmpUI(nullptr, 0, view->vst3->getSampleRate(),
                                 nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                 view->vst3->getInstancePointer(), view->lastScaleFactor);
                rect->right  = tmpUI.getWidth();
                rect->bottom = tmpUI.getHeight();
# ifdef DISTRHO_OS_MAC
                const double scaleFactor = tmpUI.getScaleFactor();
                rect->right /= scaleFactor;
                rect->bottom /= scaleFactor;
# endif
                tmpUI.quit();
            }

            return V3_NOT_IMPLEMENTED;
        };

        view.set_size = []V3_API(void* self, v3_view_rect*) -> v3_result
        {
            d_stdout("dpf_plugin_view::set_size                   => %s | %p", __PRETTY_FUNCTION__ + 41, self);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);
            DISTRHO_SAFE_ASSERT_RETURN(view->uivst3 != nullptr, V3_NOT_INITIALISED);
            return V3_NOT_IMPLEMENTED;
        };

        view.on_focus = []V3_API(void* self, v3_bool state) -> v3_result
        {
            d_stdout("dpf_plugin_view::on_focus                   => %s | %p %u", __PRETTY_FUNCTION__ + 41, self, state);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);
            DISTRHO_SAFE_ASSERT_RETURN(view->uivst3 != nullptr, V3_NOT_INITIALISED);
            return V3_NOT_IMPLEMENTED;
        };

        view.set_frame = []V3_API(void* self, v3_plugin_frame* frame) -> v3_result
        {
            d_stdout("dpf_plugin_view::set_frame                  => %s | %p", __PRETTY_FUNCTION__ + 41, self);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);

            view->hostframe = frame;

            if (view->uivst3 != nullptr)
                view->uivst3->setFrame(frame);

            return V3_OK;
        };

        view.can_resize = []V3_API(void* self) -> v3_result
        {
            d_stdout("dpf_plugin_view::can_resize                 => %s | %p", __PRETTY_FUNCTION__ + 41, self);
#if DISTRHO_UI_USER_RESIZABLE
            return V3_OK;
#else
            return V3_NOT_IMPLEMENTED;
#endif
        };

        view.check_size_constraint = []V3_API(void* self, v3_view_rect*) -> v3_result
        {
            d_stdout("dpf_plugin_view::check_size_constraint      => %s | %p", __PRETTY_FUNCTION__ + 41, self);
            return V3_NOT_IMPLEMENTED;
        };
    }
};
#endif

// --------------------------------------------------------------------------------------------------------------------
// dpf_edit_controller

struct v3_edit_controller_cpp : v3_funknown {
    v3_plugin_base base;
    v3_edit_controller controller;
};

struct dpf_edit_controller : v3_edit_controller_cpp {
    ScopedPointer<dpf_plugin_view> view;
    ScopedPointer<PluginVst3>& vst3;

    dpf_edit_controller(ScopedPointer<PluginVst3>& v)
        : vst3(v)
    {
        static const uint8_t* kSupportedFactories[] = {
            v3_funknown_iid,
            v3_edit_controller_iid
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            d_stdout("dpf_edit_controller::query_interface            => %s | %p %s %p", __PRETTY_FUNCTION__ + 97, self, tuid2str(iid), iface);
            *iface = NULL;
            DISTRHO_SAFE_ASSERT_RETURN(self != nullptr, V3_NO_INTERFACE);

            for (const uint8_t* factory_iid : kSupportedFactories)
            {
                if (v3_tuid_match(factory_iid, iid))
                {
                    *iface = self;
                    return V3_OK;
                }
            }

            return V3_NO_INTERFACE;
        };

        // we only support 1 plugin per binary, so don't have to care here
        ref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_edit_controller::ref                        => %s | %p", __PRETTY_FUNCTION__ + 97, self);
            return 1;
        };

        unref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_edit_controller::unref                      => %s | %p", __PRETTY_FUNCTION__ + 97, self);
            return 0;
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_plugin_base

        base.initialise = []V3_API(void* self, struct v3_plugin_base::v3_funknown *context) -> v3_result
        {
            d_stdout("dpf_edit_controller::initialise                 => %s | %p %p", __PRETTY_FUNCTION__ + 97, self, context);
            return V3_OK;
        };

        base.terminate = []V3_API(void* self) -> v3_result
        {
            d_stdout("dpf_edit_controller::terminate                  => %s | %p", __PRETTY_FUNCTION__ + 97, self);
            return V3_OK;
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_edit_controller

        controller.set_component_state = []V3_API(void* self, v3_bstream*) -> v3_result
        {
            d_stdout("dpf_edit_controller::set_component_state        => %s | %p", __PRETTY_FUNCTION__ + 97, self);
            return V3_OK;
        };

        controller.set_state = []V3_API(void* self, v3_bstream*) -> v3_result
        {
            d_stdout("dpf_edit_controller::set_state                  => %s | %p", __PRETTY_FUNCTION__ + 97, self);
            return V3_OK;
        };

        controller.get_state = []V3_API(void* self, v3_bstream*) -> v3_result
        {
            d_stdout("dpf_edit_controller::get_state                  => %s | %p", __PRETTY_FUNCTION__ + 97, self);
            return V3_OK;
        };

        controller.get_parameter_count = []V3_API(void* self) -> int32_t
        {
            d_stdout("dpf_edit_controller::get_parameter_count        => %s | %p", __PRETTY_FUNCTION__ + 97, self);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, V3_INVALID_ARG);

            PluginVst3* const vst3 = controller->vst3.get();
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);
            return vst3->getParameterCount();
        };

        controller.get_parameter_info = []V3_API(void* self, int32_t param_idx, v3_param_info* param_info) -> v3_result
        {
            d_stdout("dpf_edit_controller::get_parameter_info         => %s | %p %i", __PRETTY_FUNCTION__ + 97, self, param_idx);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, V3_INVALID_ARG);
            DISTRHO_SAFE_ASSERT_RETURN(param_idx >= 0, V3_INVALID_ARG);

            PluginVst3* const vst3 = controller->vst3.get();
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            const uint32_t uidx = static_cast<uint32_t>(param_idx);
            DISTRHO_SAFE_ASSERT_RETURN(uidx < vst3->getParameterCount(), V3_INVALID_ARG);

            vst3->getParameterInfo(uidx, param_info);
            return V3_OK;
        };

        controller.get_parameter_string_for_value = []V3_API(void* self, v3_param_id index, double normalised, v3_str_128 output) -> v3_result
        {
            d_stdout("dpf_edit_controller::get_parameter_string_for_value => %s | %p %f", __PRETTY_FUNCTION__ + 97, self, normalised);

            const ParameterRanges& ranges(gPluginInfo->getParameterRanges(index));
            const float realvalue = ranges.getUnnormalizedValue(normalised);
            char buf[24];
            sprintf(buf, "%f", realvalue);
            strncpy_16from8(output, buf, 128);
            return V3_OK;
        };

        controller.get_parameter_value_for_string = []V3_API(void* self, v3_param_id, int16_t* input, double* output) -> v3_result
        {
            d_stdout("dpf_edit_controller::get_parameter_value_for_string => %s | %p %p %p", __PRETTY_FUNCTION__ + 97, self, input, output);
            return V3_NOT_IMPLEMENTED;
        };

        controller.normalised_parameter_to_plain = []V3_API(void* self, v3_param_id, double normalised) -> double
        {
            d_stdout("dpf_edit_controller::normalised_parameter_to_plain  => %s | %p %f", __PRETTY_FUNCTION__ + 97, self, normalised);
            return normalised;
        };

        controller.plain_parameter_to_normalised = []V3_API(void* self, v3_param_id, double normalised) -> double
        {
            d_stdout("dpf_edit_controller::plain_parameter_to_normalised  => %s | %p %f", __PRETTY_FUNCTION__ + 97, self, normalised);
            return normalised;
        };

        controller.get_parameter_normalised = []V3_API(void* self, v3_param_id param_idx) -> double
        {
            d_stdout("dpf_edit_controller::get_parameter_normalised       => %s | %p", __PRETTY_FUNCTION__ + 97, self);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, 0.0);

            PluginVst3* const vst3 = controller->vst3.get();
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, 0.0);
            DISTRHO_SAFE_ASSERT_RETURN(param_idx < vst3->getParameterCount(), 0.0);

            return vst3->getNormalizedParameterValue(param_idx);
        };

        controller.set_parameter_normalised = []V3_API(void* self, v3_param_id param_idx, double normalised) -> v3_result
        {
            d_stdout("dpf_edit_controller::set_parameter_normalised       => %s | %p %f", __PRETTY_FUNCTION__ + 97, self, normalised);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = controller->vst3.get();
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);
            DISTRHO_SAFE_ASSERT_RETURN(param_idx < vst3->getParameterCount(), V3_INVALID_ARG);

            vst3->setNormalizedParameterValue(param_idx, normalised);
            return V3_OK;
        };

        controller.set_component_handler = []V3_API(void* self, v3_component_handler**) -> v3_result
        {
            d_stdout("dpf_edit_controller::set_component_handler      => %s | %p", __PRETTY_FUNCTION__ + 97, self);
            return V3_NOT_IMPLEMENTED;
        };

        controller.create_view = []V3_API(void* self, const char* name) -> v3_plugin_view**
        {
            d_stdout("dpf_edit_controller::create_view                => %s | %p %s", __PRETTY_FUNCTION__ + 97, self, name);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, nullptr);

            if (controller->view == nullptr)
                controller->view = new dpf_plugin_view(controller->vst3);

            return (v3_plugin_view**)&controller->view;
        };
    }
};

// --------------------------------------------------------------------------------------------------------------------
// dpf_audio_processor

struct v3_audio_processor_cpp : v3_funknown {
    v3_audio_processor processor;
};

struct dpf_audio_processor : v3_audio_processor_cpp {
    ScopedPointer<PluginVst3>& vst3;

    dpf_audio_processor(ScopedPointer<PluginVst3>& v)
        : vst3(v)
    {
        static const uint8_t* kSupportedFactories[] = {
            v3_funknown_iid,
            v3_audio_processor_iid
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            d_stdout("dpf_audio_processor::query_interface         => %s | %p %s %p", __PRETTY_FUNCTION__ + 97, self, tuid2str(iid), iface);
            *iface = NULL;
            DISTRHO_SAFE_ASSERT_RETURN(self != nullptr, V3_NO_INTERFACE);

            for (const uint8_t* factory_iid : kSupportedFactories)
            {
                if (v3_tuid_match(factory_iid, iid))
                {
                    *iface = self;
                    return V3_OK;
                }
            }

            return V3_NO_INTERFACE;
        };

        // we only support 1 plugin per binary, so don't have to care here
        ref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_audio_processor::ref                     => %s | %p", __PRETTY_FUNCTION__ + 97, self);
            return 1;
        };

        unref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_audio_processor::unref                   => %s | %p", __PRETTY_FUNCTION__ + 97, self);
            return 0;
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_audio_processor

        processor.set_bus_arrangements = []V3_API(void* self,
                                                  v3_speaker_arrangement* inputs, int32_t num_inputs,
                                                  v3_speaker_arrangement* outputs, int32_t num_outputs) -> v3_result
        {
            d_stdout("dpf_audio_processor::set_bus_arrangements    => %s | %p %p %i %p %i", __PRETTY_FUNCTION__ + 97, self, inputs, num_inputs, outputs, num_outputs);
            return V3_NOT_IMPLEMENTED;
        };

        processor.get_bus_arrangement = []V3_API(void* self, int32_t bus_direction,
                                                 int32_t idx, v3_speaker_arrangement*) -> v3_result
        {
            d_stdout("dpf_audio_processor::get_bus_arrangement     => %s | %p %i %i", __PRETTY_FUNCTION__ + 97, self, bus_direction, idx);
            return V3_NOT_IMPLEMENTED;
        };

        processor.can_process_sample_size = []V3_API(void* self, int32_t symbolic_sample_size) -> v3_result
        {
            d_stdout("dpf_audio_processor::can_process_sample_size => %s | %p %i", __PRETTY_FUNCTION__ + 97, self, symbolic_sample_size);
            return symbolic_sample_size == V3_SAMPLE_32 ? V3_OK : V3_NOT_IMPLEMENTED;
        };

        processor.get_latency_samples = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_audio_processor::get_latency_samples     => %s | %p", __PRETTY_FUNCTION__ + 97, self);
#if DISTRHO_PLUGIN_WANT_LATENCY
            dpf_audio_processor* const processor = *(dpf_audio_processor**)self;
            DISTRHO_SAFE_ASSERT_RETURN(processor != nullptr, V3_NOT_INITIALISED);
            return processor->vst3->getLatencySamples();
#else
            return 0;
#endif
        };

        processor.setup_processing = []V3_API(void* self, v3_process_setup* setup) -> v3_result
        {
            d_stdout("dpf_audio_processor::setup_processing        => %s | %p", __PRETTY_FUNCTION__ + 97, self);
            d_lastBufferSize = setup->max_block_size;
            d_lastSampleRate = setup->sample_rate;

            dpf_audio_processor* const processor = *(dpf_audio_processor**)self;
            DISTRHO_SAFE_ASSERT_RETURN(processor != nullptr, V3_NOT_INITIALISED);
            DISTRHO_SAFE_ASSERT_RETURN(setup->symbolic_sample_size == V3_SAMPLE_32, V3_INVALID_ARG);
            processor->vst3->setupProcessing(setup);
            return V3_OK;
        };

        processor.set_processing = []V3_API(void* self, v3_bool state) -> v3_result
        {
            d_stdout("dpf_audio_processor::set_processing          => %s | %p %u", __PRETTY_FUNCTION__ + 97, self, state);
            dpf_audio_processor* const processor = *(dpf_audio_processor**)self;
            DISTRHO_SAFE_ASSERT_RETURN(processor != nullptr, V3_NOT_INITIALISED);
            processor->vst3->setProcessing(state);
            return V3_OK;
        };

        processor.process = []V3_API(void* self, v3_process_data* data) -> v3_result
        {
            // NOTE runs during RT
            // d_stdout("dpf_audio_processor::process                 => %s | %p", __PRETTY_FUNCTION__ + 97, self);
            dpf_audio_processor* const processor = *(dpf_audio_processor**)self;
            DISTRHO_SAFE_ASSERT_RETURN(processor != nullptr, V3_NOT_INITIALISED);
            DISTRHO_SAFE_ASSERT_RETURN(data->symbolic_sample_size == V3_SAMPLE_32, V3_INVALID_ARG);
            processor->vst3->process(data);
            return V3_OK;
        };

        processor.get_tail_samples = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_audio_processor::get_tail_samples        => %s | %p", __PRETTY_FUNCTION__ + 97, self);
            return 0;
        };
    }
};

// --------------------------------------------------------------------------------------------------------------------
// dpf_component

struct v3_component_cpp : v3_funknown {
    v3_plugin_base base;
    v3_component comp;
};

struct dpf_component : v3_component_cpp {
    std::atomic<int> refcounter;
    ScopedPointer<dpf_audio_processor> processor;
    ScopedPointer<dpf_edit_controller> controller;
    ScopedPointer<PluginVst3> vst3;

    dpf_component()
        : refcounter(1)
    {
        static const uint8_t* kSupportedBaseFactories[] = {
            v3_funknown_iid,
            v3_plugin_base_iid,
            v3_component_iid
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            d_stdout("dpf_component::query_interface         => %s | %p %s %p", __PRETTY_FUNCTION__ + 41, self, tuid2str(iid), iface);
            *iface = NULL;
            DISTRHO_SAFE_ASSERT_RETURN(self != nullptr, V3_NO_INTERFACE);

            for (const uint8_t* factory_iid : kSupportedBaseFactories)
            {
                if (v3_tuid_match(factory_iid, iid))
                {
                    *iface = self;
                    return V3_OK;
                }
            }

            if (v3_tuid_match(v3_audio_processor_iid, iid))
            {
                dpf_component* const component = *(dpf_component**)self;
                if (component->processor == nullptr)
                    component->processor = new dpf_audio_processor(component->vst3);
                *iface = &component->processor;
                return V3_OK;
            }

            if (v3_tuid_match(v3_edit_controller_iid, iid))
            {
                dpf_component* const component = *(dpf_component**)self;
                if (component->controller == nullptr)
                    component->controller = new dpf_edit_controller(component->vst3);
                *iface = &component->controller;
                return V3_OK;
            }

            return V3_NO_INTERFACE;
        };

        // we only support 1 plugin per binary, so don't have to care here
        ref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_component::ref                     => %s | %p", __PRETTY_FUNCTION__ + 41, self);
            dpf_component* const component = *(dpf_component**)self;
            return ++component->refcounter;
        };

        unref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_component::unref                   => %s | %p", __PRETTY_FUNCTION__ + 41, self);
            dpf_component* const component = *(dpf_component**)self;
            if (const int refcounter = --component->refcounter)
                return refcounter;
            // delete component;
            *(dpf_component**)self = nullptr;
            return 0;
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_plugin_base

        base.initialise = []V3_API(void* self, struct v3_plugin_base::v3_funknown* context) -> v3_result
        {
            d_stdout("dpf_component::initialise              => %s | %p %p", __PRETTY_FUNCTION__ + 41, self, context);
            dpf_component* const component = *(dpf_component**)self;

            // default early values
            if (d_lastBufferSize == 0)
                d_lastBufferSize = 2048;
            if (d_lastSampleRate <= 0.0)
                d_lastSampleRate = 44100.0;

            component->vst3 = new PluginVst3();
            return V3_OK;
        };

        base.terminate = []V3_API(void* self) -> v3_result
        {
            d_stdout("dpf_component::terminate               => %s | %p", __PRETTY_FUNCTION__ + 41, self);
            dpf_component* const component = *(dpf_component**)self;
            component->vst3 = nullptr;
            return V3_OK;
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_component

        comp.get_controller_class_id = []V3_API(void* self, v3_tuid class_id) -> v3_result
        {
            d_stdout("dpf_component::get_controller_class_id => %s | %p %s", __PRETTY_FUNCTION__ + 41, self, tuid2str(class_id));
            return V3_NOT_IMPLEMENTED;
        };

        comp.set_io_mode = []V3_API(void* self, int32_t io_mode) -> v3_result
        {
            d_stdout("dpf_component::set_io_mode             => %s | %p %i", __PRETTY_FUNCTION__ + 41, self, io_mode);
            return V3_NOT_IMPLEMENTED;
        };

        comp.get_bus_count = []V3_API(void* self, int32_t media_type, int32_t bus_direction) -> int32_t
        {
            // NOTE runs during RT
            // d_stdout("dpf_component::get_bus_count           => %s | %p %i %i", __PRETTY_FUNCTION__ + 41, self, media_type, bus_direction);

            switch (media_type)
            {
            case V3_AUDIO:
#if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
                if (bus_direction == V3_INPUT || bus_direction == V3_OUTPUT)
                {
                    dpf_component* const component = *(dpf_component**)self;
                    return component->vst3->getAudioBusCount(bus_direction == V3_INPUT);
                }
#endif
                break;

            case V3_EVENT:
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
                if (bus_direction == V3_INPUT)
                    return 1;
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
                if (bus_direction == V3_OUTPUT)
                    return 1;
#endif
                break;
            }

            return 0;
        };

        comp.get_bus_info = []V3_API(void* self, int32_t media_type, int32_t bus_direction,
                                     int32_t bus_idx, v3_bus_info* info) -> v3_result
        {
            d_stdout("dpf_component::get_bus_info            => %s | %p %i %i %i %p", __PRETTY_FUNCTION__ + 41, self, media_type, bus_direction, bus_idx, info);
            DISTRHO_SAFE_ASSERT_INT_RETURN(media_type == V3_AUDIO || media_type == V3_EVENT, media_type, V3_INVALID_ARG);
            DISTRHO_SAFE_ASSERT_INT_RETURN(bus_direction == V3_INPUT || bus_direction == V3_OUTPUT, bus_direction, V3_INVALID_ARG);
            DISTRHO_SAFE_ASSERT_INT_RETURN(bus_idx >= 0, bus_idx, V3_INVALID_ARG);

            if (media_type == V3_AUDIO)
            {
#if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
                dpf_component* const component = *(dpf_component**)self;
                return component->vst3->getBusAudioBusInfo(bus_direction == V3_INPUT,
                                                           static_cast<uint32_t>(bus_idx),
                                                           info);
#else
                return V3_INVALID_ARG;
#endif
            }
            else
            {
                if (bus_direction == V3_INPUT)
                {
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
                    DISTRHO_SAFE_ASSERT_RETURN(index == 0, V3_INVALID_ARG);
#else
                    return V3_INVALID_ARG;
#endif
                }
                else
                {
#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
                    DISTRHO_SAFE_ASSERT_RETURN(index == 0, V3_INVALID_ARG);
#else
                    return V3_INVALID_ARG;
#endif
                }
                info->media_type = V3_EVENT;
                info->direction = bus_direction;
                info->channel_count = 1;
                strncpy_16from8(info->bus_name, bus_direction == V3_INPUT ? "Event/MIDI Input"
                                                                          : "Event/MIDI Output", 128);
                info->bus_type = V3_MAIN;
                info->flags = V3_DEFAULT_ACTIVE;
                return V3_OK;
            }
        };

        comp.get_routing_info = []V3_API(void* self, v3_routing_info* input, v3_routing_info* output) -> v3_result
        {
            d_stdout("dpf_component::get_routing_info        => %s | %p %p %p", __PRETTY_FUNCTION__ + 41, self, input, output);
            return V3_NOT_IMPLEMENTED;
        };

        comp.activate_bus = []V3_API(void* self, int32_t media_type, int32_t bus_direction,
                                     int32_t bus_idx, v3_bool state) -> v3_result
        {
            d_stdout("dpf_component::activate_bus            => %s | %p %i %i %i %u", __PRETTY_FUNCTION__ + 41, self, media_type, bus_direction, bus_idx, state);
            return V3_NOT_IMPLEMENTED;
        };

        comp.set_active = []V3_API(void* self, v3_bool state) -> v3_result
        {
            d_stdout("dpf_component::set_active              => %s | %p %u", __PRETTY_FUNCTION__ + 41, self, state);
            dpf_component* const component = *(dpf_component**)self;
            component->vst3->setActive(state);
            return V3_OK;
        };

        comp.set_state = []V3_API(void* self, v3_bstream**) -> v3_result
        {
            d_stdout("dpf_component::set_state               => %s | %p", __PRETTY_FUNCTION__ + 41, self);
            return V3_NOT_IMPLEMENTED;
        };

        comp.get_state = []V3_API(void* self, v3_bstream**) -> v3_result
        {
            d_stdout("dpf_component::get_state               => %s | %p", __PRETTY_FUNCTION__ + 41, self);
            return V3_NOT_IMPLEMENTED;
        };
    }
};

// --------------------------------------------------------------------------------------------------------------------
// dpf_factory

struct v3_plugin_factory_cpp : v3_funknown {
    v3_plugin_factory   v1;
    v3_plugin_factory_2 v2;
    v3_plugin_factory_3 v3;
};

struct dpf_factory : v3_plugin_factory_cpp {
    std::vector<dpf_component*> components;

    dpf_factory()
    {
        static const uint8_t* kSupportedFactories[] = {
            v3_funknown_iid,
            v3_plugin_factory_iid,
            v3_plugin_factory_2_iid,
            v3_plugin_factory_3_iid
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            d_stdout("dpf_factory::query_interface      => %s | %p %s %p", __PRETTY_FUNCTION__ + 37, self, tuid2str(iid), iface);
            *iface = NULL;
            DISTRHO_SAFE_ASSERT_RETURN(self != nullptr, V3_NO_INTERFACE);

            for (const uint8_t* factory_iid : kSupportedFactories)
            {
                if (v3_tuid_match(factory_iid, iid))
                {
                    *iface = self;
                    return V3_OK;
                }
            }

            return V3_NO_INTERFACE;
        };

        // we only support 1 plugin per binary, so don't have to care here
        ref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_factory::ref                  => %s | %p", __PRETTY_FUNCTION__ + 37, self);
            return 1;
        };

        unref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_factory::unref                => %s | %p", __PRETTY_FUNCTION__ + 37, self);
            return 0;
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_plugin_factory

        v1.get_factory_info = []V3_API(void* self, struct v3_factory_info* const info) -> v3_result
        {
            d_stdout("dpf_factory::get_factory_info     => %s | %p %p", __PRETTY_FUNCTION__ + 37, self, info);
            DISTRHO_NAMESPACE::strncpy(info->vendor, gPluginInfo->getMaker(), sizeof(info->vendor));
            DISTRHO_NAMESPACE::strncpy(info->url, gPluginInfo->getHomePage(), sizeof(info->url));
            DISTRHO_NAMESPACE::strncpy(info->email, "", sizeof(info->email)); // TODO
            return V3_OK;
        };

        v1.num_classes = []V3_API(void* self) -> int32_t
        {
            d_stdout("dpf_factory::num_classes          => %s | %p", __PRETTY_FUNCTION__ + 37, self);
            return 1;
        };

        v1.get_class_info = []V3_API(void* self, int32_t idx, struct v3_class_info* const info) -> v3_result
        {
            d_stdout("dpf_factory::get_class_info       => %s | %p %i %p", __PRETTY_FUNCTION__ + 37, self, idx, info);
            DISTRHO_SAFE_ASSERT_RETURN(idx == 0, V3_NO_INTERFACE);

            memcpy(info->class_id, dpf_tuid_class, sizeof(v3_tuid));
            info->cardinality = 0x7FFFFFFF;
            DISTRHO_NAMESPACE::strncpy(info->category, "Audio Module Class", sizeof(info->category));
            DISTRHO_NAMESPACE::strncpy(info->name, gPluginInfo->getName(), sizeof(info->name));
            return V3_OK;
        };

        v1.create_instance = []V3_API(void* self, const v3_tuid class_id, const v3_tuid iid, void** instance) -> v3_result
        {
            d_stdout("dpf_factory::create_instance      => %s | %p %s %s %p", __PRETTY_FUNCTION__ + 37, self, tuid2str(class_id), tuid2str(iid), instance);
            DISTRHO_SAFE_ASSERT_RETURN(v3_tuid_match(class_id, *(v3_tuid*)&dpf_tuid_class) &&
                                       v3_tuid_match(iid, v3_component_iid), V3_NO_INTERFACE);

            dpf_factory* const factory = *(dpf_factory**)self;
            dpf_component* const component = new dpf_component();
            factory->components.push_back(component);
            *instance = &factory->components.back();
            return V3_OK;
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_plugin_factory_2

        v2.get_class_info_2 = []V3_API(void* self, int32_t idx, struct v3_class_info_2* info) -> v3_result
        {
            d_stdout("dpf_factory::get_class_info_2     => %s | %p %i %p", __PRETTY_FUNCTION__ + 37, self, idx, info);
            memcpy(info->class_id, dpf_tuid_class, sizeof(v3_tuid));
            info->cardinality = 0x7FFFFFFF;
            DISTRHO_NAMESPACE::strncpy(info->category, "Audio Module Class", sizeof(info->category));
            DISTRHO_NAMESPACE::strncpy(info->name, gPluginInfo->getName(), sizeof(info->name));

            info->class_flags = 0;
            DISTRHO_NAMESPACE::strncpy(info->sub_categories, "", sizeof(info->sub_categories)); // TODO
            DISTRHO_NAMESPACE::strncpy(info->vendor, gPluginInfo->getMaker(), sizeof(info->vendor));
            std::snprintf(info->version, sizeof(info->version), "%u", gPluginInfo->getVersion()); // TODO
            DISTRHO_NAMESPACE::strncpy(info->sdk_version, "Travesty", sizeof(info->sdk_version)); // TESTING use "VST 3.7" ?
            return V3_OK;
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_plugin_factory_3

        v3.get_class_info_utf16 = []V3_API(void* self, int32_t idx, struct v3_class_info_3* info) -> v3_result
        {
            d_stdout("dpf_factory::get_class_info_utf16 => %s | %p %i %p", __PRETTY_FUNCTION__ + 37, self, idx, info);
            memcpy(info->class_id, dpf_tuid_class, sizeof(v3_tuid));
            info->cardinality = 0x7FFFFFFF;
            DISTRHO_NAMESPACE::strncpy(info->category, "Audio Module Class", sizeof(info->category));
            DISTRHO_NAMESPACE::strncpy_16from8(info->name, gPluginInfo->getName(), sizeof(info->name));

            info->class_flags = 0;
            DISTRHO_NAMESPACE::strncpy(info->sub_categories, "", sizeof(info->sub_categories)); // TODO
            DISTRHO_NAMESPACE::strncpy_16from8(info->vendor, gPluginInfo->getMaker(), sizeof(info->vendor));
            // DISTRHO_NAMESPACE::snprintf16(info->version, sizeof(info->version)/sizeof(info->version[0]), "%u", gPluginInfo->getVersion()); // TODO
            DISTRHO_NAMESPACE::strncpy_16from8(info->sdk_version, "Travesty", sizeof(info->sdk_version)); // TESTING use "VST 3.7" ?
            return V3_OK;
        };

        v3.set_host_context = []V3_API (void* self, struct v3_funknown* host) -> v3_result
        {
            d_stdout("dpf_factory::set_host_context     => %s | %p %p", __PRETTY_FUNCTION__ + 37, self, host);
            return V3_NOT_IMPLEMENTED;
        };
    }
};

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
// VST3 entry point

DISTRHO_PLUGIN_EXPORT
const void* GetPluginFactory(void);

const void* GetPluginFactory(void)
{
    USE_NAMESPACE_DISTRHO;
    static const dpf_factory factory;
    static const v3_funknown* const factoryptr = &factory;
    return &factoryptr;
}

// --------------------------------------------------------------------------------------------------------------------
// OS specific module load

#if defined(DISTRHO_OS_MAC)
# define ENTRYFNNAME bundleEntry
# define EXITFNNAME bundleExit
#elif defined(DISTRHO_OS_WINDOWS)
# define ENTRYFNNAME InitDll
# define EXITFNNAME ExitDll
#else
# define ENTRYFNNAME ModuleEntry
# define EXITFNNAME ModuleExit
#endif

DISTRHO_PLUGIN_EXPORT
bool ENTRYFNNAME(void*);

bool ENTRYFNNAME(void*)
{
    USE_NAMESPACE_DISTRHO;

    if (gPluginInfo == nullptr)
    {
        d_lastBufferSize = 512;
        d_lastSampleRate = 44100.0;
        gPluginInfo = new PluginExporter(nullptr, nullptr, nullptr);
        d_lastBufferSize = 0;
        d_lastSampleRate = 0.0;

        dpf_tuid_class[2] = dpf_tuid_component[2] = dpf_tuid_controller[2]
            = dpf_tuid_processor[2] = dpf_tuid_view[2] = gPluginInfo->getUniqueId();
    }

    return true;
}

DISTRHO_PLUGIN_EXPORT
bool EXITFNNAME(void);

bool EXITFNNAME(void)
{
    USE_NAMESPACE_DISTRHO;
    gPluginInfo = nullptr;
    return true;
}

#undef ENTRYFNNAME
#undef EXITFNNAME

// --------------------------------------------------------------------------------------------------------------------
