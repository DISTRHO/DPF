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

#include <list>

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

// -----------------------------------------------------------------------

class PluginVst3
{
public:
    PluginVst3()
        : fPlugin(this, writeMidiCallback, requestParameterValueChangeCallback)
    {
    }

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
// Dummy plugin to get data from

static ScopedPointer<PluginExporter> gPluginInfo;

// --------------------------------------------------------------------------------------------------------------------
// dpf_plugin_view

struct v3_plugin_view_cpp : v3_funknown {
    v3_plugin_view view;
};

struct dpf_plugin_view : v3_plugin_view_cpp {
    dpf_plugin_view()
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
            return V3_OK;
        };

        view.attached = []V3_API(void* self, void* parent, const char* platform_type) -> v3_result
        {
            d_stdout("dpf_plugin_view::attached                   => %s | %p %p %s", __PRETTY_FUNCTION__ + 41, self, parent, platform_type);
            return V3_OK;
        };

        view.removed = []V3_API(void* self) -> v3_result
        {
            d_stdout("dpf_plugin_view::removed                    => %s | %p", __PRETTY_FUNCTION__ + 41, self);
            return V3_OK;
        };

        view.on_wheel = []V3_API(void* self, float distance) -> v3_result
        {
            d_stdout("dpf_plugin_view::on_wheel                   => %s | %p %f", __PRETTY_FUNCTION__ + 41, self, distance);
            return V3_OK;
        };

        view.on_key_down = []V3_API(void* self, int16_t key_char, int16_t key_code, int16_t modifiers) -> v3_result
        {
            d_stdout("dpf_plugin_view::on_key_down                => %s | %p %i %i %i", __PRETTY_FUNCTION__ + 41, self, key_char, key_code, modifiers);
            return V3_OK;
        };

        view.on_key_up = []V3_API(void* self, int16_t key_char, int16_t key_code, int16_t modifiers) -> v3_result
        {
            d_stdout("dpf_plugin_view::on_key_up                  => %s | %p %i %i %i", __PRETTY_FUNCTION__ + 41, self, key_char, key_code, modifiers);
            return V3_OK;
        };

        view.get_size = []V3_API(void* self, v3_view_rect*) -> v3_result
        {
            d_stdout("dpf_plugin_view::get_size                   => %s | %p", __PRETTY_FUNCTION__ + 41, self);
            return V3_OK;
        };

        view.set_size = []V3_API(void* self, v3_view_rect*) -> v3_result
        {
            d_stdout("dpf_plugin_view::set_size                   => %s | %p", __PRETTY_FUNCTION__ + 41, self);
            return V3_OK;
        };

        view.on_focus = []V3_API(void* self, v3_bool state) -> v3_result
        {
            d_stdout("dpf_plugin_view::on_focus                   => %s | %p %u", __PRETTY_FUNCTION__ + 41, self, state);
            return V3_OK;
        };

        view.set_frame = []V3_API(void* self, v3_plugin_frame*) -> v3_result
        {
            d_stdout("dpf_plugin_view::set_frame                  => %s | %p", __PRETTY_FUNCTION__ + 41, self);
            return V3_OK;
        };

        view.can_resize = []V3_API(void* self) -> v3_result
        {
            d_stdout("dpf_plugin_view::can_resize                 => %s | %p", __PRETTY_FUNCTION__ + 41, self);
            return V3_OK;
        };

        view.check_size_constraint = []V3_API(void* self, v3_view_rect*) -> v3_result
        {
            d_stdout("dpf_plugin_view::check_size_constraint      => %s | %p", __PRETTY_FUNCTION__ + 41, self);
            return V3_OK;
        };
    }
};

// --------------------------------------------------------------------------------------------------------------------
// dpf_edit_controller

struct v3_edit_controller_cpp : v3_funknown {
    v3_plugin_base base;
    v3_edit_controller controller;
};

