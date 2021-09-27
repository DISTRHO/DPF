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

#if DISTRHO_PLUGIN_HAS_UI && ! DISTRHO_PLUGIN_HAS_EMBED_UI
# undef DISTRHO_PLUGIN_HAS_UI
# define DISTRHO_PLUGIN_HAS_UI 0
#endif

#if DISTRHO_PLUGIN_HAS_UI && ! defined(HAVE_DGL) && ! DISTRHO_PLUGIN_HAS_EXTERNAL_UI
# undef DISTRHO_PLUGIN_HAS_UI
# define DISTRHO_PLUGIN_HAS_UI 0
#endif

#if DISTRHO_PLUGIN_HAS_UI
# include "../extra/RingBuffer.hpp"
#endif

#include "travesty/audio_processor.h"
#include "travesty/component.h"
#include "travesty/edit_controller.h"
#include "travesty/factory.h"
#include "travesty/message.h"

#include <atomic>
#include <map>
#include <string>
#include <vector>

/* TODO items:
 * - base component refcount handling
 * - parameter enumeration as lists
 * - hide parameter outputs?
 * - hide program parameter?
 * - deal with parameter triggers
 * - midi cc parameter mapping
 * - full MIDI1 encode and decode
 * - decode version number (0x102030 -> 1.2.3)
 * - bus arrangements
 * - optional audio buses, create dummy buffer of max_block_size length for them
 * - routing info, do we care?
 * - set sidechain bus name from port group
 * - implement getParameterValueForString (use names from enumeration if available, fallback to std::atof)
 * - set factory class_flags
 * - set factory sub_categories
 * - set factory email (needs new DPF API, useful for LV2 as well)
 * - do something with get_controller_class_id and set_io_mode?
 * - replace dpf_message_create with host-side message creation
 */

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

#if ! DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
static constexpr const writeMidiFunc writeMidiCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
static constexpr const requestParameterValueChangeFunc requestParameterValueChangeCallback = nullptr;
#endif

typedef std::map<const String, String> StringMap;

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
// custom attribute list struct, used for sending utf8 strings

struct v3_attribute_list_utf8 {
    struct v3_funknown;
    V3_API v3_result (*set_string_utf8)(void* self, const char* id, const char* string);
    V3_API v3_result (*get_string_utf8)(void* self, const char* id, char* string, uint32_t size);
};

static constexpr const v3_tuid v3_attribute_list_utf8_iid =
    V3_ID(d_cconst('D','P','F',' '),
          d_cconst('c','l','a','s'),
          d_cconst('a','t','t','r'),
          d_cconst('u','t','f','8'));

// --------------------------------------------------------------------------------------------------------------------
// Utility functions

const char* tuid2str(const v3_tuid iid)
{
    static constexpr const struct {
        v3_tuid iid;
        const char* name;
    } extra_known_iids[] = {
        { V3_ID(0x00000000,0x00000000,0x00000000,0x00000000), "(nil)" },
        // edit-controller
        { V3_ID(0xF040B4B3,0xA36045EC,0xABCDC045,0xB4D5A2CC), "{v3_component_handler2|NOT}" },
        { V3_ID(0x7F4EFE59,0xF3204967,0xAC27A3AE,0xAFB63038), "{v3_edit_controller2|NOT}" },
        { V3_ID(0x067D02C1,0x5B4E274D,0xA92D90FD,0x6EAF7240), "{v3_component_handler_bus_activation|NOT}" },
        { V3_ID(0xDF0FF9F7,0x49B74669,0xB63AB732,0x7ADBF5E5), "{v3_midi_mapping|NOT}" },
        { V3_ID(0xC1271208,0x70594098,0xB9DD34B3,0x6BB0195E), "{v3_edit_controller_host_editing|NOT}" },
        // units
        { V3_ID(0x8683B01F,0x7B354F70,0xA2651DEC,0x353AF4FF), "{v3_program_list_data|NOT}" },
        { V3_ID(0x6C389611,0xD391455D,0xB870B833,0x94A0EFDD), "{v3_unit_data|NOT}" },
        { V3_ID(0x4B5147F8,0x4654486B,0x8DAB30BA,0x163A3C56), "{v3_unit_handler|NOT}" },
        { V3_ID(0xF89F8CDF,0x699E4BA5,0x96AAC9A4,0x81452B01), "{v3_unit_handler2|NOT}" },
        { V3_ID(0x3D4BD6B5,0x913A4FD2,0xA886E768,0xA5EB92C1), "{v3_unit_info|NOT}" },
        // misc
        { V3_ID(0x0F194781,0x8D984ADA,0xBBA0C1EF,0xC011D8D0), "{v3_info_listener|NOT}" },
    };

    if (v3_tuid_match(iid, v3_audio_processor_iid))
        return "{v3_audio_processor}";
    if (v3_tuid_match(iid, v3_attribute_list_iid))
        return "{v3_attribute_list_iid}";
    if (v3_tuid_match(iid, v3_attribute_list_utf8_iid))
        return "{v3_attribute_list_utf8_iid|CUSTOM}";
    if (v3_tuid_match(iid, v3_bstream_iid))
        return "{v3_bstream}";
    if (v3_tuid_match(iid, v3_component_iid))
        return "{v3_component}";
    if (v3_tuid_match(iid, v3_component_handler_iid))
        return "{v3_component_handler}";
    if (v3_tuid_match(iid, v3_connection_point_iid))
        return "{v3_connection_point_iid}";
    if (v3_tuid_match(iid, v3_edit_controller_iid))
        return "{v3_edit_controller}";
    if (v3_tuid_match(iid, v3_event_list_iid))
        return "{v3_event_list}";
    if (v3_tuid_match(iid, v3_funknown_iid))
        return "{v3_funknown}";
    if (v3_tuid_match(iid, v3_message_iid))
        return "{v3_message_iid}";
    if (v3_tuid_match(iid, v3_param_value_queue_iid))
        return "{v3_param_value_queue}";
    if (v3_tuid_match(iid, v3_param_changes_iid))
        return "{v3_param_changes}";
    if (v3_tuid_match(iid, v3_plugin_base_iid))
        return "{v3_plugin_base}";
    if (v3_tuid_match(iid, v3_plugin_factory_iid))
        return "{v3_plugin_factory}";
    if (v3_tuid_match(iid, v3_plugin_factory_2_iid))
        return "{v3_plugin_factory_2}";
    if (v3_tuid_match(iid, v3_plugin_factory_3_iid))
        return "{v3_plugin_factory_3}";
    if (v3_tuid_match(iid, v3_plugin_frame_iid))
        return "{v3_plugin_frame}";
    if (v3_tuid_match(iid, v3_plugin_view_iid))
        return "{v3_plugin_view}";
    if (v3_tuid_match(iid, v3_plugin_view_content_scale_iid))
        return "{v3_plugin_view_content_scale_iid}";
    if (v3_tuid_match(iid, v3_plugin_view_parameter_finder_iid))
        return "{v3_plugin_view_parameter_finder}";
    if (v3_tuid_match(iid, v3_process_context_requirements_iid))
        return "{v3_process_context_requirements}";

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

    for (size_t i=0; i<ARRAY_SIZE(extra_known_iids); ++i)
    {
        if (v3_tuid_match(iid, extra_known_iids[i].iid))
            return extra_known_iids[i].name;
    }

    static char buf[46];
    std::snprintf(buf, sizeof(buf), "{0x%08X,0x%08X,0x%08X,0x%08X}",
                  (uint32_t)d_cconst(iid[ 0], iid[ 1], iid[ 2], iid[ 3]),
                  (uint32_t)d_cconst(iid[ 4], iid[ 5], iid[ 6], iid[ 7]),
                  (uint32_t)d_cconst(iid[ 8], iid[ 9], iid[10], iid[11]),
                  (uint32_t)d_cconst(iid[12], iid[13], iid[14], iid[15]));
    return buf;
}

// --------------------------------------------------------------------------------------------------------------------

static void strncpy(char* const dst, const char* const src, const size_t length)
{
    DISTRHO_SAFE_ASSERT_RETURN(length > 0,);

    if (const size_t len = std::min(std::strlen(src), length-1U))
    {
        std::memcpy(dst, src, len);
        dst[len] = '\0';
    }
    else
    {
        dst[0] = '\0';
    }
}

