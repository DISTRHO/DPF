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
#include "view.h"

#include "align_push.h"

/**
 * component handler
 */

struct v3_component_handler {
	struct v3_funknown;

	V3_API v3_result (*begin_edit)
		(void *self, v3_param_id);
	V3_API v3_result (*perform_edit)
		(void *self, v3_param_id, double value_normalised);
	V3_API v3_result (*end_edit)
		(void *self, v3_param_id);

	V3_API v3_result (*restart_component)
		(void *self, int32_t flags);
};

static const v3_tuid v3_component_handler_iid =
	V3_ID(0x93A0BEA3, 0x0BD045DB, 0x8E890B0C, 0xC1E46AC6);

/**
 * edit controller
 */

enum {
	V3_PARAM_CAN_AUTOMATE   = 1,
	V3_PARAM_READ_ONLY      = 1 << 1,
	V3_PARAM_WRAP_AROUND    = 1 << 2,
	V3_PARAM_IS_LIST        = 1 << 3,
	V3_PARAM_IS_HIDDEN      = 1 << 4,
	V3_PARAM_PROGRAM_CHANGE = 1 << 15,
	V3_PARAM_IS_BYPASS      = 1 << 16
};

struct v3_param_info {
	v3_param_id param_id;

	v3_str_128 title;
	v3_str_128 short_title;
	v3_str_128 units;

	int32_t step_count;

	double default_normalised_value;

	int32_t unit_id;
	int32_t flags;
};

struct v3_edit_controller {
	struct v3_plugin_base;

	V3_API v3_result (*set_component_state)
		(void *self, struct v3_bstream *);
	V3_API v3_result (*set_state)
		(void *self, struct v3_bstream *);
	V3_API v3_result (*get_state)
		(void *self, struct v3_bstream *);

	V3_API int32_t (*get_parameter_count)(void *self);
	V3_API v3_result (*get_param_info)
		(void *self, int32_t param_idx, struct v3_param_info *);

	V3_API v3_result (*get_param_string_for_value)
		(void *self, v3_param_id, double normalised, v3_str_128 output);
	V3_API v3_result (*get_param_value_for_string)
		(void *self, v3_param_id, int16_t *input, double *output);

	V3_API double (*normalised_param_to_plain)
		(void *self, v3_param_id, double normalised);
	V3_API double (*plain_param_to_normalised)
		(void *self, v3_param_id, double normalised);

	V3_API double (*get_param_normalised)(void *self, v3_param_id);
	V3_API v3_result (*set_param_normalised)
		(void *self, v3_param_id, double normalised);

	V3_API v3_result (*set_component_handler)
		(void *self, struct v3_component_handler **);

	V3_API struct v3_plug_view **(*create_view)
		(void *self, const char *name);
};

static const v3_tuid v3_edit_controller_iid =
	V3_ID(0xDCD7BBE3, 0x7742448D, 0xA874AACC, 0x979C759E);

#include "align_pop.h"