struct dpf_edit_controller : v3_edit_controller_cpp {
    dpf_edit_controller()
    {
        static const uint8_t* kSupportedFactories[] = {
            v3_funknown_iid,
            v3_edit_controller_iid
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            d_stdout("dpf_edit_controller::query_interface            => %s | %p %s %p", __PRETTY_FUNCTION__ + 53, self, tuid2str(iid), iface);
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
            d_stdout("dpf_edit_controller::ref                        => %s | %p", __PRETTY_FUNCTION__ + 53, self);
            return 1;
        };

        unref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_edit_controller::unref                      => %s | %p", __PRETTY_FUNCTION__ + 53, self);
            return 0;
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_plugin_base

        base.initialise = []V3_API(void* self, struct v3_plugin_base::v3_funknown *context) -> v3_result
        {
            d_stdout("dpf_edit_controller::initialise                 => %s | %p %p", __PRETTY_FUNCTION__ + 53, self, context);
            return V3_OK;
        };

        base.terminate = []V3_API(void* self) -> v3_result
        {
            d_stdout("dpf_edit_controller::terminate                  => %s | %p", __PRETTY_FUNCTION__ + 53, self);
            return V3_OK;
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_edit_controller

        controller.set_component_state = []V3_API(void* self, v3_bstream*) -> v3_result
        {
            d_stdout("dpf_edit_controller::set_component_state        => %s | %p", __PRETTY_FUNCTION__ + 53, self);
            return V3_OK;
        };

        controller.set_state = []V3_API(void* self, v3_bstream*) -> v3_result
        {
            d_stdout("dpf_edit_controller::set_state                  => %s | %p", __PRETTY_FUNCTION__ + 53, self);
            return V3_OK;
        };

        controller.get_state = []V3_API(void* self, v3_bstream*) -> v3_result
        {
            d_stdout("dpf_edit_controller::get_state                  => %s | %p", __PRETTY_FUNCTION__ + 53, self);
            return V3_OK;
        };

        controller.get_parameter_count = []V3_API(void* self) -> int32_t
        {
            d_stdout("dpf_edit_controller::get_parameter_count        => %s | %p", __PRETTY_FUNCTION__ + 53, self);
            return gPluginInfo->getParameterCount();
        };

        controller.get_parameter_info = []V3_API(void* self, int32_t param_idx, v3_param_info* param_info) -> v3_result
        {
            d_stdout("dpf_edit_controller::get_parameter_info         => %s | %p %i", __PRETTY_FUNCTION__ + 53, self, param_idx);

            // set up flags
            int32_t flags = 0;

            const auto desig = gPluginInfo->getParameterDesignation(param_idx);
            const auto hints = gPluginInfo->getParameterHints(param_idx);

            const ParameterRanges& ranges(gPluginInfo->getParameterRanges(param_idx));

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

            std::memset(param_info, 0, sizeof(v3_param_info));
            param_info->param_id = param_idx;
            param_info->flags = flags;
            param_info->step_count = step_count;
            param_info->default_normalised_value = ranges.getNormalizedValue(ranges.def);
            // int32_t unit_id;
            strncpy_16from8(param_info->title,       gPluginInfo->getParameterName(param_idx), 128);
            strncpy_16from8(param_info->short_title, gPluginInfo->getParameterShortName(param_idx), 128);
            strncpy_16from8(param_info->units,       gPluginInfo->getParameterUnit(param_idx), 128);

            /*
            v3_str_128 title;
            v3_str_128 short_title;
            v3_str_128 units;
            */

            return V3_OK;
        };

        controller.get_param_string_for_value = []V3_API(void* self, v3_param_id index, double normalised, v3_str_128 output) -> v3_result
        {
            d_stdout("dpf_edit_controller::get_param_string_for_value => %s | %p %f", __PRETTY_FUNCTION__ + 53, self, normalised);

            const ParameterRanges& ranges(gPluginInfo->getParameterRanges(index));
            const float realvalue = ranges.getUnnormalizedValue(normalised);
            char buf[24];
            sprintf(buf, "%f", realvalue);
            strncpy_16from8(output, buf, 128);
            return V3_OK;
        };

        controller.get_param_value_for_string = []V3_API(void* self, v3_param_id, int16_t* input, double* output) -> v3_result
        {
            d_stdout("dpf_edit_controller::get_param_value_for_string => %s | %p %p %p", __PRETTY_FUNCTION__ + 53, self, input, output);
            return V3_NOT_IMPLEMENTED;
        };

        controller.normalised_param_to_plain = []V3_API(void* self, v3_param_id, double normalised) -> double
        {
            d_stdout("dpf_edit_controller::normalised_param_to_plain  => %s | %p %f", __PRETTY_FUNCTION__ + 53, self, normalised);
            return normalised;
        };

        controller.plain_param_to_normalised = []V3_API(void* self, v3_param_id, double normalised) -> double
        {
            d_stdout("dpf_edit_controller::plain_param_to_normalised  => %s | %p %f", __PRETTY_FUNCTION__ + 53, self, normalised);
            return normalised;
        };

        controller.get_param_normalised = []V3_API(void* self, v3_param_id) -> double
        {
            d_stdout("dpf_edit_controller::get_param_normalised       => %s | %p", __PRETTY_FUNCTION__ + 53, self);
            return 0.0;
        };

        controller.set_param_normalised = []V3_API(void* self, v3_param_id, double normalised) -> v3_result
        {
            d_stdout("dpf_edit_controller::set_param_normalised       => %s | %p %f", __PRETTY_FUNCTION__ + 53, self, normalised);
            return V3_OK;
        };

        controller.set_component_handler = []V3_API(void* self, v3_component_handler**) -> v3_result
        {
            d_stdout("dpf_edit_controller::set_component_handler      => %s | %p", __PRETTY_FUNCTION__ + 53, self);
            return V3_OK;
        };

        controller.create_view = []V3_API(void* self, const char* name) -> v3_plug_view**
        {
            d_stdout("dpf_edit_controller::create_view                => %s | %p %s", __PRETTY_FUNCTION__ + 53, self, name);
            return nullptr;
        };
    }
};

// --------------------------------------------------------------------------------------------------------------------
// dpf_audio_processor

struct v3_audio_processor_cpp : v3_funknown {
    v3_audio_processor processor;
};

struct dpf_audio_processor : v3_audio_processor_cpp {
    dpf_audio_processor()
    {
        static const uint8_t* kSupportedFactories[] = {
            v3_funknown_iid,
            v3_audio_processor_iid
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            d_stdout("dpf_audio_processor::query_interface         => %s | %p %s %p", __PRETTY_FUNCTION__ + 53, self, tuid2str(iid), iface);
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
            d_stdout("dpf_audio_processor::ref                     => %s | %p", __PRETTY_FUNCTION__ + 53, self);
            return 1;
        };

        unref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_audio_processor::unref                   => %s | %p", __PRETTY_FUNCTION__ + 53, self);
            return 0;
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_audio_processor

        processor.set_bus_arrangements = []V3_API(void* self,
                                                  v3_speaker_arrangement* inputs, int32_t num_inputs,
                                                  v3_speaker_arrangement* outputs, int32_t num_outputs) -> v3_result
        {
            d_stdout("dpf_audio_processor::set_bus_arrangements    => %s | %p %p %i %p %i", __PRETTY_FUNCTION__ + 53, self, inputs, num_inputs, outputs, num_outputs);
            return V3_OK;
        };

        processor.get_bus_arrangement = []V3_API(void* self, int32_t bus_direction,
                                                 int32_t idx, v3_speaker_arrangement*) -> v3_result
        {
            d_stdout("dpf_audio_processor::get_bus_arrangement     => %s | %p %i %i", __PRETTY_FUNCTION__ + 53, self, bus_direction, idx);
            return V3_OK;
        };

        processor.can_process_sample_size = []V3_API(void* self, int32_t symbolic_sample_size) -> v3_result
        {
            d_stdout("dpf_audio_processor::can_process_sample_size => %s | %p %i", __PRETTY_FUNCTION__ + 53, self, symbolic_sample_size);
            return V3_OK;
        };

        processor.get_latency_samples = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_audio_processor::get_latency_samples     => %s | %p", __PRETTY_FUNCTION__ + 53, self);
            return 0;
        };