void strncpy_utf16(int16_t* const dst, const char* const src, const size_t length)
{
    DISTRHO_SAFE_ASSERT_RETURN(length > 0,);

    if (const size_t len = std::min(std::strlen(src), length-1U))
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

template<typename T, const char* const format>
static void snprintf_t(char* const dst, const T value, const size_t size)
{
    DISTRHO_SAFE_ASSERT_RETURN(size > 0,);
    std::snprintf(dst, size-1, format, value);
    dst[size-1] = '\0';
}

template<typename T, const char* const format>
static void snprintf_utf16_t(int16_t* const dst, const T value, const size_t size)
{
    DISTRHO_SAFE_ASSERT_RETURN(size > 0,);

    char* const tmpbuf = (char*)std::malloc(size);
    DISTRHO_SAFE_ASSERT_RETURN(tmpbuf != nullptr,);

    std::snprintf(tmpbuf, size-1, format, value);
    tmpbuf[size-1] = '\0';

    strncpy_utf16(dst, tmpbuf, size);
    std::free(tmpbuf);
}

static constexpr const char format_i32[] = "%d";
static constexpr const char format_f32[] = "%f";
static constexpr const char format_u32[] = "%u";

static constexpr void (*const snprintf_u32)(char*, uint32_t, size_t) = snprintf_t<uint32_t, format_u32>;
static constexpr void (*const snprintf_f32_utf16)(int16_t*, float, size_t) = snprintf_utf16_t<float, format_f32>;
static constexpr void (*const snprintf_u32_utf16)(int16_t*, uint32_t, size_t) = snprintf_utf16_t<uint32_t, format_u32>;

// --------------------------------------------------------------------------------------------------------------------
// create message object (implementation comes later)

v3_message** dpf_message_create(const char* id);

// --------------------------------------------------------------------------------------------------------------------
// dpf_plugin_view_create (implemented on UI side)

v3_plugin_view** dpf_plugin_view_create(void* instancePointer, double sampleRate);

// --------------------------------------------------------------------------------------------------------------------

/**
 * VST3 DSP class.
 *
 * All the dynamic things from VST3 get implemented here, free of complex low-level VST3 pointer things.
 * The DSP is created during the "initialise" component event, and destroyed during "terminate".
 *
 * The low-level VST3 stuff comes after.
 */
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
        : fPlugin(this, writeMidiCallback, requestParameterValueChangeCallback),
          fComponentHandler(nullptr),
#if DISTRHO_PLUGIN_HAS_UI
          fConnection(nullptr),
#endif
          fParameterOffset(fPlugin.getParameterOffset()),
          fRealParameterCount(fParameterOffset + fPlugin.getParameterCount()),
          fParameterValues(nullptr)
#if DISTRHO_PLUGIN_HAS_UI
        , fConnectedToUI(false)
        , fChangedParameterValues(nullptr)
        , fNextSampleRate(0.0)
#endif
#if DISTRHO_PLUGIN_WANT_LATENCY
        , fLastKnownLatency(fPlugin.getLatency())
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        , fHostEventOutputHandle(nullptr)
#endif
#if DISTRHO_PLUGIN_WANT_PROGRAMS
        , fCurrentProgram(0)
        , fProgramCountMinusOne(fPlugin.getProgramCount()-1)
#endif
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

        if (const uint32_t parameterCount = fPlugin.getParameterCount())
        {
            fParameterValues = new float[parameterCount];

            for (uint32_t i=0; i < parameterCount; ++i)
                fParameterValues[i] = fPlugin.getParameterDefault(i);
        }

#if DISTRHO_PLUGIN_HAS_UI
        if (fRealParameterCount != 0)
        {
            fChangedParameterValues = new bool[fRealParameterCount];
            std::memset(fChangedParameterValues, 0, sizeof(bool)*fRealParameterCount);
        }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
        for (uint32_t i=0, count=fPlugin.getStateCount(); i<count; ++i)
        {
            const String& dkey(fPlugin.getStateKey(i));
            fStateMap[dkey] = fPlugin.getStateDefaultValue(i);
        }
#endif
    }

    ~PluginVst3()
    {
        if (fParameterValues != nullptr)
        {
            delete[] fParameterValues;
            fParameterValues = nullptr;
        }

#if DISTRHO_PLUGIN_HAS_UI
        if (fChangedParameterValues != nullptr)
        {
            delete[] fChangedParameterValues;
            fChangedParameterValues = nullptr;
        }
#endif
    }

    // ----------------------------------------------------------------------------------------------------------------
    // stuff called for UI creation

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

    int32_t getBusCount(const int32_t mediaType, const int32_t busDirection) const noexcept
    {
        switch (mediaType)
        {
        case V3_AUDIO:
            if (busDirection == V3_INPUT)
                return inputBuses.audio + inputBuses.sidechain + inputBuses.numCV;
            if (busDirection == V3_OUTPUT)
                return outputBuses.audio + outputBuses.sidechain + outputBuses.numCV;
            break;
        case V3_EVENT:
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
            if (busDirection == V3_INPUT)
                return 1;
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
            if (busDirection == V3_OUTPUT)
                return 1;
#endif
            break;
        }

        return 0;
    }

    v3_result getBusInfo(const int32_t mediaType,
                         const int32_t busDirection,
                         const int32_t busIndex,
                         v3_bus_info* const info) const
    {
        DISTRHO_SAFE_ASSERT_INT_RETURN(mediaType == V3_AUDIO || mediaType == V3_EVENT, mediaType, V3_INVALID_ARG);
        DISTRHO_SAFE_ASSERT_INT_RETURN(busDirection == V3_INPUT || busDirection == V3_OUTPUT, busDirection, V3_INVALID_ARG);
        DISTRHO_SAFE_ASSERT_INT_RETURN(busIndex >= 0, busIndex, V3_INVALID_ARG);

#if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0 || DISTRHO_PLUGIN_WANT_MIDI_INPUT || DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        const uint32_t busId = static_cast<uint32_t>(busIndex);
#endif

        if (mediaType == V3_AUDIO)
        {
           #if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
            int32_t numChannels;
            v3_bus_flags flags;
            v3_bus_types busType;
            v3_str_128 busName = {};

            if (busDirection == V3_INPUT)
            {
               #if DISTRHO_PLUGIN_NUM_INPUTS > 0
                switch (busId)
                {
                case 0:
                    if (inputBuses.audio)
                    {
                        numChannels = inputBuses.numMainAudio;
                        busType = V3_MAIN;
                        flags = V3_DEFAULT_ACTIVE;
                        break;
                    }
                // fall-through
                case 1:
                    if (inputBuses.sidechain)
                    {
                        numChannels = inputBuses.numSidechain;
                        busType = V3_AUX;
                        flags = v3_bus_flags(0);
                        break;
                    }
                // fall-through
                default:
                    numChannels = 1;
                    busType = V3_AUX;
                    flags = V3_IS_CONTROL_VOLTAGE;
                    break;
                }

                if (busType == V3_MAIN)
                {
                    strncpy_utf16(busName, "Audio Input", 128);
                }
                else
                {
                    for (uint32_t i=0; i<DISTRHO_PLUGIN_NUM_INPUTS; ++i)
                    {
                        const AudioPortWithBusId& port(fPlugin.getAudioPort(true, i));

                        // TODO find port group name for sidechain buses
                        if (port.busId == busId)
                        {
                            strncpy_utf16(busName, port.name, 128);
                            break;
                        }
                    }
                }
               #else
                return V3_INVALID_ARG;
               #endif // DISTRHO_PLUGIN_NUM_INPUTS
            }
            else
            {
               #if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
                switch (busId)
                {
                case 0:
                    if (outputBuses.audio)
                    {
                        numChannels = outputBuses.numMainAudio;
                        busType = V3_MAIN;
                        flags = V3_DEFAULT_ACTIVE;
                        break;
                    }
                // fall-through
                case 1:
                    if (outputBuses.sidechain)
                    {
                        numChannels = outputBuses.numSidechain;
                        busType = V3_AUX;
                        flags = v3_bus_flags(0);
                        break;
                    }
                // fall-through
                default:
                    numChannels = 1;
                    busType = V3_AUX;
                    flags = V3_IS_CONTROL_VOLTAGE;
                    break;
                }

                if (busType == V3_MAIN)
                {
                    strncpy_utf16(busName, "Audio Output", 128);
                }
                else
                {
                    for (uint32_t i=0; i<DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
                    {
                        const AudioPortWithBusId& port(fPlugin.getAudioPort(false, i));

                        // TODO find port group name for sidechain buses
                        if (port.busId == busId)
                        {
                            strncpy_utf16(busName, port.name, 128);
                            break;
                        }
                    }
                }
               #else
                return V3_INVALID_ARG;
               #endif // DISTRHO_PLUGIN_NUM_OUTPUTS
            }

            std::memset(info, 0, sizeof(v3_bus_info));
            info->media_type = V3_AUDIO;
            info->direction = busDirection;
            info->channel_count = numChannels;
            std::memcpy(info->bus_name, busName, sizeof(busName));
            info->bus_type = busType;
            info->flags = flags;
            return V3_OK;
           #else
            return V3_INVALID_ARG;
           #endif // DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS
        }
        else
        {
            if (busDirection == V3_INPUT)
            {
               #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
                DISTRHO_SAFE_ASSERT_RETURN(busId == 0, V3_INVALID_ARG);
               #else
                return V3_INVALID_ARG;
               #endif
            }
            else
            {
               #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
                DISTRHO_SAFE_ASSERT_RETURN(busId == 0, V3_INVALID_ARG);
               #else
                return V3_INVALID_ARG;
               #endif
            }
            info->media_type = V3_EVENT;
            info->direction = busDirection;
            info->channel_count = 1;
            strncpy_utf16(info->bus_name, busDirection == V3_INPUT ? "Event/MIDI Input"
                                                                   : "Event/MIDI Output", 128);
            info->bus_type = V3_MAIN;
            info->flags = V3_DEFAULT_ACTIVE;
            return V3_OK;
        }
    }

    v3_result getRoutingInfo(v3_routing_info*, v3_routing_info*)
    {
        // TODO
        return V3_NOT_IMPLEMENTED;
    }

    v3_result activateBus(const int32_t /* mediaType */,
                          const int32_t /* busDirection */,
                          const int32_t /* busIndex */,
                          const bool /* state */)
    {
        // TODO, returning ok to make bitwig happy
        return V3_OK;
    }

    v3_result setActive(const bool active)
    {
        if (active)
            fPlugin.activate();
        else
            fPlugin.deactivateIfNeeded();

        return V3_OK;
    }

    /* state: we pack pairs of key-value strings each separated by a null/zero byte.
     * current-program comes first, then dpf key/value states and then parameters.
     * parameters are simply converted to/from strings and floats.
     * the parameter symbol is used as the "key", so it is possible to reorder them or even remove and add safely.
     * there are markers for begin and end of state and parameters, so they never conflict.
     */
    v3_result setState(v3_bstream** const stream)
    {
#if DISTRHO_PLUGIN_HAS_UI
        const bool connectedToUI = fConnection != nullptr && fConnectedToUI;
#endif
        const uint32_t paramCount = fPlugin.getParameterCount();

        String key, value;
        bool fillingKey = true; // if filling key or value
        char queryingType = 'i'; // can be 'n', 's' or 'p' (none, states, parameters)

        char buffer[512], orig;
        buffer[sizeof(buffer)-1] = '\xff';
        v3_result res;

        for (int32_t pos = 0, term = 0, read; term == 0; pos += read)
        {
            res = v3_cpp_obj(stream)->read(stream, buffer, sizeof(buffer)-1, &read);
            DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);
            DISTRHO_SAFE_ASSERT_INT_RETURN(read > 0, read, V3_INTERNAL_ERR);

            for (int32_t i = 0; i < read; ++i)
            {
                // found terminator, stop here
                if (buffer[i] == '\xfe')
                {
                    term = 1;
                    break;
                }

                // store character at read position
                orig = buffer[read];

                // place null character to create valid string
                buffer[read] = '\0';

                // append to temporary vars
                if (fillingKey)
                    key += buffer + i;
                else
                    value += buffer + i;

                // increase buffer offset by length of string
                i += std::strlen(buffer + i);

                // restore read character
                buffer[read] = orig;

                // if buffer offset points to null, we found the end of a string, lets check
                if (buffer[i] == '\0')
                {
                    // special keys
                    if (key == "__dpf_state_begin__")
                    {
                        DISTRHO_SAFE_ASSERT_INT_RETURN(queryingType == 'i' || queryingType == 'n',
                                                       queryingType, V3_INTERNAL_ERR);
                        queryingType = 's';
                        key.clear();
                        value.clear();
                        continue;
                    }
                    if (key == "__dpf_state_end__")
                    {
                        DISTRHO_SAFE_ASSERT_INT_RETURN(queryingType == 's', queryingType, V3_INTERNAL_ERR);
                        queryingType = 'n';
                        key.clear();
                        value.clear();
                        continue;
                    }
                    if (key == "__dpf_parameters_begin__")
                    {
                        DISTRHO_SAFE_ASSERT_INT_RETURN(queryingType == 'i' || queryingType == 'n',
                                                       queryingType, V3_INTERNAL_ERR);
                        queryingType = 'p';
                        key.clear();
                        value.clear();
                        continue;
                    }
                    if (key == "__dpf_parameters_end__")
                    {
                        DISTRHO_SAFE_ASSERT_INT_RETURN(queryingType == 'p', queryingType, V3_INTERNAL_ERR);
                        queryingType = 'x';
                        key.clear();
                        value.clear();
                        continue;
                    }

                    // no special key, swap between reading real key and value
                    fillingKey = !fillingKey;

                    // if there is no value yet keep reading until we have one (TODO check empty values on purpose)
                    if (value.isEmpty())
                        continue;

                    if (key == "__dpf_program__")
                    {
                        DISTRHO_SAFE_ASSERT_INT_RETURN(queryingType == 'i', queryingType, V3_INTERNAL_ERR);
                        queryingType = 'n';

                        d_stdout("found program '%s'", value.buffer());

#if DISTRHO_PLUGIN_WANT_PROGRAMS
                        const int program = std::atoi(value.buffer());
                        DISTRHO_SAFE_ASSERT_CONTINUE(program >= 0);

                        fCurrentProgram = static_cast<uint32_t>(program);
                        fPlugin.loadProgram(fCurrentProgram);

# if DISTRHO_PLUGIN_HAS_UI
                        if (connectedToUI)
                        {
                            fChangedParameterValues[0] = false;
                            sendParameterChangeToUI(0, fCurrentProgram);
                        }
# endif
#endif
                    }
                    else if (queryingType == 's')
                    {
                        d_stdout("found state '%s' '%s'", key.buffer(), value.buffer());

#if DISTRHO_PLUGIN_WANT_STATE
                        if (fPlugin.wantStateKey(key))
                        {
                            fStateMap[key] = value;
                            fPlugin.setState(key, value);

# if DISTRHO_PLUGIN_HAS_UI
                            if (connectedToUI)
                                sendStateChangeToUI(key, value);
# endif
                        }
#endif
                    }
                    else if (queryingType == 'p')
                    {
                        d_stdout("found parameter '%s' '%s'", key.buffer(), value.buffer());

                        // find parameter with this symbol, and set its value
                        for (uint32_t j=0; j < paramCount; ++j)
                        {
                            if (fPlugin.isParameterOutputOrTrigger(j))
                                continue;
                            if (fPlugin.getParameterSymbol(j) != key)
                                continue;

                            const float fvalue = fParameterValues[i] = std::atof(value.buffer());
                            fPlugin.setParameterValue(j, fvalue);
#if DISTRHO_PLUGIN_HAS_UI
                            if (connectedToUI)
                            {
                                // UI parameter updates are handled outside the read loop (after host param restart)
                                fChangedParameterValues[j + fParameterOffset] = true;
                            }
#endif
                            break;
                        }

                    }

                    key.clear();
                    value.clear();
                }
            }
        }

        if (paramCount != 0)
        {
            if (fComponentHandler != nullptr)
                v3_cpp_obj(fComponentHandler)->restart_component(fComponentHandler, V3_RESTART_PARAM_VALUES_CHANGED);

#if DISTRHO_PLUGIN_HAS_UI
            if (connectedToUI)
            {
                for (uint32_t i=0; i < paramCount; ++i)
                {
                    if (fPlugin.isParameterOutputOrTrigger(i))
                        continue;
                    fChangedParameterValues[i + fParameterOffset] = false;
                    sendParameterChangeToUI(i + fParameterOffset, fParameterValues[i]);
                }
            }
#endif
        }

        return V3_OK;
    }

    v3_result getState(v3_bstream** const stream)
    {
        const uint32_t paramCount = fPlugin.getParameterCount();
#if DISTRHO_PLUGIN_WANT_STATE
        const uint32_t stateCount = fPlugin.getStateCount();
#else
        const uint32_t stateCount = 0;
#endif

        if (stateCount == 0 && paramCount == 0)
        {
            char buffer = '\0';
            int32_t ignored;
            return v3_cpp_obj(stream)->write(stream, &buffer, 1, &ignored);
        }

#if DISTRHO_PLUGIN_WANT_FULL_STATE
        // Update current state
        for (StringMap::const_iterator cit=fStateMap.begin(), cite=fStateMap.end(); cit != cite; ++cit)
        {
            const String& key = cit->first;
            fStateMap[key] = fPlugin.getState(key);
        }
#endif

        String state;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        {
            String tmpStr("__dpf_program__\xff");
            tmpStr += String(fCurrentProgram);
            tmpStr += "\xff";

            state += tmpStr;
        }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
        if (stateCount != 0)
        {
            state += "__dpf_state_begin__\xff";

            for (StringMap::const_iterator cit=fStateMap.begin(), cite=fStateMap.end(); cit != cite; ++cit)
            {
                const String& key   = cit->first;
                const String& value = cit->second;

                // join key and value
                String tmpStr;
                tmpStr  = key;
                tmpStr += "\xff";
                tmpStr += value;
                tmpStr += "\xff";

                state += tmpStr;
            }

            state += "__dpf_state_end__\xff";
        }
#endif

        if (paramCount != 0)
        {
            state += "__dpf_parameters_begin__\xff";

            for (uint32_t i=0; i<paramCount; ++i)
            {
                if (fPlugin.isParameterOutputOrTrigger(i))
                    continue;

                // join key and value
                String tmpStr;
                tmpStr  = fPlugin.getParameterSymbol(i);
                tmpStr += "\xff";
                tmpStr += String(fPlugin.getParameterValue(i));
                tmpStr += "\xff";

                state += tmpStr;
            }

            state += "__dpf_parameters_end__\xff";
        }

        // terminator
        state += "\xfe";

        state.replace('\xff', '\0');

        // now saving state, carefully until host written bytes matches full state size
        const char* buffer = state.buffer();
        const int32_t size = static_cast<int32_t>(state.length())+1;
        v3_result res;

        for (int32_t wrtntotal = 0, wrtn; wrtntotal < size; wrtntotal += wrtn)
        {
            wrtn = 0;
            res = v3_cpp_obj(stream)->write(stream, const_cast<char*>(buffer), size - wrtntotal, &wrtn);

            DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);
            DISTRHO_SAFE_ASSERT_INT_RETURN(wrtn > 0, wrtn, V3_INTERNAL_ERR);
        }

        return V3_OK;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_audio_processor interface calls

    v3_result setBusArrangements(v3_speaker_arrangement*, int32_t, v3_speaker_arrangement*, int32_t)
    {
        // TODO
        return V3_NOT_IMPLEMENTED;
    }

    v3_result getBusArrangement(int32_t, int32_t, v3_speaker_arrangement*)
    {
        // TODO
        return V3_NOT_IMPLEMENTED;
    }

    uint32_t getLatencySamples() const noexcept
    {
#if DISTRHO_PLUGIN_WANT_LATENCY
        return fPlugin.getLatency();
#else
        return 0;
#endif
    }

    v3_result setupProcessing(v3_process_setup* const setup)
    {
        DISTRHO_SAFE_ASSERT_RETURN(setup->symbolic_sample_size == V3_SAMPLE_32, V3_INVALID_ARG);

        const bool active = fPlugin.isActive();
        fPlugin.deactivateIfNeeded();

        // TODO process_mode can be V3_REALTIME, V3_PREFETCH, V3_OFFLINE

#if DISTRHO_PLUGIN_HAS_UI
        if (d_isNotEqual(setup->sample_rate, fPlugin.getSampleRate()))
            fNextSampleRate = setup->sample_rate;
#endif

        fPlugin.setSampleRate(setup->sample_rate, true);
        fPlugin.setBufferSize(setup->max_block_size, true);

        if (active)
            fPlugin.activate();

        // TODO create dummy buffer of max_block_size length, to use for disabled buses

        return V3_OK;
    }

    v3_result setProcessing(const bool processing)
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

        return V3_OK;
    }

    v3_result process(v3_process_data* const data)
    {
        DISTRHO_SAFE_ASSERT_RETURN(data->symbolic_sample_size == V3_SAMPLE_32, V3_INVALID_ARG);

        if (! fPlugin.isActive())
        {
            // host has not activated the plugin yet, nasty!
            fPlugin.activate();
        }

#if DISTRHO_PLUGIN_WANT_TIMEPOS
        if (v3_process_context* const ctx = data->ctx)
        {
            fTimePosition.playing   = ctx->state & V3_PROCESS_CTX_PLAYING;
            fTimePosition.bbt.valid = ctx->state & (V3_PROCESS_CTX_TEMPO_VALID|V3_PROCESS_CTX_TIME_SIG_VALID);

            // ticksPerBeat is not possible with VST2
            fTimePosition.bbt.ticksPerBeat = 1920.0;

            if (ctx->state & V3_PROCESS_CTX_CONT_TIME_VALID)
                fTimePosition.frame = ctx->continuous_time_in_samples;
            else
                fTimePosition.frame = ctx->project_time_in_samples;

            if (ctx->state & V3_PROCESS_CTX_TEMPO_VALID)
                fTimePosition.bbt.beatsPerMinute = ctx->bpm;
            else
                fTimePosition.bbt.beatsPerMinute = 120.0;

            if (ctx->state & (V3_PROCESS_CTX_PROJECT_TIME_VALID|V3_PROCESS_CTX_TIME_SIG_VALID))
            {
                const double ppqPos    = std::abs(ctx->project_time_quarters);
                const int    ppqPerBar = ctx->time_sig_numerator * 4 / ctx->time_sig_denom;
                const double barBeats  = (std::fmod(ppqPos, ppqPerBar) / ppqPerBar) * ctx->time_sig_numerator;
                const double rest      =  std::fmod(barBeats, 1.0);

                fTimePosition.bbt.bar         = static_cast<int32_t>(ppqPos) / ppqPerBar + 1;
                fTimePosition.bbt.beat        = static_cast<int32_t>(barBeats - rest + 0.5) + 1;
                fTimePosition.bbt.tick        = rest * fTimePosition.bbt.ticksPerBeat;
                fTimePosition.bbt.beatsPerBar = ctx->time_sig_numerator;
                fTimePosition.bbt.beatType    = ctx->time_sig_denom;

                if (ctx->project_time_quarters < 0.0)
                {
                    --fTimePosition.bbt.bar;
                    fTimePosition.bbt.beat = ctx->time_sig_numerator - fTimePosition.bbt.beat + 1;
                    fTimePosition.bbt.tick = fTimePosition.bbt.ticksPerBeat - fTimePosition.bbt.tick - 1;
                }
            }
            else
            {
                fTimePosition.bbt.bar         = 1;
                fTimePosition.bbt.beat        = 1;
                fTimePosition.bbt.tick        = 0.0;
                fTimePosition.bbt.beatsPerBar = 4.0f;
                fTimePosition.bbt.beatType    = 4.0f;
            }

            fTimePosition.bbt.barStartTick = fTimePosition.bbt.ticksPerBeat*
                                             fTimePosition.bbt.beatsPerBar*
                                             (fTimePosition.bbt.bar-1);

            fPlugin.setTimePosition(fTimePosition);
        }
#endif

        if (data->nframes <= 0)
        {
            updateParameterOutputsAndTriggers();
            return V3_OK;
        }

        const float* inputs[DISTRHO_PLUGIN_NUM_INPUTS != 0 ? DISTRHO_PLUGIN_NUM_INPUTS : 1];
        /* */ float* outputs[DISTRHO_PLUGIN_NUM_OUTPUTS != 0 ? DISTRHO_PLUGIN_NUM_OUTPUTS : 1];

        {
            int32_t i = 0;
            if (data->inputs != nullptr)
            {
                for (; i < data->inputs->num_channels; ++i)
                {
                    DISTRHO_SAFE_ASSERT_INT_BREAK(i < DISTRHO_PLUGIN_NUM_INPUTS, i);
                    inputs[i] = data->inputs->channel_buffers_32[i];
                }
            }
            for (; i < std::max(1, DISTRHO_PLUGIN_NUM_INPUTS); ++i)
                inputs[i] = nullptr; // TODO use dummy buffer
        }

        {
            int32_t i = 0;
            if (data->outputs != nullptr)
            {
                for (; i < data->outputs->num_channels; ++i)
                {
                    DISTRHO_SAFE_ASSERT_INT_BREAK(i < DISTRHO_PLUGIN_NUM_OUTPUTS, i);
                    outputs[i] = data->outputs->channel_buffers_32[i];
                }
            }
            for (; i < std::max(1, DISTRHO_PLUGIN_NUM_OUTPUTS); ++i)
                outputs[i] = nullptr; // TODO use dummy buffer
        }

#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        fHostEventOutputHandle = data->output_events;
#endif

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        uint32_t midiEventCount = 0;

# if DISTRHO_PLUGIN_HAS_UI
        while (fNotesRingBuffer.isDataAvailableForReading())
        {
            uint8_t midiData[3];
            if (! fNotesRingBuffer.readCustomData(midiData, 3))
                break;

            MidiEvent& midiEvent(fMidiEvents[midiEventCount++]);
            midiEvent.frame = 0;
            midiEvent.size  = 3;
            std::memcpy(midiEvent.data, midiData, 3);

            if (midiEventCount == kMaxMidiEvents)
                break;
        }
# endif

        if (v3_event_list** const eventptr = data->input_events)
        {
            v3_event event;
            for (uint32_t i = 0, count = v3_cpp_obj(eventptr)->get_event_count(eventptr); i < count; ++i)
            {
                if (v3_cpp_obj(eventptr)->get_event(eventptr, i, &event) != V3_OK)
                    break;

                // check if event can be encoded as MIDI
                switch (event.type)
                {
                case V3_EVENT_NOTE_ON:
                case V3_EVENT_NOTE_OFF:
                // case V3_EVENT_DATA:
                case V3_EVENT_POLY_PRESSURE:
                // case V3_EVENT_NOTE_EXP_VALUE:
                // case V3_EVENT_NOTE_EXP_TEXT:
                // case V3_EVENT_CHORD:
                // case V3_EVENT_SCALE:
                case V3_EVENT_LEGACY_MIDI_CC_OUT:
                    break;
                default:
                    continue;
                }

                MidiEvent& midiEvent(fMidiEvents[midiEventCount++]);
                midiEvent.frame = event.sample_offset;

                // encode event as MIDI
                switch (event.type)
                {
                case V3_EVENT_NOTE_ON:
                    midiEvent.size = 3;
                    midiEvent.data[0] = 0x90 | (event.note_on.channel & 0xf);
                    midiEvent.data[1] = event.note_on.pitch;
                    midiEvent.data[2] = std::max(0, std::min(127, (int)(event.note_on.velocity * 127)));
                    midiEvent.data[3] = 0;
                    break;
                case V3_EVENT_NOTE_OFF:
                    midiEvent.size = 3;
                    midiEvent.data[0] = 0x80 | (event.note_off.channel & 0xf);
                    midiEvent.data[1] = event.note_off.pitch;
                    midiEvent.data[2] = std::max(0, std::min(127, (int)(event.note_off.velocity * 127)));
                    midiEvent.data[3] = 0;
                    break;
                case V3_EVENT_POLY_PRESSURE:
                    midiEvent.size = 3;
                    midiEvent.data[0] = 0xA0 | (event.poly_pressure.channel & 0xf);
                    midiEvent.data[1] = event.poly_pressure.pitch;
                    midiEvent.data[2] = std::max(0, std::min(127, (int)(event.poly_pressure.pressure * 127)));
                    midiEvent.data[3] = 0;
                    break;
                case V3_EVENT_LEGACY_MIDI_CC_OUT:
                    midiEvent.size = 3;
                    midiEvent.data[0] = 0xB0 | (event.midi_cc_out.channel & 0xf);
                    midiEvent.data[1] = event.midi_cc_out.cc_number;
                    midiEvent.data[2] = event.midi_cc_out.value;
                    midiEvent.data[3] = 0;
                    // midiEvent.data[3] = event.midi_cc_out.value2; // TODO check when size should be 4
                    break;
                default:
                    midiEvent.size = 0;
                    break;
                }

                if (midiEventCount == kMaxMidiEvents)
                    break;
            }
        }

        fPlugin.run(inputs, outputs, data->nframes, fMidiEvents, midiEventCount);
#else
        fPlugin.run(inputs, outputs, data->nframes);
#endif

#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        fHostEventOutputHandle = nullptr;
#endif

        updateParameterOutputsAndTriggers();
        return V3_OK;
    }

    uint32_t getTailSamples() const noexcept
    {
        return 0;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_edit_controller interface calls

#if 0
    v3_result setComponentState(v3_bstream*, void*)
    {
        // TODO
        return V3_NOT_IMPLEMENTED;
    }

    v3_result setState(v3_bstream*, void*)
    {
        // TODO
        return V3_NOT_IMPLEMENTED;
    }

    v3_result getState(v3_bstream*, void*)
    {
        // TODO
        return V3_NOT_IMPLEMENTED;
    }
#endif

    int32_t getParameterCount() const noexcept
    {
        return fRealParameterCount;
    }

    v3_result getParameterInfo(const int32_t rindex, v3_param_info* const info) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(rindex >= 0, V3_INVALID_ARG);

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (rindex == 0)
        {
            std::memset(info, 0, sizeof(v3_param_info));
            info->param_id = rindex;
            info->flags = V3_PARAM_CAN_AUTOMATE | V3_PARAM_IS_LIST | V3_PARAM_PROGRAM_CHANGE;
            info->step_count = fProgramCountMinusOne;
            strncpy_utf16(info->title, "Current Program", 128);
            strncpy_utf16(info->short_title, "Program", 128);
            return V3_OK;
        }
#endif

        const uint32_t index = static_cast<uint32_t>(rindex) - fParameterOffset;
        DISTRHO_SAFE_ASSERT_UINT_RETURN(index < fPlugin.getParameterCount(), index, V3_INVALID_ARG);

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
        info->param_id = rindex;
        info->flags = flags;
        info->step_count = step_count;
        info->default_normalised_value = ranges.getNormalizedValue(ranges.def);
        // int32_t unit_id;
        strncpy_utf16(info->title,       fPlugin.getParameterName(index), 128);
        strncpy_utf16(info->short_title, fPlugin.getParameterShortName(index), 128);
        strncpy_utf16(info->units,       fPlugin.getParameterUnit(index), 128);
        return V3_OK;
    }

    v3_result getParameterStringForValue(const v3_param_id rindex, const double normalised, v3_str_128 output)
    {
        DISTRHO_SAFE_ASSERT_UINT_RETURN(rindex < fRealParameterCount, rindex, V3_INVALID_ARG);

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (rindex == 0)
        {
            DISTRHO_SAFE_ASSERT_RETURN(normalised >= 0.0 && normalised <= 1.0, V3_INVALID_ARG);

            const uint32_t program = std::round(normalised * fProgramCountMinusOne);
            strncpy_utf16(output, fPlugin.getProgramName(program), 128);
            return V3_OK;
        }
#endif

        const ParameterRanges& ranges(fPlugin.getParameterRanges(rindex - fParameterOffset));
        snprintf_f32_utf16(output, ranges.getUnnormalizedValue(normalised), 128);
        return V3_OK;
    }

    v3_result getParameterValueForString(const v3_param_id rindex, int16_t*, double*)
    {
        DISTRHO_SAFE_ASSERT_UINT_RETURN(rindex < fRealParameterCount, rindex, V3_INVALID_ARG);

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (rindex == 0)
        {
            // TODO find program index based on name
            return V3_NOT_IMPLEMENTED;
        }
#endif

        // TODO
        return V3_NOT_IMPLEMENTED;
    }

    double normalisedParameterToPlain(const v3_param_id rindex, const double normalised)
    {
        DISTRHO_SAFE_ASSERT_UINT_RETURN(rindex < fRealParameterCount, rindex, 0.0);

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (rindex == 0)
            return std::round(normalised * fProgramCountMinusOne);
#endif

        const ParameterRanges& ranges(fPlugin.getParameterRanges(rindex - fParameterOffset));
        return ranges.getUnnormalizedValue(normalised);
    }

    double plainParameterToNormalised(const v3_param_id rindex, const double plain)
    {
        DISTRHO_SAFE_ASSERT_UINT_RETURN(rindex < fRealParameterCount, rindex, 0.0);

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (rindex == 0)
            return std::max(0.0, std::min(1.0, plain / fProgramCountMinusOne));
#endif

        const ParameterRanges& ranges(fPlugin.getParameterRanges(rindex - fParameterOffset));
        return ranges.getNormalizedValue(plain);
    }

    double getParameterNormalized(const v3_param_id rindex)
    {
        DISTRHO_SAFE_ASSERT_UINT_RETURN(rindex < fRealParameterCount, rindex, 0.0);

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (rindex == 0)
            return std::max(0.0, std::min(1.0, (double)fCurrentProgram / fProgramCountMinusOne));
#endif

        const float value = fPlugin.getParameterValue(rindex - fParameterOffset);
        const ParameterRanges& ranges(fPlugin.getParameterRanges(rindex - fParameterOffset));
        return ranges.getNormalizedValue(value);
    }

    v3_result setParameterNormalized(const v3_param_id rindex, const double value)
    {
        DISTRHO_SAFE_ASSERT_UINT_RETURN(rindex < fRealParameterCount, rindex, V3_INVALID_ARG);
        DISTRHO_SAFE_ASSERT_RETURN(value >= 0.0 && value <= 1.0, V3_INVALID_ARG);

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (rindex == 0)
        {
            fCurrentProgram = std::round(value * fProgramCountMinusOne);
            fPlugin.loadProgram(fCurrentProgram);

            for (uint32_t i=0, count=fPlugin.getParameterCount(); i < count; ++i)
            {
                if (fPlugin.isParameterOutputOrTrigger(i))
                    continue;
                fParameterValues[i] = fPlugin.getParameterValue(i);
            }

            if (fComponentHandler != nullptr)
                v3_cpp_obj(fComponentHandler)->restart_component(fComponentHandler, V3_RESTART_PARAM_VALUES_CHANGED);
        }
        else
#endif
        {
            const uint32_t index = rindex - fParameterOffset;
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

            fParameterValues[index] = realValue;
            fPlugin.setParameterValue(index, realValue);
        }

#if DISTRHO_PLUGIN_HAS_UI
        fChangedParameterValues[rindex] = true;
#endif
        return V3_OK;
    }

    v3_result setComponentHandler(v3_component_handler** const handler) noexcept
    {
        fComponentHandler = handler;
        return V3_OK;
    }

