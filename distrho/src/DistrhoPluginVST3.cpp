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
#include "../DistrhoPluginUtils.hpp"
#include "../extra/ScopedPointer.hpp"

#if DISTRHO_PLUGIN_HAS_UI && ! DISTRHO_PLUGIN_HAS_EMBED_UI
# undef DISTRHO_PLUGIN_HAS_UI
# define DISTRHO_PLUGIN_HAS_UI 0
#endif

#if DISTRHO_PLUGIN_HAS_UI && ! defined(HAVE_DGL) && ! DISTRHO_PLUGIN_HAS_EXTERNAL_UI
# undef DISTRHO_PLUGIN_HAS_UI
# define DISTRHO_PLUGIN_HAS_UI 0
#endif

// #if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS && ! DISTRHO_PLUGIN_HAS_UI
# define DPF_VST3_USES_SEPARATE_CONTROLLER
// #endif

#if DISTRHO_PLUGIN_HAS_UI
# include "../extra/RingBuffer.hpp"
#endif

#include "travesty/audio_processor.h"
#include "travesty/component.h"
#include "travesty/edit_controller.h"
#include "travesty/factory.h"
#include "travesty/host.h"

#ifdef DISTRHO_PROPER_CPP11_SUPPORT
# include <atomic>
#else
// quick and dirty std::atomic replacement for the things we need
namespace std {
    struct atomic_int {
        volatile int value;
        explicit atomic_int(volatile int v) noexcept : value(v) {}
        int operator++() volatile noexcept { return __atomic_add_fetch(&value, 1, __ATOMIC_RELAXED); }
        int operator--() volatile noexcept { return __atomic_sub_fetch(&value, 1, __ATOMIC_RELAXED); }
        operator int() volatile noexcept { return __atomic_load_n(&value, __ATOMIC_RELAXED); }
    };
}
#endif

#include <map>
#include <string>
#include <vector>

/* TODO items:
 * == parameters
 * - parameter enumeration as lists
 * - implement getParameterValueForString (use names from enumeration if available, fallback to std::atof)
 * - implement getParameterNormalized for MIDI CC params
 * - hide parameter outputs?
 * - hide program parameter?
 * - deal with parameter triggers
 * == MIDI
 * - MIDI CC changes (need to store value to give to the host?)
 * - MIDI program changes
 * - MIDI sysex
 * - append MIDI input events in a sorted way
 * == BUSES
 * - bus arrangements
 * - optional audio buses, create dummy buffer of max_block_size length for them
 * - routing info, do we care?
 * - set sidechain bus name from port group
 * == INFO
 * - set factory email (needs new DPF API, useful for LV2 as well)
 * - do something with get_controller_class_id and set_io_mode?
 * == UI
 * - proper way to create ui, from factory
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
#ifdef DISTRHO_PROPER_CPP11_SUPPORT
static_assert(sizeof(v3_tuid) == sizeof(dpf_tuid), "uid size mismatch");
#endif

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
    static constexpr const struct {
        v3_tuid iid;
        const char* name;
    } extra_known_iids[] = {
        { V3_ID(0x00000000,0x00000000,0x00000000,0x00000000), "(nil)" },
        // edit-controller
        { V3_ID(0xF040B4B3,0xA36045EC,0xABCDC045,0xB4D5A2CC), "{v3_component_handler2|NOT}" },
        { V3_ID(0x7F4EFE59,0xF3204967,0xAC27A3AE,0xAFB63038), "{v3_edit_controller2|NOT}" },
        { V3_ID(0x067D02C1,0x5B4E274D,0xA92D90FD,0x6EAF7240), "{v3_component_handler_bus_activation|NOT}" },
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
    if (v3_tuid_match(iid, v3_event_handler_iid))
        return "{v3_event_handler_iid}";
    if (v3_tuid_match(iid, v3_event_list_iid))
        return "{v3_event_list}";
    if (v3_tuid_match(iid, v3_funknown_iid))
        return "{v3_funknown}";
    if (v3_tuid_match(iid, v3_host_application_iid))
        return "{v3_host_application_iid}";
    if (v3_tuid_match(iid, v3_message_iid))
        return "{v3_message_iid}";
    if (v3_tuid_match(iid, v3_midi_mapping_iid))
        return "{v3_midi_mapping_iid}";
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
    if (v3_tuid_match(iid, v3_run_loop_iid))
        return "{v3_run_loop_iid}";
    if (v3_tuid_match(iid, v3_timer_handler_iid))
        return "{v3_timer_handler_iid}";

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

template<typename T>
static void snprintf_t(char* const dst, const T value, const char* const format, const size_t size)
{
    DISTRHO_SAFE_ASSERT_RETURN(size > 0,);
    std::snprintf(dst, size-1, format, value);
    dst[size-1] = '\0';
}

template<typename T>
static void snprintf_utf16_t(int16_t* const dst, const T value, const char* const format, const size_t size)
{
    DISTRHO_SAFE_ASSERT_RETURN(size > 0,);

    char* const tmpbuf = (char*)std::malloc(size);
    DISTRHO_SAFE_ASSERT_RETURN(tmpbuf != nullptr,);

    std::snprintf(tmpbuf, size-1, format, value);
    tmpbuf[size-1] = '\0';

    strncpy_utf16(dst, tmpbuf, size);
    std::free(tmpbuf);
}

static inline
void snprintf_u32(char* const dst, const uint32_t value, const size_t size)
{
    return snprintf_t<uint32_t>(dst, value, "%u", size);
}

static inline
void snprintf_f32_utf16(int16_t* const dst, const float value, const size_t size)
{
    return snprintf_utf16_t<float>(dst, value, "%f", size);
}

// --------------------------------------------------------------------------------------------------------------------
// handy way to create a utf16 string on the current function scope, used for message strings

struct ScopedUTF16String {
    int16_t* str;
    ScopedUTF16String(const char* const s) noexcept;
    ~ScopedUTF16String() noexcept;
    operator int16_t*() const noexcept;
};

// --------------------------------------------------------------------------------------------------------------------

ScopedUTF16String::ScopedUTF16String(const char* const s) noexcept
    : str(nullptr)
{
    const size_t len = strlen(s);
    str = (int16_t*)malloc(sizeof(int16_t) * (len + 1));
    DISTRHO_SAFE_ASSERT_RETURN(str != nullptr,);
    strncpy_utf16(str, s, len + 1);
}

ScopedUTF16String::~ScopedUTF16String() noexcept
{
    std::free(str);
}

ScopedUTF16String::operator int16_t*() const noexcept
{
    return str;
}

// --------------------------------------------------------------------------------------------------------------------
// dpf_plugin_view_create (implemented on UI side)

v3_plugin_view** dpf_plugin_view_create(v3_host_application** host, void* instancePointer, double sampleRate);

// --------------------------------------------------------------------------------------------------------------------

/**
 * VST3 DSP class.
 *
 * All the dynamic things from VST3 get implemented here, free of complex low-level VST3 pointer things.
 * The DSP is created during the "initialize" component event, and destroyed during "terminate".
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
        uint8_t audio;     // either 0 or 1
        uint8_t sidechain; // either 0 or 1
        uint32_t numMainAudio;
        uint32_t numSidechain;
        uint32_t numCV;

        BusInfo()
          : audio(0),
            sidechain(0),
            numMainAudio(0),
            numSidechain(0),
            numCV(0) {}
    } inputBuses, outputBuses;

public:
    PluginVst3(v3_host_application** const context)
        : fPlugin(this, writeMidiCallback, requestParameterValueChangeCallback),
          fComponentHandler(nullptr),
#if DISTRHO_PLUGIN_HAS_UI
          fConnection(nullptr),
          fHostContext(context),
#endif
          fParameterOffset(fPlugin.getParameterOffset()),
          fRealParameterCount(fParameterOffset + fPlugin.getParameterCount()),
          fParameterValues(nullptr),
          fChangedParameterValues(nullptr)
#if DISTRHO_PLUGIN_HAS_UI
        , fConnectedToUI(false)
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
                port.busId = inputBuses.audio;
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
                port.busId = outputBuses.audio;
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

        if (fRealParameterCount != 0)
        {
            fChangedParameterValues = new bool[fRealParameterCount];
            std::memset(fChangedParameterValues, 0, sizeof(bool)*fRealParameterCount);
        }

#if DISTRHO_PLUGIN_WANT_STATE
        for (uint32_t i=0, count=fPlugin.getStateCount(); i<count; ++i)
        {
            const String& dkey(fPlugin.getStateKey(i));
            fStateMap[dkey] = fPlugin.getStateDefaultValue(i);
        }
#endif

#if !DISTRHO_PLUGIN_HAS_UI
        // unused
        return; (void)context;
#endif
    }

    ~PluginVst3()
    {
        if (fParameterValues != nullptr)
        {
            delete[] fParameterValues;
            fParameterValues = nullptr;
        }

        if (fChangedParameterValues != nullptr)
        {
            delete[] fChangedParameterValues;
            fChangedParameterValues = nullptr;
        }
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

            if (read == 0)
                return V3_OK;

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

                            const float fvalue = fParameterValues[j] = std::atof(value.buffer());
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

        d_stdout("state restored pt1");

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

        d_stdout("state restored pt2");

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
                    break;
                // case V3_EVENT_NOTE_EXP_VALUE:
                // case V3_EVENT_NOTE_EXP_TEXT:
                // case V3_EVENT_CHORD:
                // case V3_EVENT_SCALE:
                // case V3_EVENT_LEGACY_MIDI_CC_OUT:
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
                default:
                    midiEvent.size = 0;
                    break;
                }

                if (midiEventCount == kMaxMidiEvents)
                    break;
            }
        }

        // TODO append MIDI events in a sorted way
        if (v3_param_changes** const inparamsptr = data->input_params)
        {
            v3_param_value_queue** queue;
            int32_t offset;
            double value;

            for (int32_t i = 0, count = v3_cpp_obj(inparamsptr)->get_param_count(inparamsptr); i < count; ++i)
            {
                queue = v3_cpp_obj(inparamsptr)->get_param_data(inparamsptr, i);
                DISTRHO_SAFE_ASSERT_BREAK(queue != nullptr);

                v3_param_id rindex = v3_cpp_obj(queue)->get_param_id(queue);
                DISTRHO_SAFE_ASSERT_UINT_BREAK(rindex < fRealParameterCount, rindex);

                // not supported yet
                if (rindex >= fParameterOffset)
                    continue;

# if DISTRHO_PLUGIN_WANT_PROGRAMS
                if (rindex == 0)
                    continue;
                --rindex;
# endif

                for (int32_t j = 0, pcount = v3_cpp_obj(queue)->get_point_count(queue); j < pcount; ++j)
                {
                    if (v3_cpp_obj(queue)->get_point(queue, j, &offset, &value) != V3_OK)
                        break;

                    DISTRHO_SAFE_ASSERT_BREAK(offset < data->nframes);

                    MidiEvent& midiEvent(fMidiEvents[midiEventCount++]);
                    midiEvent.frame = offset;
                    midiEvent.size = 3;
                    midiEvent.data[0] = (rindex / 130) & 0xf;

                    switch (rindex)
                    {
                    case 128: // channel pressure
                        midiEvent.data[0] |= 0xD0;
                        midiEvent.data[1] = std::max(0, std::min(127, (int)(value * 127)));
                        midiEvent.data[2] = 0;
                        midiEvent.data[3] = 0;
                        break;
                    case 129: // pitchbend
                        midiEvent.data[0] |= 0xE0;
                        midiEvent.data[1] = std::max(0, std::min(16384, (int)(value * 16384))) & 0x7f;
                        midiEvent.data[2] = std::max(0, std::min(16384, (int)(value * 16384))) >> 7;
                        midiEvent.data[3] = 0;
                        break;
                    default:
                        midiEvent.data[0] |= 0xB0;
                        midiEvent.data[1] = rindex % 130;
                        midiEvent.data[2] = std::max(0, std::min(127, (int)(value * 127)));
                        midiEvent.data[3] = 0;
                        break;
                    }

                    if (midiEventCount == kMaxMidiEvents)
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

    v3_result getParameterInfo(int32_t rindex, v3_param_info* const info) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(rindex >= 0, V3_INVALID_ARG);
        std::memset(info, 0, sizeof(v3_param_info));

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (rindex == 0)
        {
            info->param_id = rindex;
            info->flags = V3_PARAM_CAN_AUTOMATE | V3_PARAM_IS_LIST | V3_PARAM_PROGRAM_CHANGE;
            info->step_count = fProgramCountMinusOne;
            strncpy_utf16(info->title, "Current Program", 128);
            strncpy_utf16(info->short_title, "Program", 128);
            return V3_OK;
        }
        --rindex;
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if (rindex < 130*16)
        {
            info->param_id = rindex;
# if DISTRHO_PLUGIN_WANT_PROGRAMS
            ++info->param_id;
# endif
            info->flags = V3_PARAM_CAN_AUTOMATE /*| V3_PARAM_IS_HIDDEN*/;
            info->step_count = 127;
            char ccstr[24];
            snprintf(ccstr, sizeof(ccstr)-1, "MIDI Ch. %d CC %d", rindex / 130 + 1, rindex % 130);
            strncpy_utf16(info->title, ccstr, 128);
            snprintf(ccstr, sizeof(ccstr)-1, "Ch.%d CC%d", rindex / 130 + 1, rindex % 130);
            strncpy_utf16(info->short_title, ccstr+5, 128);
            return V3_OK;
        }
        rindex -= 130*16;