        processor.setup_processing = []V3_API(void* self, v3_process_setup*) -> v3_result
        {
            d_stdout("dpf_audio_processor::setup_processing        => %s | %p", __PRETTY_FUNCTION__ + 53, self);
            return V3_OK;
        };

        processor.set_processing = []V3_API(void* self, v3_bool state) -> v3_result
        {
            d_stdout("dpf_audio_processor::set_processing          => %s | %p %u", __PRETTY_FUNCTION__ + 53, self, state);
            return V3_OK;
        };

        processor.process = []V3_API(void* self, v3_process_data*) -> v3_result
        {
            d_stdout("dpf_audio_processor::process                 => %s | %p", __PRETTY_FUNCTION__ + 53, self);
            return V3_OK;
        };

        processor.get_tail_samples = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_audio_processor::get_tail_samples        => %s | %p", __PRETTY_FUNCTION__ + 53, self);
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
    dpf_audio_processor* processor = nullptr;
    dpf_edit_controller* controller = nullptr;

    dpf_component()
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
                    component->processor = new dpf_audio_processor();
                *iface = (v3_funknown*)&component->processor;
                return V3_OK;
            }

            if (v3_tuid_match(v3_edit_controller_iid, iid))
            {
                dpf_component* const component = *(dpf_component**)self;
                if (component->controller == nullptr)
                    component->controller = new dpf_edit_controller();
                *iface = (v3_funknown*)&component->controller;
                return V3_OK;
            }

