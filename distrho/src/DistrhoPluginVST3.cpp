/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2024 Filipe Coelho <falktx@falktx.com>
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

/* TODO items:
 * == parameters
 * - test parameter triggers
 * - have parameter outputs host-provided UI working in at least 1 host
 * - parameter groups via unit ids
 * - test parameter changes from DSP (aka requestParameterValueChange)
 * - implement getParameterNormalized/setParameterNormalized for MIDI CC params ?
 * - verify that latency changes works (with and without DPF_VST3_USES_SEPARATE_CONTROLLER)
 * == MIDI
 * - MIDI CC changes (need to store value to give to the host?)
 * - MIDI program changes
 * - MIDI sysex
 * == BUSES
 * - routing info, do we care?
 * == CV
 * - cv scaling to -1/+1
 * - test in at least 1 host
 * == INFO
 * - set factory email (needs new DPF API, useful for LV2 as well)
 * - do something with set_io_mode?
 */

#include "DistrhoPluginInternal.hpp"
#include "../DistrhoPluginUtils.hpp"
#include "../extra/ScopedPointer.hpp"

#define DPF_VST3_MAX_BUFFER_SIZE 32768
#define DPF_VST3_MAX_SAMPLE_RATE 384000
#define DPF_VST3_MAX_LATENCY     DPF_VST3_MAX_SAMPLE_RATE * 10

#if DISTRHO_PLUGIN_HAS_UI
# include "../extra/RingBuffer.hpp"
#endif

#include "travesty/audio_processor.h"
#include "travesty/component.h"
#include "travesty/edit_controller.h"
#include "travesty/factory.h"
#include "travesty/host.h"

#include <map>
#include <string>
#include <vector>

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

#if defined(DISTRHO_PLUGIN_BRAND_ID) && !defined(DPF_VST3_DONT_USE_BRAND_ID)
static constexpr const uint32_t dpf_id_brand = d_cconst(STRINGIFY(DISTRHO_PLUGIN_BRAND_ID));
#else
static constexpr const uint32_t dpf_id_brand = 0;
#endif

static dpf_tuid dpf_tuid_class = { dpf_id_entry, dpf_id_clas, 0, dpf_id_brand };
static dpf_tuid dpf_tuid_component = { dpf_id_entry, dpf_id_comp, 0, dpf_id_brand };
static dpf_tuid dpf_tuid_controller = { dpf_id_entry, dpf_id_ctrl, 0, dpf_id_brand };
static dpf_tuid dpf_tuid_processor = { dpf_id_entry, dpf_id_proc, 0, dpf_id_brand };
static dpf_tuid dpf_tuid_view = { dpf_id_entry, dpf_id_view, 0, dpf_id_brand };

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
        { V3_ID(0xB7F8F859,0x41234872,0x91169581,0x4F3721A3), "{v3_edit_controller_note_expression_controller|NOT}" },
        // units
        { V3_ID(0x8683B01F,0x7B354F70,0xA2651DEC,0x353AF4FF), "{v3_program_list_data|NOT}" },
        { V3_ID(0x6C389611,0xD391455D,0xB870B833,0x94A0EFDD), "{v3_unit_data|NOT}" },
        { V3_ID(0x4B5147F8,0x4654486B,0x8DAB30BA,0x163A3C56), "{v3_unit_handler|NOT}" },
        { V3_ID(0xF89F8CDF,0x699E4BA5,0x96AAC9A4,0x81452B01), "{v3_unit_handler2|NOT}" },
        { V3_ID(0x3D4BD6B5,0x913A4FD2,0xA886E768,0xA5EB92C1), "{v3_unit_info|NOT}" },
        // misc
        { V3_ID(0x309ECE78,0xEB7D4FAE,0x8B2225D9,0x09FD08B6), "{v3_audio_presentation_latency|NOT}" },
        { V3_ID(0xB4E8287F,0x1BB346AA,0x83A46667,0x68937BAB), "{v3_automation_state|NOT}" },
        { V3_ID(0x0F194781,0x8D984ADA,0xBBA0C1EF,0xC011D8D0), "{v3_info_listener|NOT}" },
        { V3_ID(0x6D21E1DC,0x91199D4B,0xA2A02FEF,0x6C1AE55C), "{v3_parameter_function_name|NOT}" },
        { V3_ID(0x8AE54FDA,0xE93046B9,0xA28555BC,0xDC98E21E), "{v3_prefetchable_support|NOT}" },
        { V3_ID(0xA81A0471,0x48C34DC4,0xAC30C9E1,0x3C8393D5), "{v3_xml_representation_stream|NOT}" },
        /*
        // seen in the wild but unknown, related to component
        { V3_ID(0x6548D671,0x997A4EA5,0x9B336A6F,0xB3E93B50), "{v3_|NOT}" },
        { V3_ID(0xC2B7896B,0x069844D5,0x8F06E937,0x33A35FF7), "{v3_|NOT}" },
        { V3_ID(0xE123DE93,0xE0F642A4,0xAE53867E,0x53F059EE), "{v3_|NOT}" },
        { V3_ID(0x83850D7B,0xC12011D8,0xA143000A,0x959B31C6), "{v3_|NOT}" },
        { V3_ID(0x9598D418,0xA00448AC,0x9C6D8248,0x065B2E5C), "{v3_|NOT}" },
        { V3_ID(0xBD386132,0x45174BAD,0xA324390B,0xFD297506), "{v3_|NOT}" },
        { V3_ID(0xD7296A84,0x23B1419C,0xAAD0FAA3,0x53BB16B7), "{v3_|NOT}" },
        { V3_ID(0x181A0AF6,0xA10947BA,0x8A6F7C7C,0x3FF37129), "{v3_|NOT}" },
        { V3_ID(0xC2B7896B,0x69A844D5,0x8F06E937,0x33A35FF7), "{v3_|NOT}" },
        // seen in the wild but unknown, related to edit controller
        { V3_ID(0x1F2F76D3,0xBFFB4B96,0xB99527A5,0x5EBCCEF4), "{v3_|NOT}" },
        { V3_ID(0x6B2449CC,0x419740B5,0xAB3C79DA,0xC5FE5C86), "{v3_|NOT}" },
        { V3_ID(0x67800560,0x5E784D90,0xB97BAB4C,0x8DC5BAA3), "{v3_|NOT}" },
        { V3_ID(0xDB51DA00,0x8FD5416D,0xB84894D8,0x7FDE73E4), "{v3_|NOT}" },
        { V3_ID(0xE90FC54F,0x76F24235,0x8AF8BD15,0x68C663D6), "{v3_|NOT}" },
        { V3_ID(0x07938E89,0xBA0D4CA8,0x8C7286AB,0xA9DDA95B), "{v3_|NOT}" },
        { V3_ID(0x42879094,0xA2F145ED,0xAC90E82A,0x99458870), "{v3_|NOT}" },
        { V3_ID(0xC3B17BC0,0x2C174494,0x80293402,0xFBC4BBF8), "{v3_|NOT}" },
        { V3_ID(0x31E29A7A,0xE55043AD,0x8B95B9B8,0xDA1FBE1E), "{v3_|NOT}" },
        { V3_ID(0x8E3C292C,0x95924F9D,0xB2590B1E,0x100E4198), "{v3_|NOT}" },
        { V3_ID(0x50553FD9,0x1D2C4C24,0xB410F484,0xC5FB9F3F), "{v3_|NOT}" },
        { V3_ID(0xF185556C,0x5EE24FC7,0x92F28754,0xB7759EA8), "{v3_|NOT}" },
        { V3_ID(0xD2CE9317,0xF24942C9,0x9742E82D,0xB10CCC52), "{v3_|NOT}" },
        { V3_ID(0xDA57E6D1,0x1F3242D1,0xAD9C1A82,0xFDB95695), "{v3_|NOT}" },
        { V3_ID(0x3ABDFC3E,0x4B964A66,0xFCD86F10,0x0D554023), "{v3_|NOT}" },
        // seen in the wild but unknown, related to view
        { V3_ID(0xAA3E50FF,0xB78840EE,0xADCD48E8,0x094CEDB7), "{v3_|NOT}" },
        { V3_ID(0x2CAE14DB,0x4DE04C6E,0x8BD2E611,0x1B31A9C2), "{v3_|NOT}" },
        { V3_ID(0xD868D61D,0x20F445F4,0x947D069E,0xC811D1E4), "{v3_|NOT}" },
        { V3_ID(0xEE49E3CA,0x6FCB44FB,0xAEBEE6C3,0x48625122), "{v3_|NOT}" },
        */
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
// dpf_plugin_view_create (implemented on UI side)

v3_plugin_view** dpf_plugin_view_create(v3_host_application** host, void* instancePointer, double sampleRate);

// --------------------------------------------------------------------------------------------------------------------

/**
 * VST3 DSP class.
 *
 * All the dynamic things from VST3 get implemented here, free of complex low-level VST3 pointer things.
 * This class is created during the "initialize" component event, and destroyed during "terminate".
 *
 * The low-level VST3 stuff comes after.
 */
class PluginVst3
{
    /* Buses: count possible buses we can provide to the host, in case they are not yet defined by the developer.
     * These values are only used if port groups aren't set.
     *
     * When port groups are not in use:
     * - 1 bus is provided for the main audio (if there is any)
     * - 1 for sidechain
     * - 1 for each cv port
     * So basically:
     * Main audio is used as first bus, if available.
     * Then sidechain, also if available.
     * And finally each CV port individually.
     *
     * MIDI will have a single bus, nothing special there.
     */
    struct BusInfo {
        uint8_t audio;     // either 0 or 1
        uint8_t sidechain; // either 0 or 1
        uint32_t groups;
        uint32_t audioPorts;
        uint32_t sidechainPorts;
        uint32_t groupPorts;
        uint32_t cvPorts;

        BusInfo()
          : audio(0),
            sidechain(0),
            groups(0),
            audioPorts(0),
            sidechainPorts(0),
            groupPorts(0),
            cvPorts(0) {}
    } inputBuses, outputBuses;

   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    /* Handy class for storing and sorting VST3 events and MIDI CC parameters.
     * It will only store events for which a MIDI conversion is possible.
     */
    struct InputEventList {
        enum Type {
            NoteOn,
            NoteOff,
            SysexData,
            PolyPressure,
            CC_Normal,
            CC_ChannelPressure,
            CC_Pitchbend,
            UI_MIDI // event from UI
        };
        struct InputEventStorage {
            Type type;
            union {
                v3_event_note_on noteOn;
                v3_event_note_off noteOff;
                v3_event_data sysexData;
                v3_event_poly_pressure polyPressure;
                uint8_t midi[3];
            };
        } eventListStorage[kMaxMidiEvents];

        struct InputEvent {
            int32_t sampleOffset;
            const InputEventStorage* storage;
            InputEvent* next;
        } eventList[kMaxMidiEvents];

        uint16_t numUsed;
        int32_t firstSampleOffset;
        int32_t lastSampleOffset;
        InputEvent* firstEvent;
        InputEvent* lastEvent;

        void init()
        {
            numUsed = 0;
            firstSampleOffset = lastSampleOffset = 0;
            firstEvent = nullptr;
        }

        uint32_t convert(MidiEvent midiEvents[kMaxMidiEvents]) const noexcept
        {
            uint32_t count = 0;

            for (const InputEvent* event = firstEvent; event != nullptr; event = event->next)
            {
                MidiEvent& midiEvent(midiEvents[count++]);
                midiEvent.frame = event->sampleOffset;

                const InputEventStorage& eventStorage(*event->storage);

                switch (eventStorage.type)
                {
                case NoteOn:
                    midiEvent.size = 3;
                    midiEvent.data[0] = 0x90 | (eventStorage.noteOn.channel & 0xf);
                    midiEvent.data[1] = eventStorage.noteOn.pitch;
                    midiEvent.data[2] = std::max(0, std::min(127, (int)(eventStorage.noteOn.velocity * 127)));
                    midiEvent.data[3] = 0;
                    break;
                case NoteOff:
                    midiEvent.size = 3;
                    midiEvent.data[0] = 0x80 | (eventStorage.noteOff.channel & 0xf);
                    midiEvent.data[1] = eventStorage.noteOff.pitch;
                    midiEvent.data[2] = std::max(0, std::min(127, (int)(eventStorage.noteOff.velocity * 127)));
                    midiEvent.data[3] = 0;
                    break;
                /* TODO
                case SysexData:
                    break;
                */
                case PolyPressure:
                    midiEvent.size = 3;
                    midiEvent.data[0] = 0xA0 | (eventStorage.polyPressure.channel & 0xf);
                    midiEvent.data[1] = eventStorage.polyPressure.pitch;
                    midiEvent.data[2] = std::max(0, std::min(127, (int)(eventStorage.polyPressure.pressure * 127)));
                    midiEvent.data[3] = 0;
                    break;
                case CC_Normal:
                    midiEvent.size = 3;
                    midiEvent.data[0] = 0xB0 | (eventStorage.midi[0] & 0xf);
                    midiEvent.data[1] = eventStorage.midi[1];
                    midiEvent.data[2] = eventStorage.midi[2];
                    break;
                case CC_ChannelPressure:
                    midiEvent.size = 2;
                    midiEvent.data[0] = 0xD0 | (eventStorage.midi[0] & 0xf);
                    midiEvent.data[1] = eventStorage.midi[1];
                    midiEvent.data[2] = 0;
                    break;
                case CC_Pitchbend:
                    midiEvent.size = 3;
                    midiEvent.data[0] = 0xE0 | (eventStorage.midi[0] & 0xf);
                    midiEvent.data[1] = eventStorage.midi[1];
                    midiEvent.data[2] = eventStorage.midi[2];
                    break;
                case UI_MIDI:
                    midiEvent.size = 3;
                    midiEvent.data[0] = eventStorage.midi[0];
                    midiEvent.data[1] = eventStorage.midi[1];
                    midiEvent.data[2] = eventStorage.midi[2];
                    break;
                default:
                    midiEvent.size = 0;
                    break;
                }
            }

            return count;
        }

        bool appendEvent(const v3_event& event) noexcept
        {
            // only save events that can be converted directly into MIDI
            switch (event.type)
            {
            case V3_EVENT_NOTE_ON:
            case V3_EVENT_NOTE_OFF:
            // case V3_EVENT_DATA:
            case V3_EVENT_POLY_PRESSURE:
                break;
            default:
                return false;
            }

            InputEventStorage& eventStorage(eventListStorage[numUsed]);

            switch (event.type)
            {
            case V3_EVENT_NOTE_ON:
                eventStorage.type = NoteOn;
                eventStorage.noteOn = event.note_on;
                break;
            case V3_EVENT_NOTE_OFF:
                eventStorage.type = NoteOff;
                eventStorage.noteOff = event.note_off;
                break;
            case V3_EVENT_DATA:
                eventStorage.type = SysexData;
                eventStorage.sysexData = event.data;
                break;
            case V3_EVENT_POLY_PRESSURE:
                eventStorage.type = PolyPressure;
                eventStorage.polyPressure = event.poly_pressure;
                break;
            default:
                return false;
            }

            eventList[numUsed].sampleOffset = event.sample_offset;
            eventList[numUsed].storage = &eventStorage;

            return placeSorted(event.sample_offset);
        }

        bool appendCC(const int32_t sampleOffset, v3_param_id paramId, const double normalized) noexcept
        {
            InputEventStorage& eventStorage(eventListStorage[numUsed]);

            paramId -= kVst3InternalParameterMidiCC_start;

            const uint8_t cc = paramId % 130;

            switch (cc)
            {
            case 128:
                eventStorage.type = CC_ChannelPressure;
                eventStorage.midi[1] = std::max(0, std::min(127, (int)(normalized * 127)));
                eventStorage.midi[2] = 0;
                break;
            case 129:
                eventStorage.type = CC_Pitchbend;
                eventStorage.midi[1] = std::max(0, std::min(16384, (int)(normalized * 16384))) & 0x7f;
                eventStorage.midi[2] = std::max(0, std::min(16384, (int)(normalized * 16384))) >> 7;
                break;
            default:
                eventStorage.type = CC_Normal;
                eventStorage.midi[1] = cc;
                eventStorage.midi[2] = std::max(0, std::min(127, (int)(normalized * 127)));
                break;
            }

            eventStorage.midi[0] = paramId / 130;

            eventList[numUsed].sampleOffset = sampleOffset;
            eventList[numUsed].storage = &eventStorage;

            return placeSorted(sampleOffset);
        }

       #if DISTRHO_PLUGIN_HAS_UI
        // NOTE always runs first
        bool appendFromUI(const uint8_t midiData[3])
        {
            InputEventStorage& eventStorage(eventListStorage[numUsed]);

            eventStorage.type = UI_MIDI;
            std::memcpy(eventStorage.midi, midiData, sizeof(uint8_t)*3);

            InputEvent* const event = &eventList[numUsed];

            event->sampleOffset = 0;
            event->storage = &eventStorage;
            event->next = nullptr;

            if (numUsed == 0)
            {
                firstEvent = lastEvent = event;
            }
            else
            {
                lastEvent->next = event;
                lastEvent = event;
            }

            return ++numUsed == kMaxMidiEvents;
        }
       #endif

    private:
        bool placeSorted(const int32_t sampleOffset) noexcept
        {
            InputEvent* const event = &eventList[numUsed];

            // initialize
            if (numUsed == 0)
            {
                firstSampleOffset = lastSampleOffset = sampleOffset;
                firstEvent = lastEvent = event;
                event->next = nullptr;
            }
            // push to the back
            else if (sampleOffset >= lastSampleOffset)
            {
                lastSampleOffset = sampleOffset;
                lastEvent->next = event;
                lastEvent = event;
                event->next = nullptr;
            }
            // push to the front
            else if (sampleOffset < firstSampleOffset)
            {
                firstSampleOffset = sampleOffset;
                event->next = firstEvent;
                firstEvent = event;
            }
            // find place in between events
            else
            {
                // keep reference out of the loop so we can check validity afterwards
                InputEvent* event2 = firstEvent;

                // iterate all events
                for (; event2 != nullptr; event2 = event2->next)
                {
                    // if offset is higher than iterated event, stop and insert in-between
                    if (sampleOffset > event2->sampleOffset)
                        break;

                    // if offset matches, find the last event with the same offset so we can push after it
                    if (sampleOffset == event2->sampleOffset)
                    {
                        event2 = event2->next;
                        for (; event2 != nullptr && sampleOffset == event2->sampleOffset; event2 = event2->next) {}
                        break;
                    }
                }

                DISTRHO_SAFE_ASSERT_RETURN(event2 != nullptr, true);

                event->next = event2->next;
                event2->next = event;
            }

            return ++numUsed == kMaxMidiEvents;
        }
    } inputEventList;
   #endif // DISTRHO_PLUGIN_WANT_MIDI_INPUT

public:
    PluginVst3(v3_host_application** const host, const bool isComponent)
        : fPlugin(this, writeMidiCallback, requestParameterValueChangeCallback, nullptr),
          fComponentHandler(nullptr),
        #if DISTRHO_PLUGIN_HAS_UI
         #if DPF_VST3_USES_SEPARATE_CONTROLLER
          fConnectionFromCompToCtrl(nullptr),
         #endif
          fConnectionFromCtrlToView(nullptr),
          fHostApplication(host),
        #endif
          fParameterCount(fPlugin.getParameterCount()),
          fVst3ParameterCount(fParameterCount + kVst3InternalParameterCount),
          fCachedParameterValues(nullptr),
          fDummyAudioBuffer(nullptr),
          fParameterValuesChangedDuringProcessing(nullptr)
       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        , fIsComponent(isComponent)
       #endif
       #if DISTRHO_PLUGIN_HAS_UI
        , fParameterValueChangesForUI(nullptr)
        , fConnectedToUI(false)
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
       #if !DPF_VST3_USES_SEPARATE_CONTROLLER
        DISTRHO_SAFE_ASSERT(isComponent);
       #endif

