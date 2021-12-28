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

#ifndef DISTRHO_PLUGIN_INFO_H_INCLUDED
#define DISTRHO_PLUGIN_INFO_H_INCLUDED

#define DISTRHO_PLUGIN_BRAND "DISTRHO"
#define DISTRHO_PLUGIN_NAME  "Info"
#define DISTRHO_PLUGIN_URI   "http://distrho.sf.net/examples/Info"

#define DISTRHO_PLUGIN_HAS_UI       1
#define DISTRHO_PLUGIN_IS_RT_SAFE   1
#define DISTRHO_PLUGIN_NUM_INPUTS   2
#define DISTRHO_PLUGIN_NUM_OUTPUTS  2
#define DISTRHO_PLUGIN_WANT_TIMEPOS 1
#define DISTRHO_UI_USER_RESIZABLE   1
#define DISTRHO_UI_USE_NANOVG       1

// only checking if supported, not actually used
#define DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST 1

#ifdef __MOD_DEVICES__
#define DISTRHO_PLUGIN_USES_MODGUI  1
#endif

enum Parameters {
    kParameterBufferSize = 0,
    kParameterCanRequestParameterValueChanges,
    kParameterTimePlaying,
    kParameterTimeFrame,
    kParameterTimeValidBBT,
    kParameterTimeBar,
    kParameterTimeBeat,
    kParameterTimeTick,
    kParameterTimeBarStartTick,
    kParameterTimeBeatsPerBar,
    kParameterTimeBeatType,
    kParameterTimeTicksPerBeat,
    kParameterTimeBeatsPerMinute,
    kParameterCount
};

#endif // DISTRHO_PLUGIN_INFO_H_INCLUDED
