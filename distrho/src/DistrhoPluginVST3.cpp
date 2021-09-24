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

// TESTING awful idea dont reuse
#include "../extra/Thread.hpp"

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

static const char* tuid2str(const v3_tuid iid)
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

// --------------------------------------------------------------------------------------------------------------------

static void strncpy(char* const dst, const char* const src, const size_t size)
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

static void strncpy_utf16(int16_t* const dst, const char* const src, const size_t size)
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
          fComponentHandlerArg(nullptr)
#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
        , fHostEventOutputHandle(nullptr)
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

                        // TODO find port group name
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

                        // TODO find port group name
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
     * states come first, and then parameters. parameters are simply converted to/from strings and floats.
     * the parameter symbol is used as the "key", so it is possible to reorder them or even remove and add safely.
     * the number of states must remain constant though.
     */
    v3_result setState(v3_bstream* const stream, void* arg)
    {
#if DISTRHO_PLUGIN_WANT_STATE
        // TODO
#endif

        if (const uint32_t paramCount = fPlugin.getParameterCount())
        {
            char buffer[32], orig;
            String key, value;
            v3_result res;
            bool fillingKey = true;

            // temporarily set locale to "C" while converting floats
            const ScopedSafeLocale ssl;

            for (int32_t pos = 0, read;; pos += read)
            {
                std::memset(buffer, '\xff', sizeof(buffer));
                res = stream->read(arg, buffer, sizeof(buffer)-1, &read);
                DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);
                DISTRHO_SAFE_ASSERT_INT_RETURN(read > 0, read, V3_INTERNAL_ERR);

                for (int32_t i = 0; i < read; ++i)
                {
                    if (buffer[i] == '\0' && pos == 0 && i == 0)
                        continue;

                    orig = buffer[read];
                    buffer[read] = '\0';

                    if (fillingKey)
                        key += buffer + i;
                    else
                        value += buffer + i;

                    i += std::strlen(buffer + i);
                    buffer[read] = orig;

                    if (buffer[i] == '\0')
                    {
                        fillingKey = !fillingKey;

                        if (value.isNotEmpty())
                        {
                            // find parameter with this symbol, and set its value
                            for (uint32_t j=0; j<paramCount; ++j)
                            {
                                if (fPlugin.isParameterOutputOrTrigger(j))
                                    continue;
                                if (fPlugin.getParameterSymbol(j) != key)
                                    continue;

                                fPlugin.setParameterValue(j, std::atof(value.buffer()));
                                break;
                            }

                            key.clear();
                            value.clear();
                        }

                        if (buffer[i+1] == '\0')
                            return V3_OK;
                    }
                }

                if (buffer[read] == '\0')
                    return V3_OK;
            }
        }

        return V3_OK;
    }

    v3_result getState(v3_bstream* const stream, void* arg)
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
            return stream->write(arg, &buffer, 1, &ignored);
        }

        String state;

#if DISTRHO_PLUGIN_WANT_FULL_STATE
        /*
        // Update current state
        for (StringMap::const_iterator cit=fStateMap.begin(), cite=fStateMap.end(); cit != cite; ++cit)
        {
            const String& key = cit->first;
            fStateMap[key] = fPlugin.getState(key);
        }
        */
#endif

#if DISTRHO_PLUGIN_WANT_STATE
        /*
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
        */