       #if DISTRHO_PLUGIN_NUM_INPUTS > 0
        std::memset(fEnabledInputs, 0, sizeof(fEnabledInputs));
        fillInBusInfoDetails<true>();
       #endif
       #if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        std::memset(fEnabledOutputs, 0, sizeof(fEnabledOutputs));
        fillInBusInfoDetails<false>();
       #endif

        if (const uint32_t extraParameterCount = fParameterCount + kVst3InternalParameterBaseCount)
        {
            fCachedParameterValues = new float[extraParameterCount];

           #if DPF_VST3_USES_SEPARATE_CONTROLLER
            fCachedParameterValues[kVst3InternalParameterBufferSize] = fPlugin.getBufferSize();
            fCachedParameterValues[kVst3InternalParameterSampleRate] = fPlugin.getSampleRate();
           #endif
           #if DISTRHO_PLUGIN_WANT_LATENCY
            fCachedParameterValues[kVst3InternalParameterLatency]    = fLastKnownLatency;
           #endif
           #if DISTRHO_PLUGIN_WANT_PROGRAMS
            fCachedParameterValues[kVst3InternalParameterProgram]    = 0.0f;
           #endif

            for (uint32_t i=0; i < fParameterCount; ++i)
                fCachedParameterValues[kVst3InternalParameterBaseCount + i] = fPlugin.getParameterDefault(i);

            fParameterValuesChangedDuringProcessing = new bool[extraParameterCount];
            std::memset(fParameterValuesChangedDuringProcessing, 0, sizeof(bool)*extraParameterCount);

           #if DISTRHO_PLUGIN_HAS_UI
            fParameterValueChangesForUI = new bool[extraParameterCount];
            std::memset(fParameterValueChangesForUI, 0, sizeof(bool)*extraParameterCount);
           #endif
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
        return; (void)host;
       #endif
    }

    ~PluginVst3()
    {
        if (fCachedParameterValues != nullptr)
        {
            delete[] fCachedParameterValues;
            fCachedParameterValues = nullptr;
        }

        if (fDummyAudioBuffer != nullptr)
        {
            delete[] fDummyAudioBuffer;
            fDummyAudioBuffer = nullptr;
        }

        if (fParameterValuesChangedDuringProcessing != nullptr)
        {
            delete[] fParameterValuesChangedDuringProcessing;
            fParameterValuesChangedDuringProcessing = nullptr;
        }

       #if DISTRHO_PLUGIN_HAS_UI
        if (fParameterValueChangesForUI != nullptr)
        {
            delete[] fParameterValueChangesForUI;
            fParameterValueChangesForUI = nullptr;
        }
       #endif
    }

    // ----------------------------------------------------------------------------------------------------------------
    // utilities and common code

    double _getNormalizedParameterValue(const uint32_t index, const double plain)
    {
        const ParameterRanges& ranges(fPlugin.getParameterRanges(index));
        return ranges.getFixedAndNormalizedValue(plain);
    }

    void _setNormalizedPluginParameterValue(const uint32_t index, const double normalized)
    {
        const ParameterRanges& ranges(fPlugin.getParameterRanges(index));
        const uint32_t hints = fPlugin.getParameterHints(index);
        float value = ranges.getUnnormalizedValue(normalized);

        // convert as needed as check for changes
        if (hints & kParameterIsBoolean)
        {
            const float midRange = ranges.min + (ranges.max - ranges.min) / 2.f;
            const bool isHigh = value > midRange;

            if (isHigh == (fCachedParameterValues[kVst3InternalParameterBaseCount + index] > midRange))
                return;

            value = isHigh ? ranges.max : ranges.min;
        }
        else if (hints & kParameterIsInteger)
        {
            const int ivalue = d_roundToInt(value);

            if (d_roundToInt(fCachedParameterValues[kVst3InternalParameterBaseCount + index]) == ivalue)
                return;

            value = ivalue;
        }
        else
        {
            // deal with low resolution of some hosts, which convert double to float internally and lose precision
            if (std::abs(ranges.getNormalizedValue(static_cast<double>(fCachedParameterValues[kVst3InternalParameterBaseCount + index])) - normalized) < 0.0000001)
                return;
        }

        fCachedParameterValues[kVst3InternalParameterBaseCount + index] = value;

      #if DISTRHO_PLUGIN_HAS_UI
       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        if (!fIsComponent)
       #endif
        {
            fParameterValueChangesForUI[kVst3InternalParameterBaseCount + index] = true;
        }
      #endif

        if (!fPlugin.isParameterOutputOrTrigger(index))
            fPlugin.setParameterValue(index, value);
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
                return inputBuses.audio + inputBuses.sidechain + inputBuses.groups + inputBuses.cvPorts;
            if (busDirection == V3_OUTPUT)
                return outputBuses.audio + outputBuses.sidechain + outputBuses.groups + outputBuses.cvPorts;
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
            if (busDirection == V3_INPUT)
            {
               #if DISTRHO_PLUGIN_NUM_INPUTS > 0
                return getAudioBusInfo<true>(busId, info);
               #else
                d_stderr("invalid input bus %d", busId);
                return V3_INVALID_ARG;
               #endif // DISTRHO_PLUGIN_NUM_INPUTS
            }
            else
            {
               #if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
                return getAudioBusInfo<false>(busId, info);
               #else
                d_stderr("invalid output bus %d", busId);
                return V3_INVALID_ARG;
               #endif // DISTRHO_PLUGIN_NUM_OUTPUTS
            }
           #else
            d_stderr("invalid bus, line %d", __LINE__);
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
                d_stderr("invalid bus, line %d", __LINE__);
                return V3_INVALID_ARG;
               #endif
            }
            else
            {
               #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
                DISTRHO_SAFE_ASSERT_RETURN(busId == 0, V3_INVALID_ARG);
               #else
                d_stderr("invalid bus, line %d", __LINE__);
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
        /*
        output->media_type = V3_AUDIO;
        output->bus_idx = 0;
        output->channel = -1;
        d_stdout("getRoutingInfo %s %d %d",
                 v3_media_type_str(input->media_type), input->bus_idx, input->channel);
        */
        return V3_NOT_IMPLEMENTED;
    }

