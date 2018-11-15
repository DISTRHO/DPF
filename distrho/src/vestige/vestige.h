/*
 * IMPORTANT: The author of DPF has no connection with the
 * author of the VeSTige VST-compatibility header, has had no
 * involvement in its creation.
 *
 * The VeSTige header is included in this package in the good-faith
 * belief that it has been cleanly and legally reverse engineered
 * without reference to the official VST SDK and without its
 * developer(s) having agreed to the VST SDK license agreement.
 */

/*
 * simple header to allow VeSTige compilation and eventually work
 *
 * Copyright (c) 2006 Javier Serrano Polo <jasp00/at/users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */
#include <stdint.h>
#ifndef _VESTIGE_H
#define _VESTIGE_H

#define CCONST(a, b, c, d)( ( ( (int) a ) << 24 ) |		\
				( ( (int) b ) << 16 ) |		\
				( ( (int) c ) << 8 ) |		\
				( ( (int) d ) << 0 ) )

enum
{
    audioMasterAutomate = 0,
    audioMasterVersion = 1,
    audioMasterCurrentId = 2,
    audioMasterIdle = 3,
    audioMasterPinConnected = 4,
    /* unsupported? 5 */
    audioMasterWantMidi = 6,
    audioMasterGetTime = 7,
    audioMasterProcessEvents = 8,
    audioMasterSetTime = 9,
    audioMasterTempoAt = 10,
    audioMasterGetNumAutomatableParameters = 11,
    audioMasterGetParameterQuantization = 12,
    audioMasterIOChanged = 13,
    audioMasterNeedIdle = 14,
    audioMasterSizeWindow = 15,
    audioMasterGetSampleRate = 16,
    audioMasterGetBlockSize = 17,
    audioMasterGetInputLatency = 18,
    audioMasterGetOutputLatency = 19,
    audioMasterGetPreviousPlug = 20,
    audioMasterGetNextPlug = 21,
    audioMasterWillReplaceOrAccumulate = 22,
    audioMasterGetCurrentProcessLevel = 23,
    audioMasterGetAutomationState = 24,
    audioMasterOfflineStart = 25,
    audioMasterOfflineRead = 26,
    audioMasterOfflineWrite = 27,
    audioMasterOfflineGetCurrentPass = 28,
    audioMasterOfflineGetCurrentMetaPass = 29,
    audioMasterSetOutputSampleRate = 30,
    /* unsupported? 31 */
    audioMasterGetSpeakerArrangement = 31, /* deprecated in 2.4? */
    audioMasterGetVendorString = 32,
    audioMasterGetProductString = 33,
    audioMasterGetVendorVersion = 34,
    audioMasterVendorSpecific = 35,
    audioMasterSetIcon = 36,
    audioMasterCanDo = 37,
    audioMasterGetLanguage = 38,
    audioMasterOpenWindow = 39,
    audioMasterCloseWindow = 40,
    audioMasterGetDirectory = 41,
    audioMasterUpdateDisplay = 42,
    audioMasterBeginEdit = 43,
    audioMasterEndEdit = 44,
    audioMasterOpenFileSelector = 45,
    audioMasterCloseFileSelector = 46, /* currently unused */
    audioMasterEditFile = 47, /* currently unused */
    audioMasterGetChunkFile = 48, /* currently unused */
    audioMasterGetInputSpeakerArrangement = 49, /* currently unused */

    effFlagsHasEditor = 1,
    effFlagsCanReplacing = (1 << 4), /* very likely */
    effFlagsIsSynth = (1 << 8), /* currently unused */

    effOpen = 0,
    effClose = 1,
    effSetProgram = 2,
    effGetProgram = 3,
    effGetProgramName = 5,
    effGetParamName = 8,
    effSetSampleRate = 10,
    effSetBlockSize = 11,
    effMainsChanged = 12,
    effEditGetRect = 13,
    effEditOpen = 14,
    effEditClose = 15,
    effEditIdle = 19,
    effEditTop = 20,
    effProcessEvents = 25,
    effGetPlugCategory = 35,
    effGetEffectName = 45,
    effGetVendorString = 47,
    effGetProductString = 48,
    effGetVendorVersion = 49,
    effCanDo = 51,
    effIdle = 53,
    effGetParameterProperties = 56,
    effGetVstVersion = 58,
    effShellGetNextPlugin = 70,
    effStartProcess = 71,
    effStopProcess = 72,

    effBeginSetProgram = 67,
    effEndSetProgram = 68,

    kEffectMagic = CCONST( 'V', 's', 't', 'P' ),

    kVstLangEnglish = 1,
    kVstMidiType = 1,

    kVstTransportChanged = 1,
    kVstTransportPlaying = (1 << 1),
    kVstTransportCycleActive = (1 << 2),
    kVstTransportRecording = (1 << 3),

    kVstAutomationWriting = (1 << 6),
    kVstAutomationReading = (1 << 7),

    kVstNanosValid = (1 << 8),
    kVstPpqPosValid = (1 << 9),
    kVstTempoValid = (1 << 10),
    kVstBarsValid = (1 << 11),
    kVstCyclePosValid = (1 << 12),
    kVstTimeSigValid = (1 << 13),
    kVstSmpteValid = (1 << 14),
    kVstClockValid = (1 << 15)
};

struct RemoteVstPlugin;