#endif

        if (paramCount != 0)
        {
            // add another separator
            state += "\xff";

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
        }

        state.replace('\xff', '\0');

        // now saving state, carefully until host written bytes matches full state size
        const char* buffer = state.buffer();
        const int32_t size = static_cast<int32_t>(state.length())+1;
        v3_result res;

        for (int32_t wrtntotal = 0, wrtn; wrtntotal < size; wrtntotal += wrtn)
        {
            wrtn = 0;
            res = stream->write(arg, const_cast<char*>(buffer), size - wrtntotal, &wrtn);

            DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);
            DISTRHO_SAFE_ASSERT_INT_RETURN(wrtn > 0, wrtn, V3_INTERNAL_ERR);
        }

        return V3_OK;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_bstream interface calls (for state support)

    v3_result read(void* /*buffer*/, int32_t /*num_bytes*/, int32_t* /*bytes_read*/)
    {
        // TODO
        return V3_NOT_IMPLEMENTED;
    }

    v3_result write(void* /*buffer*/, int32_t /*num_bytes*/, int32_t* /*bytes_written*/)
    {
        // TODO
        return V3_NOT_IMPLEMENTED;
    }

    v3_result seek(int64_t /*pos*/, int32_t /*seek_mode*/, int64_t* /*result*/)
    {
        // TODO
        return V3_NOT_IMPLEMENTED;
    }

    v3_result tell(int64_t* /*pos*/)
    {
        // TODO
        return V3_NOT_IMPLEMENTED;
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
    };

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

        if (v3_event_list** const eventarg = data->input_events)
        {
            // offset struct by sizeof(v3_funknown), because of differences between C and C++
            v3_event_list* const eventptr = (v3_event_list*)((uint8_t*)(*eventarg)+sizeof(v3_funknown));

            v3_event event;
            for (uint32_t i = 0, count = eventptr->get_event_count(eventarg); i < count; ++i)
            {
                if (eventptr->get_event(eventarg, i, &event) != V3_OK)
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

        // TODO updateParameterOutputsAndTriggers()

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
        return fPlugin.getParameterCount();
    }

    v3_result getParameterInfo(const int32_t index, v3_param_info* const info) const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(index >= 0, V3_INVALID_ARG);

        const uint32_t uindex = static_cast<uint32_t>(index);
        DISTRHO_SAFE_ASSERT_RETURN(uindex < fPlugin.getParameterCount(), V3_INVALID_ARG);

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
        strncpy_utf16(info->title,       fPlugin.getParameterName(index), 128);
        strncpy_utf16(info->short_title, fPlugin.getParameterShortName(index), 128);
        strncpy_utf16(info->units,       fPlugin.getParameterUnit(index), 128);
        return V3_OK;
    }

    v3_result getParameterStringForValue(const v3_param_id index, const double normalised, v3_str_128 output)
    {
        DISTRHO_SAFE_ASSERT_RETURN(index < fPlugin.getParameterCount(), V3_INVALID_ARG);

        snprintf_f32_utf16(output, fPlugin.getParameterRanges(index).getUnnormalizedValue(normalised), 128);
        return V3_OK;
    }

    v3_result getParameterValueForString(const v3_param_id index, int16_t*, double*)
    {
        DISTRHO_SAFE_ASSERT_RETURN(index < fPlugin.getParameterCount(), V3_INVALID_ARG);

        // TODO
        return V3_NOT_IMPLEMENTED;
    };

    double normalisedParameterToPlain(const v3_param_id index, const double normalised)
    {
        DISTRHO_SAFE_ASSERT_RETURN(index < fPlugin.getParameterCount(), V3_INVALID_ARG);

        return fPlugin.getParameterRanges(index).getUnnormalizedValue(normalised);
    };

    double plainParameterToNormalised(const v3_param_id index, const double plain)
    {
        DISTRHO_SAFE_ASSERT_RETURN(index < fPlugin.getParameterCount(), V3_INVALID_ARG);

        return fPlugin.getParameterRanges(index).getNormalizedValue(plain);
    };

    double getParameterNormalized(const v3_param_id index)
    {
        DISTRHO_SAFE_ASSERT_RETURN(index < fPlugin.getParameterCount(), 0.0);

        const float value = fPlugin.getParameterValue(index);
        return fPlugin.getParameterRanges(index).getNormalizedValue(value);
    }

    v3_result setParameterNormalized(const v3_param_id index, const double value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(index < fPlugin.getParameterCount(), V3_INVALID_ARG);

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
        return V3_OK;
    }

    v3_result setComponentHandler(v3_component_handler* const handler, void* const arg) noexcept
    {
        fComponentHandler = handler;
        fComponentHandlerArg = arg;
        return V3_OK;
    }

    // ----------------------------------------------------------------------------------------------------------------

private:
    // Plugin
    PluginExporter fPlugin;

    // VST3 stuff
    v3_component_handler* fComponentHandler;
    void*                 fComponentHandlerArg;

    // Temporary data
#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    MidiEvent fMidiEvents[kMaxMidiEvents];
#endif
#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    v3_event_list** fHostEventOutputHandle;
#endif
#if DISTRHO_PLUGIN_WANT_TIMEPOS
    TimePosition fTimePosition;
#endif