    v3_result activateBus(const int32_t mediaType,
                          const int32_t busDirection,
                          const int32_t busIndex,
                          const bool state) noexcept
    {
        DISTRHO_SAFE_ASSERT_INT_RETURN(busDirection == V3_INPUT || busDirection == V3_OUTPUT, busDirection, V3_INVALID_ARG);
        DISTRHO_SAFE_ASSERT_INT_RETURN(busIndex >= 0, busIndex, V3_INVALID_ARG);

        if (mediaType == V3_AUDIO)
        {
           #if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
            const uint32_t busId = static_cast<uint32_t>(busIndex);

            if (busDirection == V3_INPUT)
            {
               #if DISTRHO_PLUGIN_NUM_INPUTS > 0
                for (uint32_t i=0; i<DISTRHO_PLUGIN_NUM_INPUTS; ++i)
                {
                    const AudioPortWithBusId& port(fPlugin.getAudioPort(true, i));

                    if (port.busId == busId)
                        fEnabledInputs[i] = state;
                }
               #endif
            }
            else
            {
               #if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
                for (uint32_t i=0; i<DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
                {
                    const AudioPortWithBusId& port(fPlugin.getAudioPort(false, i));

                    if (port.busId == busId)
                        fEnabledOutputs[i] = state;
                }
               #endif
            }
           #endif
        }

        return V3_OK;

       #if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS == 0
        // unused
        (void)state;
       #endif
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
        const bool connectedToUI = fConnectionFromCtrlToView != nullptr && fConnectedToUI;
       #endif
        bool componentValuesChanged = false;
        String key, value;
        bool empty = true;
        bool hasValue = false;
        bool fillingKey = true; // if filling key or value
        char queryingType = 'i'; // can be 'n', 's' or 'p' (none, states, parameters)

        char buffer[512], orig;
        buffer[sizeof(buffer)-1] = '\xff';
        v3_result res;

        for (int32_t terminated = 0, read; terminated == 0;)
        {
            read = -1;
            res = v3_cpp_obj(stream)->read(stream, buffer, sizeof(buffer)-1, &read);
            DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);
            DISTRHO_SAFE_ASSERT_INT_RETURN(read > 0, read, V3_INTERNAL_ERR);

            if (read == 0)
                return empty ? V3_INVALID_ARG : V3_OK;

            empty = false;
            for (int32_t i = 0; i < read; ++i)
            {
                // found terminator, stop here
                if (buffer[i] == '\xfe')
                {
                    terminated = 1;
                    break;
                }

                // store character at read position
                orig = buffer[read];

                // place null character to create valid string
                buffer[read] = '\0';

                // append to temporary vars
                if (fillingKey)
                {
                    key += buffer + i;
                }
                else
                {
                    value += buffer + i;
                    hasValue = true;
                }

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
                        hasValue = false;
                        continue;
                    }
                    if (key == "__dpf_state_end__")
                    {
                        DISTRHO_SAFE_ASSERT_INT_RETURN(queryingType == 's', queryingType, V3_INTERNAL_ERR);
                        queryingType = 'n';
                        key.clear();
                        value.clear();
                        hasValue = false;
                        continue;
                    }
                    if (key == "__dpf_parameters_begin__")
                    {
                        DISTRHO_SAFE_ASSERT_INT_RETURN(queryingType == 'i' || queryingType == 'n',
                                                       queryingType, V3_INTERNAL_ERR);
                        queryingType = 'p';
                        key.clear();
                        value.clear();
                        hasValue = false;
                        continue;
                    }
                    if (key == "__dpf_parameters_end__")
                    {
                        DISTRHO_SAFE_ASSERT_INT_RETURN(queryingType == 'p', queryingType, V3_INTERNAL_ERR);
                        queryingType = 'x';
                        key.clear();
                        value.clear();
                        hasValue = false;
                        continue;
                    }

                    // no special key, swap between reading real key and value
                    fillingKey = !fillingKey;

                    // if there is no value yet keep reading until we have one
                    if (! hasValue)
                        continue;

                    if (key == "__dpf_program__")
                    {
                        DISTRHO_SAFE_ASSERT_INT_RETURN(queryingType == 'i', queryingType, V3_INTERNAL_ERR);
                        queryingType = 'n';

                        d_debug("found program '%s'", value.buffer());

                      #if DISTRHO_PLUGIN_WANT_PROGRAMS
                        const int program = std::atoi(value.buffer());
                        DISTRHO_SAFE_ASSERT_CONTINUE(program >= 0);

                        fCurrentProgram = static_cast<uint32_t>(program);
                        fPlugin.loadProgram(fCurrentProgram);

                       #if DISTRHO_PLUGIN_HAS_UI
                        if (connectedToUI)
                        {
                            fParameterValueChangesForUI[kVst3InternalParameterProgram] = false;
                            sendParameterSetToUI(kVst3InternalParameterProgram, program);
                        }
                       #endif
                      #endif
                    }
                    else if (queryingType == 's')
                    {
                        d_debug("found state '%s' '%s'", key.buffer(), value.buffer());

                       #if DISTRHO_PLUGIN_WANT_STATE
                        if (fPlugin.wantStateKey(key))
                        {
                            fStateMap[key] = value;
                            fPlugin.setState(key, value);

                           #if DISTRHO_PLUGIN_HAS_UI
                            if (connectedToUI)
                                sendStateSetToUI(key, value);
                           #endif
                        }
                       #endif
                    }
                    else if (queryingType == 'p')
                    {
                        d_debug("found parameter '%s' '%s'", key.buffer(), value.buffer());
                        float fvalue;

                        // find parameter with this symbol, and set its value
                        for (uint32_t j=0; j < fParameterCount; ++j)
                        {
                            if (fPlugin.isParameterOutputOrTrigger(j))
                                continue;
                            if (fPlugin.getParameterSymbol(j) != key)
                                continue;

                            if (fPlugin.getParameterHints(j) & kParameterIsInteger)
                            {
                                fvalue = std::atoi(value.buffer());
                            }
                            else
                            {
                                const ScopedSafeLocale ssl;
                                fvalue = std::atof(value.buffer());
                            }

                            fCachedParameterValues[kVst3InternalParameterBaseCount + j] = fvalue;

                           #if DPF_VST3_USES_SEPARATE_CONTROLLER
                            // If this is the component make sure the controller also knows about the state change
                            if (fIsComponent)
                            {
                                componentValuesChanged = true;
                                fParameterValuesChangedDuringProcessing[kVst3InternalParameterBaseCount + j] = true;
                            }
                           #else
                            componentValuesChanged = true;
                           #endif

                           #if DISTRHO_PLUGIN_HAS_UI
                            if (connectedToUI)
                            {
                                // UI parameter updates are handled outside the read loop (after host param restart)
                                fParameterValueChangesForUI[kVst3InternalParameterBaseCount + j] = true;
                            }
                           #endif
                            fPlugin.setParameterValue(j, fvalue);
                            break;
                        }
                    }

                    key.clear();
                    value.clear();
                    hasValue = false;
                }
            }
        }

        if (fComponentHandler != nullptr && componentValuesChanged)
            v3_cpp_obj(fComponentHandler)->restart_component(fComponentHandler, V3_RESTART_PARAM_VALUES_CHANGED);

       #if DISTRHO_PLUGIN_HAS_UI
        if (connectedToUI)
        {
            for (uint32_t i=0; i<fParameterCount; ++i)
            {
                if (fPlugin.isParameterOutputOrTrigger(i))
                    continue;
                fParameterValueChangesForUI[kVst3InternalParameterBaseCount + i] = false;
                sendParameterSetToUI(kVst3InternalParameterCount + i,
                                     fCachedParameterValues[kVst3InternalParameterBaseCount + i]);
            }
        }
       #endif

        return V3_OK;
    }

    v3_result getState(v3_bstream** const stream)
    {
        const uint32_t paramCount = fPlugin.getParameterCount();
       #if DISTRHO_PLUGIN_WANT_STATE
        const uint32_t stateCount = fPlugin.getStateCount();
       #else
        constexpr const uint32_t stateCount = 0;
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
            const String& key(cit->first);
            fStateMap[key] = fPlugin.getStateValue(key);
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
                const String& key(cit->first);
                const String& value(cit->second);

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
                if (fPlugin.getParameterHints(i) & kParameterIsInteger)
                    tmpStr += String(d_roundToInt(fPlugin.getParameterValue(i)));
                else
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
            res = v3_cpp_obj(stream)->write(stream, const_cast<char*>(buffer) + wrtntotal, size - wrtntotal, &wrtn);

            DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);
            DISTRHO_SAFE_ASSERT_INT_RETURN(wrtn > 0, wrtn, V3_INTERNAL_ERR);
        }

        return V3_OK;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_audio_processor interface calls

    v3_result setBusArrangements(v3_speaker_arrangement* const inputs, const int32_t numInputs,
                                 v3_speaker_arrangement* const outputs, const int32_t numOutputs)
    {
       #if DISTRHO_PLUGIN_NUM_INPUTS > 0
        DISTRHO_SAFE_ASSERT_RETURN(numInputs >= 0, V3_INVALID_ARG);
        if (!setAudioBusArrangement<true>(inputs, static_cast<uint32_t>(numInputs)))
            return V3_INTERNAL_ERR;
       #else
        DISTRHO_SAFE_ASSERT_RETURN(numInputs == 0, V3_INVALID_ARG);
        // unused
        (void)inputs;
       #endif

       #if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        DISTRHO_SAFE_ASSERT_RETURN(numOutputs >= 0, V3_INVALID_ARG);
        if (!setAudioBusArrangement<false>(outputs, static_cast<uint32_t>(numOutputs)))
            return V3_INTERNAL_ERR;
       #else
        DISTRHO_SAFE_ASSERT_RETURN(numOutputs == 0, V3_INVALID_ARG);
        // unused
        (void)outputs;
       #endif

        return V3_OK;
    }

    v3_result getBusArrangement(const int32_t busDirection, const int32_t busIndex, v3_speaker_arrangement* const speaker) const noexcept
    {
        DISTRHO_SAFE_ASSERT_INT_RETURN(busDirection == V3_INPUT || busDirection == V3_OUTPUT, busDirection, V3_INVALID_ARG);
        DISTRHO_SAFE_ASSERT_INT_RETURN(busIndex >= 0, busIndex, V3_INVALID_ARG);
        DISTRHO_SAFE_ASSERT_RETURN(speaker != nullptr, V3_INVALID_ARG);

       #if DISTRHO_PLUGIN_NUM_INPUTS > 0 || DISTRHO_PLUGIN_NUM_OUTPUTS > 0
        const uint32_t busId = static_cast<uint32_t>(busIndex);
       #endif

        if (busDirection == V3_INPUT)
        {
           #if DISTRHO_PLUGIN_NUM_INPUTS > 0
            if (getAudioBusArrangement<true>(busId, speaker))
                return V3_OK;
           #endif
            d_stderr("invalid input bus arrangement %d, line %d", busIndex, __LINE__);
            return V3_INVALID_ARG;
        }
        else
        {
           #if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
            if (getAudioBusArrangement<false>(busId, speaker))
                return V3_OK;
           #endif
            d_stderr("invalid output bus arrangement %d, line %d", busIndex, __LINE__);
            return V3_INVALID_ARG;
        }
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

        fPlugin.setSampleRate(setup->sample_rate, true);
        fPlugin.setBufferSize(setup->max_block_size, true);

      #if DPF_VST3_USES_SEPARATE_CONTROLLER
        fCachedParameterValues[kVst3InternalParameterBufferSize] = setup->max_block_size;
        fParameterValuesChangedDuringProcessing[kVst3InternalParameterBufferSize] = true;

        fCachedParameterValues[kVst3InternalParameterSampleRate] = setup->sample_rate;
        fParameterValuesChangedDuringProcessing[kVst3InternalParameterSampleRate] = true;
       #if DISTRHO_PLUGIN_HAS_UI
        fParameterValueChangesForUI[kVst3InternalParameterSampleRate] = true;
       #endif
      #endif

        if (active)
            fPlugin.activate();

        delete[] fDummyAudioBuffer;
        fDummyAudioBuffer = new float[setup->max_block_size];

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
            fPlugin.deactivateIfNeeded();
        }

        return V3_OK;
    }

    v3_result process(v3_process_data* const data)
    {
        DISTRHO_SAFE_ASSERT_RETURN(data->symbolic_sample_size == V3_SAMPLE_32, V3_INVALID_ARG);
        // d_debug("process %i", data->symbolic_sample_size);

        // activate plugin if not done yet
        if (! fPlugin.isActive())
            fPlugin.activate();

       #if DISTRHO_PLUGIN_WANT_TIMEPOS
        if (v3_process_context* const ctx = data->ctx)
        {
            fTimePosition.playing = ctx->state & V3_PROCESS_CTX_PLAYING;

            // ticksPerBeat is not possible with VST3
            fTimePosition.bbt.ticksPerBeat = 1920.0;

            if (ctx->state & V3_PROCESS_CTX_PROJECT_TIME_VALID)
                fTimePosition.frame = ctx->project_time_in_samples;
            else if (ctx->state & V3_PROCESS_CTX_CONT_TIME_VALID)
                fTimePosition.frame = ctx->continuous_time_in_samples;

            if (ctx->state & V3_PROCESS_CTX_TEMPO_VALID)
                fTimePosition.bbt.beatsPerMinute = ctx->bpm;
            else
                fTimePosition.bbt.beatsPerMinute = 120.0;

            if ((ctx->state & (V3_PROCESS_CTX_PROJECT_TIME_VALID|V3_PROCESS_CTX_TIME_SIG_VALID)) == (V3_PROCESS_CTX_PROJECT_TIME_VALID|V3_PROCESS_CTX_TIME_SIG_VALID))
            {
                const double ppqPos    = std::abs(ctx->project_time_quarters);
                const int    ppqPerBar = ctx->time_sig_numerator * 4 / ctx->time_sig_denom;
                const double barBeats  = (std::fmod(ppqPos, ppqPerBar) / ppqPerBar) * ctx->time_sig_numerator;
                const double rest      =  std::fmod(barBeats, 1.0);

                fTimePosition.bbt.valid       = true;
                fTimePosition.bbt.bar         = static_cast<int32_t>(ppqPos) / ppqPerBar + 1;
                fTimePosition.bbt.beat        = static_cast<int32_t>(barBeats - rest) + 1;
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

            fPlugin.setTimePosition(fTimePosition);
        }
       #endif

        if (data->nframes <= 0)
        {
            updateParametersFromProcessing(data->output_params, 0);
            return V3_OK;
        }

        const float* inputs[DISTRHO_PLUGIN_NUM_INPUTS != 0 ? DISTRHO_PLUGIN_NUM_INPUTS : 1];
        /* */ float* outputs[DISTRHO_PLUGIN_NUM_OUTPUTS != 0 ? DISTRHO_PLUGIN_NUM_OUTPUTS : 1];

        std::memset(fDummyAudioBuffer, 0, sizeof(float)*data->nframes);

        {
            int32_t i = 0;
           #if DISTRHO_PLUGIN_NUM_INPUTS > 0
            if (data->inputs != nullptr)
            {
                for (int32_t b = 0; b < data->num_input_buses; ++b) {
                    for (int32_t j = 0; j < data->inputs[b].num_channels; ++j)
                    {
                        DISTRHO_SAFE_ASSERT_INT_BREAK(i < DISTRHO_PLUGIN_NUM_INPUTS, i);
                        if (!fEnabledInputs[i] && i < DISTRHO_PLUGIN_NUM_INPUTS) {
                            inputs[i++] = fDummyAudioBuffer;
                            continue;
                        }

                        inputs[i++] = data->inputs[b].channel_buffers_32[j];
                    }
                }
            }
           #endif
            for (; i < std::max(1, DISTRHO_PLUGIN_NUM_INPUTS); ++i)
                inputs[i] = fDummyAudioBuffer;
        }

        {
            int32_t i = 0;
           #if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
            if (data->outputs != nullptr)
            {
                for (int32_t b = 0; b < data->num_output_buses; ++b) {
                    for (int32_t j = 0; j < data->outputs[b].num_channels; ++j)
                    {
                        DISTRHO_SAFE_ASSERT_INT_BREAK(i < DISTRHO_PLUGIN_NUM_OUTPUTS, i);
                        if (!fEnabledOutputs[i] && i < DISTRHO_PLUGIN_NUM_OUTPUTS) {
                            outputs[i++] = fDummyAudioBuffer;
                            continue;
                        }

                        outputs[i++] = data->outputs[b].channel_buffers_32[j];
                    }
                }
            }
           #endif
            for (; i < std::max(1, DISTRHO_PLUGIN_NUM_OUTPUTS); ++i)
                outputs[i] = fDummyAudioBuffer;
        }

       #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        fHostEventOutputHandle = data->output_events;
       #endif

      #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        bool canAppendMoreEvents = true;
        inputEventList.init();

       #if DISTRHO_PLUGIN_HAS_UI
        while (fNotesRingBuffer.isDataAvailableForReading())
        {
            uint8_t midiData[3];
            if (! fNotesRingBuffer.readCustomData(midiData, 3))
                break;

            if (inputEventList.appendFromUI(midiData))
            {
                canAppendMoreEvents = false;
                break;
            }
        }
       #endif

        if (canAppendMoreEvents)
        {
            if (v3_event_list** const eventptr = data->input_events)
            {
                v3_event event;
                for (uint32_t i = 0, count = v3_cpp_obj(eventptr)->get_event_count(eventptr); i < count; ++i)
                {
                    if (v3_cpp_obj(eventptr)->get_event(eventptr, i, &event) != V3_OK)
                        break;

                    if (inputEventList.appendEvent(event))
                    {
                        canAppendMoreEvents = false;
                        break;
                    }
                }
            }
        }
      #endif

        if (v3_param_changes** const inparamsptr = data->input_params)
        {
            int32_t offset;
            double normalized;

            for (int32_t i = 0, count = v3_cpp_obj(inparamsptr)->get_param_count(inparamsptr); i < count; ++i)
            {
                v3_param_value_queue** const queue = v3_cpp_obj(inparamsptr)->get_param_data(inparamsptr, i);
                DISTRHO_SAFE_ASSERT_BREAK(queue != nullptr);

                const v3_param_id rindex = v3_cpp_obj(queue)->get_param_id(queue);
                DISTRHO_SAFE_ASSERT_UINT_BREAK(rindex < fVst3ParameterCount, rindex);

               #if DPF_VST3_HAS_INTERNAL_PARAMETERS
                if (rindex < kVst3InternalParameterCount)
                {
                   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
                    // if there are any MIDI CC events as parameter changes, handle them here
                    if (canAppendMoreEvents && rindex >= kVst3InternalParameterMidiCC_start && rindex <= kVst3InternalParameterMidiCC_end)
                    {
                        for (int32_t j = 0, pcount = v3_cpp_obj(queue)->get_point_count(queue); j < pcount; ++j)
                        {
                            if (v3_cpp_obj(queue)->get_point(queue, j, &offset, &normalized) != V3_OK)
                                break;

                            if (inputEventList.appendCC(offset, rindex, normalized))
                            {
                                canAppendMoreEvents = false;
                                break;
                            }
                        }
                    }
                   #endif
                    continue;
                }
               #endif

                if (v3_cpp_obj(queue)->get_point_count(queue) <= 0)
                    continue;

                // if there are any parameter changes at frame 0, handle them here
                if (v3_cpp_obj(queue)->get_point(queue, 0, &offset, &normalized) != V3_OK)
                    break;

                if (offset != 0)
                    continue;

                const uint32_t index = rindex - kVst3InternalParameterCount;
                _setNormalizedPluginParameterValue(index, normalized);
            }
        }

       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        const uint32_t midiEventCount = inputEventList.convert(fMidiEvents);
        fPlugin.run(inputs, outputs, data->nframes, fMidiEvents, midiEventCount);
       #else
        fPlugin.run(inputs, outputs, data->nframes);
       #endif

       #if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        fHostEventOutputHandle = nullptr;
       #endif

        // if there are any parameter changes after frame 0, set them here
        if (v3_param_changes** const inparamsptr = data->input_params)
        {
            int32_t offset;
            double normalized;

            for (int32_t i = 0, count = v3_cpp_obj(inparamsptr)->get_param_count(inparamsptr); i < count; ++i)
            {
                v3_param_value_queue** const queue = v3_cpp_obj(inparamsptr)->get_param_data(inparamsptr, i);
                DISTRHO_SAFE_ASSERT_BREAK(queue != nullptr);

                const v3_param_id rindex = v3_cpp_obj(queue)->get_param_id(queue);
                DISTRHO_SAFE_ASSERT_UINT_BREAK(rindex < fVst3ParameterCount, rindex);

               #if DPF_VST3_HAS_INTERNAL_PARAMETERS
                if (rindex < kVst3InternalParameterCount)
                    continue;
               #endif

                const int32_t pcount = v3_cpp_obj(queue)->get_point_count(queue);

                if (pcount <= 0)
                    continue;

                if (v3_cpp_obj(queue)->get_point(queue, pcount - 1, &offset, &normalized) != V3_OK)
                    break;

                if (offset == 0)
                    continue;

                const uint32_t index = rindex - kVst3InternalParameterCount;
                _setNormalizedPluginParameterValue(index, normalized);
            }
        }

        updateParametersFromProcessing(data->output_params, data->nframes - 1);
        return V3_OK;
    }

    uint32_t getTailSamples() const noexcept
    {
        return 0;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_edit_controller interface calls

    int32_t getParameterCount() const noexcept
    {
        return fVst3ParameterCount;
    }

    v3_result getParameterInfo(const int32_t rindex, v3_param_info* const info) const noexcept
    {
        std::memset(info, 0, sizeof(v3_param_info));
        DISTRHO_SAFE_ASSERT_RETURN(rindex >= 0, V3_INVALID_ARG);

        // TODO hash the parameter symbol
        info->param_id = rindex;

      #if DPF_VST3_USES_SEPARATE_CONTROLLER || DISTRHO_PLUGIN_WANT_LATENCY || DISTRHO_PLUGIN_WANT_PROGRAMS
        switch (rindex)
        {
       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        case kVst3InternalParameterBufferSize:
            info->flags = V3_PARAM_READ_ONLY | V3_PARAM_IS_HIDDEN;
            info->step_count = DPF_VST3_MAX_BUFFER_SIZE - 1;
            strncpy_utf16(info->title, "Buffer Size", 128);
            strncpy_utf16(info->short_title, "Buffer Size", 128);
            strncpy_utf16(info->units, "frames", 128);
            return V3_OK;
        case kVst3InternalParameterSampleRate:
            info->flags = V3_PARAM_READ_ONLY | V3_PARAM_IS_HIDDEN;
            strncpy_utf16(info->title, "Sample Rate", 128);
            strncpy_utf16(info->short_title, "Sample Rate", 128);
            strncpy_utf16(info->units, "frames", 128);
            return V3_OK;
       #endif
       #if DISTRHO_PLUGIN_WANT_LATENCY
        case kVst3InternalParameterLatency:
            info->flags = V3_PARAM_READ_ONLY | V3_PARAM_IS_HIDDEN;
            strncpy_utf16(info->title, "Latency", 128);
            strncpy_utf16(info->short_title, "Latency", 128);
            strncpy_utf16(info->units, "frames", 128);
            return V3_OK;
       #endif
       #if DISTRHO_PLUGIN_WANT_PROGRAMS
        case kVst3InternalParameterProgram:
            info->flags = V3_PARAM_CAN_AUTOMATE | V3_PARAM_IS_LIST | V3_PARAM_PROGRAM_CHANGE | V3_PARAM_IS_HIDDEN;
            info->step_count = fProgramCountMinusOne;
            strncpy_utf16(info->title, "Current Program", 128);
            strncpy_utf16(info->short_title, "Program", 128);
            return V3_OK;
       #endif
        }
      #endif

       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if (rindex < kVst3InternalParameterCount)
        {
            const uint32_t index = static_cast<uint32_t>(rindex - kVst3InternalParameterMidiCC_start);
            info->flags = V3_PARAM_CAN_AUTOMATE | V3_PARAM_IS_HIDDEN;
            info->step_count = 127;
            char ccstr[24];
            snprintf(ccstr, sizeof(ccstr), "MIDI Ch. %d CC %d", static_cast<uint8_t>(index / 130) + 1, index % 130);
            strncpy_utf16(info->title, ccstr, 128);
            snprintf(ccstr, sizeof(ccstr), "Ch.%d CC%d", index / 130 + 1, index % 130);
            strncpy_utf16(info->short_title, ccstr+5, 128);
            return V3_OK;
        }
       #endif

        const uint32_t index = static_cast<uint32_t>(rindex - kVst3InternalParameterCount);
        DISTRHO_SAFE_ASSERT_UINT_RETURN(index < fParameterCount, index, V3_INVALID_ARG);

        // set up flags
        int32_t flags = 0;

        const ParameterEnumerationValues& enumValues(fPlugin.getParameterEnumValues(index));
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

        if (hints & kParameterIsOutput)
            flags |= V3_PARAM_READ_ONLY;
        else if (hints & kParameterIsAutomatable)
            flags |= V3_PARAM_CAN_AUTOMATE;

        // set up step_count
        int32_t step_count = 0;

        if (hints & kParameterIsBoolean)
            step_count = 1;
        else if (hints & kParameterIsInteger)
            step_count = ranges.max - ranges.min;

        if (enumValues.count >= 2 && enumValues.restrictedMode)
        {
            flags |= V3_PARAM_IS_LIST;
            step_count = enumValues.count - 1;
        }

        info->flags = flags;
        info->step_count = step_count;
        info->default_normalised_value = ranges.getNormalizedValue(ranges.def);
        // int32_t unit_id;
        strncpy_utf16(info->title,       fPlugin.getParameterName(index), 128);
        strncpy_utf16(info->short_title, fPlugin.getParameterShortName(index), 128);
        strncpy_utf16(info->units,       fPlugin.getParameterUnit(index), 128);
        return V3_OK;
    }

    v3_result getParameterStringForValue(const v3_param_id rindex, const double normalized, v3_str_128 output)
    {
        DISTRHO_SAFE_ASSERT_RETURN(normalized >= 0.0 && normalized <= 1.0, V3_INVALID_ARG);

      #if DPF_VST3_USES_SEPARATE_CONTROLLER || DISTRHO_PLUGIN_WANT_LATENCY || DISTRHO_PLUGIN_WANT_PROGRAMS
        switch (rindex)
        {
       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        case kVst3InternalParameterBufferSize:
            snprintf_i32_utf16(output, d_roundToIntPositive(normalized * DPF_VST3_MAX_BUFFER_SIZE), 128);
            return V3_OK;
        case kVst3InternalParameterSampleRate:
            snprintf_i32_utf16(output, d_roundToIntPositive(normalized * DPF_VST3_MAX_SAMPLE_RATE), 128);
            return V3_OK;
       #endif
       #if DISTRHO_PLUGIN_WANT_LATENCY
        case kVst3InternalParameterLatency:
            snprintf_i32_utf16(output, d_roundToIntPositive(normalized * DPF_VST3_MAX_LATENCY), 128);
            return V3_OK;
       #endif
       #if DISTRHO_PLUGIN_WANT_PROGRAMS
        case kVst3InternalParameterProgram:
            const uint32_t program = d_roundToIntPositive(normalized * fProgramCountMinusOne);
            strncpy_utf16(output, fPlugin.getProgramName(program), 128);
            return V3_OK;
       #endif
        }
      #endif

       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if (rindex < kVst3InternalParameterCount)
        {
            snprintf_i32_utf16(output, d_roundToIntPositive(normalized * 127), 128);
            return V3_OK;
        }
       #endif

        const uint32_t index = static_cast<uint32_t>(rindex - kVst3InternalParameterCount);
        DISTRHO_SAFE_ASSERT_UINT_RETURN(index < fParameterCount, index, V3_INVALID_ARG);

        const ParameterEnumerationValues& enumValues(fPlugin.getParameterEnumValues(index));
        const ParameterRanges& ranges(fPlugin.getParameterRanges(index));
        const uint32_t hints = fPlugin.getParameterHints(index);
        float value = ranges.getUnnormalizedValue(normalized);

        if (hints & kParameterIsBoolean)
        {
            const float midRange = ranges.min + (ranges.max - ranges.min) * 0.5f;
            value = value > midRange ? ranges.max : ranges.min;
        }
        else if (hints & kParameterIsInteger)
        {
            value = std::round(value);
        }

        for (uint32_t i=0; i < enumValues.count; ++i)
        {
            if (d_isEqual(enumValues.values[i].value, value))
            {
                strncpy_utf16(output, enumValues.values[i].label, 128);
                return V3_OK;
            }
        }

        if (hints & kParameterIsInteger)
            snprintf_i32_utf16(output, value, 128);
        else
            snprintf_f32_utf16(output, value, 128);

        return V3_OK;
    }

    v3_result getParameterValueForString(const v3_param_id rindex, int16_t* const input, double* const output)
    {
      #if DPF_VST3_USES_SEPARATE_CONTROLLER || DISTRHO_PLUGIN_WANT_LATENCY || DISTRHO_PLUGIN_WANT_PROGRAMS
        switch (rindex)
        {
       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        case kVst3InternalParameterBufferSize:
            *output = static_cast<double>(std::atoi(ScopedUTF8String(input))) / DPF_VST3_MAX_BUFFER_SIZE;
            return V3_OK;
        case kVst3InternalParameterSampleRate:
            *output = std::atof(ScopedUTF8String(input)) / DPF_VST3_MAX_SAMPLE_RATE;
            return V3_OK;
       #endif
       #if DISTRHO_PLUGIN_WANT_LATENCY
        case kVst3InternalParameterLatency:
            *output = std::atof(ScopedUTF8String(input)) / DPF_VST3_MAX_LATENCY;
            return V3_OK;
       #endif
       #if DISTRHO_PLUGIN_WANT_PROGRAMS
        case kVst3InternalParameterProgram:
            for (uint32_t i=0, count=fPlugin.getProgramCount(); i < count; ++i)
            {
                if (strcmp_utf16(input, fPlugin.getProgramName(i)))
                {
                    *output = static_cast<double>(i) / static_cast<double>(fProgramCountMinusOne);
                    return V3_OK;
                }
            }
            return V3_INVALID_ARG;
       #endif
        }
      #endif

       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if (rindex < kVst3InternalParameterCount)
        {
            // TODO find CC/channel based on name
            return V3_NOT_IMPLEMENTED;
        }
       #endif

        const uint32_t index = static_cast<uint32_t>(rindex - kVst3InternalParameterCount);
        DISTRHO_SAFE_ASSERT_UINT_RETURN(index < fParameterCount, index, V3_INVALID_ARG);

        const ParameterEnumerationValues& enumValues(fPlugin.getParameterEnumValues(index));
        const ParameterRanges& ranges(fPlugin.getParameterRanges(index));

        for (uint32_t i=0; i < enumValues.count; ++i)
        {
            if (strcmp_utf16(input, enumValues.values[i].label))
            {
                *output = ranges.getNormalizedValue(enumValues.values[i].value);
                return V3_OK;
            }
        }

        const ScopedUTF8String input8(input);

        float value;
        if (fPlugin.getParameterHints(index) & kParameterIsInteger)
            value = std::atoi(input8);
        else
            value = std::atof(input8);

        *output = ranges.getNormalizedValue(value);
        return V3_OK;
    }

    double normalizedParameterToPlain(const v3_param_id rindex, const double normalized)
    {
        DISTRHO_SAFE_ASSERT_RETURN(normalized >= 0.0 && normalized <= 1.0, 0.0);

      #if DPF_VST3_USES_SEPARATE_CONTROLLER || DISTRHO_PLUGIN_WANT_LATENCY || DISTRHO_PLUGIN_WANT_PROGRAMS
        switch (rindex)
        {
       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        case kVst3InternalParameterBufferSize:
            return std::round(normalized * DPF_VST3_MAX_BUFFER_SIZE);
        case kVst3InternalParameterSampleRate:
            return normalized * DPF_VST3_MAX_SAMPLE_RATE;
       #endif
       #if DISTRHO_PLUGIN_WANT_LATENCY
        case kVst3InternalParameterLatency:
            return normalized * DPF_VST3_MAX_LATENCY;
       #endif
       #if DISTRHO_PLUGIN_WANT_PROGRAMS
        case kVst3InternalParameterProgram:
            return std::round(normalized * fProgramCountMinusOne);
       #endif
        }
      #endif

       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if (rindex < kVst3InternalParameterCount)
            return std::round(normalized * 127);
       #endif

        const uint32_t index = static_cast<uint32_t>(rindex - kVst3InternalParameterCount);
        DISTRHO_SAFE_ASSERT_UINT2_RETURN(index < fParameterCount, index, fParameterCount, 0.0);

        const ParameterRanges& ranges(fPlugin.getParameterRanges(index));
        const uint32_t hints = fPlugin.getParameterHints(index);
        float value = ranges.getUnnormalizedValue(normalized);

        if (hints & kParameterIsBoolean)
        {
            const float midRange = ranges.min + (ranges.max - ranges.min) / 2.0f;
            value = value > midRange ? ranges.max : ranges.min;
        }
        else if (hints & kParameterIsInteger)
        {
            value = std::round(value);
        }

        return value;
    }

    double plainParameterToNormalized(const v3_param_id rindex, const double plain)
    {
      #if DPF_VST3_USES_SEPARATE_CONTROLLER || DISTRHO_PLUGIN_WANT_LATENCY || DISTRHO_PLUGIN_WANT_PROGRAMS
        switch (rindex)
        {
       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        case kVst3InternalParameterBufferSize:
            return std::max<double>(0.0, std::min<double>(1.0, plain / DPF_VST3_MAX_BUFFER_SIZE));
        case kVst3InternalParameterSampleRate:
            return std::max<double>(0.0, std::min<double>(1.0, plain / DPF_VST3_MAX_SAMPLE_RATE));
       #endif
       #if DISTRHO_PLUGIN_WANT_LATENCY
        case kVst3InternalParameterLatency:
            return std::max<double>(0.0, std::min<double>(1.0, plain / DPF_VST3_MAX_LATENCY));
       #endif
       #if DISTRHO_PLUGIN_WANT_PROGRAMS
        case kVst3InternalParameterProgram:
            return std::max<double>(0.0, std::min<double>(1.0, plain / fProgramCountMinusOne));
       #endif
        }
      #endif

       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if (rindex < kVst3InternalParameterCount)
            return std::max<double>(0.0, std::min<double>(1.0, plain / 127));
       #endif

        const uint32_t index = static_cast<uint32_t>(rindex - kVst3InternalParameterCount);
        DISTRHO_SAFE_ASSERT_UINT2_RETURN(index < fParameterCount, index, fParameterCount, 0.0);

        return _getNormalizedParameterValue(index, plain);
    }

    double getParameterNormalized(const v3_param_id rindex)
    {
       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        // TODO something to do here?
        if (
           #if !DPF_VST3_PURE_MIDI_INTERNAL_PARAMETERS
            rindex >= kVst3InternalParameterMidiCC_start &&
           #endif
            rindex <= kVst3InternalParameterMidiCC_end)
            return 0.0;
       #endif

      #if DPF_VST3_USES_SEPARATE_CONTROLLER || DISTRHO_PLUGIN_WANT_LATENCY || DISTRHO_PLUGIN_WANT_PROGRAMS
        switch (rindex)
        {
       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        case kVst3InternalParameterBufferSize:
        case kVst3InternalParameterSampleRate:
       #endif
       #if DISTRHO_PLUGIN_WANT_LATENCY
        case kVst3InternalParameterLatency:
       #endif
       #if DISTRHO_PLUGIN_WANT_PROGRAMS
        case kVst3InternalParameterProgram:
       #endif
            return plainParameterToNormalized(rindex, fCachedParameterValues[rindex]);
        }
      #endif

        const uint32_t index = static_cast<uint32_t>(rindex - kVst3InternalParameterCount);
        DISTRHO_SAFE_ASSERT_UINT2_RETURN(index < fParameterCount, index, fParameterCount, 0.0);

        return _getNormalizedParameterValue(index, fCachedParameterValues[kVst3InternalParameterBaseCount + index]);
    }

    v3_result setParameterNormalized(const v3_param_id rindex, const double normalized)
    {
        DISTRHO_SAFE_ASSERT_RETURN(normalized >= 0.0 && normalized <= 1.0, V3_INVALID_ARG);

       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        // TODO something to do here?
        if (
           #if !DPF_VST3_PURE_MIDI_INTERNAL_PARAMETERS
            rindex >= kVst3InternalParameterMidiCC_start &&
           #endif
            rindex <= kVst3InternalParameterMidiCC_end)
            return V3_INVALID_ARG;
       #endif

       #if DPF_VST3_USES_SEPARATE_CONTROLLER || DISTRHO_PLUGIN_WANT_LATENCY || DISTRHO_PLUGIN_WANT_PROGRAMS
        if (rindex < kVst3InternalParameterBaseCount)
        {
            fCachedParameterValues[rindex] = normalizedParameterToPlain(rindex, normalized);
            int flags = 0;

            switch (rindex)
            {
           #if DPF_VST3_USES_SEPARATE_CONTROLLER
            case kVst3InternalParameterBufferSize:
                fPlugin.setBufferSize(fCachedParameterValues[rindex], true);
                break;
            case kVst3InternalParameterSampleRate:
                fPlugin.setSampleRate(fCachedParameterValues[rindex], true);
                break;
           #endif
           #if DISTRHO_PLUGIN_WANT_LATENCY
            case kVst3InternalParameterLatency:
                flags = V3_RESTART_LATENCY_CHANGED;
                break;
           #endif
           #if DISTRHO_PLUGIN_WANT_PROGRAMS
            case kVst3InternalParameterProgram:
                flags = V3_RESTART_PARAM_VALUES_CHANGED;
                fCurrentProgram = fCachedParameterValues[rindex];
                fPlugin.loadProgram(fCurrentProgram);

                for (uint32_t i=0; i<fParameterCount; ++i)
                {
                    if (fPlugin.isParameterOutputOrTrigger(i))
                        continue;
                    fCachedParameterValues[kVst3InternalParameterBaseCount + i] = fPlugin.getParameterValue(i);
                }

               #if DISTRHO_PLUGIN_HAS_UI
                fParameterValueChangesForUI[kVst3InternalParameterProgram] = true;
               #endif
                break;
           #endif
            }

            if (fComponentHandler != nullptr && flags != 0)
                v3_cpp_obj(fComponentHandler)->restart_component(fComponentHandler, flags);

            return V3_OK;
        }
       #endif

        DISTRHO_SAFE_ASSERT_UINT2_RETURN(rindex >= kVst3InternalParameterCount, rindex, kVst3InternalParameterCount, V3_INVALID_ARG);

       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        const uint32_t index = static_cast<uint32_t>(rindex - kVst3InternalParameterCount);
        DISTRHO_SAFE_ASSERT_UINT2_RETURN(index < fParameterCount, index, fParameterCount, V3_INVALID_ARG);

        if (fIsComponent) {
            DISTRHO_SAFE_ASSERT_RETURN(!fPlugin.isParameterOutputOrTrigger(index), V3_INVALID_ARG);
        }

        _setNormalizedPluginParameterValue(index, normalized);
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

   #if DPF_VST3_USES_SEPARATE_CONTROLLER
    void comp2ctrl_connect(v3_connection_point** const other)
    {
        fConnectionFromCompToCtrl = other;
    }

    void comp2ctrl_disconnect()
    {
        fConnectionFromCompToCtrl = nullptr;
    }

    v3_result comp2ctrl_notify(v3_message** const message)
    {
        const char* const msgid = v3_cpp_obj(message)->get_message_id(message);
        DISTRHO_SAFE_ASSERT_RETURN(msgid != nullptr, V3_INVALID_ARG);

        v3_attribute_list** const attrs = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrs != nullptr, V3_INVALID_ARG);

       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if (std::strcmp(msgid, "midi") == 0)
            return notify_midi(attrs);
       #endif

       #if DISTRHO_PLUGIN_WANT_STATE
        if (std::strcmp(msgid, "state-set") == 0)
            return notify_state(attrs);
       #endif

        d_stderr("comp2ctrl_notify received unknown msg '%s'", msgid);

        return V3_NOT_IMPLEMENTED;
    }
   #endif // DPF_VST3_USES_SEPARATE_CONTROLLER

    // ----------------------------------------------------------------------------------------------------------------

    void ctrl2view_connect(v3_connection_point** const other)
    {
        DISTRHO_SAFE_ASSERT(fConnectedToUI == false);

        fConnectionFromCtrlToView = other;
        fConnectedToUI = false;
    }

    void ctrl2view_disconnect()
    {
        fConnectedToUI = false;
        fConnectionFromCtrlToView = nullptr;
    }

    v3_result ctrl2view_notify(v3_message** const message)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fConnectionFromCtrlToView != nullptr, V3_INTERNAL_ERR);

        const char* const msgid = v3_cpp_obj(message)->get_message_id(message);
        DISTRHO_SAFE_ASSERT_RETURN(msgid != nullptr, V3_INVALID_ARG);

        if (std::strcmp(msgid, "init") == 0)
        {
            fConnectedToUI = true;

           #if DPF_VST3_USES_SEPARATE_CONTROLLER
            fParameterValueChangesForUI[kVst3InternalParameterSampleRate] = false;
            sendParameterSetToUI(kVst3InternalParameterSampleRate,
                                 fCachedParameterValues[kVst3InternalParameterSampleRate]);
           #endif

           #if DISTRHO_PLUGIN_WANT_PROGRAMS
            fParameterValueChangesForUI[kVst3InternalParameterProgram] = false;
            sendParameterSetToUI(kVst3InternalParameterProgram, fCurrentProgram);
           #endif

           #if DISTRHO_PLUGIN_WANT_FULL_STATE
            // Update current state from plugin side
            for (StringMap::const_iterator cit=fStateMap.begin(), cite=fStateMap.end(); cit != cite; ++cit)
            {
                const String& key(cit->first);
                fStateMap[key] = fPlugin.getStateValue(key);
            }
           #endif

           #if DISTRHO_PLUGIN_WANT_STATE
            // Set state
            for (StringMap::const_iterator cit=fStateMap.begin(), cite=fStateMap.end(); cit != cite; ++cit)
            {
                const String& key(cit->first);
                const String& value(cit->second);

                sendStateSetToUI(key, value);
            }
           #endif

            for (uint32_t i=0; i<fParameterCount; ++i)
            {
                fParameterValueChangesForUI[kVst3InternalParameterBaseCount + i] = false;
                sendParameterSetToUI(kVst3InternalParameterCount + i,
                                     fCachedParameterValues[kVst3InternalParameterBaseCount + i]);
            }

            sendReadyToUI();
            return V3_OK;
        }

        DISTRHO_SAFE_ASSERT_RETURN(fConnectedToUI, V3_INTERNAL_ERR);

        v3_attribute_list** const attrs = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrs != nullptr, V3_INVALID_ARG);

        if (std::strcmp(msgid, "idle") == 0)
        {
           #if DPF_VST3_USES_SEPARATE_CONTROLLER
            if (fParameterValueChangesForUI[kVst3InternalParameterSampleRate])
            {
                fParameterValueChangesForUI[kVst3InternalParameterSampleRate] = false;
                sendParameterSetToUI(kVst3InternalParameterSampleRate,
                                     fCachedParameterValues[kVst3InternalParameterSampleRate]);
            }
           #endif

           #if DISTRHO_PLUGIN_WANT_PROGRAMS
            if (fParameterValueChangesForUI[kVst3InternalParameterProgram])
            {
                fParameterValueChangesForUI[kVst3InternalParameterProgram] = false;
                sendParameterSetToUI(kVst3InternalParameterProgram, fCurrentProgram);
            }
           #endif

            for (uint32_t i=0; i<fParameterCount; ++i)
            {
                if (! fParameterValueChangesForUI[kVst3InternalParameterBaseCount + i])
                    continue;

                fParameterValueChangesForUI[kVst3InternalParameterBaseCount + i] = false;
                sendParameterSetToUI(kVst3InternalParameterCount + i,
                                     fCachedParameterValues[kVst3InternalParameterBaseCount + i]);
            }

            sendReadyToUI();
            return V3_OK;
        }

        if (std::strcmp(msgid, "close") == 0)
        {
            fConnectedToUI = false;
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
            DISTRHO_SAFE_ASSERT_INT2_RETURN(rindex >= kVst3InternalParameterCount,
                                            rindex, fParameterCount, V3_INTERNAL_ERR);
            DISTRHO_SAFE_ASSERT_INT2_RETURN(rindex < kVst3InternalParameterCount + fParameterCount,
                                            rindex, fParameterCount, V3_INTERNAL_ERR);

            res = v3_cpp_obj(attrs)->get_int(attrs, "started", &started);
            DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);
            DISTRHO_SAFE_ASSERT_INT_RETURN(started == 0 || started == 1, started, V3_INTERNAL_ERR);

            return started != 0 ? v3_cpp_obj(fComponentHandler)->begin_edit(fComponentHandler, rindex)
                                : v3_cpp_obj(fComponentHandler)->end_edit(fComponentHandler, rindex);
        }

        if (std::strcmp(msgid, "parameter-set") == 0)
        {
            DISTRHO_SAFE_ASSERT_RETURN(fComponentHandler != nullptr, V3_INTERNAL_ERR);

            int64_t rindex;
            double value;
            v3_result res;

            res = v3_cpp_obj(attrs)->get_int(attrs, "rindex", &rindex);
            DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);
            DISTRHO_SAFE_ASSERT_INT2_RETURN(rindex >= kVst3InternalParameterCount,
                                            rindex, fParameterCount, V3_INTERNAL_ERR);
            DISTRHO_SAFE_ASSERT_INT2_RETURN(rindex < kVst3InternalParameterCount + fParameterCount,
                                            rindex, fParameterCount, V3_INTERNAL_ERR);

            res = v3_cpp_obj(attrs)->get_float(attrs, "value", &value);
            DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);

            const uint32_t index = rindex - kVst3InternalParameterCount;
            const double normalized = _getNormalizedParameterValue(index, value);

            fCachedParameterValues[kVst3InternalParameterBaseCount + index] = value;

            if (! fPlugin.isParameterOutputOrTrigger(index))
                fPlugin.setParameterValue(index, value);

            return v3_cpp_obj(fComponentHandler)->perform_edit(fComponentHandler, rindex, normalized);
        }

       #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
        if (std::strcmp(msgid, "midi") == 0)
        {
           #if DPF_VST3_USES_SEPARATE_CONTROLLER
            DISTRHO_SAFE_ASSERT_RETURN(fConnectionFromCompToCtrl != nullptr, V3_INTERNAL_ERR);
            return v3_cpp_obj(fConnectionFromCompToCtrl)->notify(fConnectionFromCompToCtrl, message);
           #else
            return notify_midi(attrs);
           #endif
        }
       #endif

       #if DISTRHO_PLUGIN_WANT_STATE
        if (std::strcmp(msgid, "state-set") == 0)
        {
            const v3_result res = notify_state(attrs);

           #if DPF_VST3_USES_SEPARATE_CONTROLLER
            if (res != V3_OK)
                return res;

            // notify component of the change
            DISTRHO_SAFE_ASSERT_RETURN(fConnectionFromCompToCtrl != nullptr, V3_INTERNAL_ERR);
            return v3_cpp_obj(fConnectionFromCompToCtrl)->notify(fConnectionFromCompToCtrl, message);
           #else
            return res;
           #endif
        }
       #endif

        d_stderr("ctrl2view_notify received unknown msg '%s'", msgid);

        return V3_NOT_IMPLEMENTED;
    }

   #if DISTRHO_PLUGIN_WANT_STATE
    v3_result notify_state(v3_attribute_list** const attrs)
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

        res = v3_cpp_obj(attrs)->get_string(attrs, "key", key16, sizeof(int16_t)*(keyLength+1));
        DISTRHO_SAFE_ASSERT_INT2_RETURN(res == V3_OK, res, keyLength, res);

        if (valueLength != 0)
        {
            res = v3_cpp_obj(attrs)->get_string(attrs, "value", value16, sizeof(int16_t)*(valueLength+1));
            DISTRHO_SAFE_ASSERT_INT2_RETURN(res == V3_OK, res, valueLength, res);
        }

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
            const String dkey(key);
            fStateMap[dkey] = value;
        }

        std::free(key16);
        std::free(value16);
        return V3_OK;
    }
   #endif // DISTRHO_PLUGIN_WANT_STATE

   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    v3_result notify_midi(v3_attribute_list** const attrs)
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
   #endif // DISTRHO_PLUGIN_WANT_MIDI_INPUT
