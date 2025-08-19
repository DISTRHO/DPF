// Copyright 2025 Filipe Coelho <falktx@falktx.com>
// SPDX-License-Identifier: ISC

#pragma once

#ifdef __cplusplus
extern "C" {
# define MAPI_API extern "C"
#else
# define MAPI_API
#endif

/* Define MAPI_EXPORT for exporting function symbols */
#ifdef _WIN32
# define MAPI_EXPORT MAPI_API __declspec (dllexport)
#else
# define MAPI_EXPORT MAPI_API __attribute__ ((visibility("default")))
#endif

/** Handle used through-out this API. */
typedef void* mapi_handle_t;

/**
   Create an effect.
   @param sample_rate Sample rate in Hz to use for the effect.
   @return A handle for the new effect, or NULL if creation failed.
*/
MAPI_EXPORT
mapi_handle_t mapi_create(unsigned int sample_rate);

/**
   Process an effect.
   @param handle A previously created effect.
   @param ins An array of audio buffers used for audio input.
   @param outs An array of audio buffers used for audio output.
   @param frames How many frames to process.
   @note Input and output buffers might share the same location in memory,
         typically referred to as "in-place processing".
*/
MAPI_EXPORT
void mapi_process(mapi_handle_t filter,
                  const float* const* ins,
                  float** outs,
                  unsigned int frames);

/**
   Set an effect parameter.
   @param handle A previously created effect.
   @param index A known index for this effect.
   @param value A normalized value between 0 and 1, scaled internally by the effect as necessary.
*/
MAPI_EXPORT
void mapi_set_parameter(mapi_handle_t filter, unsigned int index, float value);

/**
   Set an effect state, using strings for both key and value.
   @param handle A previously created effect.
   @param key A known key for this effect, must not be NULL or empty.
   @param value A non-NULL value, allowed to be empty.
*/
MAPI_EXPORT
void mapi_set_state(mapi_handle_t filter, const char* key, const char* value);

/**
   Destroy a previously created effect.
   @param handle A previously created effect.
*/
MAPI_EXPORT
void mapi_destroy(mapi_handle_t filter);

#ifdef __cplusplus
}
#endif
