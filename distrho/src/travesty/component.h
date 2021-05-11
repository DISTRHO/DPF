/*
 * travesty, pure C interface to steinberg VST3 SDK
 * Copyright (C) 2021 Filipe Coelho <falktx@falktx.com>
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

#pragma once

#include "base.h"
#include "bstream.h"

#include "align_push.h"

/**
 * buses
 */

enum v3_media_types {
	V3_AUDIO = 0,
	V3_EVENT
};

inline static const char *
v3_media_type_str(int32_t type)
{
	switch (type) {
	case V3_AUDIO: return "V3_AUDIO";
	case V3_EVENT: return "V3_EVENT";
	default: return "[unknown]";
	}
}

enum v3_bus_direction {
	V3_INPUT = 0,
	V3_OUTPUT
};

inline static const char *
v3_bus_direction_str(int32_t d)
{
	switch (d) {
	case V3_INPUT: return "V3_INPUT";
	case V3_OUTPUT: return "V3_OUTPUT";
	default: return "[unknown]";
	}
}

enum v3_bus_types {
	V3_MAIN = 0,
	V3_AUX
};

enum v3_bus_flags {
	V3_DEFAULT_ACTIVE     = 1,
	V3_IS_CONTROL_VOLTAGE = 1 << 1
};

struct v3_bus_info {
	int32_t media_type;
	int32_t direction;
	int32_t channel_count;

	v3_str_128 bus_name;
	int32_t bus_type;
	uint32_t flags;
};

/**
 * component
 */

struct v3_routing_info;

struct v3_component {
	struct v3_plugin_base;

	V3_API v3_result (*get_controller_class_id)
		(void *self, v3_tuid class_id);

	V3_API v3_result (*set_io_mode)
		(void *self, int32_t io_mode);

	V3_API int32_t (*get_bus_count)
		(void *self, int32_t media_type, int32_t bus_direction);
	V3_API v3_result (*get_bus_info)
		(void *self, int32_t media_type, int32_t bus_direction,
		 int32_t bus_idx, struct v3_bus_info *bus_info);
	V3_API v3_result (*get_routing_info)
		(void *self, struct v3_routing_info *input,
		 struct v3_routing_info *output);
	V3_API v3_result (*activate_bus)
		(void *self, int32_t media_type, int32_t bus_direction,
		 int32_t bus_idx, v3_bool state);

	V3_API v3_result (*set_active)
		(void *self, v3_bool state);

	V3_API v3_result (*set_state)
		(void *self, struct v3_bstream **);
	V3_API v3_result (*get_state)
		(void *self, struct v3_bstream **);
};

static const v3_tuid v3_component_iid =
	V3_ID(0xE831FF31, 0xF2D54301, 0x928EBBEE, 0x25697802);

#include "align_pop.h"
