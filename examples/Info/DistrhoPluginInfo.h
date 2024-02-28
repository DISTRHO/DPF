/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2024 Filipe Coelho <falktx@falktx.com>
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

#define DISTRHO_PLUGIN_BRAND   "DISTRHO"
#define DISTRHO_PLUGIN_NAME    "Info"
#define DISTRHO_PLUGIN_URI     "http://distrho.sf.net/examples/Info"
#define DISTRHO_PLUGIN_CLAP_ID "studio.kx.distrho.examples.info"

#define DISTRHO_PLUGIN_BRAND_ID  Dstr
#define DISTRHO_PLUGIN_UNIQUE_ID dNfo

#define DISTRHO_PLUGIN_HAS_UI       1
#define DISTRHO_PLUGIN_IS_RT_SAFE   1
#define DISTRHO_PLUGIN_NUM_INPUTS   2
#define DISTRHO_PLUGIN_NUM_OUTPUTS  2
#define DISTRHO_PLUGIN_USES_MODGUI  1
#define DISTRHO_PLUGIN_WANT_TIMEPOS 1
#define DISTRHO_UI_DEFAULT_WIDTH    405
#define DISTRHO_UI_DEFAULT_HEIGHT   256
#define DISTRHO_UI_FILE_BROWSER     0
#define DISTRHO_UI_USER_RESIZABLE   1
#define DISTRHO_UI_USE_NANOVG       1

// only checking if supported, not actually used
#define DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST 1

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