#endif

    // ----------------------------------------------------------------------------------------------------------------

private:
    // Plugin
    PluginExporter fPlugin;

    // VST3 stuff
    v3_component_handler** fComponentHandler;
  #if DISTRHO_PLUGIN_HAS_UI
   #if DPF_VST3_USES_SEPARATE_CONTROLLER
    v3_connection_point** fConnectionFromCompToCtrl;
   #endif
    v3_connection_point** fConnectionFromCtrlToView;
    v3_host_application** const fHostApplication;
  #endif

    // Temporary data
    const uint32_t fParameterCount;
    const uint32_t fVst3ParameterCount; // full offset + real
    float* fCachedParameterValues; // basic offset + real
    float* fDummyAudioBuffer;
    bool* fParameterValuesChangedDuringProcessing; // basic offset + real
   #if DISTRHO_PLUGIN_NUM_INPUTS > 0
    bool fEnabledInputs[DISTRHO_PLUGIN_NUM_INPUTS];
   #endif
   #if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
    bool fEnabledOutputs[DISTRHO_PLUGIN_NUM_OUTPUTS];
   #endif
   #if DPF_VST3_USES_SEPARATE_CONTROLLER
    const bool fIsComponent;
   #endif
   #if DISTRHO_PLUGIN_HAS_UI
    bool* fParameterValueChangesForUI; // basic offset + real
    bool fConnectedToUI;
   #endif
   #if DISTRHO_PLUGIN_WANT_LATENCY
    uint32_t fLastKnownLatency;
   #endif
  #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    MidiEvent fMidiEvents[kMaxMidiEvents];
   #if DISTRHO_PLUGIN_HAS_UI
    SmallStackRingBuffer fNotesRingBuffer;
   #endif
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
    // helper functions for dealing with buses

   #if DISTRHO_PLUGIN_NUM_INPUTS+DISTRHO_PLUGIN_NUM_OUTPUTS > 0
    template<bool isInput>
    void fillInBusInfoDetails()
    {
        constexpr const uint32_t numPorts = isInput ? DISTRHO_PLUGIN_NUM_INPUTS : DISTRHO_PLUGIN_NUM_OUTPUTS;
        BusInfo& busInfo(isInput ? inputBuses : outputBuses);
        bool* const enabledPorts = isInput
                                #if DISTRHO_PLUGIN_NUM_INPUTS > 0
                                 ? fEnabledInputs
                                #else
                                 ? nullptr
                                #endif
                                #if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
                                 : fEnabledOutputs;
                                #else
                                 : nullptr;
                                #endif

        std::vector<uint32_t> visitedPortGroups;
        for (uint32_t i=0; i<numPorts; ++i)
        {
            const AudioPortWithBusId& port(fPlugin.getAudioPort(isInput, i));

            if (port.groupId != kPortGroupNone)
            {
                const std::vector<uint32_t>::iterator end = visitedPortGroups.end();
                if (std::find(visitedPortGroups.begin(), end, port.groupId) == end)
                {
                    visitedPortGroups.push_back(port.groupId);
                    ++busInfo.groups;
                }
                ++busInfo.groupPorts;
                continue;
            }

            if (port.hints & kAudioPortIsCV)
                ++busInfo.cvPorts;
            else if (port.hints & kAudioPortIsSidechain)
                ++busInfo.sidechainPorts;
            else
                ++busInfo.audioPorts;
        }

        if (busInfo.audioPorts != 0)
            busInfo.audio = 1;
        if (busInfo.sidechainPorts != 0)
            busInfo.sidechain = 1;

        uint32_t busIdForCV = 0;
        const std::vector<uint32_t>::iterator vpgStart = visitedPortGroups.begin();
        const std::vector<uint32_t>::iterator vpgEnd = visitedPortGroups.end();

        for (uint32_t i=0; i<numPorts; ++i)
        {
            AudioPortWithBusId& port(fPlugin.getAudioPort(isInput, i));

            if (port.groupId != kPortGroupNone)
            {
                port.busId = std::find(vpgStart, vpgEnd, port.groupId) - vpgStart;

                if (busInfo.audio == 0 && (port.hints & kAudioPortIsSidechain) == 0x0)
                    enabledPorts[i] = true;
            }
            else
            {
                if (port.hints & kAudioPortIsCV)
                {
                    port.busId = busInfo.audio + busInfo.sidechain + busIdForCV++;
                }
                else if (port.hints & kAudioPortIsSidechain)
                {
                    port.busId = busInfo.audio;
                }
                else
                {
                    port.busId = 0;
                    enabledPorts[i] = true;
                }

                port.busId += busInfo.groups;
            }
        }
    }

    template<bool isInput>
    v3_result getAudioBusInfo(const uint32_t busId, v3_bus_info* const info) const
    {
        constexpr const uint32_t numPorts = isInput ? DISTRHO_PLUGIN_NUM_INPUTS : DISTRHO_PLUGIN_NUM_OUTPUTS;
        const BusInfo& busInfo(isInput ? inputBuses : outputBuses);

        int32_t numChannels;
        uint32_t flags;
        v3_bus_types busType;
        v3_str_128 busName = {};

        if (busId < busInfo.groups)
        {
            numChannels = 0;

            for (uint32_t i=0; i<numPorts; ++i)
            {
                const AudioPortWithBusId& port(fPlugin.getAudioPort(isInput, i));

                if (port.busId == busId)
                {
                    const PortGroupWithId& group(fPlugin.getPortGroupById(port.groupId));

                    switch (port.groupId)
                    {
                    case kPortGroupStereo:
                    case kPortGroupMono:
                        if (busId == 0)
                        {
                            strncpy_utf16(busName, isInput ? "Audio Input" : "Audio Output", 128);
                            break;
                        }
                    // fall-through
                    default:
                        if (group.name.isNotEmpty())
                            strncpy_utf16(busName, group.name, 128);
                        else
                            strncpy_utf16(busName, port.name, 128);
                        break;
                    }

                    numChannels = fPlugin.getAudioPortCountWithGroupId(isInput, port.groupId);

                    if (port.hints & kAudioPortIsCV)
                    {
                        busType = V3_MAIN;
                        flags = V3_IS_CONTROL_VOLTAGE;
                    }
                    else if (port.hints & kAudioPortIsSidechain)
                    {
                        busType = V3_AUX;
                        flags = 0;
                    }
                    else
                    {
                        busType = V3_MAIN;
                        flags = busInfo.audio == 0 ? V3_DEFAULT_ACTIVE : 0;
                    }
                    break;
                }
            }

            DISTRHO_SAFE_ASSERT_RETURN(numChannels != 0, V3_INTERNAL_ERR);
        }
        else
        {
            switch (busId - busInfo.groups)
            {
            case 0:
                if (busInfo.audio)
                {
                    numChannels = busInfo.audioPorts;
                    busType = V3_MAIN;
                    flags = V3_DEFAULT_ACTIVE;
                    break;
                }
            // fall-through
            case 1:
                if (busInfo.sidechain)
                {
                    numChannels = busInfo.sidechainPorts;
                    busType = V3_AUX;
                    flags = 0;
                    break;
                }
            // fall-through
            default:
                numChannels = 1;
                busType = V3_MAIN;
                flags = V3_IS_CONTROL_VOLTAGE;
                break;
            }

            if (busType == V3_MAIN && flags != V3_IS_CONTROL_VOLTAGE)
            {
                strncpy_utf16(busName, isInput ? "Audio Input" : "Audio Output", 128);
            }
            else
            {
                for (uint32_t i=0; i<numPorts; ++i)
                {
                    const AudioPortWithBusId& port(fPlugin.getAudioPort(isInput, i));

                    if (port.busId == busId)
                    {
                        String groupName;
                        if (busInfo.groups)
                            groupName = fPlugin.getPortGroupById(port.groupId).name;
                        if (groupName.isEmpty())
                            groupName = port.name;
                        strncpy_utf16(busName, groupName, 128);
                        break;
                    }
                }
            }
        }

        // d_debug("getAudioBusInfo %d %d %d", (int)isInput, busId, numChannels);
        std::memset(info, 0, sizeof(v3_bus_info));
        info->media_type = V3_AUDIO;
        info->direction = isInput ? V3_INPUT : V3_OUTPUT;
        info->channel_count = numChannels;
        std::memcpy(info->bus_name, busName, sizeof(busName));
        info->bus_type = busType;
        info->flags = flags;
        return V3_OK;
    }

    // someone please tell me what is up with these..
    static inline v3_speaker_arrangement portCountToSpeaker(const uint32_t portCount)
    {
        DISTRHO_SAFE_ASSERT_RETURN(portCount != 0, 0);

        switch (portCount)
        {
        // regular mono
        case 1: return V3_SPEAKER_M;
        // regular stereo
        case 2: return V3_SPEAKER_L | V3_SPEAKER_R;
        // stereo with center channel
        case 3: return V3_SPEAKER_L | V3_SPEAKER_R | V3_SPEAKER_C;
        // stereo with surround (quadro)
        case 4: return V3_SPEAKER_L | V3_SPEAKER_R | V3_SPEAKER_LS | V3_SPEAKER_RS;
        // regular 5.0
        case 5: return V3_SPEAKER_L | V3_SPEAKER_R | V3_SPEAKER_LS | V3_SPEAKER_RS | V3_SPEAKER_C;
        // regular 6.0
        case 6: return V3_SPEAKER_L | V3_SPEAKER_R | V3_SPEAKER_LS | V3_SPEAKER_RS | V3_SPEAKER_SL | V3_SPEAKER_SR;
        // regular 7.0
        case 7: return V3_SPEAKER_L | V3_SPEAKER_R | V3_SPEAKER_LS | V3_SPEAKER_RS | V3_SPEAKER_SL | V3_SPEAKER_SR | V3_SPEAKER_C;
        // regular 8.0
        case 8: return V3_SPEAKER_L | V3_SPEAKER_R | V3_SPEAKER_LS | V3_SPEAKER_RS | V3_SPEAKER_SL | V3_SPEAKER_SR | V3_SPEAKER_C | V3_SPEAKER_S;
        // regular 8.1
        case 9: return V3_SPEAKER_L | V3_SPEAKER_R | V3_SPEAKER_LS | V3_SPEAKER_RS | V3_SPEAKER_SL | V3_SPEAKER_SR | V3_SPEAKER_C | V3_SPEAKER_S | V3_SPEAKER_LFE;
        // cinema 10.0
        case 10: return (
            V3_SPEAKER_L | V3_SPEAKER_R |
            V3_SPEAKER_LS | V3_SPEAKER_RS |
            V3_SPEAKER_SL | V3_SPEAKER_SR |
            V3_SPEAKER_LC | V3_SPEAKER_RC |
            V3_SPEAKER_C | V3_SPEAKER_S);
        // cinema 10.1
        case 11: return (
            V3_SPEAKER_L | V3_SPEAKER_R |
            V3_SPEAKER_LS | V3_SPEAKER_RS |
            V3_SPEAKER_SL | V3_SPEAKER_SR |
            V3_SPEAKER_LC | V3_SPEAKER_RC |
            V3_SPEAKER_C | V3_SPEAKER_S | V3_SPEAKER_LFE);
        default:
            d_stderr("portCountToSpeaker error: got weirdly big number ports %u in a single bus", portCount);
            return 0;
        }
    }

    template<bool isInput>
    v3_speaker_arrangement getSpeakerArrangementForAudioPort(const BusInfo& busInfo, const uint32_t portGroupId, const uint32_t busId) const noexcept
    {
        switch (portGroupId)
        {
        case kPortGroupMono:
            return V3_SPEAKER_M;
        case kPortGroupStereo:
            return V3_SPEAKER_L | V3_SPEAKER_R;
        }

        if (busId < busInfo.groups)
            return portCountToSpeaker(fPlugin.getAudioPortCountWithGroupId(isInput, portGroupId));

        if (busInfo.audio != 0 && busId == busInfo.groups)
            return portCountToSpeaker(busInfo.audioPorts);

        if (busInfo.sidechain != 0 && busId == busInfo.groups + busInfo.audio)
            return portCountToSpeaker(busInfo.sidechainPorts);

        return V3_SPEAKER_M;
    }

    template<bool isInput>
    bool getAudioBusArrangement(uint32_t busId, v3_speaker_arrangement* const speaker) const
    {
        constexpr const uint32_t numPorts = isInput ? DISTRHO_PLUGIN_NUM_INPUTS : DISTRHO_PLUGIN_NUM_OUTPUTS;
        const BusInfo& busInfo(isInput ? inputBuses : outputBuses);

        for (uint32_t i=0; i<numPorts; ++i)
        {
            const AudioPortWithBusId& port(fPlugin.getAudioPort(isInput, i));

            if (port.busId != busId)
            {
                // d_debug("port.busId != busId: %d %d", port.busId, busId);
                continue;
            }

            *speaker = getSpeakerArrangementForAudioPort<isInput>(busInfo, port.groupId, busId);
            // d_debug("getAudioBusArrangement %d enabled by value %lx", busId, *speaker);
            return true;
        }

        return false;
    }

    template<bool isInput>
    bool setAudioBusArrangement(v3_speaker_arrangement* const speakers, const uint32_t numBuses)
    {
        constexpr const uint32_t numPorts = isInput ? DISTRHO_PLUGIN_NUM_INPUTS : DISTRHO_PLUGIN_NUM_OUTPUTS;
        BusInfo& busInfo(isInput ? inputBuses : outputBuses);
        bool* const enabledPorts = isInput
                                #if DISTRHO_PLUGIN_NUM_INPUTS > 0
                                 ? fEnabledInputs
                                #else
                                 ? nullptr
                                #endif
                                #if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
                                 : fEnabledOutputs;
                                #else
                                 : nullptr;
                                #endif

        bool ok = true;

        for (uint32_t busId=0; busId<numBuses; ++busId)
        {
            const v3_speaker_arrangement arr = speakers[busId];

            // d_debug("setAudioBusArrangement %d %d | %d %lx", (int)isInput, numBuses, busId, arr);

            for (uint32_t i=0; i<numPorts; ++i)
            {
                AudioPortWithBusId& port(fPlugin.getAudioPort(isInput, i));

                if (port.busId != busId)
                {
                    // d_debug("setAudioBusArrangement port.busId != busId: %d %d", port.busId, busId);
                    continue;
                }

                // get the only valid speaker arrangement for this bus, assuming enabled
                const v3_speaker_arrangement earr = getSpeakerArrangementForAudioPort<isInput>(busInfo, port.groupId, busId);

                // fail if host tries to map it to anything else
                // FIXME should we allow to map speaker to zero as a way to disable it?
                if (earr != arr /* && arr != 0 */)
                {
                    ok = false;
                    continue;
                }

                enabledPorts[i] = arr != 0;
            }
        }

        // disable any buses outside of the requested arrangement
        const uint32_t totalBuses = busInfo.audio + busInfo.sidechain + busInfo.groups + busInfo.cvPorts;

        for (uint32_t busId=numBuses; busId<totalBuses; ++busId)
        {
            for (uint32_t i=0; i<numPorts; ++i)
            {
                const AudioPortWithBusId& port(fPlugin.getAudioPort(isInput, i));

                if (port.busId == busId)
                {
                    enabledPorts[i] = false;
                    break;
                }
            }
        }

        return ok;
    }
   #endif

    // ----------------------------------------------------------------------------------------------------------------
    // helper functions called during process, cannot block

    void updateParametersFromProcessing(v3_param_changes** const outparamsptr, const int32_t offset)
    {
        DISTRHO_SAFE_ASSERT_RETURN(outparamsptr != nullptr,);

        float curValue, defValue;
        double normalized;

       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        for (v3_param_id i=kVst3InternalParameterBufferSize; i<=kVst3InternalParameterSampleRate; ++i)
        {
            if (! fParameterValuesChangedDuringProcessing[i])
                continue;

            normalized = plainParameterToNormalized(i, fCachedParameterValues[i]);
            fParameterValuesChangedDuringProcessing[i] = false;
            addParameterDataToHostOutputEvents(outparamsptr, i, normalized);
        }
       #endif

        for (uint32_t i=0; i<fParameterCount; ++i)
        {
            if (fPlugin.isParameterOutput(i))
            {
                // NOTE: no output parameter support in VST3, simulate it here
                curValue = fPlugin.getParameterValue(i);

                if (d_isEqual(curValue, fCachedParameterValues[kVst3InternalParameterBaseCount + i]))
                    continue;
            }
            else if (fPlugin.isParameterTrigger(i))
            {
                // NOTE: no trigger parameter support in VST3, simulate it here
                defValue = fPlugin.getParameterDefault(i);
                curValue = fPlugin.getParameterValue(i);

                if (d_isEqual(curValue, defValue))
                    continue;

                curValue = defValue;
                fPlugin.setParameterValue(i, curValue);
            }
            else if (fParameterValuesChangedDuringProcessing[kVst3InternalParameterBaseCount + i])
            {
                fParameterValuesChangedDuringProcessing[kVst3InternalParameterBaseCount + i] = false;
                curValue = fPlugin.getParameterValue(i);
            }
            else
            {
                continue;
            }

            fCachedParameterValues[kVst3InternalParameterBaseCount + i] = curValue;
           #if DISTRHO_PLUGIN_HAS_UI
            fParameterValueChangesForUI[kVst3InternalParameterBaseCount + i] = true;
           #endif

            normalized = _getNormalizedParameterValue(i, curValue);

            if (! addParameterDataToHostOutputEvents(outparamsptr, kVst3InternalParameterCount + i, normalized, offset))
                break;
        }

       #if DISTRHO_PLUGIN_WANT_LATENCY
        const uint32_t latency = fPlugin.getLatency();

        if (fLastKnownLatency != latency)
        {
            fLastKnownLatency = latency;

            normalized = plainParameterToNormalized(kVst3InternalParameterLatency,
                                                    fCachedParameterValues[kVst3InternalParameterLatency]);
            addParameterDataToHostOutputEvents(outparamsptr, kVst3InternalParameterLatency, normalized);
        }
       #endif
    }

    bool addParameterDataToHostOutputEvents(v3_param_changes** const outparamsptr,
                                            const v3_param_id paramId,
                                            const double normalized,
                                            const int32_t offset = 0)
    {
        int32_t index = 0;
        v3_param_value_queue** const queue = v3_cpp_obj(outparamsptr)->add_param_data(outparamsptr,
                                                                                      &paramId, &index);
        DISTRHO_SAFE_ASSERT_RETURN(queue != nullptr, false);
        DISTRHO_SAFE_ASSERT_RETURN(v3_cpp_obj(queue)->add_point(queue, 0, normalized, &index) == V3_OK, false);

        /* FLStudio gets confused with this one, skip it for now
        if (offset != 0)
            v3_cpp_obj(queue)->add_point(queue, offset, normalized, &index);
        */

        return true;

        // unused at the moment, buggy VST3 hosts :/
        (void)offset;
    }

   #if DISTRHO_PLUGIN_HAS_UI
    // ----------------------------------------------------------------------------------------------------------------
    // helper functions called during message passing, can block

    v3_message** createMessage(const char* const id) const
    {
        DISTRHO_SAFE_ASSERT_RETURN(fHostApplication != nullptr, nullptr);

        v3_tuid iid;
        memcpy(iid, v3_message_iid, sizeof(v3_tuid));
        v3_message** msg = nullptr;
        const v3_result res = v3_cpp_obj(fHostApplication)->create_instance(fHostApplication, iid, iid, (void**)&msg);
        DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_TRUE, res, nullptr);
        DISTRHO_SAFE_ASSERT_RETURN(msg != nullptr, nullptr);

        v3_cpp_obj(msg)->set_message_id(msg, id);
        return msg;
    }

    void sendParameterSetToUI(const v3_param_id rindex, const double value) const
    {
        v3_message** const message = createMessage("parameter-set");
        DISTRHO_SAFE_ASSERT_RETURN(message != nullptr,);

        v3_attribute_list** const attrlist = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrlist != nullptr,);

        v3_cpp_obj(attrlist)->set_int(attrlist, "__dpf_msg_target__", 2);
        v3_cpp_obj(attrlist)->set_int(attrlist, "rindex", rindex);
        v3_cpp_obj(attrlist)->set_float(attrlist, "value", value);
        v3_cpp_obj(fConnectionFromCtrlToView)->notify(fConnectionFromCtrlToView, message);

        v3_cpp_obj_unref(message);
    }

    void sendStateSetToUI(const char* const key, const char* const value) const
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
        v3_cpp_obj(fConnectionFromCtrlToView)->notify(fConnectionFromCtrlToView, message);

        v3_cpp_obj_unref(message);
    }

    void sendReadyToUI() const
    {
        v3_message** const message = createMessage("ready");
        DISTRHO_SAFE_ASSERT_RETURN(message != nullptr,);

        v3_attribute_list** const attrlist = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrlist != nullptr,);

        v3_cpp_obj(attrlist)->set_int(attrlist, "__dpf_msg_target__", 2);
        v3_cpp_obj(fConnectionFromCtrlToView)->notify(fConnectionFromCtrlToView, message);

        v3_cpp_obj_unref(message);
    }
   #endif

    // ----------------------------------------------------------------------------------------------------------------
    // DPF callbacks

   #if DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
    bool requestParameterValueChange(const uint32_t index, float)
    {
        fParameterValuesChangedDuringProcessing[kVst3InternalParameterBaseCount + index] = true;
        return true;
    }

    static bool requestParameterValueChangeCallback(void* const ptr, const uint32_t index, const float value)
    {
        return static_cast<PluginVst3*>(ptr)->requestParameterValueChange(index, value);
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
                event.midi_cc_out.value2 = data[3];
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

    static bool writeMidiCallback(void* const ptr, const MidiEvent& midiEvent)
    {
        return static_cast<PluginVst3*>(ptr)->writeMidi(midiEvent);
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

// --------------------------------------------------------------------------------------------------------------------
// Store components that we can't delete properly, to be cleaned up on module unload

struct dpf_component;

static std::vector<dpf_component**> gComponentGarbage;

static uint32_t handleUncleanComponent(dpf_component** const componentptr)
{
    gComponentGarbage.push_back(componentptr);
    return 0;
}

#if DPF_VST3_USES_SEPARATE_CONTROLLER
// --------------------------------------------------------------------------------------------------------------------
// Store controllers that we can't delete properly, to be cleaned up on module unload

struct dpf_edit_controller;

static std::vector<dpf_edit_controller**> gControllerGarbage;

static uint32_t handleUncleanController(dpf_edit_controller** const controllerptr)
{
    gControllerGarbage.push_back(controllerptr);
    return 0;
}

// --------------------------------------------------------------------------------------------------------------------
// dpf_comp2ctrl_connection_point

struct dpf_comp2ctrl_connection_point : v3_connection_point_cpp {
    std::atomic_int refcounter;
    ScopedPointer<PluginVst3>& vst3;
    v3_connection_point** other;

    dpf_comp2ctrl_connection_point(ScopedPointer<PluginVst3>& v)
        : refcounter(1),
          vst3(v),
          other(nullptr)
    {
        // v3_funknown, single instance
        query_interface = query_interface_connection_point;
        ref = dpf_single_instance_ref<dpf_comp2ctrl_connection_point>;
        unref = dpf_single_instance_unref<dpf_comp2ctrl_connection_point>;

        // v3_connection_point
        point.connect = connect;
        point.disconnect = disconnect;
        point.notify = notify;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_funknown

    static v3_result V3_API query_interface_connection_point(void* const self, const v3_tuid iid, void** const iface)
    {
        dpf_comp2ctrl_connection_point* const point = *static_cast<dpf_comp2ctrl_connection_point**>(self);

        if (v3_tuid_match(iid, v3_funknown_iid) ||
            v3_tuid_match(iid, v3_connection_point_iid))
        {
            d_debug("dpf_comp2ctrl_connection_point => %p %s %p | OK", self, tuid2str(iid), iface);
            ++point->refcounter;
            *iface = self;
            return V3_OK;
        }

        d_debug("dpf_comp2ctrl_connection_point => %p %s %p | WARNING UNSUPPORTED", self, tuid2str(iid), iface);

        *iface = nullptr;
        return V3_NO_INTERFACE;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_connection_point

    static v3_result V3_API connect(void* const self, v3_connection_point** const other)
    {
        d_debug("dpf_comp2ctrl_connection_point::connect => %p %p", self, other);
        dpf_comp2ctrl_connection_point* const point = *static_cast<dpf_comp2ctrl_connection_point**>(self);
        DISTRHO_SAFE_ASSERT_RETURN(point->other == nullptr, V3_INVALID_ARG);
        DISTRHO_SAFE_ASSERT_RETURN(point->other != other, V3_INVALID_ARG);

        point->other = other;

        if (PluginVst3* const vst3 = point->vst3)
            vst3->comp2ctrl_connect(other);

        return V3_OK;
    }

    static v3_result V3_API disconnect(void* const self, v3_connection_point** const other)
    {
        d_debug("dpf_comp2ctrl_connection_point => %p %p", self, other);
        dpf_comp2ctrl_connection_point* const point = *static_cast<dpf_comp2ctrl_connection_point**>(self);
        DISTRHO_SAFE_ASSERT_RETURN(point->other != nullptr, V3_INVALID_ARG);
        DISTRHO_SAFE_ASSERT_RETURN(point->other == other, V3_INVALID_ARG);

        if (PluginVst3* const vst3 = point->vst3)
            vst3->comp2ctrl_disconnect();

        point->other = nullptr;

        return V3_OK;
    }

    static v3_result V3_API notify(void* const self, v3_message** const message)
    {
        dpf_comp2ctrl_connection_point* const point = *static_cast<dpf_comp2ctrl_connection_point**>(self);

        PluginVst3* const vst3 = point->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        v3_connection_point** const other = point->other;
        DISTRHO_SAFE_ASSERT_RETURN(other != nullptr, V3_NOT_INITIALIZED);

        v3_attribute_list** const attrlist = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrlist != nullptr, V3_INVALID_ARG);

        int64_t target = 0;
        const v3_result res = v3_cpp_obj(attrlist)->get_int(attrlist, "__dpf_msg_target__", &target);
        DISTRHO_SAFE_ASSERT_RETURN(res == V3_OK, res);
        DISTRHO_SAFE_ASSERT_INT_RETURN(target == 1, target, V3_INTERNAL_ERR);

        // view -> edit controller -> component
        return vst3->comp2ctrl_notify(message);
    }
};
#endif // DPF_VST3_USES_SEPARATE_CONTROLLER

#if DISTRHO_PLUGIN_HAS_UI
// --------------------------------------------------------------------------------------------------------------------
// dpf_ctrl2view_connection_point

struct dpf_ctrl2view_connection_point : v3_connection_point_cpp {
    ScopedPointer<PluginVst3>& vst3;
    v3_connection_point** other;

    dpf_ctrl2view_connection_point(ScopedPointer<PluginVst3>& v)
        : vst3(v),
          other(nullptr)
    {
        // v3_funknown, single instance, used internally
        query_interface = nullptr;
        ref = nullptr;
        unref = nullptr;

        // v3_connection_point
        point.connect = connect;
        point.disconnect = disconnect;
        point.notify = notify;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_connection_point

    static v3_result V3_API connect(void* const self, v3_connection_point** const other)
    {
        d_debug("dpf_ctrl2view_connection_point::connect => %p %p", self, other);
        dpf_ctrl2view_connection_point* const point = *static_cast<dpf_ctrl2view_connection_point**>(self);
        DISTRHO_SAFE_ASSERT_RETURN(point->other == nullptr, V3_INVALID_ARG);
        DISTRHO_SAFE_ASSERT_RETURN(point->other != other, V3_INVALID_ARG);

        point->other = other;

        if (PluginVst3* const vst3 = point->vst3)
            vst3->ctrl2view_connect(other);

        return V3_OK;
    }

    static v3_result V3_API disconnect(void* const self, v3_connection_point** const other)
    {
        d_debug("dpf_ctrl2view_connection_point::disconnect => %p %p", self, other);
        dpf_ctrl2view_connection_point* const point = *static_cast<dpf_ctrl2view_connection_point**>(self);
        DISTRHO_SAFE_ASSERT_RETURN(point->other != nullptr, V3_INVALID_ARG);
        DISTRHO_SAFE_ASSERT_RETURN(point->other == other, V3_INVALID_ARG);

        if (PluginVst3* const vst3 = point->vst3)
            vst3->ctrl2view_disconnect();

        v3_cpp_obj_unref(point->other);
        point->other = nullptr;

        return V3_OK;
    }

    static v3_result V3_API notify(void* const self, v3_message** const message)
    {
        dpf_ctrl2view_connection_point* const point = *static_cast<dpf_ctrl2view_connection_point**>(self);

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

        if (target == 1)
        {
            // view -> edit controller
            return vst3->ctrl2view_notify(message);
        }
        else
        {
            // edit controller -> view
            return v3_cpp_obj(other)->notify(other, message);
        }
    }
};
#endif // DISTRHO_PLUGIN_HAS_UI

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
            d_debug("query_interface_midi_mapping => %p %s %p | OK", self, tuid2str(iid), iface);
            *iface = self;
            return V3_OK;
        }

        d_debug("query_interface_midi_mapping => %p %s %p | WARNING UNSUPPORTED", self, tuid2str(iid), iface);

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

        *id = kVst3InternalParameterMidiCC_start + channel * 130 + cc;
        return V3_TRUE;
    }

    DISTRHO_PREVENT_HEAP_ALLOCATION
};
#endif // DISTRHO_PLUGIN_WANT_MIDI_INPUT

// --------------------------------------------------------------------------------------------------------------------
// dpf_edit_controller

struct dpf_edit_controller : v3_edit_controller_cpp {
    std::atomic_int refcounter;
   #if DISTRHO_PLUGIN_HAS_UI
    ScopedPointer<dpf_ctrl2view_connection_point> connectionCtrl2View;
   #endif
   #if DPF_VST3_USES_SEPARATE_CONTROLLER
    ScopedPointer<dpf_comp2ctrl_connection_point> connectionComp2Ctrl;
    ScopedPointer<PluginVst3> vst3;
   #else
    ScopedPointer<PluginVst3>& vst3;
    bool initialized;
   #endif
    // cached values
    v3_component_handler** handler;
    v3_host_application** const hostApplicationFromFactory;
   #if !DPF_VST3_USES_SEPARATE_CONTROLLER
    v3_host_application** const hostApplicationFromComponent;
    v3_host_application** hostApplicationFromComponentInitialize;
   #endif
    v3_host_application** hostApplicationFromInitialize;

   #if DPF_VST3_USES_SEPARATE_CONTROLLER
    dpf_edit_controller(v3_host_application** const hostApp)
        : refcounter(1),
          vst3(nullptr),
   #else
    dpf_edit_controller(ScopedPointer<PluginVst3>& v, v3_host_application** const hostApp, v3_host_application** const hostComp)
        : refcounter(1),
          vst3(v),
          initialized(false),
   #endif
          handler(nullptr),
          hostApplicationFromFactory(hostApp),
         #if !DPF_VST3_USES_SEPARATE_CONTROLLER
          hostApplicationFromComponent(hostComp),
          hostApplicationFromComponentInitialize(nullptr),
         #endif
          hostApplicationFromInitialize(nullptr)
    {
        d_debug("dpf_edit_controller() with hostApplication %p", hostApplicationFromFactory);

        // make sure host application is valid through out this controller lifetime
        if (hostApplicationFromFactory != nullptr)
            v3_cpp_obj_ref(hostApplicationFromFactory);
       #if !DPF_VST3_USES_SEPARATE_CONTROLLER
        if (hostApplicationFromComponent != nullptr)
            v3_cpp_obj_ref(hostApplicationFromComponent);
       #endif

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

    ~dpf_edit_controller()
    {
        d_debug("~dpf_edit_controller()");
       #if DISTRHO_PLUGIN_HAS_UI
        connectionCtrl2View = nullptr;
       #endif
       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        connectionComp2Ctrl = nullptr;
        vst3 = nullptr;
       #endif

       #if !DPF_VST3_USES_SEPARATE_CONTROLLER
        if (hostApplicationFromComponent != nullptr)
            v3_cpp_obj_unref(hostApplicationFromComponent);
       #endif
        if (hostApplicationFromFactory != nullptr)
            v3_cpp_obj_unref(hostApplicationFromFactory);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_funknown

    static v3_result V3_API query_interface_edit_controller(void* const self, const v3_tuid iid, void** const iface)
    {
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

        if (v3_tuid_match(iid, v3_funknown_iid) ||
            v3_tuid_match(iid, v3_plugin_base_iid) ||
            v3_tuid_match(iid, v3_edit_controller_iid))
        {
            d_debug("query_interface_edit_controller => %p %s %p | OK", self, tuid2str(iid), iface);
            ++controller->refcounter;
            *iface = self;
            return V3_OK;
        }

        if (v3_tuid_match(iid, v3_midi_mapping_iid))
        {
           #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
            d_debug("query_interface_edit_controller => %p %s %p | OK convert static", self, tuid2str(iid), iface);
            static dpf_midi_mapping midi_mapping;
            static dpf_midi_mapping* midi_mapping_ptr = &midi_mapping;
            *iface = &midi_mapping_ptr;
            return V3_OK;
           #else
            d_debug("query_interface_edit_controller => %p %s %p | reject unused", self, tuid2str(iid), iface);
            *iface = nullptr;
            return V3_NO_INTERFACE;
           #endif
        }

        if (v3_tuid_match(iid, v3_connection_point_iid))
        {
           #if DPF_VST3_USES_SEPARATE_CONTROLLER
            d_debug("query_interface_edit_controller => %p %s %p | OK convert %p",
                    self, tuid2str(iid), iface, controller->connectionComp2Ctrl.get());

            if (controller->connectionComp2Ctrl == nullptr)
                controller->connectionComp2Ctrl = new dpf_comp2ctrl_connection_point(controller->vst3);
            else
                ++controller->connectionComp2Ctrl->refcounter;
            *iface = &controller->connectionComp2Ctrl;
            return V3_OK;
           #else
            d_debug("query_interface_edit_controller => %p %s %p | reject unwanted", self, tuid2str(iid), iface);
            *iface = nullptr;
            return V3_NO_INTERFACE;
           #endif
        }

        d_debug("query_interface_edit_controller => %p %s %p | WARNING UNSUPPORTED", self, tuid2str(iid), iface);
        *iface = nullptr;
        return V3_NO_INTERFACE;
    }

    static uint32_t V3_API ref_edit_controller(void* const self)
    {
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);
        const int refcount = ++controller->refcounter;
        d_debug("dpf_edit_controller::ref => %p | refcount %i", self, refcount);
        return refcount;
    }

    static uint32_t V3_API unref_edit_controller(void* const self)
    {
        dpf_edit_controller** const controllerptr = static_cast<dpf_edit_controller**>(self);
        dpf_edit_controller* const controller = *controllerptr;

        if (const int refcount = --controller->refcounter)
        {
            d_debug("dpf_edit_controller::unref => %p | refcount %i", self, refcount);
            return refcount;
        }

       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        /**
         * Some hosts will have unclean instances of a few of the controller child classes at this point.
         * We check for those here, going through the whole possible chain to see if it is safe to delete.
         * If not, we add this controller to the `gControllerGarbage` global which will take care of it during unload.
         */

        bool unclean = false;

        if (dpf_comp2ctrl_connection_point* const point = controller->connectionComp2Ctrl)
        {
            if (const int refcount = point->refcounter)
            {
                unclean = true;
                d_stderr("DPF warning: asked to delete controller while component connection point still active (refcount %d)", refcount);
            }
        }

        if (unclean)
            return handleUncleanController(controllerptr);

        d_debug("dpf_edit_controller::unref => %p | refcount is zero, deleting everything now!", self);

        delete controller;
        delete controllerptr;
       #else
        d_debug("dpf_edit_controller::unref => %p | refcount is zero, deletion will be done by component later", self);
       #endif
        return 0;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_plugin_base

    static v3_result V3_API initialize(void* const self, v3_funknown** const context)
    {
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

        // check if already initialized
       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        DISTRHO_SAFE_ASSERT_RETURN(controller->vst3 == nullptr, V3_INVALID_ARG);
       #else
        DISTRHO_SAFE_ASSERT_RETURN(! controller->initialized, V3_INVALID_ARG);
       #endif

        // query for host application
        v3_host_application** hostApplication = nullptr;
        if (context != nullptr)
            v3_cpp_obj_query_interface(context, v3_host_application_iid, &hostApplication);

        d_debug("dpf_edit_controller::initialize => %p %p | host %p", self, context, hostApplication);

        // save it for later so we can unref it
        controller->hostApplicationFromInitialize = hostApplication;

       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        // provide the factory application to the plugin if this new one is missing
        if (hostApplication == nullptr)
            hostApplication = controller->hostApplicationFromFactory;

        // default early values
        if (d_nextBufferSize == 0)
            d_nextBufferSize = 1024;
        if (d_nextSampleRate <= 0.0)
            d_nextSampleRate = 44100.0;

        d_nextCanRequestParameterValueChanges = true;

        // create the actual plugin
        controller->vst3 = new PluginVst3(hostApplication, false);

        // set connection point if needed
        if (dpf_comp2ctrl_connection_point* const point = controller->connectionComp2Ctrl)
        {
            if (point->other != nullptr)
                controller->vst3->comp2ctrl_connect(point->other);
        }
       #else
        // mark as initialized
        controller->initialized = true;
       #endif

        return V3_OK;
    }

    static v3_result V3_API terminate(void* self)
    {
        d_debug("dpf_edit_controller::terminate => %p", self);
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        // check if already terminated
        DISTRHO_SAFE_ASSERT_RETURN(controller->vst3 != nullptr, V3_INVALID_ARG);

        // delete actual plugin
        controller->vst3 = nullptr;
       #else
        // check if already terminated
        DISTRHO_SAFE_ASSERT_RETURN(controller->initialized, V3_INVALID_ARG);

        // mark as uninitialzed
        controller->initialized = false;
       #endif

        // unref host application received during initialize
        if (controller->hostApplicationFromInitialize != nullptr)
        {
            v3_cpp_obj_unref(controller->hostApplicationFromInitialize);
            controller->hostApplicationFromInitialize = nullptr;
        }

        return V3_OK;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_edit_controller

    static v3_result V3_API set_component_state(void* const self, v3_bstream** const stream)
    {
        d_debug("dpf_edit_controller::set_component_state => %p %p", self, stream);

       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

        PluginVst3* const vst3 = controller->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->setState(stream);
       #else
        return V3_OK;

        // unused
        (void)self;
        (void)stream;
       #endif
    }

    static v3_result V3_API set_state(void* const self, v3_bstream** const stream)
    {
        d_debug("dpf_edit_controller::set_state => %p %p", self, stream);

       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);
        DISTRHO_SAFE_ASSERT_RETURN(controller->vst3 != nullptr, V3_NOT_INITIALIZED);
       #endif

        return V3_NOT_IMPLEMENTED;

        // maybe unused
        (void)self;
        (void)stream;
    }

    static v3_result V3_API get_state(void* const self, v3_bstream** const stream)
    {
        d_debug("dpf_edit_controller::get_state => %p %p", self, stream);

       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);
        DISTRHO_SAFE_ASSERT_RETURN(controller->vst3 != nullptr, V3_NOT_INITIALIZED);
       #endif

        return V3_NOT_IMPLEMENTED;

        // maybe unused
        (void)self;
        (void)stream;
    }

    static int32_t V3_API get_parameter_count(void* self)
    {
        // d_debug("dpf_edit_controller::get_parameter_count => %p", self);
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

        PluginVst3* const vst3 = controller->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->getParameterCount();
    }

    static v3_result V3_API get_parameter_info(void* self, int32_t param_idx, v3_param_info* param_info)
    {
        // d_debug("dpf_edit_controller::get_parameter_info => %p %i", self, param_idx);
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

        PluginVst3* const vst3 = controller->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->getParameterInfo(param_idx, param_info);
    }

    static v3_result V3_API get_parameter_string_for_value(void* self, v3_param_id index, double normalized, v3_str_128 output)
    {
        // NOTE very noisy, called many times
        // d_debug("dpf_edit_controller::get_parameter_string_for_value => %p %u %f %p", self, index, normalized, output);
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

        PluginVst3* const vst3 = controller->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->getParameterStringForValue(index, normalized, output);
    }

    static v3_result V3_API get_parameter_value_for_string(void* self, v3_param_id index, int16_t* input, double* output)
    {
        d_debug("dpf_edit_controller::get_parameter_value_for_string => %p %u %p %p", self, index, input, output);
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

        PluginVst3* const vst3 = controller->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->getParameterValueForString(index, input, output);
    }

    static double V3_API normalised_parameter_to_plain(void* self, v3_param_id index, double normalized)
    {
        d_debug("dpf_edit_controller::normalised_parameter_to_plain => %p %u %f", self, index, normalized);
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

        PluginVst3* const vst3 = controller->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->normalizedParameterToPlain(index, normalized);
    }

    static double V3_API plain_parameter_to_normalised(void* self, v3_param_id index, double plain)
    {
        d_debug("dpf_edit_controller::plain_parameter_to_normalised => %p %u %f", self, index, plain);
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

        PluginVst3* const vst3 = controller->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->plainParameterToNormalized(index, plain);
    }

    static double V3_API get_parameter_normalised(void* self, v3_param_id index)
    {
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

        PluginVst3* const vst3 = controller->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, 0.0);

        return vst3->getParameterNormalized(index);
    }

    static v3_result V3_API set_parameter_normalised(void* const self, const v3_param_id index, const double normalized)
    {
        // d_debug("dpf_edit_controller::set_parameter_normalised => %p %u %f", self, index, normalized);
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

        PluginVst3* const vst3 = controller->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->setParameterNormalized(index, normalized);
    }

    static v3_result V3_API set_component_handler(void* self, v3_component_handler** handler)
    {
        d_debug("dpf_edit_controller::set_component_handler => %p %p", self, handler);
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

        controller->handler = handler;

        if (PluginVst3* const vst3 = controller->vst3)
            return vst3->setComponentHandler(handler);

        return V3_NOT_INITIALIZED;
    }

    static v3_plugin_view** V3_API create_view(void* self, const char* name)
    {
        d_debug("dpf_edit_controller::create_view => %p %s", self, name);

       #if DISTRHO_PLUGIN_HAS_UI
        dpf_edit_controller* const controller = *static_cast<dpf_edit_controller**>(self);

        d_debug("create_view has contexts %p %p",
                controller->hostApplicationFromFactory, controller->hostApplicationFromInitialize);

        // plugin must be initialized
        PluginVst3* const vst3 = controller->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, nullptr);

        d_debug("dpf_edit_controller::create_view => %p %s | edit-ctrl %p, factory %p",
                self, name,
                controller->hostApplicationFromInitialize,
                controller->hostApplicationFromFactory);

        // we require a host application for message creation
        v3_host_application** const host = controller->hostApplicationFromInitialize != nullptr
                                         ? controller->hostApplicationFromInitialize
                                        #if !DPF_VST3_USES_SEPARATE_CONTROLLER
                                         : controller->hostApplicationFromComponent != nullptr
                                         ? controller->hostApplicationFromComponent
                                         : controller->hostApplicationFromComponentInitialize != nullptr
                                         ? controller->hostApplicationFromComponentInitialize
                                        #endif
                                         : controller->hostApplicationFromFactory;
        DISTRHO_SAFE_ASSERT_RETURN(host != nullptr, nullptr);

        v3_plugin_view** const view = dpf_plugin_view_create(host,
                                                             vst3->getInstancePointer(),
                                                             vst3->getSampleRate());
        DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, nullptr);

        v3_connection_point** uiconn = nullptr;
        if (v3_cpp_obj_query_interface(view, v3_connection_point_iid, &uiconn) == V3_OK)
        {
            d_debug("view connection query ok %p", uiconn);
            controller->connectionCtrl2View = new dpf_ctrl2view_connection_point(controller->vst3);

            v3_connection_point** const ctrlconn = (v3_connection_point**)&controller->connectionCtrl2View;

            v3_cpp_obj(uiconn)->connect(uiconn, ctrlconn);
            v3_cpp_obj(ctrlconn)->connect(ctrlconn, uiconn);
        }
        else
        {
            controller->connectionCtrl2View = nullptr;
        }

        return view;
       #else
        return nullptr;
       #endif

        // maybe unused
        (void)self;
        (void)name;
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
        if (v3_tuid_match(iid, v3_funknown_iid) ||
            v3_tuid_match(iid, v3_process_context_requirements_iid))
        {
            d_debug("query_interface_process_context_requirements => %p %s %p | OK", self, tuid2str(iid), iface);
            *iface = self;
            return V3_OK;
        }

        d_debug("query_interface_process_context_requirements => %p %s %p | WARNING UNSUPPORTED", self, tuid2str(iid), iface);

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
        dpf_audio_processor* const processor = *static_cast<dpf_audio_processor**>(self);

        if (v3_tuid_match(iid, v3_funknown_iid) ||
            v3_tuid_match(iid, v3_audio_processor_iid))
        {
            d_debug("query_interface_audio_processor => %p %s %p | OK", self, tuid2str(iid), iface);
            ++processor->refcounter;
            *iface = self;
            return V3_OK;
        }

        if (v3_tuid_match(iid, v3_process_context_requirements_iid))
        {
            d_debug("query_interface_audio_processor => %p %s %p | OK convert static", self, tuid2str(iid), iface);
            static dpf_process_context_requirements context_req;
            static dpf_process_context_requirements* context_req_ptr = &context_req;
            *iface = &context_req_ptr;
            return V3_OK;
        }

        d_debug("query_interface_audio_processor => %p %s %p | WARNING UNSUPPORTED", self, tuid2str(iid), iface);

        *iface = nullptr;
        return V3_NO_INTERFACE;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_audio_processor

    static v3_result V3_API set_bus_arrangements(void* const self,
                                                 v3_speaker_arrangement* const inputs, const int32_t num_inputs,
                                                 v3_speaker_arrangement* const outputs, const int32_t num_outputs)
    {
        // NOTE this is called a bunch of times in JUCE hosts
        d_debug("dpf_audio_processor::set_bus_arrangements => %p %p %i %p %i",
                self, inputs, num_inputs, outputs, num_outputs);
        dpf_audio_processor* const processor = *static_cast<dpf_audio_processor**>(self);

        PluginVst3* const vst3 = processor->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return processor->vst3->setBusArrangements(inputs, num_inputs, outputs, num_outputs);
    }

    static v3_result V3_API get_bus_arrangement(void* const self, const int32_t bus_direction,
                                                const int32_t idx, v3_speaker_arrangement* const arr)
    {
        d_debug("dpf_audio_processor::get_bus_arrangement => %p %s %i %p",
                self, v3_bus_direction_str(bus_direction), idx, arr);
        dpf_audio_processor* const processor = *static_cast<dpf_audio_processor**>(self);

        PluginVst3* const vst3 = processor->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return processor->vst3->getBusArrangement(bus_direction, idx, arr);
    }

    static v3_result V3_API can_process_sample_size(void*, const int32_t symbolic_sample_size)
    {
        // NOTE runs during RT
        // d_debug("dpf_audio_processor::can_process_sample_size => %i", symbolic_sample_size);
        return symbolic_sample_size == V3_SAMPLE_32 ? V3_OK : V3_NOT_IMPLEMENTED;
    }

    static uint32_t V3_API get_latency_samples(void* const self)
    {
        d_debug("dpf_audio_processor::get_latency_samples => %p", self);
        dpf_audio_processor* const processor = *static_cast<dpf_audio_processor**>(self);

        PluginVst3* const vst3 = processor->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, 0);

        return processor->vst3->getLatencySamples();
    }

    static v3_result V3_API setup_processing(void* const self, v3_process_setup* const setup)
    {
        d_debug("dpf_audio_processor::setup_processing => %p %p", self, setup);
        dpf_audio_processor* const processor = *static_cast<dpf_audio_processor**>(self);

        PluginVst3* const vst3 = processor->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        d_debug("dpf_audio_processor::setup_processing => %p %p | %d %f", self, setup, setup->max_block_size, setup->sample_rate);

        d_nextBufferSize = setup->max_block_size;
        d_nextSampleRate = setup->sample_rate;
        return processor->vst3->setupProcessing(setup);
    }

    static v3_result V3_API set_processing(void* const self, const v3_bool state)
    {
        d_debug("dpf_audio_processor::set_processing => %p %u", self, state);
        dpf_audio_processor* const processor = *static_cast<dpf_audio_processor**>(self);

        PluginVst3* const vst3 = processor->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return processor->vst3->setProcessing(state);
    }

    static v3_result V3_API process(void* const self, v3_process_data* const data)
    {
        // NOTE runs during RT
        // d_debug("dpf_audio_processor::process => %p", self);
        dpf_audio_processor* const processor = *static_cast<dpf_audio_processor**>(self);

        PluginVst3* const vst3 = processor->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return processor->vst3->process(data);
    }

    static uint32_t V3_API get_tail_samples(void* const self)
    {
        d_debug("dpf_audio_processor::get_tail_samples => %p", self);
        dpf_audio_processor* const processor = *static_cast<dpf_audio_processor**>(self);

        PluginVst3* const vst3 = processor->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, 0);

        return processor->vst3->getTailSamples();
    }
};

