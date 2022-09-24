// This was created from released VST2.x plugins, and is technically under the 2-clause BSD license.
// Depending on which country you are in, Steinberg can do fuck all about this. Notable countries for
// this are most members of the United States of America, the entirety of Europe, Japan, and Russia.
// Consult a lawyer if you don't know if clean room reverse engineering is allowed in your country.

// See README.md for all information.

// Known additional information:
// - Function call standard seems to be stdcall.
// - Everything is aligned to 8 bytes.

// VST Versioning:
// - Base-10, thus can't store many version numbers.
// - Always four components, with the major one being able to store the most numbers.
// - Format is A...ABCD, so 1.2.3.4 would turn into 1234.

#pragma once
#ifndef VST2SDK_VST_H
#define VST2SDK_VST_H

#define VST_FUNCTION_INTERFACE __cdecl
#define VST_ALIGNMENT 8
#define VST_MAGICNUMBER 'VstP'

// Common VST buffer lengths:
// 8: OpCodes(GetLabel, GetName, GetValue)
#define VST_BUFFER_8 8
// 16:
#define VST_BUFFER_16 16
// 24: OpCodes?
#define VST_BUFFER_24 24
// 32: OpCodes(EffectName)
#define VST_BUFFER_32 32
#define VST_EFFECT_BUFFER_SIZE 32
// 64: OpCodes(ProductName, VendorName)
#define VST_BUFFER_64 64
#define VST_VENDOR_BUFFER_SIZE VST_BUFFER_64
#define VST_PRODUCT_BUFFER_SIZE VST_BUFFER_64
#define VST_NAME_BUFFER_SIZE VST_BUFFER_64
// 100:
#define VST_BUFFER_100 100

#define VST_MAX_CHANNELS 32 // Couldn't find any audio editing software which would attempt to add more channels.

#pragma pack(push, VST_ALIGNMENT)

