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

/**
 * base IPlugFrame stuff
 */

struct v3_view_rect {
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;
};

#define V3_VIEW_PLATFORM_TYPE_HWND "HWND"
#define V3_VIEW_PLATFORM_TYPE_NSVIEW "NSView"
#define V3_VIEW_PLATFORM_TYPE_X11 "X11EmbedWindowID"

#if defined(__APPLE__)
# define V3_VIEW_PLATFORM_TYPE_NATIVE V3_VIEW_PLATFORM_TYPE_NSVIEW
#elif defined(_WIN32)
# define V3_VIEW_PLATFORM_TYPE_NATIVE V3_VIEW_PLATFORM_TYPE_HWND
#else
# define V3_VIEW_PLATFORM_TYPE_NATIVE V3_VIEW_PLATFORM_TYPE_X11
#endif

struct v3_plug_frame;

struct v3_plug_view {
	struct v3_funknown;

	V3_API v3_result (*is_platform_type_supported)
		(void *self, const char *platform_type);

	V3_API v3_result (*attached)
		(void *self, void *parent, const char *platform_type);
	V3_API v3_result (*removed)(void *self);

	V3_API v3_result (*on_wheel)(void *self, float distance);
	V3_API v3_result (*on_key_down)
		(void *self, int16_t key_char, int16_t key_code, int16_t modifiers);
	V3_API v3_result (*on_key_up)
		(void *self, int16_t key_char, int16_t key_code, int16_t modifiers);

	V3_API v3_result (*get_size)
		(void *self, struct v3_view_rect *);
	V3_API v3_result (*set_size)
		(void *self, struct v3_view_rect *);

	V3_API v3_result (*on_focus)
		(void *self, v3_bool state);

	V3_API v3_result (*set_frame)
		(void *self, struct v3_plug_frame *);
	V3_API v3_result (*can_resize)(void *self);
	V3_API v3_result (*check_size_constraint)
		(void *self, struct v3_view_rect *);
};

static const v3_tuid v3_plug_view_iid =
	V3_ID(0x5BC32507, 0xD06049EA, 0xA6151B52, 0x2B755B29);

struct v3_plug_frame {
	struct v3_funknown;

	V3_API v3_result (*resize_view)
		(void *self, struct v3_plug_view *, struct v3_view_rect *);
};

static const v3_tuid v3_plug_frame_iid =
	V3_ID(0x367FAF01, 0xAFA94693, 0x8D4DA2A0, 0xED0882A3);

/**
 * steinberg content scaling support
 * (same IID/iface as presonus view scaling)
 */

struct v3_plug_view_content_scale_steinberg {
	struct v3_funknown;

	V3_API v3_result (*set_content_scale_factor)
		(void *self, float factor);
};

static const v3_tuid v3_plug_view_content_scale_steinberg_iid =
	V3_ID(0x65ED9690, 0x8AC44525, 0x8AADEF7A, 0x72EA703F);

/**
 * support for querying the view to find what control is underneath the mouse
 */

struct v3_plug_view_param_finder {
	struct v3_funknown;

	V3_API v3_result (*find_parameter)
		(void *self, int32_t x, int32_t y, v3_param_id *);
};

static const v3_tuid v3_plug_view_param_finder_iid =
	V3_ID(0x0F618302, 0x215D4587, 0xA512073C, 0x77B9D383);