#endif

        const uint32_t index = static_cast<uint32_t>(rindex);
        DISTRHO_SAFE_ASSERT_UINT_RETURN(index < fPlugin.getParameterCount(), index, V3_INVALID_ARG);

        // set up flags
        int32_t flags = 0;

        const ParameterRanges& ranges(fPlugin.getParameterRanges(index));
        const uint32_t hints = fPlugin.getParameterHints(index);

        switch (fPlugin.getParameterDesignation(index))
        {
        case kParameterDesignationNull:
            break;
        case kParameterDesignationBypass:
            flags |= V3_PARAM_IS_BYPASS;
            break;
        }

        if (hints & kParameterIsAutomatable)
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

        info->param_id = index + fParameterOffset;
        info->flags = flags;
        info->step_count = step_count;
        info->default_normalised_value = ranges.getNormalizedValue(ranges.def);
        // int32_t unit_id;
        strncpy_utf16(info->title,       fPlugin.getParameterName(index), 128);
        strncpy_utf16(info->short_title, fPlugin.getParameterShortName(index), 128);
        strncpy_utf16(info->units,       fPlugin.getParameterUnit(index), 128);
        return V3_OK;
    }

    v3_result getParameterStringForValue(v3_param_id rindex, const double normalised, v3_str_128 output)
    {
        DISTRHO_SAFE_ASSERT_UINT_RETURN(rindex < fRealParameterCount, rindex, V3_INVALID_ARG);
        DISTRHO_SAFE_ASSERT_RETURN(normalised >= 0.0 && normalised <= 1.0, V3_INVALID_ARG);


#if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (rindex == 0)
        {
            const uint32_t program = std::round(normalised * fProgramCountMinusOne);
            strncpy_utf16(output, fPlugin.getProgramName(program), 128);
            return V3_OK;
        }
        --rindex;
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if (rindex < 130*16)
        {
            snprintf_f32_utf16(output, std::round(normalised * 127), 128);
            return V3_OK;
        }
        rindex -= 130*16;
#endif

        const ParameterRanges& ranges(fPlugin.getParameterRanges(rindex));
        snprintf_f32_utf16(output, ranges.getUnnormalizedValue(normalised), 128);
        return V3_OK;
    }

    v3_result getParameterValueForString(v3_param_id rindex, int16_t*, double*)
    {
        DISTRHO_SAFE_ASSERT_UINT_RETURN(rindex < fRealParameterCount, rindex, V3_INVALID_ARG);

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (rindex == 0)
        {
            // TODO find program index based on name
            return V3_NOT_IMPLEMENTED;
        }
        --rindex;
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if (rindex < 130*16)
        {
            // TODO find CC/channel based on name
            return V3_NOT_IMPLEMENTED;
        }
        rindex -= 130*16;
#endif

        // TODO
        return V3_NOT_IMPLEMENTED;
    }

    double normalisedParameterToPlain(v3_param_id rindex, const double normalised)
    {
        DISTRHO_SAFE_ASSERT_UINT_RETURN(rindex < fRealParameterCount, rindex, 0.0);

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (rindex == 0)
            return std::round(normalised * fProgramCountMinusOne);
        --rindex;
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if (rindex < 130*16)
            return std::round(normalised * 127);
        rindex -= 130*16;
#endif

        const ParameterRanges& ranges(fPlugin.getParameterRanges(rindex));
        return ranges.getUnnormalizedValue(normalised);
    }

    double plainParameterToNormalised(v3_param_id rindex, const double plain)
    {
        DISTRHO_SAFE_ASSERT_UINT_RETURN(rindex < fRealParameterCount, rindex, 0.0);

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (rindex == 0)
            return std::max(0.0, std::min(1.0, plain / fProgramCountMinusOne));
        --rindex;
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if (rindex < 130*16)
            return std::max(0.0, std::min(1.0, plain / 127));
        rindex -= 130*16;
#endif

        const ParameterRanges& ranges(fPlugin.getParameterRanges(rindex));
        return ranges.getNormalizedValue(plain);
    }

    double getParameterNormalized(v3_param_id rindex)
    {
        DISTRHO_SAFE_ASSERT_UINT_RETURN(rindex < fRealParameterCount, rindex, 0.0);

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        if (rindex == 0)
            return std::max(0.0, std::min(1.0, (double)fCurrentProgram / fProgramCountMinusOne));
        --rindex;
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if (rindex < 130*16)
        {
            // TODO
            return 0.0;
        }
        rindex -= 130*16;
#endif

        const float value = fPlugin.getParameterValue(rindex);
        const ParameterRanges& ranges(fPlugin.getParameterRanges(rindex));
        return ranges.getNormalizedValue(value);
    }

    v3_result setParameterNormalized(v3_param_id rindex, const double value)
    {
        DISTRHO_SAFE_ASSERT_UINT_RETURN(rindex < fRealParameterCount, rindex, V3_INVALID_ARG);
        DISTRHO_SAFE_ASSERT_RETURN(value >= 0.0 && value <= 1.0, V3_INVALID_ARG);

#if DISTRHO_PLUGIN_HAS_UI
        const uint32_t orindex = rindex;
#endif
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

# if DISTRHO_PLUGIN_HAS_UI
            fChangedParameterValues[rindex] = true;
# endif
            return V3_OK;
        }
        --rindex;
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if (rindex < 130*16)
        {
            // TODO
            fChangedParameterValues[rindex] = true;
            return V3_NOT_IMPLEMENTED;
        }
        rindex -= 130*16;
#endif

        const uint32_t hints = fPlugin.getParameterHints(rindex);
        const ParameterRanges& ranges(fPlugin.getParameterRanges(rindex));

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

        fParameterValues[rindex] = realValue;
        fPlugin.setParameterValue(rindex, realValue);