#if DISTRHO_PLUGIN_HAS_UI
    // ----------------------------------------------------------------------------------------------------------------
    // v3_connection_point interface calls

    void connect(v3_connection_point** const other)
    {
        DISTRHO_SAFE_ASSERT(fConnectedToUI == false);

        fConnection = other;
        fConnectedToUI = false;
    }

    void disconnect()
    {
        fConnection = nullptr;
        fConnectedToUI = false;
    }

    v3_result notify(v3_message** const message)
    {
        const char* const msgid = v3_cpp_obj(message)->get_message_id(message);
        DISTRHO_SAFE_ASSERT_RETURN(msgid != nullptr, V3_INVALID_ARG);

        v3_attribute_list** const attrs = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrs != nullptr, V3_INVALID_ARG);

# if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if (std::strcmp(msgid, "midi") == 0)
        {
            uint8_t* data;
            uint32_t size;
            v3_result res;

            res = v3_cpp_obj(attrs)->get_binary(attrs, "data", (const void**)&data, &size);
            DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);

            // known maximum size
            DISTRHO_SAFE_ASSERT_UINT_RETURN(size == 3, size, V3_INTERNAL_ERR);

            return fNotesRingBuffer.writeCustomData(data, size) && fNotesRingBuffer.commitWrite() ? V3_OK : V3_NOMEM;
        }