#if DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
    bool requestParameterValueChange(const uint32_t index, const float value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fComponentHandler != nullptr, false);
        DISTRHO_SAFE_ASSERT_RETURN(fComponentHandlerArg != nullptr, false);

        if (fComponentHandler->begin_edit(fComponentHandlerArg, index) != V3_OK)
            return false;

        const double normalized = fPlugin.getParameterRanges(index).getNormalizedValue(value);
        const bool ret = fComponentHandler->perform_edit(fComponentHandlerArg, index, normalized) == V3_OK;

        fComponentHandler->end_edit(fComponentHandlerArg, index);
        return ret;
    }

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

        // offset struct by sizeof(v3_funknown), because of differences between C and C++
        v3_event_list* const hostptr = (v3_event_list*)((uint8_t*)(*fHostEventOutputHandle)+sizeof(v3_funknown));

        return hostptr->add_event(fHostEventOutputHandle, &event) == V3_OK;
    }

    static bool writeMidiCallback(void* ptr, const MidiEvent& midiEvent)
    {
        return ((PluginVst3*)ptr)->writeMidi(midiEvent);
    }
#endif

};

#if DISTRHO_PLUGIN_HAS_UI
// --------------------------------------------------------------------------------------------------------------------
// dpf_plugin_view_create (called from DSP side)

v3_funknown** dpf_plugin_view_create(void* instancePointer, double sampleRate);
#endif

// --------------------------------------------------------------------------------------------------------------------
// dpf_edit_controller

struct v3_edit_controller_cpp : v3_funknown {
    v3_plugin_base base;
    v3_edit_controller controller;
};

struct dpf_edit_controller : v3_edit_controller_cpp {
    std::atomic<int> refcounter;
    ScopedPointer<dpf_edit_controller>* self;
    ScopedPointer<PluginVst3>& vst3;
    bool initialized;

    dpf_edit_controller(ScopedPointer<dpf_edit_controller>* const s, ScopedPointer<PluginVst3>& v)
        : refcounter(1),
          self(s),
          vst3(v),
          initialized(false)
    {
        static const uint8_t* kSupportedInterfaces[] = {
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

        ref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_edit_controller::ref                        => %p", self);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, 0);

            return ++controller->refcounter;
        };

        unref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_edit_controller::unref                      => %p", self);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, 0);

            if (const int refcounter = --controller->refcounter)
                return refcounter;

            *(dpf_edit_controller**)self = nullptr;
            *controller->self = nullptr;
            return 0;
        };

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
            // offset struct by sizeof(v3_funknown), because of differences between C and C++
            v3_bstream* const streamptr
                = stream != nullptr ? (v3_bstream*)((uint8_t*)stream+sizeof(v3_funknown))
                                    : nullptr;