#if DISTRHO_PLUGIN_HAS_UI
        fChangedParameterValues[orindex] = true;
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

            for (uint32_t i=0, rindex; i<fRealParameterCount; ++i)
            {
                if (! fChangedParameterValues[i])
                    continue;

                fChangedParameterValues[i] = false;

                rindex = i;
# if DISTRHO_PLUGIN_WANT_PROGRAMS
                if (rindex == 0)
                {
                    sendParameterChangeToUI(rindex, fCurrentProgram);
                    continue;
                }
                --rindex;
# endif
# if DISTRHO_PLUGIN_WANT_MIDI_INPUT
                if (rindex < 130*16)
                    continue;
                rindex -= 130*16;
# endif

                sendParameterChangeToUI(rindex + fParameterOffset, fParameterValues[rindex]);
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

            const uint32_t index = static_cast<uint32_t>(rindex - fParameterOffset);
            return requestParameterValueChange(index, value) ? V3_OK : V3_INTERNAL_ERR;
        }

# if DISTRHO_PLUGIN_WANT_STATE
        if (std::strcmp(msgid, "state-set") == 0)
        {
            int64_t keyLength = -1;
            int64_t valueLength = -1;
            v3_result res;

            res = v3_cpp_obj(attrs)->get_int(attrs, "key:length", &keyLength);
            DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);
            DISTRHO_SAFE_ASSERT_INT_RETURN(keyLength >= 0, keyLength, V3_INTERNAL_ERR);

            res = v3_cpp_obj(attrs)->get_int(attrs, "value:length", &valueLength);
            DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);
            DISTRHO_SAFE_ASSERT_INT_RETURN(valueLength >= 0, valueLength, V3_INTERNAL_ERR);

            int16_t* const key16 = (int16_t*)std::malloc(sizeof(int16_t)*(keyLength + 1));
            DISTRHO_SAFE_ASSERT_RETURN(key16 != nullptr, V3_NOMEM);

            int16_t* const value16 = (int16_t*)std::malloc(sizeof(int16_t)*(valueLength + 1));
            DISTRHO_SAFE_ASSERT_RETURN(value16 != nullptr, V3_NOMEM);

            res = v3_cpp_obj(attrs)->get_string(attrs, "key", key16, sizeof(int16_t)*keyLength);
            DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);

            res = v3_cpp_obj(attrs)->get_string(attrs, "value", value16, sizeof(int16_t)*valueLength);
            DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);

            // do cheap inline conversion
            char* const key = (char*)key16;
            char* const value = (char*)value16;

            for (int64_t i=0; i<keyLength; ++i)
                key[i] = key16[i];
            for (int64_t i=0; i<valueLength; ++i)
                value[i] = value16[i];

            key[keyLength] = '\0';
            value[valueLength] = '\0';

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
                        std::free(key16);
                        std::free(value16);
                        return V3_OK;
                    }
                }

                d_stderr("Failed to find plugin state with key \"%s\"", key);
            }

            std::free(key16);
            std::free(value16);
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
    v3_host_application** const fHostContext;
#endif

    // Temporary data
    const uint32_t fParameterOffset;
    const uint32_t fRealParameterCount; // regular parameters + current program
    float* fParameterValues;
    bool* fChangedParameterValues;