# endif

        if (std::strcmp(msgid, "init") == 0)
        {
            DISTRHO_SAFE_ASSERT_RETURN(fConnection != nullptr, V3_INTERNAL_ERR);
            fConnectedToUI = true;

            if (const double sampleRate = fNextSampleRate)
            {
                fNextSampleRate = 0.0;
                sendSampleRateToUI(sampleRate);
            }

# if DISTRHO_PLUGIN_WANT_PROGRAMS
            fChangedParameterValues[0] = false;
            sendParameterChangeToUI(0, fCurrentProgram);
# endif

# if DISTRHO_PLUGIN_WANT_FULL_STATE
            // Update current state from plugin side
            for (StringMap::const_iterator cit=fStateMap.begin(), cite=fStateMap.end(); cit != cite; ++cit)
            {
                const String& key = cit->first;
                fStateMap[key] = fPlugin.getState(key);
            }
# endif

# if DISTRHO_PLUGIN_WANT_STATE
            // Set state
            for (StringMap::const_iterator cit=fStateMap.begin(), cite=fStateMap.end(); cit != cite; ++cit)
            {
                const String& key   = cit->first;
                const String& value = cit->second;

                sendStateChangeToUI(key, value);
            }
# endif

            for (uint32_t i=fParameterOffset; i<fRealParameterCount; ++i)
            {
                fChangedParameterValues[i] = false;
                sendParameterChangeToUI(i, fPlugin.getParameterValue(i - fParameterOffset));
            }

            sendReadyToUI();
            return V3_OK;
        }

        if (std::strcmp(msgid, "idle") == 0)
        {
            DISTRHO_SAFE_ASSERT_RETURN(fConnection != nullptr, V3_INTERNAL_ERR);
            DISTRHO_SAFE_ASSERT_RETURN(fConnectedToUI, V3_INTERNAL_ERR);

            for (uint32_t i=0; i<fRealParameterCount; ++i)
            {
                if (! fChangedParameterValues[i])
                    continue;

                fChangedParameterValues[i] = false;

# if DISTRHO_PLUGIN_WANT_PROGRAMS
                if (i == 0)
                    sendParameterChangeToUI(i, fCurrentProgram);
                else
# endif
                    sendParameterChangeToUI(i, fParameterValues[i - fParameterOffset]);
            }

            sendReadyToUI();
            return V3_OK;
        }

        if (std::strcmp(msgid, "close") == 0)
        {
            fConnectedToUI = false;
            DISTRHO_SAFE_ASSERT_RETURN(fConnection != nullptr, V3_INTERNAL_ERR);
            return V3_OK;
        }

        if (std::strcmp(msgid, "parameter-edit") == 0)
        {
            DISTRHO_SAFE_ASSERT_RETURN(fComponentHandler != nullptr, V3_INTERNAL_ERR);

            int64_t rindex;
            int64_t started;
            v3_result res;

            res = v3_cpp_obj(attrs)->get_int(attrs, "rindex", &rindex);
            DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);

            res = v3_cpp_obj(attrs)->get_int(attrs, "started", &started);
            DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);

            rindex -= fParameterOffset;
            DISTRHO_SAFE_ASSERT_RETURN(rindex >= 0, V3_INTERNAL_ERR);

            return started != 0 ? v3_cpp_obj(fComponentHandler)->begin_edit(fComponentHandler, rindex)
                                : v3_cpp_obj(fComponentHandler)->end_edit(fComponentHandler, rindex);
        }

        if (std::strcmp(msgid, "parameter-set") == 0)
        {
            int64_t rindex;
            double value;
            v3_result res;

            res = v3_cpp_obj(attrs)->get_int(attrs, "rindex", &rindex);
            DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);
            DISTRHO_SAFE_ASSERT_INT_RETURN(rindex >= fParameterOffset, rindex, V3_INTERNAL_ERR);

            res = v3_cpp_obj(attrs)->get_float(attrs, "value", &value);
            DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);

            const uint32_t index = static_cast<uint32_t>(rindex -= fParameterOffset);
            return requestParameterValueChange(index, value) ? V3_OK : V3_INTERNAL_ERR;
        }

# if DISTRHO_PLUGIN_WANT_STATE
        if (std::strcmp(msgid, "state-set") == 0)
        {
            int16_t* key16;
            int16_t* value16;
            uint32_t keySize, valueSize;
            v3_result res;

            res = v3_cpp_obj(attrs)->get_binary(attrs, "key", (const void**)&key16, &keySize);
            DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);

            res = v3_cpp_obj(attrs)->get_binary(attrs, "value", (const void**)&value16, &valueSize);
            DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);

            // do cheap inline conversion
            char* const key = (char*)key16;
            char* const value = (char*)value16;

            for (uint32_t i=0; i<keySize/sizeof(int16_t); ++i)
                key[i] = key16[i];
            for (uint32_t i=0; i<valueSize/sizeof(int16_t); ++i)
                value[i] = value16[i];

            fPlugin.setState(key, value);

            // save this key as needed
            if (fPlugin.wantStateKey(key))
            {
                for (StringMap::iterator it=fStateMap.begin(), ite=fStateMap.end(); it != ite; ++it)
                {
                    const String& dkey(it->first);

                    if (dkey == key)
                    {
                        it->second = value;
                        return V3_OK;
                    }
                }

                d_stderr("Failed to find plugin state with key \"%s\"", key);
            }

            return V3_OK;
        }
# endif

        return V3_NOT_IMPLEMENTED;
    }
#endif

    // ----------------------------------------------------------------------------------------------------------------

private:
    // Plugin
    PluginExporter fPlugin;

    // VST3 stuff
    v3_component_handler** fComponentHandler;
#if DISTRHO_PLUGIN_HAS_UI
    v3_connection_point** fConnection;
#endif

    // Temporary data
    const uint32_t fParameterOffset;
    const uint32_t fRealParameterCount; // regular parameters + current program
    float* fParameterValues;
#if DISTRHO_PLUGIN_HAS_UI
    bool fConnectedToUI;
    bool* fChangedParameterValues;
    double fNextSampleRate; // if not zero, report to UI
#endif
#if DISTRHO_PLUGIN_WANT_LATENCY
    uint32_t fLastKnownLatency;
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    MidiEvent fMidiEvents[kMaxMidiEvents];
# if DISTRHO_PLUGIN_HAS_UI
    SmallStackRingBuffer fNotesRingBuffer;
# endif
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    v3_event_list** fHostEventOutputHandle;
#endif
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    uint32_t fCurrentProgram;
    const uint32_t fProgramCountMinusOne;
#endif
#if DISTRHO_PLUGIN_WANT_STATE
    StringMap fStateMap;
#endif
#if DISTRHO_PLUGIN_WANT_TIMEPOS
    TimePosition fTimePosition;
#endif

    // ----------------------------------------------------------------------------------------------------------------
    // helper functions called during process, cannot block

    void updateParameterOutputsAndTriggers()
    {
        float curValue;

        for (uint32_t i=0, count=fPlugin.getParameterCount(); i < count; ++i)
        {
            if (fPlugin.isParameterOutput(i))
            {
                // NOTE: no output parameter support in VST3, simulate it here
                curValue = fPlugin.getParameterValue(i);

                if (d_isEqual(curValue, fParameterValues[i]))
                    continue;

                fParameterValues[i] = curValue;
#if DISTRHO_PLUGIN_HAS_UI
                fChangedParameterValues[i] = true;
#endif
            }
            else if (fPlugin.isParameterTrigger(i))
            {
                // NOTE: no trigger support in VST3 parameters, simulate it here
                curValue = fPlugin.getParameterValue(i);

                if (d_isEqual(curValue, fPlugin.getParameterDefault(i)))
                    continue;

                fPlugin.setParameterValue(i, curValue);
#if DISTRHO_PLUGIN_HAS_UI
                fChangedParameterValues[i] = true;
#endif
            }
            else
            {
                continue;
            }

            requestParameterValueChange(i, curValue);
        }

#if DISTRHO_PLUGIN_WANT_LATENCY
        const uint32_t latency = fPlugin.getLatency();

        if (fLastKnownLatency != latency)
        {
            fLastKnownLatency = latency;

            if (fComponentHandler != nullptr)
                v3_cpp_obj(fComponentHandler)->restart_component(fComponentHandler, V3_RESTART_LATENCY_CHANGED);
        }
#endif
    }