            return V3_NO_INTERFACE;
        };

        // we only support 1 plugin per binary, so don't have to care here
        ref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_component::ref                     => %s | %p", __PRETTY_FUNCTION__ + 41, self);
            return 1;
        };

        unref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_component::unref                   => %s | %p", __PRETTY_FUNCTION__ + 41, self);
            return 0;
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_plugin_base

        base.initialise = []V3_API(void* self, struct v3_plugin_base::v3_funknown *context) -> v3_result
        {
            d_stdout("dpf_component::initialise              => %s | %p %p", __PRETTY_FUNCTION__ + 41, self, context);
            return V3_OK;
        };

        base.terminate = []V3_API(void* self) -> v3_result
        {
            d_stdout("dpf_component::terminate               => %s | %p", __PRETTY_FUNCTION__ + 41, self);
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
            return V3_INTERNAL_ERR;
        };

        comp.get_bus_count = []V3_API(void* self, int32_t media_type, int32_t bus_direction) -> int32_t
        {
            d_stdout("dpf_component::get_bus_count           => %s | %p %i %i", __PRETTY_FUNCTION__ + 41, self, media_type, bus_direction);
            return 0;
        };

        comp.get_bus_info = []V3_API(void* self, int32_t media_type, int32_t bus_direction,
                                     int32_t bus_idx, v3_bus_info* bus_info) -> v3_result
        {
            d_stdout("dpf_component::get_bus_info            => %s | %p %i %i %i %p", __PRETTY_FUNCTION__ + 41, self, media_type, bus_direction, bus_idx, bus_info);
            return V3_INTERNAL_ERR;
        };

        comp.get_routing_info = []V3_API(void* self, v3_routing_info* input, v3_routing_info* output) -> v3_result
        {
            d_stdout("dpf_component::get_routing_info        => %s | %p %p %p", __PRETTY_FUNCTION__ + 41, self, input, output);
            return V3_INTERNAL_ERR;
        };

        comp.activate_bus = []V3_API(void* self, int32_t media_type, int32_t bus_direction,
                                     int32_t bus_idx, v3_bool state) -> v3_result
        {
            d_stdout("dpf_component::activate_bus            => %s | %p %i %i %i %u", __PRETTY_FUNCTION__ + 41, self, media_type, bus_direction, bus_idx, state);
            return V3_INTERNAL_ERR;
        };

        comp.set_active = []V3_API(void* self, v3_bool state) -> v3_result
        {
            d_stdout("dpf_component::set_active              => %s | %p %u", __PRETTY_FUNCTION__ + 41, self, state);
            return V3_INTERNAL_ERR;
        };

        comp.set_state = []V3_API(void* self, v3_bstream**) -> v3_result
        {
            d_stdout("dpf_component::set_state               => %s | %p", __PRETTY_FUNCTION__ + 41, self);
            return V3_INTERNAL_ERR;
        };

        comp.get_state = []V3_API(void* self, v3_bstream**) -> v3_result
        {
            d_stdout("dpf_component::get_state               => %s | %p", __PRETTY_FUNCTION__ + 41, self);
            return V3_INTERNAL_ERR;
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
    std::list<dpf_component*> components;

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