#ifdef __cplusplus
#ifdef DISTRHO_PROPER_CPP11_SUPPORT
#include <cinttypes>
#else
#include <inttypes.h>
#endif
extern "C" {
#else
#include <inttypes.h>
#endif

/*******************************************************************************
|* Enumeration
|*/
enum VST_VERSION {
	VST_VERSION_1       = 0,    // Anything before 2.0, used by official plug-ins.
	VST_VERSION_1_0_0_0 = 1000, // 1.0, used by some third-party plug-ins.
	VST_VERSION_1_1_0_0 = 1100, // 1.1, used by some third-party plug-ins.
	VST_VERSION_2       = 2,    // 2.0, used by official plug-ins.
	VST_VERSION_2_0_0_0 = 2000, // 2.0, used by some third-party plug-ins.
	VST_VERSION_2_1_0_0 = 2100, // 2.1
	VST_VERSION_2_2_0_0 = 2200, // 2.2
	VST_VERSION_2_3_0_0 = 2300, // 2.3
	VST_VERSION_2_4_0_0 = 2400, // 2.4

	// Pad to force 32-bit number.
	_VST_VERSION_PAD = 0xFFFFFFFFul,
};

enum VST_CATEGORY {
	VST_CATEGORY_UNCATEGORIZED = 0x00,
	VST_CATEGORY_01            = 0x01,
	VST_CATEGORY_02            = 0x02,
	VST_CATEGORY_03            = 0x03,
	VST_CATEGORY_04            = 0x04,
	VST_CATEGORY_05            = 0x05,
	VST_CATEGORY_06            = 0x06,
	VST_CATEGORY_07            = 0x07,
	VST_CATEGORY_RESTORATION   = 0x08, // Denoising and similar effects.
	VST_CATEGORY_09            = 0x09,
	VST_CATEGORY_CONTAINER     = 0x0A, // Plugin contains more than one Plugin.
	VST_CATEGORY_0B            = 0x0B,
	VST_CATEGORY_MAX, // Not part of specification, marks maximum category.

	// Pad to force 32-bit number.
	_VST_CATEGORY_PAD = 0xFFFFFFFFul,
};

enum VST_EFFECT_OPCODE {
	/* Create/Initialize the effect (if it has not been created already).
	 * 
	 * @return Always 0.
	 */
	VST_EFFECT_OPCODE_00         = 0x00,
	VST_EFFECT_OPCODE_CREATE     = 0x00,
	VST_EFFECT_OPCODE_INITIALIZE = 0x00,

	/* Destroy the effect (if there is any) and free its memory.
	 *
	 * This should destroy the actual object created by VST_ENTRYPOINT.
	 * 
	 * @return Always 0.
	 */
	VST_EFFECT_OPCODE_01      = 0x01,
	VST_EFFECT_OPCODE_DESTROY = 0x01,

	/* Set Program
	 *
	 *
	 */
	VST_EFFECT_OPCODE_02 = 0x02,

	/* Get Program
	 *
	 *
	 */
	VST_EFFECT_OPCODE_03 = 0x03,

	/* Set Program Name
	 *
	 *
	 */
	VST_EFFECT_OPCODE_04 = 0x04,

	/* Get Program Name
	 *
	 * "Returns 0. If ptr is valid, sets the first byte of ptr to 0 then returns 0."
	 */
	VST_EFFECT_OPCODE_05 = 0x05,

	/* Get the value? label for the parameter.
	 * 
	 * @param p_int1 Parameter index.
	 * @param p_ptr 'char[8]'
	 * @return 0 on success, 1 on failure.
	 */
	VST_EFFECT_OPCODE_06             = 0x06,
	VST_EFFECT_OPCODE_PARAM_GETLABEL = 0x06,

	/* Get the string value for the parameter.
	 * 
	 * @param p_int1 Parameter index.
	 * @param p_ptr 'char[8]'
	 * @return 0 on success, 1 on failure.
	 */
	VST_EFFECT_OPCODE_07             = 0x07,
	VST_EFFECT_OPCODE_PARAM_GETVALUE = 0x07,

	/* Get the name for the parameter.
	 * 
	 * @param p_int1 Parameter index.
	 * @param p_ptr 'char[8]'
	 * @return 0 on success, 1 on failure.
	 */
	VST_EFFECT_OPCODE_08            = 0x08,
	VST_EFFECT_OPCODE_PARAM_GETNAME = 0x08,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_09 = 0x09,

	/* Set the new sample rate for the plugin to use.
	 * 
	 * @param p_float New sample rate as a float (double on 64-bit because register upgrades).
	 */
	VST_EFFECT_OPCODE_0A              = 0x0A,
	VST_EFFECT_OPCODE_SETSAMPLERATE   = 0x0A,
	VST_EFFECT_OPCODE_SET_SAMPLE_RATE = 0x0A,

	/* Sets the block size, which is the maximum number of samples passed into the effect via process calls.
	 *
	 * @param p_int2 The maximum number of samples to be passed in.
	 */
	VST_EFFECT_OPCODE_0B             = 0x0B,
	VST_EFFECT_OPCODE_SETBLOCKSIZE   = 0x0B,
	VST_EFFECT_OPCODE_SET_BLOCK_SIZE = 0x0B,

	/* Effect processing should be suspended/paused.
	 *
	 * Unclear if this is should result in a flush of buffers.
	 * 
	 * @param p_int2 0 if the effect should suspend processing, 1 if it should resume.
	 */
	VST_EFFECT_OPCODE_0C      = 0x0C,
	VST_EFFECT_OPCODE_SUSPEND = 0x0C,

	/* Retrieve the client rect size of the plugins window.
	 * If no window has been created, returns the default rect.
	 *
	 * @param p_ptr Pointer of type 'struct vst_rect*'.
	 * @return On success, returns 1 and updates p_ptr to the rect. On failure, returns 0.
	 */
	VST_EFFECT_OPCODE_0D             = 0x0D,
	VST_EFFECT_OPCODE_WINDOW_GETRECT = 0x0D,

	/* Create the window for the plugin.
	 * 
	 * @param p_ptr HWND of the parent window.
	 * @return 0 on failure, or HWND on success.
	 */
	VST_EFFECT_OPCODE_0E            = 0x0E,
	VST_EFFECT_OPCODE_WINDOW_CREATE = 0x0E,

	/* Destroy the plugins window.
	 * 
	 * @return Always 0.
	 */
	VST_EFFECT_OPCODE_0F             = 0x0F,
	VST_EFFECT_OPCODE_WINDOW_DESTROY = 0x0F,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_10 = 0x10,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_11 = 0x11,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_12 = 0x12,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_13 = 0x13,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_14 = 0x14,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_15 = 0x15,

	/* Always returns the FourCC 'NvEF' (0x4E764566).
	 */
	VST_EFFECT_OPCODE_16 = 0x16,

	/* Get Chunk
	 *
	 *
	 */
	VST_EFFECT_OPCODE_17 = 0x17,

	/* Set Chunk
	 *
	 *
	 */
	VST_EFFECT_OPCODE_18 = 0x18,

	// VST2.x starts here.

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_19 = 0x19,

	/* Can the parameter be automated?
	 *
	 * @param p_int1 Index of the parameter.
	 * @return 1 if the parameter can be automated, otherwise 0.
	 */
	VST_EFFECT_OPCODE_1A                  = 0x1A,
	VST_EFFECT_OPCODE_PARAM_ISAUTOMATABLE = 0x1A,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_1B = 0x1B,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_1C = 0x1C,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_1D = 0x1D, // See VST_EFFECT_OPCODE_05

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_1E = 0x1E,

	/* Input connected.
	 *
	 *
	 */
	VST_EFFECT_OPCODE_1F = 0x1F,

	/* Input disconnected.
	 *
	 *
	 */
	VST_EFFECT_OPCODE_20 = 0x20,

	/* Retrieve the name of the input channel at the given index.
	 *
	 * @param p_int1 Index of the input to get the name for.
	 * @param p_ptr Pointer to a char* buffer able to hold at minimum 20 characters. Might need to be 32 even.
	 * @return 0 on failure, 1 on success.
	 */
	VST_EFFECT_OPCODE_21                   = 0x21,
	VST_EFFECT_OPCODE_INPUT_GETCHANNELNAME = 0x21,
	VST_EFFECT_OPCODE_INPUT_CHANNEL_NAME   = 0x21,

	/* Retrieve the name of the output channel at the given index.
	 *
	 * @param p_int1 Index of the output to get the name for.
	 * @param p_ptr Pointer to a char* buffer able to hold at minimum 20 characters. Might need to be 32 even.
	 * @return 0 on failure, 1 on success.
	 */
	VST_EFFECT_OPCODE_22                    = 0x22,
	VST_EFFECT_OPCODE_OUTPUT_GETCHANNELNAME = 0x22,
	VST_EFFECT_OPCODE_OUTPUT_CHANNEL_NAME   = 0x22,

	/* Retrieve category of this effect.
	 *
	 * @return The category that this effect is in, see VST_CATEGORY.
	 */
	VST_EFFECT_OPCODE_23              = 0x23,
	VST_EFFECT_OPCODE_EFFECT_CATEGORY = 0x23,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_24 = 0x24,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_25 = 0x25,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_26 = 0x26,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_27 = 0x27,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_28 = 0x28,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_29 = 0x29,

	/* Set the speaker arrangement 
	 *
	 * @param p_int2 (vst_speaker_arrangement*) Pointer to a pointer to the speaker arrangement for the input.
	 * @param p_ptr (vst_speaker_arrangement*) Pointer to a pointer to the speaker arrangement for the output.
	 */
	VST_EFFECT_OPCODE_2A                      = 0x2A,
	VST_EFFECT_OPCODE_SET_SPEAKER_ARRANGEMENT = 0x2A,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_2B = 0x2B,

	/* Enable/Disable bypassing the effect.
	 *
	 * @param p_int2 Zero if bypassing the effect is disabled, otherwise 1.
	 */
	VST_EFFECT_OPCODE_2C     = 0x2C,
	VST_EFFECT_OPCODE_BYPASS = 0x2C,

	/* Retrieve the effect name into the ptr buffer.
	 *
	 * @param p_ptr char[64] Buffer containing a zero-terminated effect information string. May be shorter than 64 bytes on older hosts.
	 * @return Always 0, even on failure.
	 */
	VST_EFFECT_OPCODE_2D          = 0x2D,
	VST_EFFECT_OPCODE_GETNAME     = 0x2D,
	VST_EFFECT_OPCODE_EFFECT_NAME = 0x2D,

	/* Translate an error code to a string.
	 *
	 * @param p_ptr char[256] Buffer that should contain a zero-terminated error string.
	 */
	VST_EFFECT_OPCODE_2E              = 0x2E,
	VST_EFFECT_OPCODE_TRANSLATE_ERROR = 0x2E,

	/* Retrieve the vendor name into the ptr buffer.
	 *
	 * @param p_ptr char[64] Buffer containing a zero-terminated vendor information string. May be shorter than 64 bytes on older hosts.
	 * @return Always 0, even on failure.
	 */
	VST_EFFECT_OPCODE_2F          = 0x2F,
	VST_EFFECT_OPCODE_GETVENDOR   = 0x2F,
	VST_EFFECT_OPCODE_VENDOR_NAME = 0x2F,

	/* See VST_EFFECT_OPCODE_GETNAME
	 *
	 * Rarely used, if at all even supported. Not sure what the purpose of this is even.
	 */
	VST_EFFECT_OPCODE_30           = 0x30,
	VST_EFFECT_OPCODE_GETNAME2     = 0x30,
	VST_EFFECT_OPCODE_PRODUCT_NAME = 0x30,

	/* Retrieve the vendor version in return value.
	 *
	 * @return Version.
	 */
	VST_EFFECT_OPCODE_31               = 0x31,
	VST_EFFECT_OPCODE_GETVENDORVERSION = 0x31,
	VST_EFFECT_OPCODE_VENDOR_VERSION   = 0x31,

	/* User defined OP Code, for custom interaction.
	 * 
	 */
	VST_EFFECT_OPCODE_32     = 0x32,
	VST_EFFECT_OPCODE_CUSTOM = 0x32,

	/* Test for support of a specific named feature.
	 * 
	 * @param p_ptr Pointer to a zero-terminated buffer containing the feature name.
	 * @return Non-zero if the feature is supported, otherwise 0.
	 */
	VST_EFFECT_OPCODE_33       = 0x33,
	VST_EFFECT_OPCODE_SUPPORTS = 0x33,

	/* Number of samples that are at the tail at the end of playback.
	 * 
	 * @return 0 or 1 for no tail, > 1 for number of samples to tail.
	 */
	VST_EFFECT_OPCODE_34             = 0x34,
	VST_EFFECT_OPCODE_GETTAILSAMPLES = 0x34,
	VST_EFFECT_OPCODE_TAIL_SAMPLES   = 0x34,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_35 = 0x35,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_36 = 0x36,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_37 = 0x37,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_38 = 0x38,

	/* Parameter Properties
	 *
	 * @param p_ptr vst_parameter_properties*
	 * @return 1 if supported, otherwise 0.
	 */
	VST_EFFECT_OPCODE_39                       = 0x39,
	VST_EFFECT_OPCODE_GET_PARAMETER_PROPERTIES = VST_EFFECT_OPCODE_39,

	/* Retrieve the VST Version supported.
	 *
	 * @return Return 0 for <2.0, 2 for 2.0, 2100 for 2.1, 2200 for 2.2, 2300 for 2.3, etc.
	 */
	VST_EFFECT_OPCODE_3A          = 0x3A,
	VST_EFFECT_OPCODE_VST_VERSION = 0x3A,

	// VST 2.1 or later

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_3B = 0x3B,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_3C = 0x3C,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_3D = 0x3D,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_3E = 0x3E,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_3F = 0x3F,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_40 = 0x40,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_41 = 0x41,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_42 = 0x42,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_43 = 0x43,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_44 = 0x44,

	// VST 2.3 or later

	/* Retrieve the speaker arrangement.
	 *
	 * @param p_int2 (vst_speaker_arrangement**) Pointer to a pointer to the speaker arrangement for the input.
	 * @param p_ptr (vst_speaker_arrangement**) Pointer to a pointer to the speaker arrangement for the output.
	 */
	VST_EFFECT_OPCODE_45                      = 0x45,
	VST_EFFECT_OPCODE_GET_SPEAKER_ARRANGEMENT = 0x45,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_46 = 0x46,

	/* Begin processing of audio.
	 *
	 * 
	 * 
	 */
	VST_EFFECT_OPCODE_PROCESS_BEGIN = 0x47,

	/* End processing of audio.
	 *
	 * 
	 *
	 */
	VST_EFFECT_OPCODE_PROCESS_END = 0x48,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_49 = 0x49,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_4A = 0x4A,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_4B = 0x4B,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_4C = 0x4C,

	// VST 2.4 or later

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_4D = 0x4D,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_4E = 0x4E,

	/*
	 *
	 *
	 */
	VST_EFFECT_OPCODE_4F = 0x4F,

	// Highest number of known OPCODE.
	VST_EFFECT_OPCODE_MAX,

	// Pad to force 32-bit number.
	_VST_EFFECT_OPCODE_PAD = 0xFFFFFFFFul,
};

enum VST_HOST_OPCODE {
	/*
	 * @param int1 -1 or Parameter Index
	 * @return Expected to return... something.
	 */
	VST_HOST_OPCODE_00 = 0x00, // cb(vst, 0x00, ?, 0, 0);
	VST_HOST_OPCODE_01 = 0x01,
	VST_HOST_OPCODE_02 = 0x02, // bool cb(0, 0x02, 0, 0, 0);
	VST_HOST_OPCODE_03 = 0x03,
	VST_HOST_OPCODE_04 = 0x04,
	VST_HOST_OPCODE_05 = 0x05,
	VST_HOST_OPCODE_06 = 0x06,
	VST_HOST_OPCODE_07 = 0x07,
	VST_HOST_OPCODE_08 = 0x08,
	VST_HOST_OPCODE_09 = 0x09,
	VST_HOST_OPCODE_0A = 0x0A,
	VST_HOST_OPCODE_0B = 0x0B,
	VST_HOST_OPCODE_0C = 0x0C,
	VST_HOST_OPCODE_0D = 0x0D,
	VST_HOST_OPCODE_0E = 0x0E,
	VST_HOST_OPCODE_0F = 0x0F,
	VST_HOST_OPCODE_10 = 0x10,
	VST_HOST_OPCODE_11 = 0x11,
	VST_HOST_OPCODE_12 = 0x12,
	VST_HOST_OPCODE_13 = 0x13,
	VST_HOST_OPCODE_14 = 0x14,
	VST_HOST_OPCODE_15 = 0x15,
	VST_HOST_OPCODE_16 = 0x16,
	VST_HOST_OPCODE_17 = 0x17,
	VST_HOST_OPCODE_18 = 0x18,
	VST_HOST_OPCODE_19 = 0x19,
	VST_HOST_OPCODE_1A = 0x1A,
	VST_HOST_OPCODE_1B = 0x1B,
	VST_HOST_OPCODE_1C = 0x1C,
	VST_HOST_OPCODE_1D = 0x1D,
	VST_HOST_OPCODE_1E = 0x1E,
	VST_HOST_OPCODE_1F = 0x1F,
	VST_HOST_OPCODE_20 = 0x20,
	VST_HOST_OPCODE_21 = 0x21,
	VST_HOST_OPCODE_22 = 0x22,
	VST_HOST_OPCODE_23 = 0x23,
	VST_HOST_OPCODE_24 = 0x24,
	VST_HOST_OPCODE_25 = 0x25,
	VST_HOST_OPCODE_26 = 0x26,
	VST_HOST_OPCODE_27 = 0x27,
	VST_HOST_OPCODE_28 = 0x28,
	VST_HOST_OPCODE_29 = 0x29,
	VST_HOST_OPCODE_2A = 0x2A,

	/* Parameter gained focus.
	 *
	 * @param int1 Parameter index.
	 */
	VST_HOST_OPCODE_2B = 0x2B,

	/* Parameter lost focus.
	 * 
	 * @param int1 Parameter index.
	 */
	VST_HOST_OPCODE_2C = 0x2C,

	VST_HOST_OPCODE_2D = 0x2D,
	VST_HOST_OPCODE_2E = 0x2E,
	VST_HOST_OPCODE_2F = 0x2F,

	// Highest number of known OPCODE.
	VST_HOST_OPCODE_MAX,

	// Pad to force 32-bit number.
	_VST_HOST_OPCODE_PAD = 0xFFFFFFFFul,
};

enum VST_ARRANGEMENT_TYPE {
	/* Custom speaker arrangement.
	 *
	 * Accidentally discovered through random testing.
	 */
	VST_ARRANGEMENT_TYPE_CUSTOM = -2,

	/* Unknown/Empty speaker layout.
	 *
	 */
	VST_ARRANGEMENT_TYPE_UNKNOWN = -1,

	/* Mono
	 */
	VST_ARRANGEMENT_TYPE_MONO = 0,

	/* Stereo
	 */
	VST_ARRANGEMENT_TYPE_STEREO = 1,

	/* 5.1
	 */
	VST_ARRANGEMENT_TYPE_5_1 = 0x0F,

	// Pad to force 32-bit number.
	_VST_ARRANGEMENT_TYPE_PAD = 0xFFFFFFFFul,
};

enum VST_SPEAKER_TYPE {
	VST_SPEAKER_TYPE_MONO       = 0,
	VST_SPEAKER_TYPE_LEFT       = 1,
	VST_SPEAKER_TYPE_RIGHT      = 2,
	VST_SPEAKER_TYPE_CENTER     = 3,
	VST_SPEAKER_TYPE_LFE        = 4,
	VST_SPEAKER_TYPE_LEFT_SIDE  = 5,
	VST_SPEAKER_TYPE_RIGHT_SIDE = 6,

	// Pad to force 32-bit number.
	_VST_SPEAKER_TYPE_PAD = 0xFFFFFFFFul,
};

enum VST_PARAMETER_FLAGS {
	/**
	 * Parameter is an on/off switch.
	 */
	VST_PARAMETER_FLAGS_SWITCH = 1,
	/**
	 * Limits defined by integers.
	 */
	VST_PARAMETER_FLAGS_INTEGER_LIMITS = 1 << 1,
	/**
	 * Uses float steps.
	 */
	VST_PARAMETER_FLAGS_STEP_FLOAT = 1 << 2,
	/**
	 * Uses integer steps.
	 */
	VST_PARAMETER_FLAGS_STEP_INT = 1 << 3,
	/**
	 * Respect index variable for display ordering.
	 */
	VST_PARAMETER_FLAGS_INDEX = 1 << 4,
	/**
	 * Respect category value and names.
	 */
	VST_PARAMETER_FLAGS_CATEGORY = 1 << 5,
	VST_PARAMETER_FLAGS_UNKNOWN6 = 1 << 6,
	_VST_PARAMETER_FLAGS_PAD     = 0xFFFFFFFFul,
};

/*******************************************************************************
|* Structures
|*/

struct vst_rect {
	int16_t top;
	int16_t left;
	int16_t bottom;
	int16_t right;
};

struct vst_effect {
	int32_t magic_number; // Should always be VST_MAGICNUMBER

	// 64-bit adds 4-byte padding here to align pointers.

	/* Control the VST through an opcode and up to four parameters.
	 * 
	 * @param this Pointer to the effect itself.
	 * @param opcode The opcode to run, see VST_EFFECT_OPCODES.
	 * @param p_int1 Parameter, see VST_EFFECT_OPCODES.
	 * @param p_int2 Parameter, see VST_EFFECT_OPCODES.
	 * @param p_ptr Parameter, see VST_EFFECT_OPCODES.
	 * @param p_float Parameter, see VST_EFFECT_OPCODES.
	 */
	intptr_t(VST_FUNCTION_INTERFACE* control)(vst_effect* pthis, VST_EFFECT_OPCODE opcode, int32_t p_int1,
											  intptr_t p_int2, void* p_ptr, float p_float);

	/* Process the given number of samples in inputs and outputs.
	 *
	 * Different to process_float how? Never seen any difference.
	 * 
	 * @param pthis Pointer to the effect itself.
	 * @param inputs Pointer to an array of 'const float[samples]' with size numInputs.
	 * @param outputs Pointer to an array of 'float[samples]' with size numOutputs.
	 * @param samples Number of samples per channel in inputs.
	 */
	void(VST_FUNCTION_INTERFACE* process)(vst_effect* pthis, const float* const* inputs, float** outputs,
										  int32_t samples);

	/* Updates the value for the parameter at the given index, or does nothing if out of bounds.
	 * 
	 * @param pthis Pointer to the effect itself.
	 * @param index Parameter index.
	 * @param value New value for the parameter.
	 */
	void(VST_FUNCTION_INTERFACE* set_parameter)(vst_effect* pthis, uint32_t index, float value);

	/* Returns the value stored for the parameter at index, or 0 if out of bounds.
	 * 
	 * @param pthis Pointer to the effect itself.
	 * @param index Parameter index.
	 * @return float Value of the parameter.
	 */
	float(VST_FUNCTION_INTERFACE* get_parameter)(vst_effect* pthis, uint32_t index);

	int32_t num_programs; // Number of possible programs.
	int32_t num_params;   // Number of possible parameters.
	int32_t num_inputs;   // Number of inputs.
	int32_t num_outputs;  // Number of outputs.

	/* Bitflags
	 * 
	 * Bit		Description
	 * 1		Effect has "Editor"
	 * 2		Unknown (Found in: ReaDelay)
	 * 3		Unknown (Found in: ReaDelay)
	 * 4		Unknown (Found in: ReaDelay)
	 * 5		Has process_float (Found in: ReaDelay, ReaComp, ReaControlMIDI, ReaStream, ReaFir)
	 * 6		Unknown (Found in: ReaControlMIDI, ReaStream, ReaFir)
	 * 10		Unknown (Found in: ReaFir)
	 * 13		Has process_double (Found in: ReaControlMIDI)
	 */
	int32_t flags;

	// 64-bit adds 4-byte padding here to align pointers.

	void* _unknown_ptr_00[2];

	/* Initial delay before processing of samples can actually begin in Samples.
	 *
	 * Should be updated before or during handling the 0x47 control call. 
	 */
	int32_t delay;

	int32_t _unknown_int32_00[2]; // Unknown int32_t values.
	float   _unknown_float_00;    // Seems to always be 1.0

	void* effect_internal; // Pointer to Plugin internal data
	void* host_internal;   // Pointer to Host internal data.

	/* Id of the plugin.
	 *
	 * Due to this not being enough for uniqueness, it should not be used alone 
	 * for indexing. Ideally you want to index like this:
	 *     [unique_id][module_name][version][flags]
	 * If any of the checks after unique_id fail, you default to the first 
	 * possible choice.
	 */
	int32_t unique_id;

	/* Plugin version
	 * 
	 * Unrelated to the minimum VST Version, but often the same.
	 */
	int32_t version;

	// There is no padding here if everything went right.

	/* Process the given number of single samples in inputs and outputs.
	 *
	 * @param pthis Pointer to the effect itself.
	 * @param inputs Pointer to an array of 'const float[samples]' with size numInputs.
	 * @param outputs Pointer to an array of 'float[samples]' with size numOutputs.
	 * @param samples Number of samples per channel in inputs.
	 */
	void(VST_FUNCTION_INTERFACE* process_float)(vst_effect* pthis, const float* const* inputs, float** outputs,
												int32_t samples);

	/* Process the given number of double samples in inputs and outputs.
	 *
	 * Used only by 2.4 hosts and plugins, possibly restricted to said version.
	 * 
	 * @param pthis Pointer to the effect itself.
	 * @param inputs Pointer to an array of 'const double[samples]' with size numInputs.
	 * @param outputs Pointer to an array of 'double[samples]' with size numOutputs.
	 * @param samples Number of samples per channel in inputs.
	 */
	void(VST_FUNCTION_INTERFACE* process_double)(vst_effect* pthis, const double* const* inputs, double** outputs,
												 int32_t samples);

	// Everything after this is unknown and was present in reacomp-standalone.dll.
	uint8_t _unknown[56]; // 56-bytes of something. Could also just be 52-bytes.
};

struct vst_parameter_properties {
	float step_f32;
	float step_small_f32;
	float step_large_f32;

	char name[VST_BUFFER_64];

	uint32_t flags;
	int32_t  min_value_i32;
	int32_t  max_value_i32;
	int32_t  step_i32;

	char label[VST_BUFFER_8];

	uint16_t index;

	uint16_t category;
	uint16_t num_parameters_in_category;
	uint16_t _unknown_00;

	char category_label[VST_BUFFER_24];

	char _unknown_01[VST_BUFFER_16];
};

struct vst_speaker_properties {
	float            _unknown_00; // 10.0 if LFE, otherwise random? Never exceeds -PI to PI range.
	float            _unknown_04; // 10.0 if LFE, otherwise random? Never exceeds -PI to PI range.
	float            _unknown_08; // 0.0 if LFE, otherwise 1.0.
	float            _unknown_0C;
	char             name[VST_BUFFER_64];
	VST_SPEAKER_TYPE type;

	uint8_t _unknown[28]; // Padding detected from testing.
};

struct vst_speaker_arrangement {
	VST_ARRANGEMENT_TYPE   type;                       // See VST_SPEAKER_ARRANGEMENT_TYPE
	int32_t                channels;                   // Number of channels in speakers.
	vst_speaker_properties speakers[VST_MAX_CHANNELS]; // Array of speaker properties, actual size defined by channels.
};

/* Callback used by the plugin to interface with the host.
 * 
 * @param opcode See VST_HOST_OPCODE
 * @param p_str Zero terminated string or null on call.
 * @return ?
 */
typedef intptr_t (*vst_host_callback)(vst_effect* plugin, VST_HOST_OPCODE opcode, int32_t p_int1, int64_t p_int2,
									  void* p_str, int32_t p_int3);

/* Entry point for VST2.x plugins.
 *
 * @return A new instance of the VST2.x effect.
 */
#define VST_ENTRYPOINT vst_effect* VSTPluginMain(vst_host_callback callback)
#define VST_ENTRYPOINT_WINDOWS                   \
	vst_effect* MAIN(vst_host_callback callback) \
	{                                            \
		return VSTPluginMain(callback);          \
	}
#define VST_ENTRYPOINT_MACOS                           \
	vst_effect* main_macho(vst_host_callback callback) \
	{                                                  \
		return VSTPluginMain(callback);                \
	}

#ifdef __cplusplus
}
#endif

// Variable size variant of vst_speaker_arrangement.
#ifdef __cplusplus
template<size_t T>
struct vst_speaker_arrangement_t {
	VST_ARRANGEMENT_TYPE   type;        // See VST_SPEAKER_ARRANGEMENT_TYPE
	int32_t                channels;    // Number of channels in speakers.
	vst_speaker_properties speakers[T]; // Array of speaker properties, actual size defined by channels.
};
#endif

#pragma pack(pop)

#endif