            return vst3->setComponentState(streamptr, stream);
#endif
            return V3_NOT_IMPLEMENTED;
        };

        controller.set_state = []V3_API(void* self, v3_bstream* stream) -> v3_result
        {
            d_stdout("dpf_edit_controller::set_state                  => %p %p", self, stream);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = controller->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

#if 0
            // offset struct by sizeof(v3_funknown), because of differences between C and C++
            v3_bstream* const streamptr
                = stream != nullptr ? (v3_bstream*)((uint8_t*)stream+sizeof(v3_funknown))
                                    : nullptr;

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
            // offset struct by sizeof(v3_funknown), because of differences between C and C++
            v3_bstream* const streamptr
                = stream != nullptr ? (v3_bstream*)((uint8_t*)stream+sizeof(v3_funknown))
                                    : nullptr;

            return vst3->getState(stream);
#endif
            return V3_NOT_IMPLEMENTED;
        };

        controller.get_parameter_count = []V3_API(void* self) -> int32_t
        {
            d_stdout("dpf_edit_controller::get_parameter_count        => %p", self);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = controller->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return vst3->getParameterCount();
        };

        controller.get_parameter_info = []V3_API(void* self, int32_t param_idx, v3_param_info* param_info) -> v3_result
        {
            d_stdout("dpf_edit_controller::get_parameter_info         => %p %i", self, param_idx);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = controller->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return vst3->getParameterInfo(param_idx, param_info);
        };

        controller.get_parameter_string_for_value = []V3_API(void* self, v3_param_id index, double normalised, v3_str_128 output) -> v3_result
        {
            d_stdout("dpf_edit_controller::get_parameter_string_for_value => %p %u %f %p", self, index, normalised, output);
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
            d_stdout("dpf_edit_controller::plain_parameter_to_normalised  => %p %f", self, plain);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = controller->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return vst3->plainParameterToNormalised(index, plain);
        };

        controller.get_parameter_normalised = []V3_API(void* self, v3_param_id index) -> double
        {
            d_stdout("dpf_edit_controller::get_parameter_normalised       => %p", self);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, 0.0);

            PluginVst3* const vst3 = controller->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, 0.0);

            return vst3->getParameterNormalized(index);
        };

        controller.set_parameter_normalised = []V3_API(void* self, v3_param_id index, double normalised) -> v3_result
        {
            d_stdout("dpf_edit_controller::set_parameter_normalised       => %p %f", self, normalised);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = controller->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

//             if (dpf_plugin_view* const view = controller->view)
//             {
//                 if (UIVst3* const uivst3 = view->uivst3)
//                 {
//                     uivst3->setParameterValueFromDSP(index, vst3->normalisedParameterToPlain(index, normalised));
//                 }
//             }

            return vst3->setParameterNormalized(index, normalised);
        };

        controller.set_component_handler = []V3_API(void* self, v3_component_handler** handler) -> v3_result
        {
            d_stdout("dpf_edit_controller::set_component_handler      => %p %p", self, handler);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = controller->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

//             v3_component_handler_cpp** const cpphandler = (v3_component_handler_cpp**)handler;
//
//             controller->handler = cpphandler;
//
//             if (controller->view != nullptr)
//             {
//                 controller->view->handler = cpphandler;
//
//                 if (UIVst3* const uivst3 = controller->view->uivst3)
//                     uivst3->setHandler(cpphandler);
//             }

            // offset struct by sizeof(v3_funknown), because of differences between C and C++
            v3_component_handler* const handlerptr
                = handler != nullptr ? (v3_component_handler*)((uint8_t*)*(handler)+sizeof(v3_funknown))
                                     : nullptr;

            return vst3->setComponentHandler(handlerptr, handler);
        };

        controller.create_view = []V3_API(void* self, const char* name) -> v3_plugin_view**
        {
            d_stdout("dpf_edit_controller::create_view                => %p %s", self, name);
            dpf_edit_controller* const controller = *(dpf_edit_controller**)self;
            DISTRHO_SAFE_ASSERT_RETURN(controller != nullptr, nullptr);

            PluginVst3* const vst3 = controller->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, nullptr);

#if DISTRHO_PLUGIN_HAS_UI
            return (v3_plugin_view**)dpf_plugin_view_create(vst3->getInstancePointer(), vst3->getSampleRate());
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
    std::atomic<int> refcounter;
    ScopedPointer<dpf_audio_processor>* self;
    ScopedPointer<PluginVst3>& vst3;

    dpf_audio_processor(ScopedPointer<dpf_audio_processor>* const s, ScopedPointer<PluginVst3>& v)
        : refcounter(1),
          self(s),
          vst3(v)
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

        ref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_audio_processor::ref                     => %p", self);
            dpf_audio_processor* const processor = *(dpf_audio_processor**)self;
            DISTRHO_SAFE_ASSERT_RETURN(processor != nullptr, 0);

            return ++processor->refcounter;
        };

        unref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_audio_processor::unref                   => %p", self);
            dpf_audio_processor* const processor = *(dpf_audio_processor**)self;
            DISTRHO_SAFE_ASSERT_RETURN(processor != nullptr, 0);

            if (const int refcounter = --processor->refcounter)
                return refcounter;

            *(dpf_audio_processor**)self = nullptr;
            *processor->self = nullptr;
            return 0;
        };

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
// dpf_state_stream

struct v3_bstream_cpp : v3_funknown {
    v3_bstream stream;
};

struct dpf_state_stream : v3_bstream_cpp {
    ScopedPointer<PluginVst3>& vst3;