#if DISTRHO_PLUGIN_HAS_UI
    // ----------------------------------------------------------------------------------------------------------------
    // helper functions called during message passing, can block

    void sendSampleRateToUI(const double sampleRate)
    {
        v3_message** const message = dpf_message_create("sample-rate");
        DISTRHO_SAFE_ASSERT_RETURN(message != nullptr,);

        v3_attribute_list** const attrlist = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrlist != nullptr,);

        v3_cpp_obj(attrlist)->set_int(attrlist, "__dpf_msg_target__", 2);
        v3_cpp_obj(attrlist)->set_float(attrlist, "value", sampleRate);
        v3_cpp_obj(fConnection)->notify(fConnection, message);

        v3_cpp_obj_unref(message);
    }

    void sendParameterChangeToUI(const v3_param_id rindex, const double value)
    {
        v3_message** const message = dpf_message_create("parameter-set");
        DISTRHO_SAFE_ASSERT_RETURN(message != nullptr,);

        v3_attribute_list** const attrlist = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrlist != nullptr,);

        v3_cpp_obj(attrlist)->set_int(attrlist, "__dpf_msg_target__", 2);
        v3_cpp_obj(attrlist)->set_int(attrlist, "rindex", rindex);
        v3_cpp_obj(attrlist)->set_float(attrlist, "value", value);
        v3_cpp_obj(fConnection)->notify(fConnection, message);

        v3_cpp_obj_unref(message);
    }

    void sendStateChangeToUI(const char* const key, const char* const value)
    {
        v3_message** const message = dpf_message_create("state-set");
        DISTRHO_SAFE_ASSERT_RETURN(message != nullptr,);

        v3_attribute_list** const attrlist = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrlist != nullptr,);

        v3_attribute_list_utf8** utf8attrlist = nullptr;
        DISTRHO_SAFE_ASSERT_RETURN(v3_cpp_obj_query_interface(attrlist, v3_attribute_list_utf8_iid, &utf8attrlist) == V3_OK,);
        DISTRHO_SAFE_ASSERT_RETURN(utf8attrlist != nullptr,);

        v3_cpp_obj(attrlist)->set_int(attrlist, "__dpf_msg_target__", 2);
        v3_cpp_obj(utf8attrlist)->set_string_utf8(utf8attrlist, "key", key);
        v3_cpp_obj(utf8attrlist)->set_string_utf8(utf8attrlist, "value", value);
        v3_cpp_obj(fConnection)->notify(fConnection, message);

        v3_cpp_obj_unref(message);
    }

    void sendReadyToUI()
    {
        v3_message** const message = dpf_message_create("ready");
        DISTRHO_SAFE_ASSERT_RETURN(message != nullptr,);

        v3_attribute_list** const attrlist = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrlist != nullptr,);

        v3_cpp_obj(attrlist)->set_int(attrlist, "__dpf_msg_target__", 2);
        v3_cpp_obj(fConnection)->notify(fConnection, message);

        v3_cpp_obj_unref(message);
    }
#endif

    // ----------------------------------------------------------------------------------------------------------------
    // DPF callbacks

    bool requestParameterValueChange(const uint32_t index, const float value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fComponentHandler != nullptr, false);

        const uint32_t rindex = index + fParameterOffset;
        const double normalized = fPlugin.getParameterRanges(index).getNormalizedValue(value);

        const v3_result res_edit = v3_cpp_obj(fComponentHandler)->begin_edit(fComponentHandler, rindex);
        DISTRHO_SAFE_ASSERT_INT_RETURN(res_edit == V3_TRUE || res_edit == V3_FALSE, res_edit, res_edit);

        const v3_result res_perf = v3_cpp_obj(fComponentHandler)->perform_edit(fComponentHandler, rindex, normalized);

        if (res_perf == V3_TRUE)
            fParameterValues[index] = value;

        if (res_edit == V3_TRUE)
            v3_cpp_obj(fComponentHandler)->end_edit(fComponentHandler, rindex);

        return res_perf == V3_TRUE;
    }

#if DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
    static bool requestParameterValueChangeCallback(void* const ptr, const uint32_t index, const float value)
    {
        return ((PluginVst3*)ptr)->requestParameterValueChange(index, value);
    }
#endif

#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    bool writeMidi(const MidiEvent& midiEvent)
    {
        DISTRHO_CUSTOM_SAFE_ASSERT_ONCE_RETURN("MIDI output unsupported", fHostEventOutputHandle != nullptr, false);

        v3_event event;
        std::memset(&event, 0, sizeof(event));
        event.sample_offset = midiEvent.frame;

        const uint8_t* const data = midiEvent.size > MidiEvent::kDataSize ? midiEvent.dataExt : midiEvent.data;

        switch (data[0] & 0xf0)
        {
        case 0x80:
            event.type = V3_EVENT_NOTE_OFF;
            event.note_off.channel = data[0] & 0xf;
            event.note_off.pitch = data[1];
            event.note_off.velocity = (float)data[2] / 127.0f;
            // int32_t note_id;
            // float tuning;
            break;
        case 0x90:
            event.type = V3_EVENT_NOTE_ON;
            event.note_on.channel = data[0] & 0xf;
            event.note_on.pitch = data[1];
            // float tuning;
            event.note_on.velocity = (float)data[2] / 127.0f;
            // int32_t length;
            // int32_t note_id;
            break;
        case 0xA0:
            event.type = V3_EVENT_POLY_PRESSURE;
            event.poly_pressure.channel = data[0] & 0xf;
            event.poly_pressure.pitch = data[1];
            event.poly_pressure.pressure = (float)data[2] / 127.0f;
            // int32_t note_id;
            break;
        case 0xB0:
            event.type = V3_EVENT_LEGACY_MIDI_CC_OUT;
            event.midi_cc_out.channel = data[0] & 0xf;
            event.midi_cc_out.cc_number = data[1];
            event.midi_cc_out.value = data[2];
            event.midi_cc_out.value2 = midiEvent.size == 4 ? data[3] : 0;
            break;
        default:
            return true;
        }

        return v3_cpp_obj(fHostEventOutputHandle)->add_event(fHostEventOutputHandle, &event) == V3_OK;
    }

    static bool writeMidiCallback(void* ptr, const MidiEvent& midiEvent)
    {
        return ((PluginVst3*)ptr)->writeMidi(midiEvent);
    }
#endif

};

// --------------------------------------------------------------------------------------------------------------------

/**
 * VST3 low-level pointer thingies follow, proceed with care.
 */

// --------------------------------------------------------------------------------------------------------------------

struct dpf_attribute_value {
    char type; // one of: i, f, s, b
    union {
        int64_t integer;
        double v_float;
        int16_t* string;
        struct {
            void* ptr;
            uint32_t size;
        } binary;
    };
};