struct _VstMidiEvent
{
	/* 00 */
	int type;
	/* 04 */
	int byteSize;
	/* 08 */
	int deltaFrames;
	/* 0c? */
	int flags;
	/* 10? */
	int noteLength;
	/* 14? */
	int noteOffset;
	/* 18 */
	char midiData[4];
	/* 1c? */
	char detune;
	/* 1d? */
	char noteOffVelocity;
	/* 1e? */
	char reserved1;
	/* 1f? */
	char reserved2;
};

typedef struct _VstMidiEvent VstMidiEvent;


struct _VstEvent
{
	char dump[sizeof (VstMidiEvent)];

};

typedef struct _VstEvent VstEvent;

struct _VstEvents
{
	/* 00 */
	int numEvents;
	/* 04 */
	void *reserved;
	/* 08 */
	VstEvent * events[2];
};

enum Vestige2StringConstants
{
	VestigeMaxNameLen       = 64,
	VestigeMaxLabelLen      = 64,
	VestigeMaxShortLabelLen = 8,
	VestigeMaxCategLabelLen = 24,
	VestigeMaxFileNameLen   = 100
};


enum VstPlugCategory
{
	kPlugCategUnknown = 0,
	kPlugCategEffect,
	kPlugCategSynth,
	kPlugCategAnalysis,
	kPlugCategMastering,
	kPlugCategSpacializer,
	kPlugCategRoomFx,
	kPlugSurroundFx,
	kPlugCategRestoration,
	kPlugCategOfflineProcess,
	kPlugCategShell,
	kPlugCategGenerator,
	kPlugCategMaxCount
};

typedef struct _VstEvents VstEvents;

struct _VstParameterProperties
{
	float stepFloat;              /* float step */
	float smallStepFloat;         /* small float step */
	float largeStepFloat;         /* large float step */
	char label[VestigeMaxLabelLen];  /* parameter label */
	int32_t flags;               /* @see VstParameterFlags */
	int32_t minInteger;          /* integer minimum */
	int32_t maxInteger;          /* integer maximum */
	int32_t stepInteger;         /* integer step */
	int32_t largeStepInteger;    /* large integer step */
	char shortLabel[VestigeMaxShortLabelLen]; /* short label, recommended: 6 + delimiter */
	int16_t displayIndex;        /* index where this parameter should be displayed (starting with 0) */
	int16_t category;            /* 0: no category, else group index + 1 */
	int16_t numParametersInCategory; /* number of parameters in category */
	int16_t reserved;            /* zero */
	char categoryLabel[VestigeMaxCategLabelLen]; /* category label, e.g. "Osc 1"  */
	char future[16];              /* reserved for future use */
};

typedef struct _VstParameterProperties VstParameterProperties;

enum VstParameterFlags
{
	kVstParameterIsSwitch                = 1 << 0,  /* parameter is a switch (on/off) */
	kVstParameterUsesIntegerMinMax       = 1 << 1,  /* minInteger, maxInteger valid */
	kVstParameterUsesFloatStep           = 1 << 2,  /* stepFloat, smallStepFloat, largeStepFloat valid */
	kVstParameterUsesIntStep             = 1 << 3,  /* stepInteger, largeStepInteger valid */
	kVstParameterSupportsDisplayIndex    = 1 << 4,  /* displayIndex valid */
	kVstParameterSupportsDisplayCategory = 1 << 5,  /* category, etc. valid */
	kVstParameterCanRamp                 = 1 << 6   /* set if parameter value can ramp up/down */
};

struct _AEffect
{
	/* Never use virtual functions!!! */
	/* 00-03 */
	int magic;
	/* dispatcher 04-07 */
	intptr_t (* dispatcher) (struct _AEffect *, int, int, intptr_t, void *, float);
	/* process, quite sure 08-0b */
	void (* process) (struct _AEffect *, float **, float **, int);
	/* setParameter 0c-0f */
	void (* setParameter) (struct _AEffect *, int, float);
	/* getParameter 10-13 */
	float (* getParameter) (struct _AEffect *, int);
	/* programs 14-17 */
	int numPrograms;
	/* Params 18-1b */
	int numParams;
	/* Input 1c-1f */
	int numInputs;
	/* Output 20-23 */
	int numOutputs;
	/* flags 24-27 */
	int flags;
	/* Fill somewhere 28-2b */
	void *ptr1;
	void *ptr2;
	int initialDelay;
	/* Zeroes 30-33 34-37 38-3b */
	char empty2[4 + 4];
	/* 1.0f 3c-3f */
	float unkown_float;
	/* An object? pointer 40-43 */
	void *object;
	/* Zeroes 44-47 */
	void *user;
	/* Id 48-4b */
	int32_t uniqueID;
	/* plugin version 4c-4f */
	int32_t version;
	/* processReplacing 50-53 */
	void (* processReplacing) (struct _AEffect *, float **, float **, int);
};

typedef struct _AEffect AEffect;

typedef struct _VstTimeInfo
{
    /* info from online documentation of VST provided by Steinberg */

    double samplePos;
    double sampleRate;
    double nanoSeconds;
    double ppqPos;
    double tempo;
    double barStartPos;
    double cycleStartPos;
    double cycleEndPos;
    int32_t   timeSigNumerator;
    int32_t   timeSigDenominator;
    int32_t   smpteOffset;
    int32_t   smpteFrameRate;
    int32_t   samplesToNextClock;
    int32_t   flags;

} VstTimeInfo;

typedef intptr_t (* audioMasterCallback) (AEffect *, int32_t, int32_t, intptr_t, void *, float);

#endif