#if DISTRHO_PLUGIN_HAS_UI
    bool fConnectedToUI;
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

    v3_message** createMessage(const char* const id) const
    {
        DISTRHO_SAFE_ASSERT_RETURN(fHostContext != nullptr, nullptr);

        v3_tuid iid;
        memcpy(iid, v3_message_iid, sizeof(v3_tuid));
        v3_message** msg = nullptr;
        const v3_result res = v3_cpp_obj(fHostContext)->create_instance(fHostContext, iid, iid, (void**)&msg);
        DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_TRUE, res, nullptr);
        DISTRHO_SAFE_ASSERT_RETURN(msg != nullptr, nullptr);

        v3_cpp_obj(msg)->set_message_id(msg, id);
        return msg;
    }

    void sendSampleRateToUI(const double sampleRate) const
    {
        v3_message** const message = createMessage("sample-rate");
        DISTRHO_SAFE_ASSERT_RETURN(message != nullptr,);

        v3_attribute_list** const attrlist = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrlist != nullptr,);

        v3_cpp_obj(attrlist)->set_int(attrlist, "__dpf_msg_target__", 2);
        v3_cpp_obj(attrlist)->set_float(attrlist, "value", sampleRate);
        v3_cpp_obj(fConnection)->notify(fConnection, message);

        v3_cpp_obj_unref(message);
    }

    void sendParameterChangeToUI(const v3_param_id rindex, const double value) const
    {
        v3_message** const message = createMessage("parameter-set");
        DISTRHO_SAFE_ASSERT_RETURN(message != nullptr,);

        v3_attribute_list** const attrlist = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrlist != nullptr,);

        v3_cpp_obj(attrlist)->set_int(attrlist, "__dpf_msg_target__", 2);
        v3_cpp_obj(attrlist)->set_int(attrlist, "rindex", rindex);
        v3_cpp_obj(attrlist)->set_float(attrlist, "value", value);
        v3_cpp_obj(fConnection)->notify(fConnection, message);

        v3_cpp_obj_unref(message);
    }

    void sendStateChangeToUI(const char* const key, const char* const value) const
    {
        v3_message** const message = createMessage("state-set");
        DISTRHO_SAFE_ASSERT_RETURN(message != nullptr,);

        v3_attribute_list** const attrlist = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrlist != nullptr,);

        v3_cpp_obj(attrlist)->set_int(attrlist, "__dpf_msg_target__", 2);
        v3_cpp_obj(attrlist)->set_int(attrlist, "key:length", std::strlen(key));
        v3_cpp_obj(attrlist)->set_int(attrlist, "value:length", std::strlen(value));
        v3_cpp_obj(attrlist)->set_string(attrlist, "key", ScopedUTF16String(key));
        v3_cpp_obj(attrlist)->set_string(attrlist, "value", ScopedUTF16String(value));
        v3_cpp_obj(fConnection)->notify(fConnection, message);

        v3_cpp_obj_unref(message);
    }

    void sendReadyToUI() const
    {
        v3_message** const message = createMessage("ready");
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
            if (midiEvent.size == 4)
                event.midi_cc_out.value2 = midiEvent.size == 4;
            break;
        /* TODO how do we deal with program changes??
        case 0xC0:
            break;
        */
        case 0xD0:
            event.type = V3_EVENT_LEGACY_MIDI_CC_OUT;
            event.midi_cc_out.channel = data[0] & 0xf;
            event.midi_cc_out.cc_number = 128;
            event.midi_cc_out.value = data[1];
            break;
        case 0xE0:
            event.type = V3_EVENT_LEGACY_MIDI_CC_OUT;
            event.midi_cc_out.channel = data[0] & 0xf;
            event.midi_cc_out.cc_number = 129;
            event.midi_cc_out.value = data[1];
            event.midi_cc_out.value2 = data[2];
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
// v3_funknown for static instances

static uint32_t V3_API dpf_static_ref(void*) { return 1; }
static uint32_t V3_API dpf_static_unref(void*) { return 0; }

// --------------------------------------------------------------------------------------------------------------------
// v3_funknown for classes with a single instance

template<class T>
static uint32_t V3_API dpf_single_instance_ref(void* const self)
{
    return ++(*static_cast<T**>(self))->refcounter;
}

template<class T>
static uint32_t V3_API dpf_single_instance_unref(void* const self)
{
    return --(*static_cast<T**>(self))->refcounter;
}

#if DISTRHO_PLUGIN_HAS_UI
// --------------------------------------------------------------------------------------------------------------------
// dpf_dsp_connection_point

enum ConnectionPointType {
    kConnectionPointComponent,
    kConnectionPointController,
    kConnectionPointBridge
};

static const char* ConnectionPointType2str(const ConnectionPointType type)
{
    switch (type)
    {
    case kConnectionPointComponent:
        return "kConnectionPointComponent";
    case kConnectionPointController:
        return "kConnectionPointController";
    case kConnectionPointBridge:
        return "kConnectionPointBridge";
    }

    return "[unknown]";
}

struct dpf_dsp_connection_point : v3_connection_point_cpp {
    std::atomic_int refcounter;
    ScopedPointer<PluginVst3>& vst3;
    const ConnectionPointType type;
    v3_connection_point** other;
    v3_connection_point** bridge; // when type is controller this points to ctrl<->view point
    bool shortcircuit; // plugin as controller, should pass directly to view

    dpf_dsp_connection_point(const ConnectionPointType t, ScopedPointer<PluginVst3>& v)
        : refcounter(1),
          vst3(v),
          type(t),
          other(nullptr),
          bridge(nullptr),
          shortcircuit(false)
    {
        // v3_funknown, single instance
        query_interface = query_interface_connection_point;
        ref = dpf_single_instance_ref<dpf_dsp_connection_point>;
        unref = dpf_single_instance_unref<dpf_dsp_connection_point>;

        // v3_connection_point
        point.connect = connect;
        point.disconnect = disconnect;
        point.notify = notify;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_funknown

    static v3_result V3_API query_interface_connection_point(void* self, const v3_tuid iid, void** iface)
    {
        if (v3_tuid_match(iid, v3_funknown_iid))
        {
            *iface = self;
            return V3_OK;
        }

        if (v3_tuid_match(iid, v3_connection_point_iid))
        {
            *iface = self;
            return V3_OK;
        }

        *iface = nullptr;
        return V3_NO_INTERFACE;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_connection_point

    static v3_result V3_API connect(void* self, v3_connection_point** other)
    {
        dpf_dsp_connection_point* const point = *(dpf_dsp_connection_point**)self;
        DISTRHO_SAFE_ASSERT_RETURN(point != nullptr, V3_NOT_INITIALIZED);
        d_stdout("DSP|dpf_dsp_connection_point::connect    => %p %p | %d:%s %d",
                 self, other, point->type, ConnectionPointType2str(point->type), point->shortcircuit);
        DISTRHO_SAFE_ASSERT_RETURN(point->other == nullptr, V3_INVALID_ARG);
        DISTRHO_SAFE_ASSERT(point->bridge == nullptr);

        point->other = other;

        if (point->type == kConnectionPointComponent)
            if (PluginVst3* const vst3 = point->vst3)
                vst3->connect((v3_connection_point**)self);

        return V3_OK;
    }

    static v3_result V3_API disconnect(void* self, v3_connection_point** other)
    {
        dpf_dsp_connection_point* const point = *(dpf_dsp_connection_point**)self;
        DISTRHO_SAFE_ASSERT_RETURN(point != nullptr, V3_NOT_INITIALIZED);
        d_stdout("DSP|dpf_dsp_connection_point::disconnect => %p %p | %d:%s %d",
                 self, other, point->type, ConnectionPointType2str(point->type), point->shortcircuit);
        DISTRHO_SAFE_ASSERT_RETURN(point->other != nullptr, V3_INVALID_ARG);

        if (point->type == kConnectionPointComponent)
            if (PluginVst3* const vst3 = point->vst3)
                vst3->disconnect();

        if (point->type == kConnectionPointBridge)
            v3_cpp_obj_unref(point->other);

        point->other = nullptr;
        point->bridge = nullptr;

        return V3_OK;
    }

    static v3_result V3_API notify(void* self, v3_message** message)
    {
        dpf_dsp_connection_point* const point = *(dpf_dsp_connection_point**)self;
        DISTRHO_SAFE_ASSERT_RETURN(point != nullptr, V3_NOT_INITIALIZED);

        PluginVst3* const vst3 = point->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        v3_connection_point** const other = point->other;
        DISTRHO_SAFE_ASSERT_RETURN(other != nullptr, V3_NOT_INITIALIZED);

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
                DISTRHO_SAFE_ASSERT_RETURN(bridge != nullptr, V3_NOT_INITIALIZED);
                return v3_cpp_obj(bridge)->notify(bridge, message);
            }

        // message belongs to bridge (aka ui)
        case kConnectionPointBridge:
            if (target == 1)
            {
                // view -> edit controller -> component
                v3_connection_point** const bridge = point->bridge;
                DISTRHO_SAFE_ASSERT_RETURN(bridge != nullptr, V3_NOT_INITIALIZED);
                return v3_cpp_obj(bridge)->notify(bridge, message);
            }
            else
            {
                // message is from component to controller to view
                return v3_cpp_obj(other)->notify(other, message);
            }
        }

        return V3_INTERNAL_ERR;
    }
};
#endif

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
// --------------------------------------------------------------------------------------------------------------------
// dpf_midi_mapping

struct dpf_midi_mapping : v3_midi_mapping_cpp {
    dpf_midi_mapping()
    {
        // v3_funknown, static
        query_interface = query_interface_midi_mapping;
        ref = dpf_static_ref;
        unref = dpf_static_unref;

        // v3_midi_mapping
        map.get_midi_controller_assignment = get_midi_controller_assignment;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_funknown

    static v3_result V3_API query_interface_midi_mapping(void* const self, const v3_tuid iid, void** const iface)
    {
        if (v3_tuid_match(iid, v3_funknown_iid) ||
            v3_tuid_match(iid, v3_midi_mapping_iid))
        {
            *iface = self;
            return V3_OK;
        }

        *iface = nullptr;
        return V3_NO_INTERFACE;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_midi_mapping

    static v3_result V3_API get_midi_controller_assignment(void*, const int32_t bus, const int16_t channel, const int16_t cc, v3_param_id* const id)
    {
        DISTRHO_SAFE_ASSERT_INT_RETURN(bus == 0, bus, V3_FALSE);
        DISTRHO_SAFE_ASSERT_INT_RETURN(channel >= 0 && channel < 16, channel, V3_FALSE);
        DISTRHO_SAFE_ASSERT_INT_RETURN(cc >= 0 && cc < 130, cc, V3_FALSE);

# if DISTRHO_PLUGIN_WANT_PROGRAMS
        static constexpr const v3_param_id offset = 1;
# else
        static constexpr const v3_param_id offset = 0;
# endif

        *id = offset + channel * 130 + cc;
        return V3_TRUE;
    }

    DISTRHO_PREVENT_HEAP_ALLOCATION
};
#endif

// --------------------------------------------------------------------------------------------------------------------
// dpf_edit_controller

struct dpf_edit_controller : v3_edit_controller_cpp {
    std::atomic_int refcounter;
#if DISTRHO_PLUGIN_HAS_UI
    ScopedPointer<dpf_dsp_connection_point> connectionComp;   // kConnectionPointController
    ScopedPointer<dpf_dsp_connection_point> connectionBridge; // kConnectionPointBridge
#endif
#ifdef DPF_VST3_USES_SEPARATE_CONTROLLER
    ScopedPointer<PluginVst3> vst3;
#else
    ScopedPointer<PluginVst3>& vst3;
    bool initialized;
#endif
    // cached values
    v3_component_handler** handler;
    v3_host_application** hostContextFromFactory;
    v3_host_application** hostContextFromInitialize;

#ifdef DPF_VST3_USES_SEPARATE_CONTROLLER
    dpf_edit_controller(v3_host_application** const host)
        : refcounter(1),
          vst3(nullptr),
#else
    dpf_edit_controller(ScopedPointer<PluginVst3>& v, v3_host_application** const host)
        : refcounter(1),
          vst3(v),
          initialized(false),
#endif
          handler(nullptr),
          hostContextFromFactory(host),
          hostContextFromInitialize(nullptr)
    {
        d_stdout("dpf_edit_controller() has context %p", hostContextFromFactory);

        // make sure context is valid through out this controller lifetime
        if (hostContextFromFactory != nullptr)
            v3_cpp_obj_ref(hostContextFromFactory);

        // v3_funknown, everything custom
        query_interface = query_interface_edit_controller;
        ref = ref_edit_controller;
        unref = unref_edit_controller;

        // v3_plugin_base
        base.initialize = initialize;
        base.terminate = terminate;

        // v3_edit_controller
        ctrl.set_component_state = set_component_state;
        ctrl.set_state = set_state;
        ctrl.get_state = get_state;
        ctrl.get_parameter_count = get_parameter_count;
        ctrl.get_parameter_info = get_parameter_info;
        ctrl.get_parameter_string_for_value = get_parameter_string_for_value;
        ctrl.get_parameter_value_for_string = get_parameter_value_for_string;
        ctrl.normalised_parameter_to_plain = normalised_parameter_to_plain;
        ctrl.plain_parameter_to_normalised = plain_parameter_to_normalised;
        ctrl.get_parameter_normalised = get_parameter_normalised;
        ctrl.set_parameter_normalised = set_parameter_normalised;
        ctrl.set_component_handler = set_component_handler;
        ctrl.create_view = create_view;
    }

    void cleanup()
    {
        if (hostContextFromFactory != nullptr)
        {
            v3_cpp_obj_unref(hostContextFromFactory);
            hostContextFromFactory = nullptr;
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_funknown

    static v3_result V3_API query_interface_edit_controller(void* const self, const v3_tuid iid, void** const iface)
    {
        d_stdout("query_interface_edit_controller => %p %s %p", self, tuid2str(iid), iface);

        if (v3_tuid_match(iid, v3_funknown_iid) ||
            v3_tuid_match(iid, v3_plugin_base_iid) ||
            v3_tuid_match(iid, v3_edit_controller_iid))
        {
            *iface = self;
            return V3_OK;
        }

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if (v3_tuid_match(iid, v3_midi_mapping_iid))
        {
            static dpf_midi_mapping midi_mapping;
            static dpf_midi_mapping* midi_mapping_ptr = &midi_mapping;
            *iface = &midi_mapping_ptr;
            return V3_OK;
        }
#endif

#if DISTRHO_PLUGIN_HAS_UI
        if (v3_tuid_match(iid, v3_connection_point_iid))
        {
            dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

            if (controller->connectionComp == nullptr)
                controller->connectionComp = new dpf_dsp_connection_point(kConnectionPointController,
                                                                          controller->vst3);
            else
                ++controller->connectionComp->refcounter;
            *iface = &controller->connectionComp;
            return V3_OK;
        }
#endif

        *iface = nullptr;
        return V3_NO_INTERFACE;
    }

    static uint32_t V3_API ref_edit_controller(void* const self)
    {
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);
        const int refcount = ++controller->refcounter;
        d_stdout("dpf_edit_controller::ref => %p | refcount %i", self, refcount);
        return refcount;
    }

    static uint32_t V3_API unref_edit_controller(void* const self)
    {
        dpf_edit_controller** const controllerptr = static_cast<dpf_edit_controller**>(self);
        dpf_edit_controller* const controller = *controllerptr;

        if (const int refcount = --controller->refcounter)
        {
            d_stdout("dpf_edit_controller::unref => %p | refcount %i", self, refcount);
            return refcount;
        }

#ifdef DPF_VST3_USES_SEPARATE_CONTROLLER
        d_stdout("dpf_edit_controller::unref => %p | refcount is zero, deleting everything now!", self);
        controller->cleanup();
        delete controller;
        delete controllerptr;
#else
        d_stdout("dpf_edit_controller::unref => %p is zero, doing cleanup", self);
        controller->cleanup();
#endif
        return 0;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_plugin_base

    static v3_result V3_API initialize(void* const self, v3_plugin_base::v3_funknown** const context)
    {
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

#ifdef DPF_VST3_USES_SEPARATE_CONTROLLER
        DISTRHO_SAFE_ASSERT_RETURN(controller->vst3 == nullptr, V3_INVALID_ARG);
#else
        DISTRHO_SAFE_ASSERT_RETURN(! controller->initialized, V3_INVALID_ARG);
#endif

        // query for host context
        v3_host_application** host = nullptr;
        if (context != nullptr)
            v3_cpp_obj_query_interface(context, v3_host_application_iid, &host);

        d_stdout("dpf_edit_controller::initialize => %p %p | host %p", self, context, host);

        // save it for later so we can unref it
        controller->hostContextFromInitialize = host;

#ifdef DPF_VST3_USES_SEPARATE_CONTROLLER
        // provide the factory context to the plugin if this new one is missing
        if (host == nullptr)
            host = controller->hostContextFromFactory;

        controller->vst3 = new PluginVst3(host);
#else
        controller->initialized = true;
#endif

        return V3_OK;
    }

    static v3_result V3_API terminate(void* self)
    {
        d_stdout("dpf_edit_controller::terminate => %p", self);
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

#ifdef DPF_VST3_USES_SEPARATE_CONTROLLER
        DISTRHO_SAFE_ASSERT_RETURN(controller->vst3 != nullptr, V3_INVALID_ARG);

        controller->vst3 = nullptr;
#else
        const bool initialized = controller->initialized;
        DISTRHO_SAFE_ASSERT_RETURN(initialized, V3_INVALID_ARG);

        controller->initialized = false;
#endif

#if DISTRHO_PLUGIN_HAS_UI
        // take the chance to do some cleanup if possible (we created the bridge, we need to destroy it)
        if (controller->connectionBridge != nullptr)
            if (controller->connectionBridge->refcounter == 1)
                controller->connectionBridge = nullptr;

        if (controller->connectionComp != nullptr && controller->connectionComp->shortcircuit)
            if (controller->connectionComp->refcounter == 1)
                controller->connectionComp = nullptr;
#endif

        // unref context received during initialize
        if (controller->hostContextFromInitialize != nullptr)
        {
            v3_cpp_obj_unref(controller->hostContextFromInitialize);
            controller->hostContextFromInitialize = nullptr;
        }

        return V3_OK;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_edit_controller

    static v3_result V3_API set_component_state(void* const self, v3_bstream** const stream)
    {
        d_stdout("dpf_edit_controller::set_component_state => %p %p", self, stream);
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

#ifdef DPF_VST3_USES_SEPARATE_CONTROLLER
        PluginVst3* const vst3 = controller->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->setState(stream);
#else
        return V3_OK;
#endif
    }

    static v3_result V3_API set_state(void* const self, v3_bstream** const stream)
    {
        d_stdout("dpf_edit_controller::set_state => %p %p", self, stream);
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

#ifdef DPF_VST3_USES_SEPARATE_CONTROLLER
        DISTRHO_SAFE_ASSERT_RETURN(controller->vst3 != nullptr, V3_NOT_INITIALIZED);
#else
        DISTRHO_SAFE_ASSERT_RETURN(controller->initialized, V3_INVALID_ARG);
#endif

        return V3_NOT_IMPLEMENTED;
    }

    static v3_result V3_API get_state(void* const self, v3_bstream** const stream)
    {
        d_stdout("dpf_edit_controller::get_state => %p %p", self, stream);
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

#ifdef DPF_VST3_USES_SEPARATE_CONTROLLER
        DISTRHO_SAFE_ASSERT_RETURN(controller->vst3 != nullptr, V3_NOT_INITIALIZED);
#else
        DISTRHO_SAFE_ASSERT_RETURN(controller->initialized, V3_INVALID_ARG);
#endif

        return V3_NOT_IMPLEMENTED;
    }

    static int32_t V3_API get_parameter_count(void* self)
    {
        // d_stdout("dpf_edit_controller::get_parameter_count => %p", self);
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

        PluginVst3* const vst3 = controller->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->getParameterCount();
    }

    static v3_result V3_API get_parameter_info(void* self, int32_t param_idx, v3_param_info* param_info)
    {
        // d_stdout("dpf_edit_controller::get_parameter_info => %p %i", self, param_idx);
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

        PluginVst3* const vst3 = controller->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->getParameterInfo(param_idx, param_info);
    }

    static v3_result V3_API get_parameter_string_for_value(void* self, v3_param_id index, double normalised, v3_str_128 output)
    {
        // NOTE very noisy, called many times
        // d_stdout("dpf_edit_controller::get_parameter_string_for_value => %p %u %f %p", self, index, normalised, output);
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

        PluginVst3* const vst3 = controller->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->getParameterStringForValue(index, normalised, output);
    }

    static v3_result V3_API get_parameter_value_for_string(void* self, v3_param_id index, int16_t* input, double* output)
    {
        d_stdout("dpf_edit_controller::get_parameter_value_for_string => %p %u %p %p", self, index, input, output);
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

        PluginVst3* const vst3 = controller->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->getParameterValueForString(index, input, output);
    }

    static double V3_API normalised_parameter_to_plain(void* self, v3_param_id index, double normalised)
    {
        d_stdout("dpf_edit_controller::normalised_parameter_to_plain => %p %u %f", self, index, normalised);
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

        PluginVst3* const vst3 = controller->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->normalisedParameterToPlain(index, normalised);
    }

    static double V3_API plain_parameter_to_normalised(void* self, v3_param_id index, double plain)
    {
        d_stdout("dpf_edit_controller::plain_parameter_to_normalised => %p %u %f", self, index, plain);
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

        PluginVst3* const vst3 = controller->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->plainParameterToNormalised(index, plain);
    }

    static double V3_API get_parameter_normalised(void* self, v3_param_id index)
    {
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

        PluginVst3* const vst3 = controller->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, 0.0);

        return vst3->getParameterNormalized(index);
    }

    static v3_result V3_API set_parameter_normalised(void* self, v3_param_id index, double normalised)
    {
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

        PluginVst3* const vst3 = controller->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->setParameterNormalized(index, normalised);
    }

    static v3_result V3_API set_component_handler(void* self, v3_component_handler** handler)
    {
        d_stdout("dpf_edit_controller::set_component_handler => %p %p", self, handler);
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

        controller->handler = handler;

        if (PluginVst3* const vst3 = controller->vst3)
            return vst3->setComponentHandler(handler);

        return V3_NOT_INITIALIZED;
    }

    static v3_plugin_view** V3_API create_view(void* self, const char* name)
    {
        d_stdout("dpf_edit_controller::create_view => %p %s", self, name);
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

        d_stdout("create_view has contexts %p %p",
                 controller->hostContextFromFactory, controller->hostContextFromInitialize);

#if DISTRHO_PLUGIN_HAS_UI
        // plugin must be initialized
        PluginVst3* const vst3 = controller->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, nullptr);

        d_stdout("dpf_edit_controller::create_view => %p %s | edit-ctrl %p, factory %p",
                 self, name,
                 controller->hostContextFromInitialize,
                 controller->hostContextFromFactory);

        // we require a host context for message creation
        v3_host_application** host = controller->hostContextFromInitialize != nullptr
                                   ? controller->hostContextFromInitialize
                                   : controller->hostContextFromFactory;
        DISTRHO_SAFE_ASSERT_RETURN(host != nullptr, nullptr);

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

        v3_plugin_view** const view = dpf_plugin_view_create(host,
                                                             vst3->getInstancePointer(),
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
    }
};

// --------------------------------------------------------------------------------------------------------------------
// dpf_process_context_requirements

struct dpf_process_context_requirements : v3_process_context_requirements_cpp {
    dpf_process_context_requirements()
    {
        // v3_funknown, static
        query_interface = query_interface_process_context_requirements;
        ref = dpf_static_ref;
        unref = dpf_static_unref;

        // v3_process_context_requirements
        req.get_process_context_requirements = get_process_context_requirements;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_funknown

    static v3_result V3_API query_interface_process_context_requirements(void* const self, const v3_tuid iid, void** const iface)
    {
        d_stdout("query_interface_process_context_requirements => %p %s %p", self, tuid2str(iid), iface);
        if (v3_tuid_match(iid, v3_funknown_iid) ||
            v3_tuid_match(iid, v3_process_context_requirements_iid))
        {
            *iface = self;
            return V3_OK;
        }

        *iface = nullptr;
        return V3_NO_INTERFACE;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_process_context_requirements

    static uint32_t V3_API get_process_context_requirements(void*)
    {
#if DISTRHO_PLUGIN_WANT_TIMEPOS
        return 0x0
            | V3_PROCESS_CTX_NEED_CONTINUOUS_TIME  // V3_PROCESS_CTX_CONT_TIME_VALID
            | V3_PROCESS_CTX_NEED_PROJECT_TIME     // V3_PROCESS_CTX_PROJECT_TIME_VALID
            | V3_PROCESS_CTX_NEED_TEMPO            // V3_PROCESS_CTX_TEMPO_VALID
            | V3_PROCESS_CTX_NEED_TIME_SIG         // V3_PROCESS_CTX_TIME_SIG_VALID
            | V3_PROCESS_CTX_NEED_TRANSPORT_STATE; // V3_PROCESS_CTX_PLAYING
#else
        return 0x0;
#endif
    }

    DISTRHO_PREVENT_HEAP_ALLOCATION
};

// --------------------------------------------------------------------------------------------------------------------
// dpf_audio_processor

struct dpf_audio_processor : v3_audio_processor_cpp {
    std::atomic_int refcounter;
    ScopedPointer<PluginVst3>& vst3;

    dpf_audio_processor(ScopedPointer<PluginVst3>& v)
        : refcounter(1),
          vst3(v)
    {
        // v3_funknown, single instance
        query_interface = query_interface_audio_processor;
        ref = dpf_single_instance_ref<dpf_audio_processor>;
        unref = dpf_single_instance_unref<dpf_audio_processor>;

        // v3_audio_processor
        proc.set_bus_arrangements = set_bus_arrangements;
        proc.get_bus_arrangement = get_bus_arrangement;
        proc.can_process_sample_size = can_process_sample_size;
        proc.get_latency_samples = get_latency_samples;
        proc.setup_processing = setup_processing;
        proc.set_processing = set_processing;
        proc.process = process;
        proc.get_tail_samples = get_tail_samples;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_funknown

    static v3_result V3_API query_interface_audio_processor(void* const self, const v3_tuid iid, void** const iface)
    {
        d_stdout("query_interface_audio_processor => %p %s %p", self, tuid2str(iid), iface);

        if (v3_tuid_match(iid, v3_funknown_iid) ||
            v3_tuid_match(iid, v3_audio_processor_iid))
        {
            *iface = self;
            return V3_OK;
        }

        if (v3_tuid_match(iid, v3_process_context_requirements_iid))
        {
            static dpf_process_context_requirements context_req;
            static dpf_process_context_requirements* context_req_ptr = &context_req;
            *iface = &context_req_ptr;
            return V3_OK;
        }

        *iface = nullptr;
        return V3_NO_INTERFACE;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_audio_processor

    static v3_result V3_API set_bus_arrangements(void* const self,
                                                 v3_speaker_arrangement* const inputs, const int32_t num_inputs,
                                                 v3_speaker_arrangement* const outputs, const int32_t num_outputs)
    {
        // NOTE this is called a bunch of times
        // d_stdout("dpf_audio_processor::set_bus_arrangements => %p %p %i %p %i", self, inputs, num_inputs, outputs, num_outputs);
        dpf_audio_processor* const processor = *static_cast<dpf_audio_processor**>(self);

        PluginVst3* const vst3 = processor->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return processor->vst3->setBusArrangements(inputs, num_inputs, outputs, num_outputs);
    }

    static v3_result V3_API get_bus_arrangement(void* const self, const int32_t bus_direction,
                                                const int32_t idx, v3_speaker_arrangement* const arr)
    {
        d_stdout("dpf_audio_processor::get_bus_arrangement => %p %i %i %p", self, bus_direction, idx, arr);
        dpf_audio_processor* const processor = *static_cast<dpf_audio_processor**>(self);

        PluginVst3* const vst3 = processor->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return processor->vst3->getBusArrangement(bus_direction, idx, arr);
    }

    static v3_result V3_API can_process_sample_size(void*, const int32_t symbolic_sample_size)
    {
        d_stdout("dpf_audio_processor::can_process_sample_size => %i", symbolic_sample_size);
        return symbolic_sample_size == V3_SAMPLE_32 ? V3_OK : V3_NOT_IMPLEMENTED;
    }

    static uint32_t V3_API get_latency_samples(void* const self)
    {
        d_stdout("dpf_audio_processor::get_latency_samples => %p", self);
        dpf_audio_processor* const processor = *static_cast<dpf_audio_processor**>(self);

        PluginVst3* const vst3 = processor->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, 0);

        return processor->vst3->getLatencySamples();
    }

    static v3_result V3_API setup_processing(void* const self, v3_process_setup* const setup)
    {
        d_stdout("dpf_audio_processor::setup_processing => %p", self);
        dpf_audio_processor* const processor = *static_cast<dpf_audio_processor**>(self);

        PluginVst3* const vst3 = processor->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        d_nextBufferSize = setup->max_block_size;
        d_nextSampleRate = setup->sample_rate;
        return processor->vst3->setupProcessing(setup);
    }

    static v3_result V3_API set_processing(void* const self, const v3_bool state)
    {
        d_stdout("dpf_audio_processor::set_processing => %p %u", self, state);
        dpf_audio_processor* const processor = *static_cast<dpf_audio_processor**>(self);

        PluginVst3* const vst3 = processor->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return processor->vst3->setProcessing(state);
    }

    static v3_result V3_API process(void* const self, v3_process_data* const data)
    {
        // NOTE runs during RT
        // d_stdout("dpf_audio_processor::process => %p", self);
        dpf_audio_processor* const processor = *static_cast<dpf_audio_processor**>(self);

        PluginVst3* const vst3 = processor->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return processor->vst3->process(data);
    }

    static uint32_t V3_API get_tail_samples(void* const self)
    {
        d_stdout("dpf_audio_processor::get_tail_samples => %p", self);
        dpf_audio_processor* const processor = *static_cast<dpf_audio_processor**>(self);

        PluginVst3* const vst3 = processor->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, 0);

        return processor->vst3->getTailSamples();
    }
};

// --------------------------------------------------------------------------------------------------------------------
// Store components that we can't delete properly, to be cleaned up on module unload

struct dpf_component;

std::vector<dpf_component**> gComponentGarbage;

static v3_result handleUncleanComponent(dpf_component** const componentptr)
{
    gComponentGarbage.push_back(componentptr);
    return V3_INVALID_ARG;
}

// --------------------------------------------------------------------------------------------------------------------
// dpf_component

struct dpf_component : v3_component_cpp {
    std::atomic_int refcounter;
    ScopedPointer<dpf_audio_processor> processor;
#if DISTRHO_PLUGIN_HAS_UI
    ScopedPointer<dpf_dsp_connection_point> connection; // kConnectionPointComponent
#endif
    ScopedPointer<dpf_edit_controller> controller;
    ScopedPointer<PluginVst3> vst3;
    v3_host_application** const hostContextFromFactory;
    v3_host_application** hostContextFromInitialize;

    dpf_component(v3_host_application** const h)
        : refcounter(1),
          hostContextFromFactory(h),
          hostContextFromInitialize(nullptr)
    {
        // make sure context is valid through out this component lifetime
        if (hostContextFromFactory != nullptr)
            v3_cpp_obj_ref(hostContextFromFactory);

        // v3_funknown, everything custom
        query_interface = query_interface_component;
        ref = ref_component;
        unref = unref_component;

        // v3_plugin_base
        base.initialize = initialize;
        base.terminate = terminate;

        // v3_component
        comp.get_controller_class_id = get_controller_class_id;
        comp.set_io_mode = set_io_mode;
        comp.get_bus_count = get_bus_count;
        comp.get_bus_info = get_bus_info;
        comp.get_routing_info = get_routing_info;
        comp.activate_bus = activate_bus;
        comp.set_active = set_active;
        comp.set_state = set_state;
        comp.get_state = get_state;
    }

    void cleanup()
    {
        d_stdout("dpf_component::cleanup");
        vst3 = nullptr;
        processor = nullptr;
#if DISTRHO_PLUGIN_HAS_UI
        connection = nullptr;
#endif
        if (controller != nullptr)
        {
            controller->cleanup();
            controller = nullptr;
        }

        if (hostContextFromFactory != nullptr)
            v3_cpp_obj_unref(hostContextFromFactory);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_funknown

    static v3_result V3_API query_interface_component(void* const self, const v3_tuid iid, void** const iface)
    {
        d_stdout("query_interface_component => %p %s %p", self, tuid2str(iid), iface);

        if (v3_tuid_match(iid, v3_funknown_iid) ||
            v3_tuid_match(iid, v3_plugin_base_iid) ||
            v3_tuid_match(iid, v3_component_iid))
        {
            *iface = self;
            return V3_OK;
        }

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if (v3_tuid_match(iid, v3_midi_mapping_iid))
        {
            static dpf_midi_mapping midi_mapping;
            static dpf_midi_mapping* midi_mapping_ptr = &midi_mapping;
            *iface = &midi_mapping_ptr;
            return V3_OK;
        }
#endif

        /*
        dpf_component* const component = *static_cast<dpf_component**>(self);

        if (v3_tuid_match(iid, v3_audio_processor_iid))
        {
            if (component->processor == nullptr)
                component->processor = new dpf_audio_processor(component->vst3);
            else
                ++component->processor->refcounter;
            *iface = &component->processor;
            return V3_OK;
        }

#if DISTRHO_PLUGIN_HAS_UI
        if (v3_tuid_match(iid, v3_connection_point_iid))
        {
            if (component->connection == nullptr)
                component->connection = new dpf_dsp_connection_point(kConnectionPointComponent,
                                                                     component->vst3);
            else
                ++component->connection->refcounter;
            *iface = &component->connection;
            return V3_OK;
        }
#endif

        if (v3_tuid_match(iid, v3_edit_controller_iid))
        {
            d_stdout("query_interface_component called with context %p", component->hostContextFromFactory);

            if (component->controller == nullptr)
                component->controller = new dpf_edit_controller(component->vst3, component->hostContextFromFactory);
            else
                ++component->controller->refcounter;
            *iface = &component->controller;
            return V3_OK;
        }
        */

        *iface = nullptr;
        return V3_NO_INTERFACE;
    }

    static uint32_t V3_API ref_component(void* const self)
    {
        dpf_component* const component = *static_cast<dpf_component**>(self);
        const int refcount = ++component->refcounter;
        d_stdout("dpf_component::ref => %p | refcount %i", self, refcount);
        return refcount;
    }

    static uint32_t V3_API unref_component(void* const self)
    {
        dpf_component** const componentptr = static_cast<dpf_component**>(self);
        dpf_component* const component = *componentptr;

        if (const int refcount = --component->refcounter)
        {
            d_stdout("dpf_component::unref => %p | refcount %i", self, refcount);
            return refcount;
        }

        /**
         * Some hosts will have unclean instances of a few of the component child classes at this point.
         * We check for those here, going through the whole possible chain to see if it is safe to delete.
         * If not, we add this component to the `gComponentGarbage` global which will take care of it during unload.
         */

        bool unclean = false;
        if (dpf_audio_processor* const proc = component->processor)
        {
            if (const int refcount = proc->refcounter)
            {
                unclean = true;
                d_stderr("DPF warning: asked to delete component while audio processor still active (refcount %d)", refcount);
            }
        }

#if DISTRHO_PLUGIN_HAS_UI
        if (dpf_dsp_connection_point* const conn = component->connection)
        {
            if (const int refcount = conn->refcounter)
            {
                unclean = true;
                d_stderr("DPF warning: asked to delete component while connection point still active (refcount %d)", refcount);
            }
        }
#endif

        if (dpf_edit_controller* const ctrl = component->controller)
        {
            if (const int refcount = ctrl->refcounter)
            {
                unclean = true;
                d_stderr("DPF warning: asked to delete component while edit controller still active (refcount %d)", refcount);
            }

#if DISTRHO_PLUGIN_HAS_UI
            if (dpf_dsp_connection_point* const comp = ctrl->connectionComp)
            {
                if (const int refcount = comp->refcounter)
                {
                    unclean = true;
                    d_stderr("DPF warning: asked to delete component while edit controller connection point still active (refcount %d)", refcount);
                }
            }

            if (dpf_dsp_connection_point* const bridge = ctrl->connectionBridge)
            {
                if (const int refcount = bridge->refcounter)
                {
                    unclean = true;
                    d_stderr("DPF warning: asked to delete component while view bridge connection still active (refcount %d)", refcount);
                }
            }
#endif
        }

        if (unclean)
            return handleUncleanComponent(componentptr);

        d_stdout("dpf_component::unref => %p | refcount is zero, deleting everything now!", self);

        if (component->hostContextFromFactory != nullptr)
            v3_cpp_obj_unref(component->hostContextFromFactory);

        delete component;
        delete componentptr;
        return 0;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_plugin_base

    static v3_result V3_API initialize(void* const self, v3_plugin_base::v3_funknown** const context)
    {
        dpf_component* const component = *static_cast<dpf_component**>(self);

        // check if already initialized
        DISTRHO_SAFE_ASSERT_RETURN(component->vst3 == nullptr, V3_INVALID_ARG);

        // query for host context
        v3_host_application** host = nullptr;
        if (context != nullptr)
            v3_cpp_obj_query_interface(context, v3_host_application_iid, &host);

        d_stdout("dpf_component::initialize => %p %p | host %p", self, context, host);

        // save it for later so we can unref it
        component->hostContextFromInitialize = host;

        // provide the factory context to the plugin if this new one is missing
        if (host == nullptr)
            host = component->hostContextFromFactory;

        // default early values
        if (d_nextBufferSize == 0)
            d_nextBufferSize = 2048;
        if (d_nextSampleRate <= 0.0)
            d_nextSampleRate = 44100.0;

        d_nextCanRequestParameterValueChanges = true;

        // create the actual plugin
        component->vst3 = new PluginVst3(host);

        return V3_OK;
    }

    static v3_result V3_API terminate(void* const self)
    {
        d_stdout("dpf_component::terminate => %p", self);
        dpf_component* const component = *static_cast<dpf_component**>(self);

        // check if already terminated
        DISTRHO_SAFE_ASSERT_RETURN(component->vst3 != nullptr, V3_INVALID_ARG);

        // delete actual plugin
        component->vst3 = nullptr;

        // unref context received during initialize
        if (component->hostContextFromInitialize != nullptr)
        {
            v3_cpp_obj_unref(component->hostContextFromInitialize);
            component->hostContextFromInitialize = nullptr;
        }

        return V3_OK;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_component

    static v3_result V3_API get_controller_class_id(void*, v3_tuid class_id)
    {
        d_stdout("dpf_component::get_controller_class_id => %p", class_id);

#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
        return V3_NOT_IMPLEMENTED;
#else
        std::memcpy(class_id, dpf_tuid_controller, sizeof(v3_tuid));
        return V3_OK;
#endif
    }

    static v3_result V3_API set_io_mode(void* const self, const int32_t io_mode)
    {
        d_stdout("dpf_component::set_io_mode => %p %i", self, io_mode);

        dpf_component* const component = *static_cast<dpf_component**>(self);

        PluginVst3* const vst3 = component->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        // TODO
        return V3_NOT_IMPLEMENTED;
    }

    static int32_t V3_API get_bus_count(void* const self, const int32_t media_type, const int32_t bus_direction)
    {
        // NOTE runs during RT
        // d_stdout("dpf_component::get_bus_count => %p %i %i", self, media_type, bus_direction);
        dpf_component* const component = *static_cast<dpf_component**>(self);

        PluginVst3* const vst3 = component->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->getBusCount(media_type, bus_direction);
    }

    static v3_result V3_API get_bus_info(void* const self, const int32_t media_type, const int32_t bus_direction,
                                         const int32_t bus_idx, v3_bus_info* const info)
    {
        d_stdout("dpf_component::get_bus_info => %p %i %i %i %p", self, media_type, bus_direction, bus_idx, info);
        dpf_component* const component = *static_cast<dpf_component**>(self);

        PluginVst3* const vst3 = component->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->getBusInfo(media_type, bus_direction, bus_idx, info);
    }

    static v3_result V3_API get_routing_info(void* const self, v3_routing_info* const input, v3_routing_info* const output)
    {
        d_stdout("dpf_component::get_routing_info => %p %p %p", self, input, output);
        dpf_component* const component = *static_cast<dpf_component**>(self);

        PluginVst3* const vst3 = component->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->getRoutingInfo(input, output);
    }

    static v3_result V3_API activate_bus(void* const self, const int32_t media_type, const int32_t bus_direction,
                                         const int32_t bus_idx, const v3_bool state)
    {
        // NOTE this is called a bunch of times
        // d_stdout("dpf_component::activate_bus => %p %i %i %i %u", self, media_type, bus_direction, bus_idx, state);
        dpf_component* const component = *static_cast<dpf_component**>(self);

        PluginVst3* const vst3 = component->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->activateBus(media_type, bus_direction, bus_idx, state);
    }

    static v3_result V3_API set_active(void* const self, const v3_bool state)
    {
        d_stdout("dpf_component::set_active => %p %u", self, state);
        dpf_component* const component = *static_cast<dpf_component**>(self);

        PluginVst3* const vst3 = component->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return component->vst3->setActive(state);
    }

    static v3_result V3_API set_state(void* const self, v3_bstream** const stream)
    {
        d_stdout("dpf_component::set_state => %p", self);
        dpf_component* const component = *static_cast<dpf_component**>(self);

        PluginVst3* const vst3 = component->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->setState(stream);
    }

    static v3_result V3_API get_state(void* const self, v3_bstream** const stream)
    {
        d_stdout("dpf_component::get_state => %p %p", self, stream);
        dpf_component* const component = *static_cast<dpf_component**>(self);

        PluginVst3* const vst3 = component->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->getState(stream);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// Dummy plugin to get data from

static const PluginExporter& _getPluginInfo()
{
    d_nextBufferSize = 512;
    d_nextSampleRate = 44100.0;
    d_nextPluginIsDummy = true;
    d_nextCanRequestParameterValueChanges = true;
    static const PluginExporter gPluginInfo(nullptr, nullptr, nullptr);
    d_nextBufferSize = 0;
    d_nextSampleRate = 0.0;
    d_nextPluginIsDummy = false;
    d_nextCanRequestParameterValueChanges = false;

    return gPluginInfo;
}

static const PluginExporter& getPluginInfo()
{
    static const PluginExporter& info(_getPluginInfo());
    return info;
}

static const char* getPluginCategories()
{
    static String categories;
    static bool firstInit = true;

    if (firstInit)
    {
#ifdef DISTRHO_PLUGIN_VST3_CATEGORIES
        categories = DISTRHO_PLUGIN_VST3_CATEGORIES;
#elif DISTRHO_PLUGIN_IS_SYNTH
        categories = "Instrument";
#endif
#if (DISTRHO_PLUGIN_NUM_INPUTS == 0 || DISTRHO_PLUGIN_NUM_INPUTS == 1) && DISTRHO_PLUGIN_NUM_OUTPUTS == 1
        if (categories.isNotEmpty())
            categories += "|";
        categories += "Mono";
#elif (DISTRHO_PLUGIN_NUM_INPUTS == 0 || DISTRHO_PLUGIN_NUM_INPUTS == 2) && DISTRHO_PLUGIN_NUM_OUTPUTS == 2
        if (categories.isNotEmpty())
            categories += "|";
        categories += "Stereo";
#endif
        firstInit = false;
    }

    return categories.buffer();
}

static const char* getPluginVersion()
{
    static String version;

    if (version.isEmpty())
    {
        const uint32_t versionNum = getPluginInfo().getVersion();

        char versionBuf[64];
        snprintf(versionBuf, sizeof(versionBuf)-1, "%d.%d.%d",
                 (versionNum >> 16) & 0xff,
                 (versionNum >>  8) & 0xff,
                 (versionNum >>  0) & 0xff);
        versionBuf[sizeof(versionBuf)-1] = '\0';
        version = versionBuf;
    }

    return version.buffer();
}

// --------------------------------------------------------------------------------------------------------------------
// dpf_factory

struct dpf_factory : v3_plugin_factory_cpp {
    // cached values
    v3_funknown** hostContext;

    dpf_factory()
        : hostContext(nullptr)
    {
        dpf_tuid_class[2] = dpf_tuid_component[2] = dpf_tuid_controller[2]
            = dpf_tuid_processor[2] = dpf_tuid_view[2] = getPluginInfo().getUniqueId();

        // v3_funknown, static
        query_interface = query_interface_factory;
        ref = dpf_static_ref;
        unref = dpf_static_unref;

        // v3_plugin_factory
        v1.get_factory_info = get_factory_info;
        v1.num_classes = num_classes;
        v1.get_class_info = get_class_info;
        v1.create_instance = create_instance;

        // v3_plugin_factory_2
        v2.get_class_info_2 = get_class_info_2;

        // v3_plugin_factory_3
        v3.get_class_info_utf16 = get_class_info_utf16;
        v3.set_host_context = set_host_context;
    }

    ~dpf_factory()
    {
        // unref old context if there is one
        if (hostContext != nullptr)
            v3_cpp_obj_unref(hostContext);

        if (gComponentGarbage.size() == 0)
            return;

        d_stdout("DPF notice: cleaning up previously undeleted components now");

        for (std::vector<dpf_component**>::iterator it = gComponentGarbage.begin();
            it != gComponentGarbage.end(); ++it)
        {
            dpf_component** const componentptr = *it;
            dpf_component* const component = *componentptr;
            component->cleanup();
            delete component;
            delete componentptr;
        }

        gComponentGarbage.clear();
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_funknown

    static v3_result V3_API query_interface_factory(void* const self, const v3_tuid iid, void** const iface)
    {
        d_stdout("query_interface_factory => %p %s %p", self, tuid2str(iid), iface);

        if (v3_tuid_match(iid, v3_funknown_iid) ||
            v3_tuid_match(iid, v3_plugin_factory_iid) ||
            v3_tuid_match(iid, v3_plugin_factory_2_iid) ||
            v3_tuid_match(iid, v3_plugin_factory_3_iid))
        {
            *iface = self;
            return V3_OK;
        }

        *iface = nullptr;
        return V3_NO_INTERFACE;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_plugin_factory

    static v3_result V3_API get_factory_info(void*, v3_factory_info* const info)
    {
        d_stdout("dpf_factory::get_factory_info => %p", info);
        std::memset(info, 0, sizeof(*info));

        info->flags = 0x10;
        DISTRHO_NAMESPACE::strncpy(info->vendor, getPluginInfo().getMaker(), ARRAY_SIZE(info->vendor));
        DISTRHO_NAMESPACE::strncpy(info->url, getPluginInfo().getHomePage(), ARRAY_SIZE(info->url));
        // DISTRHO_NAMESPACE::strncpy(info->email, "", ARRAY_SIZE(info->email)); // TODO
        return V3_OK;
    }

    static int32_t V3_API num_classes(void*)
    {
        d_stdout("dpf_factory::num_classes");
        return 1;
    }

    static v3_result V3_API get_class_info(void*, const int32_t idx, v3_class_info* const info)
    {
        d_stdout("dpf_factory::get_class_info => %i %p", idx, info);
        std::memset(info, 0, sizeof(*info));
        DISTRHO_SAFE_ASSERT_RETURN(idx == 0, V3_INVALID_ARG);

        info->cardinality = 0x7FFFFFFF;
        std::memcpy(info->class_id, dpf_tuid_class, sizeof(v3_tuid));
        DISTRHO_NAMESPACE::strncpy(info->category, "Audio Module Class", ARRAY_SIZE(info->category));
        DISTRHO_NAMESPACE::strncpy(info->name, getPluginInfo().getName(), ARRAY_SIZE(info->name));
        return V3_OK;
    }

    static v3_result V3_API create_instance(void* self, const v3_tuid class_id, const v3_tuid iid, void** const instance)
    {
        d_stdout("dpf_factory::create_instance => %p %s %s %p", self, tuid2str(class_id), tuid2str(iid), instance);
        dpf_factory* const factory = *static_cast<dpf_factory**>(self);

        // query for host context
        v3_host_application** host = nullptr;
        if (factory->hostContext != nullptr)
            v3_cpp_obj_query_interface(factory->hostContext, v3_host_application_iid, &host);

        // create component
        if (v3_tuid_match(class_id, *(const v3_tuid*)&dpf_tuid_class) && v3_tuid_match(iid, v3_component_iid))
        {
            dpf_component** const componentptr = new dpf_component*;
            *componentptr = new dpf_component(host);
            *instance = static_cast<void*>(componentptr);
            return V3_OK;
        }

        // create edit controller
        if (v3_tuid_match(class_id, *(const v3_tuid*)&dpf_tuid_controller) && v3_tuid_match(iid, v3_edit_controller_iid))
        {
            dpf_edit_controller** const controllerptr = new dpf_edit_controller*;
            *controllerptr = new dpf_edit_controller(host);
            *instance = static_cast<void*>(controllerptr);
            return V3_OK;
        }

        // unsupported, roll back host application
        if (host != nullptr)
            v3_cpp_obj_unref(host);

        return V3_NO_INTERFACE;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_plugin_factory_2

    static v3_result V3_API get_class_info_2(void*, const int32_t idx, v3_class_info_2* const info)
    {
        d_stdout("dpf_factory::get_class_info_2 => %i %p", idx, info);
        std::memset(info, 0, sizeof(*info));
        DISTRHO_SAFE_ASSERT_RETURN(idx == 0, V3_INVALID_ARG);

        info->cardinality = 0x7FFFFFFF;
#ifdef DPF_VST3_USES_SEPARATE_CONTROLLER
        info->class_flags = V3_DISTRIBUTABLE;
#endif
        std::memcpy(info->class_id, dpf_tuid_class, sizeof(v3_tuid));
        DISTRHO_NAMESPACE::strncpy(info->category, "Audio Module Class", ARRAY_SIZE(info->category));
        DISTRHO_NAMESPACE::strncpy(info->sub_categories, getPluginCategories(), ARRAY_SIZE(info->sub_categories));
        DISTRHO_NAMESPACE::strncpy(info->name, getPluginInfo().getName(), ARRAY_SIZE(info->name));
        DISTRHO_NAMESPACE::strncpy(info->vendor, getPluginInfo().getMaker(), ARRAY_SIZE(info->vendor));
        DISTRHO_NAMESPACE::strncpy(info->version, getPluginVersion(), ARRAY_SIZE(info->version));
        DISTRHO_NAMESPACE::strncpy(info->sdk_version, "Travesty", ARRAY_SIZE(info->sdk_version));
        return V3_OK;
    }

    // ------------------------------------------------------------------------------------------------------------
    // v3_plugin_factory_3

    static v3_result V3_API get_class_info_utf16(void*, const int32_t idx, v3_class_info_3* const info)
    {
        d_stdout("dpf_factory::get_class_info_utf16 => %i %p", idx, info);
        std::memset(info, 0, sizeof(*info));
        DISTRHO_SAFE_ASSERT_RETURN(idx == 0, V3_INVALID_ARG);

        info->cardinality = 0x7FFFFFFF;
#ifdef DPF_VST3_USES_SEPARATE_CONTROLLER
        info->class_flags = V3_DISTRIBUTABLE;
#endif
        std::memcpy(info->class_id, dpf_tuid_class, sizeof(v3_tuid));
        DISTRHO_NAMESPACE::strncpy(info->category, "Audio Module Class", ARRAY_SIZE(info->category));
        DISTRHO_NAMESPACE::strncpy(info->sub_categories, getPluginCategories(), ARRAY_SIZE(info->sub_categories));
        DISTRHO_NAMESPACE::strncpy_utf16(info->name, getPluginInfo().getName(), ARRAY_SIZE(info->name));
        DISTRHO_NAMESPACE::strncpy_utf16(info->vendor, getPluginInfo().getMaker(), ARRAY_SIZE(info->vendor));
        DISTRHO_NAMESPACE::strncpy_utf16(info->version, getPluginVersion(), ARRAY_SIZE(info->version));
        DISTRHO_NAMESPACE::strncpy_utf16(info->sdk_version, "Travesty", ARRAY_SIZE(info->sdk_version));
        return V3_OK;
    }

    static v3_result V3_API set_host_context(void* const self, v3_funknown** const context)
    {
        d_stdout("dpf_factory::set_host_context => %p %p", self, context);
        dpf_factory* const factory = *static_cast<dpf_factory**>(self);

        // unref old context if there is one
        if (factory->hostContext != nullptr)
            v3_cpp_obj_unref(factory->hostContext);

        // store new context
        factory->hostContext = context;

        // make sure the object keeps being valid for a while
        if (context != nullptr)
            v3_cpp_obj_ref(context);

        return V3_OK;
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
    // find plugin bundle
    USE_NAMESPACE_DISTRHO;
    static String bundlePath;
    if (bundlePath.isEmpty())
    {
        String tmpPath(getBinaryFilename());
        tmpPath.truncate(tmpPath.rfind(DISTRHO_OS_SEP));
        tmpPath.truncate(tmpPath.rfind(DISTRHO_OS_SEP));
        DISTRHO_SAFE_ASSERT_RETURN(tmpPath.endsWith("/Contents"), true);

        tmpPath.truncate(tmpPath.rfind('/'));
        bundlePath = tmpPath;
        d_nextBundlePath = bundlePath.buffer();
    }

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