static void dpf_attribute_list_free(std::map<std::string, dpf_attribute_value>& attrs)
{
    for (auto& it : attrs)
    {
        dpf_attribute_value& v(it.second);

        switch (v.type)
        {
        case 's':
        case 'b':
            std::free(v.binary.ptr);
            break;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
// dpf_attribute_list_utf8 (the custom variant)

struct v3_attribute_list_utf8_cpp : v3_funknown {
    v3_attribute_list_utf8 attr;
};

struct dpf_attribute_list_utf8 : v3_attribute_list_utf8_cpp {
    std::map<std::string, dpf_attribute_value>& attrs;

    dpf_attribute_list_utf8(std::map<std::string, dpf_attribute_value>& a)
        : attrs(a)
    {
        static const uint8_t* kSupportedInterfaces[] = {
            v3_funknown_iid,
            v3_attribute_list_utf8_iid
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            d_stdout("dpf_attribute_list_utf8::query_interface => %p %s %p", self, tuid2str(iid), iface);
            *iface = NULL;
            DISTRHO_SAFE_ASSERT_RETURN(self != nullptr, V3_NO_INTERFACE);

            for (const uint8_t* interface_iid : kSupportedInterfaces)
            {
                if (v3_tuid_match(interface_iid, iid))
                {
                    *iface = self;
                    return V3_OK;
                }
            }

            return V3_NO_INTERFACE;
        };

        // there is only a single instance of this, so we don't have to care here
        ref = []V3_API(void*) -> uint32_t { return 1; };
        unref = []V3_API(void*) -> uint32_t { return 0; };

        // ------------------------------------------------------------------------------------------------------------
        // v3_attribute_list_utf8

        attr.set_string_utf8 = []V3_API(void* self, const char* id, const char* string) -> v3_result
        {
            dpf_attribute_list_utf8* const attr = *(dpf_attribute_list_utf8**)self;
            DISTRHO_SAFE_ASSERT_RETURN(attr != nullptr, V3_NOT_INITIALISED);

            const uint32_t size = std::strlen(string) + 1;
            int16_t* const copy = (int16_t*)std::malloc(sizeof(int16_t) * size);
            DISTRHO_SAFE_ASSERT_RETURN(copy != nullptr, V3_NOMEM);

            DISTRHO_NAMESPACE::strncpy_utf16(copy, string, size);

            dpf_attribute_value& attrval(attr->attrs[id]);
            attrval.type = 's';
            attrval.binary.ptr = copy;
            attrval.binary.size = sizeof(int16_t) * size;
            return V3_OK;
        };

        attr.get_string_utf8 = []V3_API(void* self, const char* id, char*, uint32_t) -> v3_result
        {
            dpf_attribute_list_utf8* const attr = *(dpf_attribute_list_utf8**)self;
            DISTRHO_SAFE_ASSERT_RETURN(attr != nullptr, V3_NOT_INITIALISED);

            if (attr->attrs.find(id) == attr->attrs.end())
                return V3_INVALID_ARG;

            return V3_NOT_IMPLEMENTED;
        };
    }
};

// --------------------------------------------------------------------------------------------------------------------
// dpf_attribute_list

struct dpf_attribute_list : v3_attribute_list_cpp {
    ScopedPointer<dpf_attribute_list_utf8> attrutf8;
    std::map<std::string, dpf_attribute_value> attrs;

    dpf_attribute_list()
    {
        static const uint8_t* kSupportedInterfacesBase[] = {
            v3_funknown_iid,
            v3_attribute_list_iid
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            // d_stdout("dpf_attribute_list::query_interface => %p %s %p", self, tuid2str(iid), iface);
            *iface = NULL;
            DISTRHO_SAFE_ASSERT_RETURN(self != nullptr, V3_NO_INTERFACE);

            for (const uint8_t* interface_iid : kSupportedInterfacesBase)
            {
                if (v3_tuid_match(interface_iid, iid))
                {
                    *iface = self;
                    return V3_OK;
                }
            }

            dpf_attribute_list* const attr = *(dpf_attribute_list**)self;
            DISTRHO_SAFE_ASSERT_RETURN(attr != nullptr, V3_NO_INTERFACE);

            if (v3_tuid_match(v3_attribute_list_utf8_iid, iid))
            {
                if (attr->attrutf8 == nullptr)
                    attr->attrutf8 = new dpf_attribute_list_utf8(attr->attrs);
                *iface = &attr->attrutf8;
                return V3_OK;
            }

            return V3_NO_INTERFACE;
        };

        // there is only a single instance of this, so we don't have to care here
        ref = []V3_API(void*) -> uint32_t { return 1; };
        unref = []V3_API(void*) -> uint32_t { return 0; };

        // ------------------------------------------------------------------------------------------------------------
        // v3_attribute_list

        attrlist.set_int = []V3_API(void* self, const char* id, int64_t value) -> v3_result
        {
            dpf_attribute_list* const attr = *(dpf_attribute_list**)self;
            DISTRHO_SAFE_ASSERT_RETURN(attr != nullptr, V3_NOT_INITIALISED);

            dpf_attribute_value& attrval(attr->attrs[id]);
            attrval.type = 'i';
            attrval.integer = value;
            return V3_OK;
        };

        attrlist.get_int = []V3_API(void* self, const char* id, int64_t* value) -> v3_result
        {
            dpf_attribute_list* const attr = *(dpf_attribute_list**)self;
            DISTRHO_SAFE_ASSERT_RETURN(attr != nullptr, V3_NOT_INITIALISED);

            if (attr->attrs.find(id) == attr->attrs.end())
                return V3_INVALID_ARG;

            const dpf_attribute_value& attrval(attr->attrs[id]);
            DISTRHO_SAFE_ASSERT_RETURN(attrval.type == 'i', V3_INVALID_ARG);

            *value = attrval.integer;
            return V3_OK;
        };

        attrlist.set_float = []V3_API(void* self, const char* id, double value) -> v3_result
        {
            dpf_attribute_list* const attr = *(dpf_attribute_list**)self;
            DISTRHO_SAFE_ASSERT_RETURN(attr != nullptr, V3_NOT_INITIALISED);

            dpf_attribute_value& attrval(attr->attrs[id]);
            attrval.type = 'f';
            attrval.v_float = value;
            return V3_OK;
        };

        attrlist.get_float = []V3_API(void* self, const char* id, double* value) -> v3_result
        {
            dpf_attribute_list* const attr = *(dpf_attribute_list**)self;
            DISTRHO_SAFE_ASSERT_RETURN(attr != nullptr, V3_NOT_INITIALISED);

            if (attr->attrs.find(id) == attr->attrs.end())
                return V3_INVALID_ARG;

            const dpf_attribute_value& attrval(attr->attrs[id]);
            DISTRHO_SAFE_ASSERT_RETURN(attrval.type == 'f', V3_INVALID_ARG);

            *value = attrval.v_float;
            return V3_OK;
        };

        attrlist.set_string = []V3_API(void* self, const char* id, const int16_t* /*string*/) -> v3_result
        {
            dpf_attribute_list* const attr = *(dpf_attribute_list**)self;
            DISTRHO_SAFE_ASSERT_RETURN(attr != nullptr, V3_NOT_INITIALISED);

            dpf_attribute_value& attrval(attr->attrs[id]);
            attrval.type = 's';
            attrval.binary.ptr = nullptr;
            attrval.binary.size = 0;
            return V3_NOT_IMPLEMENTED;
        };

        attrlist.get_string = []V3_API(void* self, const char* id, int16_t* /*string*/, uint32_t /*size*/) -> v3_result
        {
            dpf_attribute_list* const attr = *(dpf_attribute_list**)self;
            DISTRHO_SAFE_ASSERT_RETURN(attr != nullptr, V3_NOT_INITIALISED);

            if (attr->attrs.find(id) == attr->attrs.end())
                return V3_INVALID_ARG;

            const dpf_attribute_value& attrval(attr->attrs[id]);
            DISTRHO_SAFE_ASSERT_RETURN(attrval.type == 's', V3_INVALID_ARG);

            return V3_NOT_IMPLEMENTED;
        };

        attrlist.set_binary = []V3_API(void* self, const char* id, const void* data, uint32_t size) -> v3_result
        {
            dpf_attribute_list* const attr = *(dpf_attribute_list**)self;
            DISTRHO_SAFE_ASSERT_RETURN(attr != nullptr, V3_NOT_INITIALISED);

            void* const copy = std::malloc(size);
            DISTRHO_SAFE_ASSERT_RETURN(copy != nullptr, V3_NOMEM);

            std::memcpy(copy, data, size);

            dpf_attribute_value& attrval(attr->attrs[id]);
            attrval.type = 'b';
            attrval.binary.ptr = copy;
            attrval.binary.size = size;
            return V3_NOT_IMPLEMENTED;
        };

        attrlist.get_binary = []V3_API(void* self, const char* id, const void** data, uint32_t* size) -> v3_result
        {
            dpf_attribute_list* const attr = *(dpf_attribute_list**)self;
            DISTRHO_SAFE_ASSERT_RETURN(attr != nullptr, V3_NOT_INITIALISED);

            if (attr->attrs.find(id) == attr->attrs.end())
                return V3_INVALID_ARG;

            const dpf_attribute_value& attrval(attr->attrs[id]);
            DISTRHO_SAFE_ASSERT_RETURN(attrval.type == 's' || attrval.type == 'b', V3_INVALID_ARG);

            *data = attrval.binary.ptr;
            *size = attrval.binary.size;
            return V3_OK;
        };
    }
};

// --------------------------------------------------------------------------------------------------------------------
// dpf_message

struct dpf_message : v3_message_cpp {
    std::atomic<int> refcounter;
    ScopedPointer<dpf_message>* self;
    ScopedPointer<dpf_attribute_list> attrlist;
    String id;

    dpf_message(ScopedPointer<dpf_message>* const s, const char* const id2)
        : self(s),
          id(id2)
    {
        static const uint8_t* kSupportedInterfaces[] = {
            v3_funknown_iid,
            v3_message_iid
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            d_stdout("dpf_plugin_view::query_interface         => %p %s %p", self, tuid2str(iid), iface);
            *iface = NULL;
            DISTRHO_SAFE_ASSERT_RETURN(self != nullptr, V3_NO_INTERFACE);

            for (const uint8_t* interface_iid : kSupportedInterfaces)
            {
                if (v3_tuid_match(interface_iid, iid))
                {
                    *iface = self;
                    return V3_OK;
                }
            }

            return V3_NO_INTERFACE;
        };

        ref = []V3_API(void* const self) -> uint32_t
        {
            d_stdout("dpf_message::ref                     => %p", self);
            dpf_message** const messageptr = static_cast<dpf_message**>(self);
            DISTRHO_SAFE_ASSERT_RETURN(messageptr != nullptr, 0);
            dpf_message* const message = *messageptr;
            DISTRHO_SAFE_ASSERT_RETURN(message != nullptr, 0);

            return ++message->refcounter;
        };

        unref = []V3_API(void* const self) -> uint32_t
        {
            dpf_message** const messageptr = static_cast<dpf_message**>(self);
            DISTRHO_SAFE_ASSERT_RETURN(messageptr != nullptr, 0);
            dpf_message* const message = *messageptr;
            DISTRHO_SAFE_ASSERT_RETURN(message != nullptr, 0);

            if (const int refcounter = --message->refcounter)
                return refcounter;

            if (message->attrlist != nullptr)
                dpf_attribute_list_free(message->attrlist->attrs);

            *message->self = nullptr;
            delete messageptr;
            return 0;
        };

        msg.get_message_id = []V3_API(void* const self) -> const char*
        {
            dpf_message* const message = *(dpf_message**)self;
            DISTRHO_SAFE_ASSERT_RETURN(message != nullptr, nullptr);

            return message->id;
        };

        msg.set_message_id = []V3_API(void* const self, const char* const id) -> void
        {
            d_stdout("dpf_message::set_message_id => %p %s", self, id);
            dpf_message* const message = *(dpf_message**)self;
            DISTRHO_SAFE_ASSERT_RETURN(message != nullptr,);

            message->id = id;
        };

        msg.get_attributes = []V3_API(void* const self) -> v3_attribute_list**
        {
            dpf_message* const message = *(dpf_message**)self;
            DISTRHO_SAFE_ASSERT_RETURN(message != nullptr, nullptr);

            if (message->attrlist == nullptr)
                message->attrlist = new dpf_attribute_list();

            return (v3_attribute_list**)&message->attrlist;
        };
    }
};

v3_message** dpf_message_create(const char* const id)
{
    ScopedPointer<dpf_message>* const messageptr = new ScopedPointer<dpf_message>;
    *messageptr = new dpf_message(messageptr, id);
    return static_cast<v3_message**>(static_cast<void*>(messageptr));
}

#if DISTRHO_PLUGIN_HAS_UI
// --------------------------------------------------------------------------------------------------------------------
// dpf_dsp_connection_point

enum ConnectionPointType {
    kConnectionPointComponent,
    kConnectionPointController,
    kConnectionPointBridge
};

struct v3_connection_point_cpp : v3_funknown {
    v3_connection_point point;
};

struct dpf_dsp_connection_point : v3_connection_point_cpp {
    ScopedPointer<PluginVst3>& vst3;
    const ConnectionPointType type;
    v3_connection_point** other;
    v3_connection_point** bridge; // when type is controller this points to ctrl<->view point
    bool shortcircuit; // plugin as controller, should pass directly to view

    dpf_dsp_connection_point(const ConnectionPointType t, ScopedPointer<PluginVst3>& v)
        : vst3(v),
          type(t),
          other(nullptr),
          bridge(nullptr),
          shortcircuit(false)
    {
        static const uint8_t* kSupportedInterfaces[] = {
            v3_funknown_iid,
            v3_connection_point_iid
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            d_stdout("dpf_dsp_connection_point::query_interface => %p %s %p", self, tuid2str(iid), iface);
            *iface = NULL;
            DISTRHO_SAFE_ASSERT_RETURN(self != nullptr, V3_NO_INTERFACE);

            for (const uint8_t* interface_iid : kSupportedInterfaces)
            {
                if (v3_tuid_match(interface_iid, iid))
                {
                    *iface = self;
                    return V3_OK;
                }
            }

            return V3_NO_INTERFACE;
        };

        // there is only a single instance of this, so we don't have to care here
        ref = []V3_API(void*) -> uint32_t { return 1; };
        unref = []V3_API(void*) -> uint32_t { return 0; };

        // ------------------------------------------------------------------------------------------------------------
        // v3_connection_point

        point.connect = []V3_API(void* self, struct v3_connection_point** other) -> v3_result
        {
            d_stdout("dpf_dsp_connection_point::connect         => %p %p", self, other);
            dpf_dsp_connection_point* const point = *(dpf_dsp_connection_point**)self;
            DISTRHO_SAFE_ASSERT_RETURN(point != nullptr, V3_NOT_INITIALISED);
            DISTRHO_SAFE_ASSERT_RETURN(point->other == nullptr, V3_INVALID_ARG);
            DISTRHO_SAFE_ASSERT(point->bridge == nullptr);

            point->other = other;

            if (point->type == kConnectionPointComponent)
                if (PluginVst3* const vst3 = point->vst3)
                    vst3->connect((v3_connection_point**)self);

            return V3_OK;
        };

        point.disconnect = []V3_API(void* self, struct v3_connection_point** other) -> v3_result
        {
            d_stdout("dpf_dsp_connection_point::disconnect      => %p %p", self, other);
            dpf_dsp_connection_point* const point = *(dpf_dsp_connection_point**)self;
            DISTRHO_SAFE_ASSERT_RETURN(point != nullptr, V3_NOT_INITIALISED);
            DISTRHO_SAFE_ASSERT_RETURN(point->other != nullptr, V3_INVALID_ARG);

            point->other = nullptr;
            point->bridge = nullptr;

            if (point->type == kConnectionPointComponent)
                if (PluginVst3* const vst3 = point->vst3)
                    vst3->disconnect();

            return V3_OK;
        };

        point.notify = []V3_API(void* self, struct v3_message** message) -> v3_result
        {
            dpf_dsp_connection_point* const point = *(dpf_dsp_connection_point**)self;
            DISTRHO_SAFE_ASSERT_RETURN(point != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = point->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            v3_connection_point** const other = point->other;
            DISTRHO_SAFE_ASSERT_RETURN(other != nullptr, V3_NOT_INITIALISED);

            v3_attribute_list** const attrlist = v3_cpp_obj(message)->get_attributes(message);
            DISTRHO_SAFE_ASSERT_RETURN(attrlist != nullptr, V3_INVALID_ARG);

            int64_t target = 0;
            const v3_result res = v3_cpp_obj(attrlist)->get_int(attrlist, "__dpf_msg_target__", &target);
            DISTRHO_SAFE_ASSERT_RETURN(res == V3_OK, res);
            DISTRHO_SAFE_ASSERT_INT_RETURN(target == 1 || target == 2, target, V3_INTERNAL_ERR);

            switch (point->type)
            {
            // message belongs to component (aka plugin)
            case kConnectionPointComponent:
                if (target == 1)
                {
                    // view -> edit controller -> component
                    return vst3->notify(message);
                }
                else
                {
                    // message is from component to controller to view
                    return v3_cpp_obj(other)->notify(other, message);
                }

            // message belongs to edit controller
            case kConnectionPointController:
                if (target == 1)
                {
                    // we are in view<->dsp short-circuit, all happens in the controller without bridge
                    if (point->shortcircuit)
                        return vst3->notify(message);

                    // view -> edit controller -> component
                    return v3_cpp_obj(other)->notify(other, message);
                }
                else
                {
                    // we are in view<->dsp short-circuit, all happens in the controller without bridge
                    if (point->shortcircuit)
                        return v3_cpp_obj(other)->notify(other, message);

                    // message is from component to controller to view
                    v3_connection_point** const bridge = point->bridge;
                    DISTRHO_SAFE_ASSERT_RETURN(bridge != nullptr, V3_NOT_INITIALISED);
                    return v3_cpp_obj(bridge)->notify(bridge, message);
                }

            // message belongs to bridge (aka ui)
            case kConnectionPointBridge:
                if (target == 1)
                {
                    // view -> edit controller -> component
                    v3_connection_point** const bridge = point->bridge;
                    DISTRHO_SAFE_ASSERT_RETURN(bridge != nullptr, V3_NOT_INITIALISED);
                    return v3_cpp_obj(bridge)->notify(bridge, message);
                }
                else
                {
                    // message is from component to controller to view
                    return v3_cpp_obj(other)->notify(other, message);
                }
            }

            return V3_INTERNAL_ERR;
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
#if DISTRHO_PLUGIN_HAS_UI
    ScopedPointer<dpf_dsp_connection_point> connectionComp;   // kConnectionPointController
    ScopedPointer<dpf_dsp_connection_point> connectionBridge; // kConnectionPointBridge
#endif
    ScopedPointer<PluginVst3>& vst3;
    bool initialized;
    // cached values
    v3_component_handler** handler;

    dpf_edit_controller(ScopedPointer<PluginVst3>& v)
        : vst3(v),
          initialized(false),
          handler(nullptr)
    {
        static const uint8_t* kSupportedInterfacesBase[] = {
            v3_funknown_iid,
            v3_edit_controller_iid
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            d_stdout("dpf_edit_controller::query_interface            => %p %s %p", self, tuid2str(iid), iface);
            *iface = NULL;
            DISTRHO_SAFE_ASSERT_RETURN(self != nullptr, V3_NO_INTERFACE);

            for (const uint8_t* interface_iid : kSupportedInterfacesBase)
            {
                if (v3_tuid_match(interface_iid, iid))
                {
                    *iface = self;
                    return V3_OK;
                }
            }

#if DISTRHO_PLUGIN_HAS_UI
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, V3_NO_INTERFACE);

            if (v3_tuid_match(v3_connection_point_iid, iid))
            {
                if (controller->connectionComp == nullptr)
                    controller->connectionComp = new dpf_dsp_connection_point(kConnectionPointController,
                                                                              controller->vst3);
                *iface = &controller->connectionComp;
                return V3_OK;
            }
#endif

            return V3_NO_INTERFACE;
        };

        // there is only a single instance of this, so we don't have to care here
        ref = []V3_API(void*) -> uint32_t { return 1; };
        unref = []V3_API(void*) -> uint32_t { return 0; };

        // ------------------------------------------------------------------------------------------------------------
        // v3_plugin_base

        base.initialise = []V3_API(void* self, struct v3_plugin_base::v3_funknown *context) -> v3_result
        {
            d_stdout("dpf_edit_controller::initialise                 => %p %p", self, context);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, V3_NOT_INITIALISED);

            const bool initialized = controller->initialized;
            DISTRHO_SAFE_ASSERT_RETURN(! initialized, V3_INVALID_ARG);

            controller->initialized = true;
            return V3_OK;
        };

        base.terminate = []V3_API(void* self) -> v3_result
        {
            d_stdout("dpf_edit_controller::terminate                  => %p", self);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, V3_NOT_INITIALISED);

            const bool initialized = controller->initialized;
            DISTRHO_SAFE_ASSERT_RETURN(initialized, V3_INVALID_ARG);

            controller->initialized = false;
            return V3_OK;
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_edit_controller

        controller.set_component_state = []V3_API(void* self, v3_bstream* stream) -> v3_result
        {
            d_stdout("dpf_edit_controller::set_component_state        => %p %p", self, stream);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = controller->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

#if 0
            return vst3->setComponentState(stream);
#endif

            // TODO, returning ok to make renoise happy
            return V3_OK;
        };

        controller.set_state = []V3_API(void* self, v3_bstream* stream) -> v3_result
        {
            d_stdout("dpf_edit_controller::set_state                  => %p %p", self, stream);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = controller->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

#if 0
            return vst3->setState(stream);
#endif
            return V3_NOT_IMPLEMENTED;
        };

        controller.get_state = []V3_API(void* self, v3_bstream* stream) -> v3_result
        {
            d_stdout("dpf_edit_controller::get_state                  => %p %p", self, stream);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = controller->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

#if 0
            return vst3->getState(stream);
#endif
            return V3_NOT_IMPLEMENTED;
        };

        controller.get_parameter_count = []V3_API(void* self) -> int32_t
        {
            // d_stdout("dpf_edit_controller::get_parameter_count        => %p", self);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = controller->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return vst3->getParameterCount();
        };

        controller.get_parameter_info = []V3_API(void* self, int32_t param_idx, v3_param_info* param_info) -> v3_result
        {
            // d_stdout("dpf_edit_controller::get_parameter_info         => %p %i", self, param_idx);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = controller->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return vst3->getParameterInfo(param_idx, param_info);
        };

        controller.get_parameter_string_for_value = []V3_API(void* self, v3_param_id index, double normalised, v3_str_128 output) -> v3_result
        {
            // NOTE very noisy, called many times
            // d_stdout("dpf_edit_controller::get_parameter_string_for_value => %p %u %f %p", self, index, normalised, output);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = controller->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return vst3->getParameterStringForValue(index, normalised, output);
        };

        controller.get_parameter_value_for_string = []V3_API(void* self, v3_param_id index, int16_t* input, double* output) -> v3_result
        {
            d_stdout("dpf_edit_controller::get_parameter_value_for_string => %p %u %p %p", self, index, input, output);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = controller->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return vst3->getParameterValueForString(index, input, output);
        };

        controller.normalised_parameter_to_plain = []V3_API(void* self, v3_param_id index, double normalised) -> double
        {
            d_stdout("dpf_edit_controller::normalised_parameter_to_plain  => %p %u %f", self, index, normalised);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = controller->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return vst3->normalisedParameterToPlain(index, normalised);
        };

        controller.plain_parameter_to_normalised = []V3_API(void* self, v3_param_id index, double plain) -> double
        {
            d_stdout("dpf_edit_controller::plain_parameter_to_normalised  => %p %u %f", self, index, plain);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = controller->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return vst3->plainParameterToNormalised(index, plain);
        };

        controller.get_parameter_normalised = []V3_API(void* self, v3_param_id index) -> double
        {
            // NOTE very noisy, called many times
            // d_stdout("dpf_edit_controller::get_parameter_normalised       => %p %u", self, index);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, 0.0);

            PluginVst3* const vst3 = controller->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, 0.0);

            return vst3->getParameterNormalized(index);
        };

        controller.set_parameter_normalised = []V3_API(void* self, v3_param_id index, double normalised) -> v3_result
        {
            d_stdout("dpf_edit_controller::set_parameter_normalised       => %p %u %f", self, index, normalised);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = controller->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return vst3->setParameterNormalized(index, normalised);
        };

        controller.set_component_handler = []V3_API(void* self, v3_component_handler** handler) -> v3_result
        {
            d_stdout("dpf_edit_controller::set_component_handler      => %p %p", self, handler);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, V3_NOT_INITIALISED);

            controller->handler = handler;

            if (PluginVst3* const vst3 = controller->vst3)
                return vst3->setComponentHandler(handler);

            return V3_NOT_INITIALISED;
        };

        controller.create_view = []V3_API(void* self, const char* name) -> v3_plugin_view**
        {
            d_stdout("dpf_edit_controller::create_view                => %p %s", self, name);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, nullptr);

#if DISTRHO_PLUGIN_HAS_UI
            PluginVst3* const vst3 = controller->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, nullptr);

            // if there is a component connection, we require it to be active
            if (controller->connectionComp != nullptr)
            {
                DISTRHO_SAFE_ASSERT_RETURN(controller->connectionComp->other != nullptr, nullptr);
            }
            // otherwise short-circuit and deal with this ourselves (assume local usage)
            else
            {
                controller->connectionComp = new dpf_dsp_connection_point(kConnectionPointController,
                                                                          controller->vst3);
                controller->connectionComp->shortcircuit = true;
            }

            v3_plugin_view** const view = dpf_plugin_view_create(vst3->getInstancePointer(),
                                                                 vst3->getSampleRate());
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, nullptr);

            v3_connection_point** uiconn = nullptr;
            if (v3_cpp_obj_query_interface(view, v3_connection_point_iid, &uiconn) == V3_OK)
            {
                d_stdout("view connection query ok %p | shortcircuit %d",
                         uiconn, controller->connectionComp->shortcircuit);

                v3_connection_point** const ctrlconn = (v3_connection_point**)&controller->connectionComp;

                if (controller->connectionComp->shortcircuit)
                {
                    vst3->disconnect();

                    v3_cpp_obj(uiconn)->connect(uiconn, ctrlconn);
                    v3_cpp_obj(ctrlconn)->connect(ctrlconn, uiconn);

                    vst3->connect(ctrlconn);
                }
                else
                {
                    controller->connectionBridge = new dpf_dsp_connection_point(kConnectionPointBridge,
                                                                                controller->vst3);

                    v3_connection_point** const bridgeconn = (v3_connection_point**)&controller->connectionBridge;
                    v3_cpp_obj(uiconn)->connect(uiconn, bridgeconn);
                    v3_cpp_obj(bridgeconn)->connect(bridgeconn, uiconn);

                    controller->connectionComp->bridge = bridgeconn;
                    controller->connectionBridge->bridge = ctrlconn;
                }
            }

            return view;
#else
            return nullptr;
#endif
        };
    }
};

