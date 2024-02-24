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

/**
   The plugin name.@n
   This is used to identify your plugin before a Plugin instance can be created.
   @note This macro is required.
 */
#define DISTRHO_PLUGIN_NAME "CairoUI"

/**
   Number of audio inputs the plugin has.
   @note This macro is required.
 */
#define DISTRHO_PLUGIN_NUM_INPUTS 1

/**
   Number of audio outputs the plugin has.
   @note This macro is required.
 */
#define DISTRHO_PLUGIN_NUM_OUTPUTS 1

/**
   The plugin URI when exporting in LV2 format.
   @note This macro is required.
 */
#define DISTRHO_PLUGIN_URI "http://distrho.sf.net/examples/CairoUI"

/**
   The AudioUnit manufacturer for a plugin.
   This is a 4-character symbol with at least one non-lower case character.
   Plugins from the same brand/maker should use the same symbol.
   @note This macro is required when building AU plugins
 */
#define DISTRHO_PLUGIN_BRAND_ID Dstr

/**
   The AudioUnit subtype for a plugin.
   This is a 4-character symbol which identifies a plugin.
   It must be unique within a manufacturer's plugins, but different manufacturers can use the same subtype.
   @note This macro is required when building AU plugins
 */
#define DISTRHO_PLUGIN_UNIQUE_ID dCai

/**
   The plugin id when exporting in CLAP format, in reverse URI form.
   @note This macro is required when building CLAP plugins
*/
#define DISTRHO_PLUGIN_CLAP_ID "studio.kx.distrho.examples.cairo-ui"

/**
   Wherever the plugin has a custom %UI.
   @see DISTRHO_UI_USE_NANOVG
   @see UI
 */
#define DISTRHO_PLUGIN_HAS_UI 1

/**
   Wherever the plugin processing is realtime-safe.@n
   TODO - list rtsafe requirements
 */
#define DISTRHO_PLUGIN_IS_RT_SAFE 1

/**
   Wherever the plugin is a synth.@n
   @ref DISTRHO_PLUGIN_WANT_MIDI_INPUT is automatically enabled when this is too.
   @see DISTRHO_PLUGIN_WANT_MIDI_INPUT
 */
#define DISTRHO_PLUGIN_IS_SYNTH 0

/**
   Enable direct access between the %UI and plugin code.
   @see UI::getPluginInstancePointer()
   @note DO NOT USE THIS UNLESS STRICTLY NECESSARY!!
         Try to avoid it at all costs!
 */
#define DISTRHO_PLUGIN_WANT_DIRECT_ACCESS 0

/**
   Wherever the plugin introduces latency during audio or midi processing.
   @see Plugin::setLatency(uint32_t)
 */
#define DISTRHO_PLUGIN_WANT_LATENCY 0

/**
   Wherever the plugin wants MIDI input.@n
   This is automatically enabled if @ref DISTRHO_PLUGIN_IS_SYNTH is true.
 */
#define DISTRHO_PLUGIN_WANT_MIDI_INPUT 0

/**
   Wherever the plugin wants MIDI output.
   @see Plugin::writeMidiEvent(const MidiEvent&)
 */
#define DISTRHO_PLUGIN_WANT_MIDI_OUTPUT 0

/**
   Wherever the plugin provides its own internal programs.
   @see Plugin::initProgramName(uint32_t, String&)
   @see Plugin::loadProgram(uint32_t)
 */
#define DISTRHO_PLUGIN_WANT_PROGRAMS 0

/**
   Wherever the plugin uses internal non-parameter data.
   @see Plugin::initState(uint32_t, String&, String&)
   @see Plugin::setState(const char*, const char*)
 */
#define DISTRHO_PLUGIN_WANT_STATE 0

/**
   Wherever the plugin wants time position information from the host.
   @see Plugin::getTimePosition()
 */
#define DISTRHO_PLUGIN_WANT_TIMEPOS 0

/**
   Wherever the %UI uses NanoVG for drawing instead of the default raw OpenGL calls.@n
   When enabled your %UI instance will subclass @ref NanoWidget instead of @ref Widget.
 */
#define DISTRHO_UI_USE_NANOVG 0

/**
   Default UI width to use when creating initial and temporary windows.@n
   Setting this macro allows to skip a temporary UI from being created in certain VST2 and VST3 hosts.
   (which would normally be done for knowing the UI size before host creates a window for it)

   Value must match 1x scale factor.

   When this macro is defined, the companion DISTRHO_UI_DEFAULT_HEIGHT macro must be defined as well.
 */
#define DISTRHO_UI_DEFAULT_WIDTH 200

/**
   Default UI height to use when creating initial and temporary windows.@n
   Setting this macro allows to skip a temporary UI from being created in certain VST2 and VST3 hosts.
   (which would normally be done for knowing the UI size before host creates a window for it)

   Value must match 1x scale factor.

   When this macro is defined, the companion DISTRHO_UI_DEFAULT_WIDTH macro must be defined as well.
 */
#define DISTRHO_UI_DEFAULT_HEIGHT 200

// TODO document this
#define DISTRHO_UI_USE_CAIRO 1

// TODO document this
#define DISTRHO_UI_FILE_BROWSER 0

enum Parameters {
   kParameterKnob,
   kParameterTriState,
   kParameterButton,
   kParameterCount
};