    dpf_state_stream(ScopedPointer<PluginVst3>& v)
        : vst3(v)
    {
        static const uint8_t* kSupportedInterfaces[] = {
            v3_funknown_iid,
            v3_bstream_iid
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            d_stdout("dpf_factory::query_interface      => %p %s %p", self, tuid2str(iid), iface);
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

        // TODO
        ref = []V3_API(void*) -> uint32_t { return 1; };
        unref = []V3_API(void*) -> uint32_t { return 0; };

        // ------------------------------------------------------------------------------------------------------------
        // v3_bstream

        stream.read = []V3_API(void* self, void* buffer, int32_t num_bytes, int32_t* bytes_read) -> v3_result
        {
            d_stdout("dpf_state_stream::read  => %p %p %i %p", self, buffer, num_bytes, bytes_read);
            dpf_state_stream* const stream = *(dpf_state_stream**)self;
            DISTRHO_SAFE_ASSERT_RETURN(stream != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = stream->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return V3_NOT_IMPLEMENTED;
        };

        stream.write = []V3_API(void* self, void* buffer, int32_t num_bytes, int32_t* bytes_written) -> v3_result
        {
            d_stdout("dpf_state_stream::write => %p %p %i %p", self, buffer, num_bytes, bytes_written);
            dpf_state_stream* const stream = *(dpf_state_stream**)self;
            DISTRHO_SAFE_ASSERT_RETURN(stream != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = stream->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return V3_NOT_IMPLEMENTED;
        };

        stream.seek = []V3_API(void* self, int64_t pos, int32_t seek_mode, int64_t* result) -> v3_result
        {
            d_stdout("dpf_state_stream::seek  => %p %lu %i %p", self, pos, seek_mode, result);
            dpf_state_stream* const stream = *(dpf_state_stream**)self;
            DISTRHO_SAFE_ASSERT_RETURN(stream != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = stream->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return V3_NOT_IMPLEMENTED;
        };

        stream.tell = []V3_API(void* self, int64_t* pos) -> v3_result
        {
            d_stdout("dpf_state_stream::tell  => %p %p", self, pos);
            dpf_state_stream* const stream = *(dpf_state_stream**)self;
            DISTRHO_SAFE_ASSERT_RETURN(stream != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = stream->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            return V3_NOT_IMPLEMENTED;
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
    ScopedPointer<dpf_edit_controller> controller;
    ScopedPointer<dpf_state_stream> stream;
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
                    component->processor = new dpf_audio_processor(&component->processor, component->vst3);
                *iface = &component->processor;
                return V3_OK;
            }

            if (v3_tuid_match(v3_edit_controller_iid, iid))
            {
                if (component->controller == nullptr)
                    component->controller = new dpf_edit_controller(&component->controller, component->vst3);
                *iface = &component->controller;
                return V3_OK;
            }

            return V3_NO_INTERFACE;
        };

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

            if (const int refcounter = --component->refcounter)
                return refcounter;

            *(dpf_component**)self = nullptr;
            *component->self = nullptr;
            return 0;
        };

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

            // offset struct by sizeof(v3_funknown), because of differences between C and C++
            v3_bstream* const streamptr
                = stream != nullptr ? (v3_bstream*)((uint8_t*)*(stream)+sizeof(v3_funknown))
                                    : nullptr;

            return vst3->setState(streamptr, stream);
        };

        comp.get_state = []V3_API(void* self, v3_bstream** stream) -> v3_result
        {
            d_stdout("dpf_component::get_state               => %p %p", self, stream);
            dpf_component* const component = *(dpf_component**)self;
            DISTRHO_SAFE_ASSERT_RETURN(component != nullptr, V3_NOT_INITIALISED);

            PluginVst3* const vst3 = component->vst3;
            DISTRHO_SAFE_ASSERT_RETURN(vst3 != nullptr, V3_NOT_INITIALISED);

            // offset struct by sizeof(v3_funknown), because of differences between C and C++
            v3_bstream* const streamptr
                = stream != nullptr ? (v3_bstream*)((uint8_t*)*(stream)+sizeof(v3_funknown))
                                    : nullptr;

            return vst3->getState(streamptr, stream);
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
            DISTRHO_NAMESPACE::snprintf_u32(info->version, gPluginInfo.getVersion(), ARRAY_SIZE(info->version));
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
            DISTRHO_NAMESPACE::snprintf_u32_utf16(info->version, gPluginInfo.getVersion(), ARRAY_SIZE(info->version));
            DISTRHO_NAMESPACE::strncpy_utf16(info->sdk_version, "Travesty", ARRAY_SIZE(info->sdk_version)); // TESTING use "VST 3.7" ?
            return V3_OK;
        };

        v3.set_host_context = []V3_API (void* self, struct v3_funknown* host) -> v3_result
        {
            d_stdout("dpf_factory::set_host_context     => %p %p", self, host);
            return V3_NOT_IMPLEMENTED;
        };
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