// --------------------------------------------------------------------------------------------------------------------
// dpf_process_context_requirements

struct v3_process_context_requirements_cpp : v3_funknown {
    v3_process_context_requirements req;
};

struct dpf_process_context_requirements : v3_process_context_requirements_cpp {
    dpf_process_context_requirements()
    {
        static const uint8_t* kSupportedInterfaces[] = {
            v3_funknown_iid,
            v3_process_context_requirements_iid
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            d_stdout("dpf_process_context_requirements::query_interface => %p %s %p", self, tuid2str(iid), iface);
            *iface = NULL;
            DISTRHO_SAFE_ASSERT_RETURN(self != nullptr, V3_NO_INTERFACE);

            for (const uint8_t* interface_iid : kSupportedInterfaces)
            {
                if (v3_tuid_match(interface_iid, iid))
                {
                    *iface = self;
                    return V3_OK;
                }
            }

            return V3_NO_INTERFACE;
        };

        // this is used statically, so we don't have to care here
        ref = []V3_API(void*) -> uint32_t { return 1; };
        unref = []V3_API(void*) -> uint32_t { return 0; };

        // ------------------------------------------------------------------------------------------------------------
        // v3_process_context_requirements

        req.get_process_context_requirements = []V3_API(void*) -> uint32_t
        {
#if DISTRHO_PLUGIN_WANT_TIMEPOS
            return 0x0
                |V3_PROCESS_CTX_NEED_CONTINUOUS_TIME  // V3_PROCESS_CTX_CONT_TIME_VALID
                |V3_PROCESS_CTX_NEED_PROJECT_TIME     // V3_PROCESS_CTX_PROJECT_TIME_VALID
                |V3_PROCESS_CTX_NEED_TEMPO            // V3_PROCESS_CTX_TEMPO_VALID
                |V3_PROCESS_CTX_NEED_TIME_SIG         // V3_PROCESS_CTX_TIME_SIG_VALID
                |V3_PROCESS_CTX_NEED_TRANSPORT_STATE; // V3_PROCESS_CTX_PLAYING
#else
            return 0x0;
#endif
        };
    }

    DISTRHO_PREVENT_HEAP_ALLOCATION
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
        static const uint8_t* kSupportedInterfacesBase[] = {
            v3_funknown_iid,
            v3_audio_processor_iid
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            d_stdout("dpf_audio_processor::query_interface         => %p %s %p", self, tuid2str(iid), iface);
            *iface = NULL;
            DISTRHO_SAFE_ASSERT_RETURN(self != nullptr, V3_NO_INTERFACE);

            for (const uint8_t* interface_iid : kSupportedInterfacesBase)
            {
                if (v3_tuid_match(interface_iid, iid))
                {
                    *iface = self;
                    return V3_OK;
                }
            }

            if (v3_tuid_match(v3_process_context_requirements_iid, iid))
            {
                static dpf_process_context_requirements context_req;
                static dpf_process_context_requirements* context_req_ptr = &context_req;;
                *iface = &context_req_ptr;
                return V3_OK;
            }

            return V3_NO_INTERFACE;
        };

        // there is only a single instance of this, so we don't have to care here
        ref = []V3_API(void*) -> uint32_t { return 1; };
        unref = []V3_API(void*) -> uint32_t { return 0; };

        // ------------------------------------------------------------------------------------------------------------
        // v3_audio_processor

        processor.set_bus_arrangements = []V3_API(void* self,
                                                  v3_speaker_arrangement* inputs, int32_t num_inputs,
                                                  v3_speaker_arrangement* outputs, int32_t num_outputs) -> v3_result
        {
            // NOTE this is called a bunch of times
            // d_stdout("dpf_audio_processor::set_bus_arrangements    => %p %p %i %p %i", self, inputs, num_inputs, outputs, num_outputs);
            dpf_audio_processor* const processor = *(dpf_audio_processor**)self;
            DISTRHO_SAFE_ASSERT_RETURN(processor != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = processor->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return processor->vst3->setBusArrangements(inputs, num_inputs, outputs, num_outputs);
        };

        processor.get_bus_arrangement = []V3_API(void* self, int32_t bus_direction,
                                                 int32_t idx, v3_speaker_arrangement* arr) -> v3_result
        {
            d_stdout("dpf_audio_processor::get_bus_arrangement     => %p %i %i %p", self, bus_direction, idx, arr);
            dpf_audio_processor* const processor = *(dpf_audio_processor**)self;
            DISTRHO_SAFE_ASSERT_RETURN(processor != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = processor->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return processor->vst3->getBusArrangement(bus_direction, idx, arr);
        };

        processor.can_process_sample_size = []V3_API(void* self, int32_t symbolic_sample_size) -> v3_result
        {
            d_stdout("dpf_audio_processor::can_process_sample_size => %p %i", self, symbolic_sample_size);
            return symbolic_sample_size == V3_SAMPLE_32 ? V3_OK : V3_NOT_IMPLEMENTED;
        };

        processor.get_latency_samples = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_audio_processor::get_latency_samples     => %p", self);
            dpf_audio_processor* const processor = *(dpf_audio_processor**)self;
            DISTRHO_SAFE_ASSERT_RETURN(processor != nullptr, 0);

            PluginVst3* const vst3 = processor->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, 0);

            return processor->vst3->getLatencySamples();
        };