// --------------------------------------------------------------------------------------------------------------------
// dpf_component

struct dpf_component : v3_component_cpp {
    std::atomic_int refcounter;
    ScopedPointer<dpf_audio_processor> processor;
   #if DPF_VST3_USES_SEPARATE_CONTROLLER
    ScopedPointer<dpf_comp2ctrl_connection_point> connectionComp2Ctrl;
   #else
    ScopedPointer<dpf_edit_controller> controller;
   #endif
    ScopedPointer<PluginVst3> vst3;
    v3_host_application** const hostApplicationFromFactory;
    v3_host_application** hostApplicationFromInitialize;

    dpf_component(v3_host_application** const host)
        : refcounter(1),
          hostApplicationFromFactory(host),
          hostApplicationFromInitialize(nullptr)
    {
        d_debug("dpf_component() with hostApplication %p", hostApplicationFromFactory);

        // make sure host application is valid through out this component lifetime
        if (hostApplicationFromFactory != nullptr)
            v3_cpp_obj_ref(hostApplicationFromFactory);

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

    ~dpf_component()
    {
        d_debug("~dpf_component()");
        processor = nullptr;
       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        connectionComp2Ctrl = nullptr;
       #else
        controller = nullptr;
       #endif
        vst3 = nullptr;

        if (hostApplicationFromFactory != nullptr)
            v3_cpp_obj_unref(hostApplicationFromFactory);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_funknown

    static v3_result V3_API query_interface_component(void* const self, const v3_tuid iid, void** const iface)
    {
        dpf_component* const component = *static_cast<dpf_component**>(self);

        if (v3_tuid_match(iid, v3_funknown_iid) ||
            v3_tuid_match(iid, v3_plugin_base_iid) ||
            v3_tuid_match(iid, v3_component_iid))
        {
            d_debug("query_interface_component => %p %s %p | OK", self, tuid2str(iid), iface);
            ++component->refcounter;
            *iface = self;
            return V3_OK;
        }

        if (v3_tuid_match(iid, v3_midi_mapping_iid))
        {
           #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
            d_debug("query_interface_component => %p %s %p | OK convert static", self, tuid2str(iid), iface);
            static dpf_midi_mapping midi_mapping;
            static dpf_midi_mapping* midi_mapping_ptr = &midi_mapping;
            *iface = &midi_mapping_ptr;
            return V3_OK;
           #else
            d_debug("query_interface_component => %p %s %p | reject unused", self, tuid2str(iid), iface);
            *iface = nullptr;
            return V3_NO_INTERFACE;
           #endif
        }

        if (v3_tuid_match(iid, v3_audio_processor_iid))
        {
            d_debug("query_interface_component => %p %s %p | OK convert %p",
                    self, tuid2str(iid), iface, component->processor.get());

            if (component->processor == nullptr)
                component->processor = new dpf_audio_processor(component->vst3);
            else
                ++component->processor->refcounter;
            *iface = &component->processor;
            return V3_OK;
        }

        if (v3_tuid_match(iid, v3_connection_point_iid))
        {
           #if DPF_VST3_USES_SEPARATE_CONTROLLER
            d_debug("query_interface_component => %p %s %p | OK convert %p",
                    self, tuid2str(iid), iface, component->connectionComp2Ctrl.get());

            if (component->connectionComp2Ctrl == nullptr)
                component->connectionComp2Ctrl = new dpf_comp2ctrl_connection_point(component->vst3);
            else
                ++component->connectionComp2Ctrl->refcounter;
            *iface = &component->connectionComp2Ctrl;
            return V3_OK;
           #else
            d_debug("query_interface_component => %p %s %p | reject unwanted", self, tuid2str(iid), iface);
            *iface = nullptr;
            return V3_NO_INTERFACE;
           #endif
        }

        if (v3_tuid_match(iid, v3_edit_controller_iid))
        {
           #if !DPF_VST3_USES_SEPARATE_CONTROLLER
            d_debug("query_interface_component => %p %s %p | OK convert %p",
                    self, tuid2str(iid), iface, component->controller.get());

            if (component->controller == nullptr)
                component->controller = new dpf_edit_controller(component->vst3,
                                                                component->hostApplicationFromFactory,
                                                                component->hostApplicationFromInitialize);
            else
                ++component->controller->refcounter;
            *iface = &component->controller;
            return V3_OK;
           #else
            d_debug("query_interface_component => %p %s %p | reject unwanted", self, tuid2str(iid), iface);
            *iface = nullptr;
            return V3_NO_INTERFACE;
           #endif
        }

        d_debug("query_interface_component => %p %s %p | WARNING UNSUPPORTED", self, tuid2str(iid), iface);
        *iface = nullptr;
        return V3_NO_INTERFACE;
    }

    static uint32_t V3_API ref_component(void* const self)
    {
        dpf_component* const component = *static_cast<dpf_component**>(self);
        const int refcount = ++component->refcounter;
        d_debug("dpf_component::ref => %p | refcount %i", self, refcount);
        return refcount;
    }

    static uint32_t V3_API unref_component(void* const self)
    {
        dpf_component** const componentptr = static_cast<dpf_component**>(self);
        dpf_component* const component = *componentptr;

        if (const int refcount = --component->refcounter)
        {
            d_debug("dpf_component::unref => %p | refcount %i", self, refcount);
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

       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        if (dpf_comp2ctrl_connection_point* const point = component->connectionComp2Ctrl)
        {
            if (const int refcount = point->refcounter)
            {
                unclean = true;
                d_stderr("DPF warning: asked to delete component while connection point still active (refcount %d)", refcount);
            }
        }
       #else
        if (dpf_edit_controller* const controller = component->controller)
        {
            if (const int refcount = controller->refcounter)
            {
                unclean = true;
                d_stderr("DPF warning: asked to delete component while edit controller still active (refcount %d)", refcount);
            }
        }
       #endif

        if (unclean)
            return handleUncleanComponent(componentptr);

        d_debug("dpf_component::unref => %p | refcount is zero, deleting everything now!", self);

        delete component;
        delete componentptr;
        return 0;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_plugin_base

    static v3_result V3_API initialize(void* const self, v3_funknown** const context)
    {
        dpf_component* const component = *static_cast<dpf_component**>(self);

        // check if already initialized
        DISTRHO_SAFE_ASSERT_RETURN(component->vst3 == nullptr, V3_INVALID_ARG);

        // query for host application
        v3_host_application** hostApplication = nullptr;
        if (context != nullptr)
            v3_cpp_obj_query_interface(context, v3_host_application_iid, &hostApplication);

        d_debug("dpf_component::initialize => %p %p | hostApplication %p", self, context, hostApplication);

        // save it for later so we can unref it
        component->hostApplicationFromInitialize = hostApplication;

       #if !DPF_VST3_USES_SEPARATE_CONTROLLER
        // save it in edit controller too, needed for some hosts
        if (component->controller != nullptr)
            component->controller->hostApplicationFromComponentInitialize = hostApplication;
       #endif

        // provide the factory application to the plugin if this new one is missing
        if (hostApplication == nullptr)
            hostApplication = component->hostApplicationFromFactory;

        // default early values
        if (d_nextBufferSize == 0)
            d_nextBufferSize = 1024;
        if (d_nextSampleRate <= 0.0)
            d_nextSampleRate = 44100.0;

        d_nextCanRequestParameterValueChanges = true;

        // create the actual plugin
        component->vst3 = new PluginVst3(hostApplication, true);

       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        // set connection point if needed
        if (dpf_comp2ctrl_connection_point* const point = component->connectionComp2Ctrl)
        {
            if (point->other != nullptr)
                component->vst3->comp2ctrl_connect(point->other);
        }
       #endif

        return V3_OK;
    }

    static v3_result V3_API terminate(void* const self)
    {
        d_debug("dpf_component::terminate => %p", self);
        dpf_component* const component = *static_cast<dpf_component**>(self);

        // check if already terminated
        DISTRHO_SAFE_ASSERT_RETURN(component->vst3 != nullptr, V3_INVALID_ARG);

        // delete actual plugin
        component->vst3 = nullptr;

       #if !DPF_VST3_USES_SEPARATE_CONTROLLER
        // remove previous host application saved during initialize
        if (component->controller != nullptr)
            component->controller->hostApplicationFromComponentInitialize = nullptr;
       #endif

        // unref host application received during initialize
        if (component->hostApplicationFromInitialize != nullptr)
        {
            v3_cpp_obj_unref(component->hostApplicationFromInitialize);
            component->hostApplicationFromInitialize = nullptr;
        }

        return V3_OK;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_component

    static v3_result V3_API get_controller_class_id(void*, v3_tuid class_id)
    {
        d_debug("dpf_component::get_controller_class_id => %p", class_id);

        std::memcpy(class_id, dpf_tuid_controller, sizeof(v3_tuid));
        return V3_OK;
    }

    static v3_result V3_API set_io_mode(void* const self, const int32_t io_mode)
    {
        d_debug("dpf_component::set_io_mode => %p %i", self, io_mode);

        dpf_component* const component = *static_cast<dpf_component**>(self);

        PluginVst3* const vst3 = component->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        // TODO
        return V3_NOT_IMPLEMENTED;

        // unused
        (void)io_mode;
    }

    static int32_t V3_API get_bus_count(void* const self, const int32_t media_type, const int32_t bus_direction)
    {
        // NOTE runs during RT
        // d_debug("dpf_component::get_bus_count => %p %s %s",
        //          self, v3_media_type_str(media_type), v3_bus_direction_str(bus_direction));
        dpf_component* const component = *static_cast<dpf_component**>(self);

        PluginVst3* const vst3 = component->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        const int32_t ret = vst3->getBusCount(media_type, bus_direction);
        // d_debug("dpf_component::get_bus_count returns %i", ret);
        return ret;
    }

    static v3_result V3_API get_bus_info(void* const self, const int32_t media_type, const int32_t bus_direction,
                                         const int32_t bus_idx, v3_bus_info* const info)
    {
        d_debug("dpf_component::get_bus_info => %p %s %s %i %p",
                self, v3_media_type_str(media_type), v3_bus_direction_str(bus_direction), bus_idx, info);
        dpf_component* const component = *static_cast<dpf_component**>(self);

        PluginVst3* const vst3 = component->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->getBusInfo(media_type, bus_direction, bus_idx, info);
    }

    static v3_result V3_API get_routing_info(void* const self, v3_routing_info* const input, v3_routing_info* const output)
    {
        d_debug("dpf_component::get_routing_info => %p %p %p", self, input, output);
        dpf_component* const component = *static_cast<dpf_component**>(self);

        PluginVst3* const vst3 = component->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->getRoutingInfo(input, output);
    }

    static v3_result V3_API activate_bus(void* const self, const int32_t media_type, const int32_t bus_direction,
                                         const int32_t bus_idx, const v3_bool state)
    {
        // NOTE this is called a bunch of times
        // d_debug("dpf_component::activate_bus => %p %s %s %i %u",
        //         self, v3_media_type_str(media_type), v3_bus_direction_str(bus_direction), bus_idx, state);
        dpf_component* const component = *static_cast<dpf_component**>(self);

        PluginVst3* const vst3 = component->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->activateBus(media_type, bus_direction, bus_idx, state);
    }

    static v3_result V3_API set_active(void* const self, const v3_bool state)
    {
        d_debug("dpf_component::set_active => %p %u", self, state);
        dpf_component* const component = *static_cast<dpf_component**>(self);

        PluginVst3* const vst3 = component->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return component->vst3->setActive(state);
    }

    static v3_result V3_API set_state(void* const self, v3_bstream** const stream)
    {
        d_debug("dpf_component::set_state => %p", self);
        dpf_component* const component = *static_cast<dpf_component**>(self);

        PluginVst3* const vst3 = component->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->setState(stream);
    }

    static v3_result V3_API get_state(void* const self, v3_bstream** const stream)
    {
        d_debug("dpf_component::get_state => %p %p", self, stream);
        dpf_component* const component = *static_cast<dpf_component**>(self);

        PluginVst3* const vst3 = component->vst3;
        DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALIZED);

        return vst3->getState(stream);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// Dummy plugin to get data from

static ScopedPointer<PluginExporter> sPlugin;

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
       #else
        categories = "Fx";
       #endif
        firstInit = false;

        // An empty category is considered invalid in Cubase
        DISTRHO_SAFE_ASSERT(categories.isNotEmpty());
    }

    return categories.buffer();
}

static const char* getPluginVersion()
{
    static String version;

    if (version.isEmpty())
    {
        const uint32_t versionNum = sPlugin->getVersion();

        char versionBuf[64];
        std::snprintf(versionBuf, sizeof(versionBuf)-1, "%d.%d.%d",
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
    std::atomic_int refcounter;

    // cached values
    v3_funknown** hostContext;

    dpf_factory()
        : refcounter(1),
          hostContext(nullptr)
    {
        // v3_funknown, static
        query_interface = query_interface_factory;
        ref = ref_factory;
        unref = unref_factory;

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

       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        if (gControllerGarbage.size() != 0)
        {
            d_debug("DPF notice: cleaning up previously undeleted controllers now");

            for (std::vector<dpf_edit_controller**>::iterator it = gControllerGarbage.begin();
                it != gControllerGarbage.end(); ++it)
            {
                dpf_edit_controller** const controllerptr = *it;
                dpf_edit_controller* const controller = *controllerptr;
                delete controller;
                delete controllerptr;
            }

            gControllerGarbage.clear();
        }
       #endif

        if (gComponentGarbage.size() != 0)
        {
            d_debug("DPF notice: cleaning up previously undeleted components now");

            for (std::vector<dpf_component**>::iterator it = gComponentGarbage.begin();
                it != gComponentGarbage.end(); ++it)
            {
                dpf_component** const componentptr = *it;
                dpf_component* const component = *componentptr;
                delete component;
                delete componentptr;
            }

            gComponentGarbage.clear();
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_funknown

    static v3_result V3_API query_interface_factory(void* const self, const v3_tuid iid, void** const iface)
    {
        dpf_factory* const factory = *static_cast<dpf_factory**>(self);

        if (v3_tuid_match(iid, v3_funknown_iid) ||
            v3_tuid_match(iid, v3_plugin_factory_iid) ||
            v3_tuid_match(iid, v3_plugin_factory_2_iid) ||
            v3_tuid_match(iid, v3_plugin_factory_3_iid))
        {
            d_debug("query_interface_factory => %p %s %p | OK", self, tuid2str(iid), iface);
            ++factory->refcounter;
            *iface = self;
            return V3_OK;
        }

        d_debug("query_interface_factory => %p %s %p | WARNING UNSUPPORTED", self, tuid2str(iid), iface);

        *iface = nullptr;
        return V3_NO_INTERFACE;
    }

    static uint32_t V3_API ref_factory(void* const self)
    {
        dpf_factory* const factory = *static_cast<dpf_factory**>(self);
        const int refcount = ++factory->refcounter;
        d_debug("ref_factory::ref => %p | refcount %i", self, refcount);
        return refcount;
    }

    static uint32_t V3_API unref_factory(void* const self)
    {
        dpf_factory** const factoryptr = static_cast<dpf_factory**>(self);
        dpf_factory* const factory = *factoryptr;

        if (const int refcount = --factory->refcounter)
        {
            d_debug("unref_factory::unref => %p | refcount %i", self, refcount);
            return refcount;
        }

        d_debug("unref_factory::unref => %p | refcount is zero, deleting factory", self);

        delete factory;
        delete factoryptr;
        return 0;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_plugin_factory

    static v3_result V3_API get_factory_info(void*, v3_factory_info* const info)
    {
        d_debug("dpf_factory::get_factory_info => %p", info);
        std::memset(info, 0, sizeof(*info));

        info->flags = 0x10; // unicode
        d_strncpy(info->vendor, sPlugin->getMaker(), ARRAY_SIZE(info->vendor));
        d_strncpy(info->url, sPlugin->getHomePage(), ARRAY_SIZE(info->url));
        // d_strncpy(info->email, "", ARRAY_SIZE(info->email)); // TODO
        return V3_OK;
    }

    static int32_t V3_API num_classes(void*)
    {
        d_debug("dpf_factory::num_classes");
       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        return 2; // factory can create component and edit-controller
       #else
        return 1; // factory can only create component, edit-controller must be casted
       #endif
    }

    static v3_result V3_API get_class_info(void*, const int32_t idx, v3_class_info* const info)
    {
        d_debug("dpf_factory::get_class_info => %i %p", idx, info);
        std::memset(info, 0, sizeof(*info));
        DISTRHO_SAFE_ASSERT_RETURN(idx <= 2, V3_INVALID_ARG);

        info->cardinality = 0x7FFFFFFF;
        d_strncpy(info->name, sPlugin->getName(), ARRAY_SIZE(info->name));

        if (idx == 0)
        {
            std::memcpy(info->class_id, dpf_tuid_class, sizeof(v3_tuid));
            d_strncpy(info->category, "Audio Module Class", ARRAY_SIZE(info->category));
        }
        else
        {
            std::memcpy(info->class_id, dpf_tuid_controller, sizeof(v3_tuid));
            d_strncpy(info->category, "Component Controller Class", ARRAY_SIZE(info->category));
        }

        return V3_OK;
    }

    static v3_result V3_API create_instance(void* self, const v3_tuid class_id, const v3_tuid iid, void** const instance)
    {
        d_debug("dpf_factory::create_instance => %p %s %s %p", self, tuid2str(class_id), tuid2str(iid), instance);
        dpf_factory* const factory = *static_cast<dpf_factory**>(self);

        // query for host application
        v3_host_application** hostApplication = nullptr;
        if (factory->hostContext != nullptr)
            v3_cpp_obj_query_interface(factory->hostContext, v3_host_application_iid, &hostApplication);

        // create component
        if (v3_tuid_match(class_id, *(const v3_tuid*)&dpf_tuid_class) && (v3_tuid_match(iid, v3_component_iid) ||
                                                                          v3_tuid_match(iid, v3_funknown_iid)))
        {
            dpf_component** const componentptr = new dpf_component*;
            *componentptr = new dpf_component(hostApplication);
            *instance = static_cast<void*>(componentptr);
            return V3_OK;
        }

       #if DPF_VST3_USES_SEPARATE_CONTROLLER
        // create edit controller
        if (v3_tuid_match(class_id, *(const v3_tuid*)&dpf_tuid_controller) && (v3_tuid_match(iid, v3_edit_controller_iid) ||
                                                                               v3_tuid_match(iid, v3_funknown_iid)))
        {
            dpf_edit_controller** const controllerptr = new dpf_edit_controller*;
            *controllerptr = new dpf_edit_controller(hostApplication);
            *instance = static_cast<void*>(controllerptr);
            return V3_OK;
        }
       #endif

        // unsupported, roll back host application
        if (hostApplication != nullptr)
            v3_cpp_obj_unref(hostApplication);

        return V3_NO_INTERFACE;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_plugin_factory_2

    static v3_result V3_API get_class_info_2(void*, const int32_t idx, v3_class_info_2* const info)
    {
        d_debug("dpf_factory::get_class_info_2 => %i %p", idx, info);
        std::memset(info, 0, sizeof(*info));
        DISTRHO_SAFE_ASSERT_RETURN(idx <= 2, V3_INVALID_ARG);

        info->cardinality = 0x7FFFFFFF;
       #if DPF_VST3_USES_SEPARATE_CONTROLLER || !DISTRHO_PLUGIN_HAS_UI
        info->class_flags = V3_DISTRIBUTABLE;
       #endif
        d_strncpy(info->sub_categories, getPluginCategories(), ARRAY_SIZE(info->sub_categories));
        d_strncpy(info->name, sPlugin->getName(), ARRAY_SIZE(info->name));
        d_strncpy(info->vendor, sPlugin->getMaker(), ARRAY_SIZE(info->vendor));
        d_strncpy(info->version, getPluginVersion(), ARRAY_SIZE(info->version));
        d_strncpy(info->sdk_version, "VST 3.7.4", ARRAY_SIZE(info->sdk_version));

        if (idx == 0)
        {
            std::memcpy(info->class_id, dpf_tuid_class, sizeof(v3_tuid));
            d_strncpy(info->category, "Audio Module Class", ARRAY_SIZE(info->category));
        }
        else
        {
            std::memcpy(info->class_id, dpf_tuid_controller, sizeof(v3_tuid));
            d_strncpy(info->category, "Component Controller Class", ARRAY_SIZE(info->category));
        }

        return V3_OK;
    }

    // ------------------------------------------------------------------------------------------------------------
    // v3_plugin_factory_3

    static v3_result V3_API get_class_info_utf16(void*, const int32_t idx, v3_class_info_3* const info)
    {
        d_debug("dpf_factory::get_class_info_utf16 => %i %p", idx, info);
        std::memset(info, 0, sizeof(*info));
        DISTRHO_SAFE_ASSERT_RETURN(idx <= 2, V3_INVALID_ARG);

        info->cardinality = 0x7FFFFFFF;
       #if DPF_VST3_USES_SEPARATE_CONTROLLER || !DISTRHO_PLUGIN_HAS_UI
        info->class_flags = V3_DISTRIBUTABLE;
       #endif
        d_strncpy(info->sub_categories, getPluginCategories(), ARRAY_SIZE(info->sub_categories));
        DISTRHO_NAMESPACE::strncpy_utf16(info->name, sPlugin->getName(), ARRAY_SIZE(info->name));
        DISTRHO_NAMESPACE::strncpy_utf16(info->vendor, sPlugin->getMaker(), ARRAY_SIZE(info->vendor));
        DISTRHO_NAMESPACE::strncpy_utf16(info->version, getPluginVersion(), ARRAY_SIZE(info->version));
        DISTRHO_NAMESPACE::strncpy_utf16(info->sdk_version, "VST 3.7.4", ARRAY_SIZE(info->sdk_version));

        if (idx == 0)
        {
            std::memcpy(info->class_id, dpf_tuid_class, sizeof(v3_tuid));
            d_strncpy(info->category, "Audio Module Class", ARRAY_SIZE(info->category));
        }
        else
        {
            std::memcpy(info->class_id, dpf_tuid_controller, sizeof(v3_tuid));
            d_strncpy(info->category, "Component Controller Class", ARRAY_SIZE(info->category));
        }

        return V3_OK;
    }

    static v3_result V3_API set_host_context(void* const self, v3_funknown** const context)
    {
        d_debug("dpf_factory::set_host_context => %p %p", self, context);
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
};

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
// VST3 entry point

DISTRHO_PLUGIN_EXPORT
const void* GetPluginFactory(void);

const void* GetPluginFactory(void)
{
    USE_NAMESPACE_DISTRHO;
    dpf_factory** const factoryptr = new dpf_factory*;
    *factoryptr = new dpf_factory;
    return static_cast<void*>(factoryptr);
}

// --------------------------------------------------------------------------------------------------------------------
// OS specific module load

#if defined(DISTRHO_OS_MAC)
# define ENTRYFNNAME bundleEntry
# define ENTRYFNNAMEARGS void*
# define EXITFNNAME bundleExit
#elif defined(DISTRHO_OS_WINDOWS)
# define ENTRYFNNAME InitDll
# define ENTRYFNNAMEARGS void
# define EXITFNNAME ExitDll
#else
# define ENTRYFNNAME ModuleEntry
# define ENTRYFNNAMEARGS void*
# define EXITFNNAME ModuleExit
#endif

DISTRHO_PLUGIN_EXPORT
bool ENTRYFNNAME(ENTRYFNNAMEARGS);

bool ENTRYFNNAME(ENTRYFNNAMEARGS)
{
    USE_NAMESPACE_DISTRHO;

    // find plugin bundle
    static String bundlePath;
    if (bundlePath.isEmpty())
    {
        String tmpPath(getBinaryFilename());
        tmpPath.truncate(tmpPath.rfind(DISTRHO_OS_SEP));
        tmpPath.truncate(tmpPath.rfind(DISTRHO_OS_SEP));

        if (tmpPath.endsWith(DISTRHO_OS_SEP_STR "Contents"))
        {
            tmpPath.truncate(tmpPath.rfind(DISTRHO_OS_SEP));
            bundlePath = tmpPath;
            d_nextBundlePath = bundlePath.buffer();
        }
        else
        {
            bundlePath = "error";
        }
    }

    // init dummy plugin and set uniqueId
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

        dpf_tuid_class[2] = dpf_tuid_component[2] = dpf_tuid_controller[2]
            = dpf_tuid_processor[2] = dpf_tuid_view[2] = sPlugin->getUniqueId();
    }

    return true;
}

DISTRHO_PLUGIN_EXPORT
bool EXITFNNAME(void);

bool EXITFNNAME(void)
{
    DISTRHO_NAMESPACE::sPlugin = nullptr;
    return true;
}

#undef ENTRYFNNAME
#undef EXITFNNAME

// --------------------------------------------------------------------------------------------------------------------
