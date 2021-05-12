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

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/**
 * various types
 */

typedef int32_t v3_result;

typedef int16_t v3_str_128[128];
typedef uint8_t v3_bool;

typedef uint32_t v3_param_id;


/**
 * low-level ABI nonsense
 */

typedef uint8_t v3_tuid[16];

inline static bool
v3_tuid_match(const v3_tuid a, const v3_tuid b)
{
	return memcmp(a, b, sizeof(v3_tuid)) == 0;
}

#if defined(_WIN32)
# define V3_COM_COMPAT 1
# define V3_API __stdcall
#else
# define V3_COM_COMPAT 0
# define V3_API
#endif

#if V3_COM_COMPAT
enum {
	V3_NO_INTERFACE		= 0x80004002L,
	V3_OK				= 0,
	V3_TRUE				= 0,
	V3_FALSE			= 1,
	V3_INVALID_ARG		= 0x80070057L,
	V3_NOT_IMPLEMENTED	= 0x80004001L,
	V3_INTERNAL_ERR		= 0x80004005L,
	V3_NOT_INITIALISED	= 0x8000FFFFL,
	V3_NOMEM			= 0x8007000EL
};

# define V3_ID(a, b, c, d) {												\
	((a) & 0x000000FF),														\
	((a) & 0x0000FF00) >>  8,												\
	((a) & 0x00FF0000) >> 16,												\
	((a) & 0xFF000000) >> 24,												\
																			\
	((b) & 0x00FF0000) >> 16,												\
	((b) & 0xFF000000) >> 24,												\
	((b) & 0x000000FF),														\
	((b) & 0x0000FF00) >>  8,												\
																			\
	((c) & 0xFF000000) >> 24,												\
	((c) & 0x00FF0000) >> 16,												\
	((c) & 0x0000FF00) >>  8,												\
	((c) & 0x000000FF),														\
																			\
	((d) & 0xFF000000) >> 24,												\
	((d) & 0x00FF0000) >> 16,												\
	((d) & 0x0000FF00) >>  8,												\
	((d) & 0x000000FF),														\
}

#else // V3_COM_COMPAT
enum {
	V3_NO_INTERFACE	= -1,
	V3_OK,
	V3_TRUE = V3_OK,
	V3_FALSE,
	V3_INVALID_ARG,
	V3_NOT_IMPLEMENTED,
	V3_INTERNAL_ERR,
	V3_NOT_INITIALISED,
	V3_NOMEM
};

# define V3_ID(a, b, c, d) {												\
	((a) & 0xFF000000) >> 24,												\
	((a) & 0x00FF0000) >> 16,												\
	((a) & 0x0000FF00) >>  8,												\
	((a) & 0x000000FF),														\
																			\
	((b) & 0xFF000000) >> 24,												\
	((b) & 0x00FF0000) >> 16,												\
	((b) & 0x0000FF00) >>  8,												\
	((b) & 0x000000FF),														\
																			\
	((c) & 0xFF000000) >> 24,												\
	((c) & 0x00FF0000) >> 16,												\
	((c) & 0x0000FF00) >>  8,												\
	((c) & 0x000000FF),														\
																			\
	((d) & 0xFF000000) >> 24,												\
	((d) & 0x00FF0000) >> 16,												\
	((d) & 0x0000FF00) >>  8,												\
	((d) & 0x000000FF),														\
}
#endif // V3_COM_COMPAT

/**
 * funknown
 */

struct v3_funknown {
	V3_API v3_result (*query_interface)
		(void *self, const v3_tuid iid, void **obj);

	V3_API uint32_t (*ref)(void *self);
	V3_API uint32_t (*unref)(void *self);
};

static const v3_tuid v3_funknown_iid =
	V3_ID(0x00000000, 0x00000000, 0xC0000000, 0x00000046);

/**
 * plugin base
 */

struct v3_plugin_base {
	struct v3_funknown;

	V3_API v3_result (*initialise)
		(void *self, struct v3_funknown *context);
	V3_API v3_result (*terminate)(void *self);
};

static const v3_tuid v3_plugin_base_iid =
	V3_ID(0x22888DDB, 0x156E45AE, 0x8358B348, 0x08190625);