        processor.setup_processing = []V3_API(void* self, v3_process_setup* setup) -> v3_result
        {
            d_stdout("dpf_audio_processor::setup_processing        => %p", self);
            dpf_audio_processor* const processor = *(dpf_audio_processor**)self;
            DISTRHO_SAFE_ASSERT_RETURN(processor != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = processor->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            d_lastBufferSize = setup->max_block_size;
            d_lastSampleRate = setup->sample_rate;
            return processor->vst3->setupProcessing(setup);
        };

        processor.set_processing = []V3_API(void* self, v3_bool state) -> v3_result
        {
            d_stdout("dpf_audio_processor::set_processing          => %p %u", self, state);
            dpf_audio_processor* const processor = *(dpf_audio_processor**)self;
            DISTRHO_SAFE_ASSERT_RETURN(processor != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = processor->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return processor->vst3->setProcessing(state);
        };

        processor.process = []V3_API(void* self, v3_process_data* data) -> v3_result
        {
            // NOTE runs during RT
            // d_stdout("dpf_audio_processor::process                 => %p", self);
            dpf_audio_processor* const processor = *(dpf_audio_processor**)self;
            DISTRHO_SAFE_ASSERT_RETURN(processor != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = processor->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return processor->vst3->process(data);
        };

        processor.get_tail_samples = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_audio_processor::get_tail_samples        => %p", self);
            dpf_audio_processor* const processor = *(dpf_audio_processor**)self;
            DISTRHO_SAFE_ASSERT_RETURN(processor != nullptr, 0);

            PluginVst3* const vst3 = processor->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, 0);

            return processor->vst3->getTailSamples();
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
    ScopedPointer<dpf_component>* self;
    ScopedPointer<dpf_audio_processor> processor;
#if DISTRHO_PLUGIN_HAS_UI
    ScopedPointer<dpf_dsp_connection_point> connection; // kConnectionPointComponent
#endif
    ScopedPointer<dpf_edit_controller> controller;
    ScopedPointer<PluginVst3> vst3;

    dpf_component(ScopedPointer<dpf_component>* const s)
        : refcounter(1),
          self(s)
    {
        static const uint8_t* kSupportedInterfacesBase[] = {
            v3_funknown_iid,
            v3_plugin_base_iid,
            v3_component_iid
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            d_stdout("dpf_component::query_interface         => %p %s %p", self, tuid2str(iid), iface);
            *iface = NULL;

            for (const uint8_t* interface_iid : kSupportedInterfacesBase)
            {
                if (v3_tuid_match(interface_iid, iid))
                {
                    *iface = self;
                    return V3_OK;
                }
            }

            dpf_component* const component = *(dpf_component**)self;
            DISTRHO_SAFE_ASSERT_RETURN(component != nullptr, V3_NO_INTERFACE);

            if (v3_tuid_match(v3_audio_processor_iid, iid))
            {
                if (component->processor == nullptr)
                    component->processor = new dpf_audio_processor(component->vst3);
                *iface = &component->processor;
                return V3_OK;
            }

#if DISTRHO_PLUGIN_HAS_UI
            if (v3_tuid_match(v3_connection_point_iid, iid))
            {
                if (component->connection == nullptr)
                    component->connection = new dpf_dsp_connection_point(kConnectionPointComponent,
                                                                         component->vst3);
                *iface = &component->connection;
                return V3_OK;
            }
#endif

            if (v3_tuid_match(v3_edit_controller_iid, iid))
            {
                if (component->controller == nullptr)
                    component->controller = new dpf_edit_controller(component->vst3);
                *iface = &component->controller;
                return V3_OK;
            }

            return V3_NO_INTERFACE;
        };

#if 1
        // TODO fix this up later
        ref = []V3_API(void*) -> uint32_t { return 1; };
        unref = []V3_API(void*) -> uint32_t { return 0; };
#else
        ref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_component::ref                     => %p", self);
            dpf_component* const component = *(dpf_component**)self;
            DISTRHO_SAFE_ASSERT_RETURN(component != nullptr, 0);

            return ++component->refcounter;
        };

        unref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_component::unref                   => %p", self);
            dpf_component* const component = *(dpf_component**)self;
            DISTRHO_SAFE_ASSERT_RETURN(component != nullptr, 0);

            if (const int refcount = --component->refcounter)
            {
                d_stdout("dpf_component::unref                   => %p | refcount %i", self, refcount);
                return refcount;
            }

            d_stdout("dpf_component::unref                   => %p | refcount is zero, deleting everything now!", self);
            *component->self = nullptr;
            delete (dpf_component**)self;
            return 0;
        };
#endif

        // ------------------------------------------------------------------------------------------------------------
        // v3_plugin_base

        base.initialise = []V3_API(void* self, struct v3_plugin_base::v3_funknown* context) -> v3_result
        {
            d_stdout("dpf_component::initialise              => %p %p", self, context);
            dpf_component* const component = *(dpf_component**)self;
            DISTRHO_SAFE_ASSERT_RETURN(component != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = component->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 == nullptr, V3_INVALID_ARG);

            d_lastCanRequestParameterValueChanges = true;

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
            d_stdout("dpf_component::terminate               => %p", self);
            dpf_component* const component = *(dpf_component**)self;
            DISTRHO_SAFE_ASSERT_RETURN(component != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = component->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_INVALID_ARG);

            component->vst3 = nullptr;
            return V3_OK;
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_component

        comp.get_controller_class_id = []V3_API(void* self, v3_tuid class_id) -> v3_result
        {
            d_stdout("dpf_component::get_controller_class_id => %p %s", self, tuid2str(class_id));
            dpf_component* const component = *(dpf_component**)self;
            DISTRHO_SAFE_ASSERT_RETURN(component != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = component->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            // TODO
            return V3_NOT_IMPLEMENTED;
        };

        comp.set_io_mode = []V3_API(void* self, int32_t io_mode) -> v3_result
        {
            d_stdout("dpf_component::set_io_mode             => %p %i", self, io_mode);
            dpf_component* const component = *(dpf_component**)self;
            DISTRHO_SAFE_ASSERT_RETURN(component != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = component->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            // TODO
            return V3_NOT_IMPLEMENTED;
        };

        comp.get_bus_count = []V3_API(void* self, int32_t media_type, int32_t bus_direction) -> int32_t
        {
            // NOTE runs during RT
            // d_stdout("dpf_component::get_bus_count           => %p %i %i", self, media_type, bus_direction);
            dpf_component* const component = *(dpf_component**)self;
            DISTRHO_SAFE_ASSERT_RETURN(component != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = component->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return vst3->getBusCount(media_type, bus_direction);
        };

        comp.get_bus_info = []V3_API(void* self, int32_t media_type, int32_t bus_direction,
                                     int32_t bus_idx, v3_bus_info* info) -> v3_result
        {
            d_stdout("dpf_component::get_bus_info            => %p %i %i %i %p", self, media_type, bus_direction, bus_idx, info);
            dpf_component* const component = *(dpf_component**)self;
            DISTRHO_SAFE_ASSERT_RETURN(component != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = component->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return vst3->getBusInfo(media_type, bus_direction, bus_idx, info);
        };

        comp.get_routing_info = []V3_API(void* self, v3_routing_info* input, v3_routing_info* output) -> v3_result
        {
            d_stdout("dpf_component::get_routing_info        => %p %p %p", self, input, output);
            dpf_component* const component = *(dpf_component**)self;
            DISTRHO_SAFE_ASSERT_RETURN(component != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = component->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return vst3->getRoutingInfo(input, output);
        };

        comp.activate_bus = []V3_API(void* self, int32_t media_type, int32_t bus_direction,
                                     int32_t bus_idx, v3_bool state) -> v3_result
        {
            // NOTE this is called a bunch of times
            // d_stdout("dpf_component::activate_bus            => %p %i %i %i %u", self, media_type, bus_direction, bus_idx, state);
            dpf_component* const component = *(dpf_component**)self;
            DISTRHO_SAFE_ASSERT_RETURN(component != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = component->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return vst3->activateBus(media_type, bus_direction, bus_idx, state);
        };

        comp.set_active = []V3_API(void* self, v3_bool state) -> v3_result
        {
            d_stdout("dpf_component::set_active              => %p %u", self, state);
            dpf_component* const component = *(dpf_component**)self;
            DISTRHO_SAFE_ASSERT_RETURN(component != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = component->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return component->vst3->setActive(state);
        };

        comp.set_state = []V3_API(void* self, v3_bstream** stream) -> v3_result
        {
            d_stdout("dpf_component::set_state               => %p", self);
            dpf_component* const component = *(dpf_component**)self;
            DISTRHO_SAFE_ASSERT_RETURN(component != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = component->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return vst3->setState(stream);
        };

        comp.get_state = []V3_API(void* self, v3_bstream** stream) -> v3_result
        {
            d_stdout("dpf_component::get_state               => %p %p", self, stream);
            dpf_component* const component = *(dpf_component**)self;
            DISTRHO_SAFE_ASSERT_RETURN(component != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = component->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return vst3->getState(stream);
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
    std::vector<ScopedPointer<dpf_component>*> components;

    dpf_factory()
    {
        static const uint8_t* kSupportedFactories[] = {
            v3_funknown_iid,
            v3_plugin_factory_iid,
            v3_plugin_factory_2_iid,
            v3_plugin_factory_3_iid
        };

        // ------------------------------------------------------------------------------------------------------------
        // Dummy plugin to get data from

        d_lastBufferSize = 512;
        d_lastSampleRate = 44100.0;
        d_lastCanRequestParameterValueChanges = true;
        static const PluginExporter gPluginInfo(nullptr, nullptr, nullptr);
        d_lastBufferSize = 0;
        d_lastSampleRate = 0.0;
        d_lastCanRequestParameterValueChanges = false;

        dpf_tuid_class[2] = dpf_tuid_component[2] = dpf_tuid_controller[2]
            = dpf_tuid_processor[2] = dpf_tuid_view[2] = gPluginInfo.getUniqueId();

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            d_stdout("dpf_factory::query_interface      => %p %s %p", self, tuid2str(iid), iface);
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
        ref = []V3_API(void*) -> uint32_t { return 1; };
        unref = []V3_API(void*) -> uint32_t { return 0; };

        // ------------------------------------------------------------------------------------------------------------
        // v3_plugin_factory

        v1.get_factory_info = []V3_API(void*, struct v3_factory_info* const info) -> v3_result
        {
            std::memset(info, 0, sizeof(*info));
            DISTRHO_NAMESPACE::strncpy(info->vendor, gPluginInfo.getMaker(), sizeof(info->vendor));
            DISTRHO_NAMESPACE::strncpy(info->url, gPluginInfo.getHomePage(), sizeof(info->url));
            // DISTRHO_NAMESPACE::strncpy(info->email, "", sizeof(info->email)); // TODO
            return V3_OK;
        };

        v1.num_classes = []V3_API(void*) -> int32_t
        {
            return 1;
        };

        v1.get_class_info = []V3_API(void*, int32_t idx, struct v3_class_info* const info) -> v3_result
        {
            std::memset(info, 0, sizeof(*info));
            DISTRHO_SAFE_ASSERT_RETURN(idx == 0, V3_NO_INTERFACE);

            std::memcpy(info->class_id, dpf_tuid_class, sizeof(v3_tuid));
            info->cardinality = 0x7FFFFFFF;
            DISTRHO_NAMESPACE::strncpy(info->category, "Audio Module Class", sizeof(info->category));
            DISTRHO_NAMESPACE::strncpy(info->name, gPluginInfo.getName(), sizeof(info->name));
            return V3_OK;
        };

        v1.create_instance = []V3_API(void* self, const v3_tuid class_id, const v3_tuid iid, void** instance) -> v3_result
        {
            d_stdout("dpf_factory::create_instance      => %p %s %s %p", self, tuid2str(class_id), tuid2str(iid), instance);
            DISTRHO_SAFE_ASSERT_RETURN(v3_tuid_match(class_id, *(v3_tuid*)&dpf_tuid_class) &&
                                       v3_tuid_match(iid, v3_component_iid), V3_NO_INTERFACE);

            dpf_factory* const factory = *(dpf_factory**)self;
            DISTRHO_SAFE_ASSERT_RETURN(factory != nullptr, V3_NOT_INITIALISED);

            ScopedPointer<dpf_component>* const componentptr = new ScopedPointer<dpf_component>;
            *componentptr = new dpf_component(componentptr);
            *instance = componentptr;
            factory->components.push_back(componentptr);
            return V3_OK;
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_plugin_factory_2

        v2.get_class_info_2 = []V3_API(void*, int32_t idx, struct v3_class_info_2* info) -> v3_result
        {
            std::memset(info, 0, sizeof(*info));
            DISTRHO_SAFE_ASSERT_RETURN(idx == 0, V3_NO_INTERFACE);

            std::memcpy(info->class_id, dpf_tuid_class, sizeof(v3_tuid));
            info->cardinality = 0x7FFFFFFF;
            DISTRHO_NAMESPACE::strncpy(info->category, "Audio Module Class", ARRAY_SIZE(info->category));
            DISTRHO_NAMESPACE::strncpy(info->name, gPluginInfo.getName(), ARRAY_SIZE(info->name));

            info->class_flags = 0;
            // DISTRHO_NAMESPACE::strncpy(info->sub_categories, "", sizeof(info->sub_categories)); // TODO
            DISTRHO_NAMESPACE::strncpy(info->vendor, gPluginInfo.getMaker(), ARRAY_SIZE(info->vendor));
            DISTRHO_NAMESPACE::snprintf_u32(info->version, gPluginInfo.getVersion(), ARRAY_SIZE(info->version)); // FIXME
            DISTRHO_NAMESPACE::strncpy(info->sdk_version, "Travesty", ARRAY_SIZE(info->sdk_version)); // TESTING use "VST 3.7" ?
            return V3_OK;
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_plugin_factory_3

        v3.get_class_info_utf16 = []V3_API(void*, int32_t idx, struct v3_class_info_3* info) -> v3_result
        {
            std::memset(info, 0, sizeof(*info));
            DISTRHO_SAFE_ASSERT_RETURN(idx == 0, V3_NO_INTERFACE);

            std::memcpy(info->class_id, dpf_tuid_class, sizeof(v3_tuid));
            info->cardinality = 0x7FFFFFFF;
            DISTRHO_NAMESPACE::strncpy(info->category, "Audio Module Class", ARRAY_SIZE(info->category));
            DISTRHO_NAMESPACE::strncpy_utf16(info->name, gPluginInfo.getName(), ARRAY_SIZE(info->name));

            info->class_flags = 0;
            // DISTRHO_NAMESPACE::strncpy(info->sub_categories, "", ARRAY_SIZE(info->sub_categories)); // TODO
            DISTRHO_NAMESPACE::strncpy_utf16(info->vendor, gPluginInfo.getMaker(), sizeof(info->vendor));
            DISTRHO_NAMESPACE::snprintf_u32_utf16(info->version, gPluginInfo.getVersion(), ARRAY_SIZE(info->version)); // FIXME
            DISTRHO_NAMESPACE::strncpy_utf16(info->sdk_version, "Travesty", ARRAY_SIZE(info->sdk_version)); // TESTING use "VST 3.7" ?
            return V3_OK;
        };

        v3.set_host_context = []V3_API (void* self, struct v3_funknown* host) -> v3_result
        {
            d_stdout("dpf_factory::set_host_context     => %p %p", self, host);
            return V3_NOT_IMPLEMENTED;
        };
    }

    ~dpf_factory()
    {
        d_stdout("dpf_factory deleting");

        for (ScopedPointer<dpf_component>* componentptr : components)
        {
            *componentptr = nullptr;
            delete componentptr;
        }
    }

    DISTRHO_PREVENT_HEAP_ALLOCATION
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
    return true;
}

DISTRHO_PLUGIN_EXPORT
bool EXITFNNAME(void);

bool EXITFNNAME(void)
{
    return true;
}

#undef ENTRYFNNAME
#undef EXITFNNAME

// --------------------------------------------------------------------------------------------------------------------
