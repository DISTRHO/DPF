/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2022 Filipe Coelho <falktx@falktx.com>
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

#include "DistrhoPluginInternal.hpp"
#include "../DistrhoPluginUtils.hpp"

#include "lv2/atom.h"
#include "lv2/buf-size.h"
#include "lv2/data-access.h"
#include "lv2/instance-access.h"
#include "lv2/midi.h"
#include "lv2/options.h"
#include "lv2/parameters.h"
#include "lv2/patch.h"
#include "lv2/port-groups.h"
#include "lv2/port-props.h"
#include "lv2/presets.h"
#include "lv2/resize-port.h"
#include "lv2/state.h"
#include "lv2/time.h"
#include "lv2/ui.h"
#include "lv2/units.h"
#include "lv2/urid.h"
#include "lv2/worker.h"
#include "lv2/lv2_kxstudio_properties.h"
#include "lv2/lv2_programs.h"

#ifdef DISTRHO_PLUGIN_LICENSED_FOR_MOD
# include "mod-license.h"
#endif

#ifndef DISTRHO_OS_WINDOWS
# include <unistd.h>
#endif

#include <fstream>
#include <iostream>

#ifndef DISTRHO_PLUGIN_URI
# error DISTRHO_PLUGIN_URI undefined!
#endif

#ifndef DISTRHO_PLUGIN_LV2_STATE_PREFIX
# define DISTRHO_PLUGIN_LV2_STATE_PREFIX "urn:distrho:"
#endif

#ifndef DISTRHO_PLUGIN_MINIMUM_BUFFER_SIZE
# define DISTRHO_PLUGIN_MINIMUM_BUFFER_SIZE 2048
#endif

#ifndef DISTRHO_PLUGIN_USES_MODGUI
# define DISTRHO_PLUGIN_USES_MODGUI 0
#endif

#if DISTRHO_PLUGIN_HAS_EMBED_UI
# if DISTRHO_OS_HAIKU
#  define DISTRHO_LV2_UI_TYPE "BeUI"
# elif DISTRHO_OS_MAC
#  define DISTRHO_LV2_UI_TYPE "CocoaUI"
# elif DISTRHO_OS_WINDOWS
#  define DISTRHO_LV2_UI_TYPE "WindowsUI"
# else
#  define DISTRHO_LV2_UI_TYPE "X11UI"
# endif
#else
# define DISTRHO_LV2_UI_TYPE "UI"
#endif

#define DISTRHO_LV2_USE_EVENTS_IN  (DISTRHO_PLUGIN_WANT_MIDI_INPUT || DISTRHO_PLUGIN_WANT_TIMEPOS || DISTRHO_PLUGIN_WANT_STATE)
#define DISTRHO_LV2_USE_EVENTS_OUT (DISTRHO_PLUGIN_WANT_MIDI_OUTPUT || DISTRHO_PLUGIN_WANT_STATE)

#define DISTRHO_BYPASS_PARAMETER_NAME "lv2_enabled"

// -----------------------------------------------------------------------
static const char* const lv2ManifestPluginExtensionData[] =
{
    "opts:interface",
#if DISTRHO_PLUGIN_WANT_STATE
    LV2_STATE__interface,
    LV2_WORKER__interface,
#endif
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    LV2_PROGRAMS__Interface,
#endif
#ifdef DISTRHO_PLUGIN_LICENSED_FOR_MOD
    MOD_LICENSE__interface,
#endif
    nullptr
};

static const char* const lv2ManifestPluginOptionalFeatures[] =
{
#if DISTRHO_PLUGIN_IS_RT_SAFE
    LV2_CORE__hardRTCapable,
#endif
    LV2_BUF_SIZE__boundedBlockLength,
    nullptr
};

static const char* const lv2ManifestPluginRequiredFeatures[] =
{
    "opts:options",
    LV2_URID__map,
#if DISTRHO_PLUGIN_WANT_STATE
    LV2_WORKER__schedule,
#endif
#ifdef DISTRHO_PLUGIN_LICENSED_FOR_MOD
    MOD_LICENSE__feature,
#endif
    nullptr
};

static const char* const lv2ManifestPluginSupportedOptions[] =
{
    LV2_BUF_SIZE__nominalBlockLength,
    LV2_BUF_SIZE__maxBlockLength,
    LV2_PARAMETERS__sampleRate,
    nullptr
};

#if DISTRHO_PLUGIN_HAS_UI
static const char* const lv2ManifestUiExtensionData[] =
{
    "opts:interface",
    "ui:idleInterface",
    "ui:showInterface",
#if DISTRHO_PLUGIN_WANT_PROGRAMS
    LV2_PROGRAMS__UIInterface,
#endif
    nullptr
};

static const char* const lv2ManifestUiOptionalFeatures[] =
{
#if DISTRHO_PLUGIN_HAS_EMBED_UI
# if !DISTRHO_UI_USER_RESIZABLE
    "ui:noUserResize",
# endif
    "ui:parent",
    "ui:touch",
#endif
    "ui:requestValue",
    nullptr
};

static const char* const lv2ManifestUiRequiredFeatures[] =
{
    "opts:options",
    "ui:idleInterface",
#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
    LV2_DATA_ACCESS_URI,
    LV2_INSTANCE_ACCESS_URI,
#endif
    LV2_URID__map,
    nullptr
};

static const char* const lv2ManifestUiSupportedOptions[] =
{
    LV2_PARAMETERS__sampleRate,
    nullptr
};
#endif // DISTRHO_PLUGIN_HAS_UI

static void addAttribute(DISTRHO_NAMESPACE::String& text,
                         const char* const attribute,
                         const char* const values[],
                         const uint indent,
                         const bool endInDot = false)
{
    if (values[0] == nullptr)
    {
        if (endInDot)
        {
            bool found;
            const size_t index = text.rfind(';', &found);
            if (found) text[index] = '.';
        }
        return;
    }

    const size_t attributeLength = std::strlen(attribute);

    for (uint i = 0; values[i] != nullptr; ++i)
    {
        for (uint j = 0; j < indent; ++j)
            text += " ";

        if (i == 0)
        {
            text += attribute;
        }
        else
        {
            for (uint j = 0; j < attributeLength; ++j)
                text += " ";
        }

        text += " ";

        const bool isUrl = std::strstr(values[i], "://") != nullptr || std::strncmp(values[i], "urn:", 4) == 0;
        if (isUrl) text += "<";
        text += values[i];
        if (isUrl) text += ">";
        text += values[i + 1] ? " ,\n" : (endInDot ? " .\n\n" : " ;\n\n");
    }
}

// -----------------------------------------------------------------------

DISTRHO_PLUGIN_EXPORT
void lv2_generate_ttl(const char* const basename)
{
    USE_NAMESPACE_DISTRHO

    String bundlePath(getBinaryFilename());
    if (bundlePath.isNotEmpty())
    {
        bundlePath.truncate(bundlePath.rfind(DISTRHO_OS_SEP));
    }
#ifndef DISTRHO_OS_WINDOWS
    else if (char* const cwd = ::getcwd(nullptr, 0))
    {
        bundlePath = cwd;
        std::free(cwd);
    }
#endif
    d_nextBundlePath = bundlePath.buffer();

    // Dummy plugin to get data from
    d_nextBufferSize = 512;
    d_nextSampleRate = 44100.0;
    d_nextPluginIsDummy = true;
    PluginExporter plugin(nullptr, nullptr, nullptr, nullptr);
    d_nextBufferSize = 0;
    d_nextSampleRate = 0.0;
    d_nextPluginIsDummy = false;

    const String pluginDLL(basename);
    const String pluginTTL(pluginDLL + ".ttl");

#if DISTRHO_PLUGIN_HAS_UI
    String pluginUI(pluginDLL);
# if ! DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
    pluginUI.truncate(pluginDLL.rfind("_dsp"));
    pluginUI += "_ui";
    const String uiTTL(pluginUI + ".ttl");
# endif
#endif

    // ---------------------------------------------

    {
        std::cout << "Writing manifest.ttl..."; std::cout.flush();
        std::fstream manifestFile("manifest.ttl", std::ios::out);

        String manifestString;
        manifestString += "@prefix lv2:  <" LV2_CORE_PREFIX "> .\n";
        manifestString += "@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .\n";
#if DISTRHO_PLUGIN_HAS_UI && DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
        manifestString += "@prefix opts: <" LV2_OPTIONS_PREFIX "> .\n";
#endif
#if DISTRHO_PLUGIN_WANT_PROGRAMS
        manifestString += "@prefix pset: <" LV2_PRESETS_PREFIX "> .\n";
#endif
#if DISTRHO_PLUGIN_HAS_UI
        manifestString += "@prefix ui:   <" LV2_UI_PREFIX "> .\n";
#endif
        manifestString += "\n";

        manifestString += "<" DISTRHO_PLUGIN_URI ">\n";
        manifestString += "    a lv2:Plugin ;\n";
        manifestString += "    lv2:binary <" + pluginDLL + "." DISTRHO_DLL_EXTENSION "> ;\n";
#if DISTRHO_PLUGIN_USES_MODGUI
        manifestString += "    rdfs:seeAlso <" + pluginTTL + "> ,\n";
        manifestString += "                 <modgui.ttl> .\n";
#else
        manifestString += "    rdfs:seeAlso <" + pluginTTL + "> .\n";
#endif
        manifestString += "\n";

#if DISTRHO_PLUGIN_HAS_UI
        manifestString += "<" DISTRHO_UI_URI ">\n";
        manifestString += "    a ui:" DISTRHO_LV2_UI_TYPE " ;\n";
        manifestString += "    ui:binary <" + pluginUI + "." DISTRHO_DLL_EXTENSION "> ;\n";
# if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
        addAttribute(manifestString, "lv2:extensionData", lv2ManifestUiExtensionData, 4);
        addAttribute(manifestString, "lv2:optionalFeature", lv2ManifestUiOptionalFeatures, 4);
        addAttribute(manifestString, "lv2:requiredFeature", lv2ManifestUiRequiredFeatures, 4);
        addAttribute(manifestString, "opts:supportedOption", lv2ManifestUiSupportedOptions, 4, true);
# else // DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
        manifestString += "    rdfs:seeAlso <" + uiTTL + "> .\n";
# endif // DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
        manifestString += "\n";
#endif

#if DISTRHO_PLUGIN_WANT_PROGRAMS
        const String presetSeparator(std::strstr(DISTRHO_PLUGIN_URI, "#") != nullptr ? ":" : "#");

        char strBuf[0xff+1];
        strBuf[0xff] = '\0';

        String presetString;

        // Presets
        for (uint32_t i = 0; i < plugin.getProgramCount(); ++i)
        {
            std::snprintf(strBuf, 0xff, "%03i", i+1);

            presetString  = "<" DISTRHO_PLUGIN_URI + presetSeparator + "preset" + strBuf + ">\n";
            presetString += "    a pset:Preset ;\n";
            presetString += "    lv2:appliesTo <" DISTRHO_PLUGIN_URI "> ;\n";
            presetString += "    rdfs:label \"" + plugin.getProgramName(i) + "\" ;\n";
            presetString += "    rdfs:seeAlso <presets.ttl> .\n";
            presetString += "\n";

            manifestString += presetString;
        }
#endif

        manifestFile << manifestString << std::endl;
        manifestFile.close();
        std::cout << " done!" << std::endl;
    }

    // ---------------------------------------------

    {
        std::cout << "Writing " << pluginTTL << "..."; std::cout.flush();
        std::fstream pluginFile(pluginTTL, std::ios::out);

        String pluginString;

        // header
#if DISTRHO_LV2_USE_EVENTS_IN || DISTRHO_LV2_USE_EVENTS_OUT
        pluginString += "@prefix atom:  <" LV2_ATOM_PREFIX "> .\n";
#endif
        pluginString += "@prefix doap:  <http://usefulinc.com/ns/doap#> .\n";
        pluginString += "@prefix foaf:  <http://xmlns.com/foaf/0.1/> .\n";
        pluginString += "@prefix lv2:   <" LV2_CORE_PREFIX "> .\n";
        pluginString += "@prefix midi:  <" LV2_MIDI_PREFIX "> .\n";
        pluginString += "@prefix mod:   <http://moddevices.com/ns/mod#> .\n";
        pluginString += "@prefix opts:  <" LV2_OPTIONS_PREFIX "> .\n";
        pluginString += "@prefix pg:    <" LV2_PORT_GROUPS_PREFIX "> .\n";
        pluginString += "@prefix patch: <" LV2_PATCH_PREFIX "> .\n";
        pluginString += "@prefix rdf:   <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n";
        pluginString += "@prefix rdfs:  <http://www.w3.org/2000/01/rdf-schema#> .\n";
#if DISTRHO_LV2_USE_EVENTS_IN || DISTRHO_LV2_USE_EVENTS_OUT
        pluginString += "@prefix rsz:   <" LV2_RESIZE_PORT_PREFIX "> .\n";
#endif
        pluginString += "@prefix spdx:  <http://spdx.org/rdf/terms#> .\n";
#if DISTRHO_PLUGIN_HAS_UI
        pluginString += "@prefix ui:    <" LV2_UI_PREFIX "> .\n";
#endif
        pluginString += "@prefix unit:  <" LV2_UNITS_PREFIX "> .\n";
        pluginString += "\n";

#if DISTRHO_PLUGIN_WANT_STATE
        bool hasHostVisibleState = false;

        for (uint32_t i=0, count=plugin.getStateCount(); i < count; ++i)
        {
            const uint32_t hints = plugin.getStateHints(i);

            if ((hints & kStateIsHostReadable) == 0x0)
                continue;

            pluginString += "<" DISTRHO_PLUGIN_URI "#" + plugin.getStateKey(i) + ">\n";
            pluginString += "    a lv2:Parameter ;\n";
            pluginString += "    rdfs:label \"" + plugin.getStateLabel(i) + "\" ;\n";

            const String& comment(plugin.getStateDescription(i));

            if (comment.isNotEmpty())
            {
                if (comment.contains('"') || comment.contains('\n'))
                    pluginString += "    rdfs:comment \"\"\"" + comment + "\"\"\" ;\n";
                else
                    pluginString += "    rdfs:comment \"" + comment + "\" ;\n";
            }

            if ((hints & kStateIsFilenamePath) == kStateIsFilenamePath)
                pluginString += "    rdfs:range atom:Path .\n\n";
            else
                pluginString += "    rdfs:range atom:String .\n\n";

            hasHostVisibleState = true;
        }
#endif

        // plugin
        pluginString += "<" DISTRHO_PLUGIN_URI ">\n";
#ifdef DISTRHO_PLUGIN_LV2_CATEGORY
        pluginString += "    a " DISTRHO_PLUGIN_LV2_CATEGORY ", lv2:Plugin, doap:Project ;\n";
#elif DISTRHO_PLUGIN_IS_SYNTH
        pluginString += "    a lv2:InstrumentPlugin, lv2:Plugin, doap:Project ;\n";
#else
        pluginString += "    a lv2:Plugin, doap:Project ;\n";
#endif
        pluginString += "\n";

        addAttribute(pluginString, "lv2:extensionData", lv2ManifestPluginExtensionData, 4);
        addAttribute(pluginString, "lv2:optionalFeature", lv2ManifestPluginOptionalFeatures, 4);
        addAttribute(pluginString, "lv2:requiredFeature", lv2ManifestPluginRequiredFeatures, 4);
        addAttribute(pluginString, "opts:supportedOption", lv2ManifestPluginSupportedOptions, 4);

#if DISTRHO_PLUGIN_WANT_STATE
        if (hasHostVisibleState)
        {
            for (uint32_t i=0, count=plugin.getStateCount(); i < count; ++i)
            {
                const uint32_t hints = plugin.getStateHints(i);

                if ((hints & kStateIsHostReadable) == 0x0)
                    continue;

                const String& key(plugin.getStateKey(i));

                if ((hints & kStateIsHostWritable) == kStateIsHostWritable)
                    pluginString += "    patch:writable <" DISTRHO_PLUGIN_URI "#" + key + "> ;\n";
                else
                    pluginString += "    patch:readable <" DISTRHO_PLUGIN_URI "#" + key + "> ;\n";
            }
            pluginString += "\n";
        }
#endif

        // UI
#if DISTRHO_PLUGIN_HAS_UI
        pluginString += "    ui:ui <" DISTRHO_UI_URI "> ;\n";
        pluginString += "\n";
#endif

        {
            uint32_t portIndex = 0;

#if DISTRHO_PLUGIN_NUM_INPUTS > 0
            for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_INPUTS; ++i, ++portIndex)
            {
                const AudioPort& port(plugin.getAudioPort(true, i));
                const bool cvPortScaled = port.hints & kCVPortHasScaledRange;

                if (i == 0)
                    pluginString += "    lv2:port [\n";
                else
                    pluginString += "    [\n";

                if (cvPortScaled)
                    pluginString += "        a lv2:InputPort, lv2:CVPort, mod:CVPort ;\n";
                else if (port.hints & kAudioPortIsCV)
                    pluginString += "        a lv2:InputPort, lv2:CVPort ;\n";
                else
                    pluginString += "        a lv2:InputPort, lv2:AudioPort ;\n";

                pluginString += "        lv2:index " + String(portIndex) + " ;\n";
                pluginString += "        lv2:symbol \"lv2_" + port.symbol + "\" ;\n";
                pluginString += "        lv2:name \"" + port.name + "\" ;\n";

                if (port.hints & kAudioPortIsSidechain)
                    pluginString += "        lv2:portProperty lv2:isSideChain;\n";

                if (port.groupId != kPortGroupNone)
                {
                    pluginString += "        pg:group <" DISTRHO_PLUGIN_URI "#portGroup_"
                                    + plugin.getPortGroupSymbolForId(port.groupId) + "> ;\n";

                    switch (port.groupId)
                    {
                    case kPortGroupMono:
                        pluginString += "        lv2:designation pg:center ;\n";
                        break;
                    case kPortGroupStereo:
                        if (i == 1)
                            pluginString += "        lv2:designation pg:right ;\n";
                        else
                            pluginString += "        lv2:designation pg:left ;\n";
                        break;
                    }
                }

                // set ranges
                if (port.hints & kCVPortHasBipolarRange)
                {
                    if (cvPortScaled)
                    {
                        pluginString += "        lv2:minimum -5.0 ;\n";
                        pluginString += "        lv2:maximum 5.0 ;\n";
                    }
                    else
                    {
                        pluginString += "        lv2:minimum -1.0 ;\n";
                        pluginString += "        lv2:maximum 1.0 ;\n";
                    }
                }
                else if (port.hints & kCVPortHasNegativeUnipolarRange)
                {
                    if (cvPortScaled)
                    {
                        pluginString += "        lv2:minimum -10.0 ;\n";
                        pluginString += "        lv2:maximum 0.0 ;\n";
                    }
                    else
                    {
                        pluginString += "        lv2:minimum -1.0 ;\n";
                        pluginString += "        lv2:maximum 0.0 ;\n";
                    }
                }
                else if (port.hints & kCVPortHasPositiveUnipolarRange)
                {
                    if (cvPortScaled)
                    {
                        pluginString += "        lv2:minimum 0.0 ;\n";
                        pluginString += "        lv2:maximum 10.0 ;\n";
                    }
                    else
                    {
                        pluginString += "        lv2:minimum 0.0 ;\n";
                        pluginString += "        lv2:maximum 1.0 ;\n";
                    }
                }

                if ((port.hints & (kAudioPortIsCV|kCVPortIsOptional)) == (kAudioPortIsCV|kCVPortIsOptional))
                    pluginString += "        lv2:portProperty lv2:connectionOptional;\n";

                if (i+1 == DISTRHO_PLUGIN_NUM_INPUTS)
                    pluginString += "    ] ;\n";
                else
                    pluginString += "    ] ,\n";
            }
            pluginString += "\n";
#endif

#if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
            for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i, ++portIndex)
            {
                const AudioPort& port(plugin.getAudioPort(false, i));
                const bool cvPortScaled = port.hints & kCVPortHasScaledRange;

                if (i == 0)
                    pluginString += "    lv2:port [\n";
                else
                    pluginString += "    [\n";

                if (cvPortScaled)
                    pluginString += "        a lv2:OutputPort, lv2:CVPort, mod:CVPort ;\n";
                else if (port.hints & kAudioPortIsCV)
                    pluginString += "        a lv2:OutputPort, lv2:CVPort ;\n";
                else
                    pluginString += "        a lv2:OutputPort, lv2:AudioPort ;\n";

                pluginString += "        lv2:index " + String(portIndex) + " ;\n";
                pluginString += "        lv2:symbol \"lv2_" + port.symbol + "\" ;\n";
                pluginString += "        lv2:name \"" + port.name + "\" ;\n";

                if (port.hints & kAudioPortIsSidechain)
                    pluginString += "        lv2:portProperty lv2:isSideChain;\n";

                if (port.groupId != kPortGroupNone)
                {
                    pluginString += "        pg:group <" DISTRHO_PLUGIN_URI "#portGroup_"
                                    + plugin.getPortGroupSymbolForId(port.groupId) + "> ;\n";

                    switch (port.groupId)
                    {
                    case kPortGroupMono:
                        pluginString += "        lv2:designation pg:center ;\n";
                        break;
                    case kPortGroupStereo:
                        if (i == 1)
                            pluginString += "        lv2:designation pg:right ;\n";
                        else
                            pluginString += "        lv2:designation pg:left ;\n";
                        break;
                    }
                }

                // set ranges
                if (port.hints & kCVPortHasBipolarRange)
                {
                    if (cvPortScaled)
                    {
                        pluginString += "        lv2:minimum -5.0 ;\n";
                        pluginString += "        lv2:maximum 5.0 ;\n";
                    }
                    else
                    {
                        pluginString += "        lv2:minimum -1.0 ;\n";
                        pluginString += "        lv2:maximum 1.0 ;\n";
                    }
                }
                else if (port.hints & kCVPortHasNegativeUnipolarRange)
                {
                    if (cvPortScaled)
                    {
                        pluginString += "        lv2:minimum -10.0 ;\n";
                        pluginString += "        lv2:maximum 0.0 ;\n";
                    }
                    else
                    {
                        pluginString += "        lv2:minimum -1.0 ;\n";
                        pluginString += "        lv2:maximum 0.0 ;\n";
                    }
                }
                else if (port.hints & kCVPortHasPositiveUnipolarRange)
                {
                    if (cvPortScaled)
                    {
                        pluginString += "        lv2:minimum 0.0 ;\n";
                        pluginString += "        lv2:maximum 10.0 ;\n";
                    }
                    else
                    {
                        pluginString += "        lv2:minimum 0.0 ;\n";
                        pluginString += "        lv2:maximum 1.0 ;\n";
                    }
                }

                if (i+1 == DISTRHO_PLUGIN_NUM_OUTPUTS)
                    pluginString += "    ] ;\n";
                else
                    pluginString += "    ] ,\n";
            }
            pluginString += "\n";
#endif

#if DISTRHO_LV2_USE_EVENTS_IN
            pluginString += "    lv2:port [\n";
            pluginString += "        a lv2:InputPort, atom:AtomPort ;\n";
            pluginString += "        lv2:index " + String(portIndex) + " ;\n";
            pluginString += "        lv2:name \"Events Input\" ;\n";
            pluginString += "        lv2:symbol \"lv2_events_in\" ;\n";
            pluginString += "        rsz:minimumSize " + String(DISTRHO_PLUGIN_MINIMUM_BUFFER_SIZE) + " ;\n";
            pluginString += "        atom:bufferType atom:Sequence ;\n";
# if (DISTRHO_PLUGIN_WANT_STATE && DISTRHO_PLUGIN_HAS_UI)
            pluginString += "        atom:supports atom:String ;\n";
# endif
# if DISTRHO_PLUGIN_WANT_MIDI_INPUT
            pluginString += "        atom:supports midi:MidiEvent ;\n";
# endif
# if DISTRHO_PLUGIN_WANT_TIMEPOS
            pluginString += "        atom:supports <" LV2_TIME__Position "> ;\n";
# endif
# if DISTRHO_PLUGIN_WANT_STATE
            if (hasHostVisibleState)
            {
                pluginString += "        atom:supports <" LV2_PATCH__Message "> ;\n";
                pluginString += "        lv2:designation lv2:control ;\n";
            }
# endif
            pluginString += "    ] ;\n\n";
            ++portIndex;
#endif

#if DISTRHO_LV2_USE_EVENTS_OUT
            pluginString += "    lv2:port [\n";
            pluginString += "        a lv2:OutputPort, atom:AtomPort ;\n";
            pluginString += "        lv2:index " + String(portIndex) + " ;\n";
            pluginString += "        lv2:name \"Events Output\" ;\n";
            pluginString += "        lv2:symbol \"lv2_events_out\" ;\n";
            pluginString += "        rsz:minimumSize " + String(DISTRHO_PLUGIN_MINIMUM_BUFFER_SIZE) + " ;\n";
            pluginString += "        atom:bufferType atom:Sequence ;\n";
# if (DISTRHO_PLUGIN_WANT_STATE && DISTRHO_PLUGIN_HAS_UI)
            pluginString += "        atom:supports atom:String ;\n";
# endif
# if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
            pluginString += "        atom:supports midi:MidiEvent ;\n";
# endif
# if DISTRHO_PLUGIN_WANT_STATE
            if (hasHostVisibleState)
            {
                pluginString += "        atom:supports <" LV2_PATCH__Message "> ;\n";
                pluginString += "        lv2:designation lv2:control ;\n";
            }
# endif
            pluginString += "    ] ;\n\n";
            ++portIndex;
#endif

#if DISTRHO_PLUGIN_WANT_LATENCY
            pluginString += "    lv2:port [\n";
            pluginString += "        a lv2:OutputPort, lv2:ControlPort ;\n";
            pluginString += "        lv2:index " + String(portIndex) + " ;\n";
            pluginString += "        lv2:name \"Latency\" ;\n";
            pluginString += "        lv2:symbol \"lv2_latency\" ;\n";
            pluginString += "        lv2:designation lv2:latency ;\n";
            pluginString += "        lv2:portProperty lv2:reportsLatency, lv2:integer, <" LV2_PORT_PROPS__notOnGUI "> ;\n";
            pluginString += "    ] ;\n\n";
            ++portIndex;
#endif

            for (uint32_t i=0, count=plugin.getParameterCount(); i < count; ++i, ++portIndex)
            {
                if (i == 0)
                    pluginString += "    lv2:port [\n";
                else
                    pluginString += "    [\n";

                if (plugin.isParameterOutput(i))
                    pluginString += "        a lv2:OutputPort, lv2:ControlPort ;\n";
                else
                    pluginString += "        a lv2:InputPort, lv2:ControlPort ;\n";

                pluginString += "        lv2:index " + String(portIndex) + " ;\n";

                bool designated = false;

                // designation
                if (plugin.isParameterInput(i))
                {
                    switch (plugin.getParameterDesignation(i))
                    {
                    case kParameterDesignationNull:
                        break;
                    case kParameterDesignationBypass:
                        designated = true;
                        pluginString += "        lv2:name \"Enabled\" ;\n";
                        pluginString += "        lv2:symbol \"" DISTRHO_BYPASS_PARAMETER_NAME "\" ;\n";
                        pluginString += "        lv2:default 1 ;\n";
                        pluginString += "        lv2:minimum 0 ;\n";
                        pluginString += "        lv2:maximum 1 ;\n";
                        pluginString += "        lv2:portProperty lv2:toggled , lv2:integer ;\n";
                        pluginString += "        lv2:designation lv2:enabled ;\n";
                        break;
                    }
                }

                if (! designated)
                {
                    // name and symbol
                    const String& paramName(plugin.getParameterName(i));

                    if (paramName.contains('"'))
                        pluginString += "        lv2:name \"\"\"" + paramName + "\"\"\" ;\n";
                    else
                        pluginString += "        lv2:name \"" + paramName + "\" ;\n";

                    String symbol(plugin.getParameterSymbol(i));

                    if (symbol.isEmpty())
                        symbol = "lv2_port_" + String(portIndex-1);

                    pluginString += "        lv2:symbol \"" + symbol + "\" ;\n";

                    // short name
                    const String& shortName(plugin.getParameterShortName(i));

                    if (shortName.isNotEmpty())
                        pluginString += "        lv2:shortName \"\"\"" + shortName + "\"\"\" ;\n";

                    // ranges
                    const ParameterRanges& ranges(plugin.getParameterRanges(i));

                    if (plugin.getParameterHints(i) & kParameterIsInteger)
                    {
                        if (plugin.isParameterInput(i))
                            pluginString += "        lv2:default " + String(int(ranges.def)) + " ;\n";
                        pluginString += "        lv2:minimum " + String(int(ranges.min)) + " ;\n";
                        pluginString += "        lv2:maximum " + String(int(ranges.max)) + " ;\n";
                    }
                    else
                    {
                        if (plugin.isParameterInput(i))
                            pluginString += "        lv2:default " + String(ranges.def) + " ;\n";
                        pluginString += "        lv2:minimum " + String(ranges.min) + " ;\n";
                        pluginString += "        lv2:maximum " + String(ranges.max) + " ;\n";
                    }

                    // enumeration
                    const ParameterEnumerationValues& enumValues(plugin.getParameterEnumValues(i));

                    if (enumValues.count > 0)
                    {
                        if (enumValues.count >= 2 && enumValues.restrictedMode)
                            pluginString += "        lv2:portProperty lv2:enumeration ;\n";

                        for (uint8_t j=0; j < enumValues.count; ++j)
                        {
                            const ParameterEnumerationValue& enumValue(enumValues.values[j]);

                            if (j == 0)
                                pluginString += "        lv2:scalePoint [\n";
                            else
                                pluginString += "        [\n";

                            if (enumValue.label.contains('"'))
                                pluginString += "            rdfs:label  \"\"\"" + enumValue.label + "\"\"\" ;\n";
                            else
                                pluginString += "            rdfs:label  \"" + enumValue.label + "\" ;\n";

                            if (plugin.getParameterHints(i) & kParameterIsInteger)
                            {
                                const int rounded = (int)(enumValue.value + (enumValue.value < 0.0f ? -0.5f : 0.5f));
                                pluginString += "            rdf:value " + String(rounded) + " ;\n";
                            }
                            else
                            {
                                pluginString += "            rdf:value " + String(enumValue.value) + " ;\n";
                            }

                            if (j+1 == enumValues.count)
                                pluginString += "        ] ;\n";
                            else
                                pluginString += "        ] ,\n";
                        }
                    }

                    // MIDI CC binding
                    if (const uint8_t midiCC = plugin.getParameterMidiCC(i))
                    {
                        pluginString += "        midi:binding [\n";
                        pluginString += "            a midi:Controller ;\n";
                        pluginString += "            midi:controllerNumber " + String(midiCC) + " ;\n";
                        pluginString += "        ] ;\n";
                    }

                    // unit
                    const String& unit(plugin.getParameterUnit(i));

                    if (unit.isNotEmpty() && ! unit.contains(' '))
                    {
                        String lunit(unit);
                        lunit.toLower();

                        /**/ if (lunit == "db")
                        {
                            pluginString += "        unit:unit unit:db ;\n";
                        }
                        else if (lunit == "hz")
                        {
                            pluginString += "        unit:unit unit:hz ;\n";
                        }
                        else if (lunit == "khz")
                        {
                            pluginString += "        unit:unit unit:khz ;\n";
                        }
                        else if (lunit == "mhz")
                        {
                            pluginString += "        unit:unit unit:mhz ;\n";
                        }
                        else if (lunit == "ms")
                        {
                            pluginString += "        unit:unit unit:ms ;\n";
                        }
                        else if (lunit == "s")
                        {
                            pluginString += "        unit:unit unit:s ;\n";
                        }
                        else if (lunit == "%")
                        {
                            pluginString += "        unit:unit unit:pc ;\n";
                        }
                        else
                        {
                            pluginString += "        unit:unit [\n";
                            pluginString += "            a unit:Unit ;\n";
                            pluginString += "            rdfs:label  \"" + unit + "\" ;\n";
                            pluginString += "            unit:symbol \"" + unit + "\" ;\n";
                            if (plugin.getParameterHints(i) & kParameterIsInteger)
                                pluginString += "            unit:render \"%d " + unit + "\" ;\n";
                            else
                                pluginString += "            unit:render \"%f " + unit + "\" ;\n";
                            pluginString += "        ] ;\n";
                        }
                    }

                    // comment
                    const String& comment(plugin.getParameterDescription(i));

                    if (comment.isNotEmpty())
                    {
                        if (comment.contains('"') || comment.contains('\n'))
                            pluginString += "        rdfs:comment \"\"\"" + comment + "\"\"\" ;\n";
                        else
                            pluginString += "        rdfs:comment \"" + comment + "\" ;\n";
                    }

                    // hints
                    const uint32_t hints = plugin.getParameterHints(i);

                    if (hints & kParameterIsBoolean)
                    {
                        if ((hints & kParameterIsTrigger) == kParameterIsTrigger)
                            pluginString += "        lv2:portProperty <" LV2_PORT_PROPS__trigger "> ;\n";
                        pluginString += "        lv2:portProperty lv2:toggled ;\n";
                    }
                    if (hints & kParameterIsInteger)
                        pluginString += "        lv2:portProperty lv2:integer ;\n";
                    if (hints & kParameterIsLogarithmic)
                        pluginString += "        lv2:portProperty <" LV2_PORT_PROPS__logarithmic "> ;\n";
                    if ((hints & kParameterIsAutomatable) == 0 && plugin.isParameterInput(i))
                    {
                        pluginString += "        lv2:portProperty <" LV2_PORT_PROPS__expensive "> ,\n";
                        pluginString += "                         <" LV2_KXSTUDIO_PROPERTIES__NonAutomatable "> ;\n";
                    }

                    // group
                    const uint32_t groupId = plugin.getParameterGroupId(i);

                    if (groupId != kPortGroupNone)
                        pluginString += "        pg:group <" DISTRHO_PLUGIN_URI "#portGroup_"
                                        + plugin.getPortGroupSymbolForId(groupId) + "> ;\n";

                } // ! designated

                if (i+1 == count)
                    pluginString += "    ] ;\n\n";
                else
                    pluginString += "    ] ,\n";
            }
        }

        // comment
        {
            const String comment(plugin.getDescription());

            if (comment.isNotEmpty())
            {
                if (comment.contains('"') || comment.contains('\n'))
                    pluginString += "    rdfs:comment \"\"\"" + comment + "\"\"\" ;\n\n";
                else
                    pluginString += "    rdfs:comment \"" + comment + "\" ;\n\n";
            }
        }

#ifdef DISTRHO_PLUGIN_BRAND
        // MOD
        pluginString += "    mod:brand \"" DISTRHO_PLUGIN_BRAND "\" ;\n";
        pluginString += "    mod:label \"" DISTRHO_PLUGIN_NAME "\" ;\n\n";
#endif

        // name
        {
            const String name(plugin.getName());

            if (name.contains('"'))
                pluginString += "    doap:name \"\"\"" + name + "\"\"\" ;\n";
            else
                pluginString += "    doap:name \"" + name + "\" ;\n";
        }

        // license
        {
            const String license(plugin.getLicense());

            // Using URL as license
            if (license.contains("://"))
            {
                pluginString += "    doap:license <" +  license + "> ;\n\n";
            }
            // String contaning quotes, use as-is
            else if (license.contains('"'))
            {
                pluginString += "    doap:license \"\"\"" +  license + "\"\"\" ;\n\n";
            }
            // Regular license string, convert to URL as much as we can
            else
            {
                const String uplicense(license.asUpper());

                // for reference, see https://spdx.org/licenses/

                // common licenses
                /**/ if (uplicense == "AGPL-1.0-ONLY" ||
                         uplicense == "AGPL1" ||
                         uplicense == "AGPLV1")
                {
                    pluginString += "    doap:license <http://spdx.org/licenses/AGPL-1.0-only.html> ;\n\n";
                }
                else if (uplicense == "AGPL-1.0-OR-LATER" ||
                         uplicense == "AGPL1+" ||
                         uplicense == "AGPLV1+")
                {
                    pluginString += "    doap:license <http://spdx.org/licenses/AGPL-1.0-or-later.html> ;\n\n";
                }
                else if (uplicense == "AGPL-3.0-ONLY" ||
                         uplicense == "AGPL3" ||
                         uplicense == "AGPLV3")
                {
                    pluginString += "    doap:license <http://spdx.org/licenses/AGPL-3.0-only.html> ;\n\n";
                }
                else if (uplicense == "AGPL-3.0-OR-LATER" ||
                         uplicense == "AGPL3+" ||
                         uplicense == "AGPLV3+")
                {
                    pluginString += "    doap:license <http://spdx.org/licenses/AGPL-3.0-or-later.html> ;\n\n";
                }
                else if (uplicense == "APACHE-2.0" ||
                         uplicense == "APACHE2" ||
                         uplicense == "APACHE-2")
                {
                    pluginString += "    doap:license <http://spdx.org/licenses/Apache-2.0.html> ;\n\n";
                }
                else if (uplicense == "BSD-2-CLAUSE" ||
                         uplicense == "BSD2" ||
                         uplicense == "BSD-2")
                {
                    pluginString += "    doap:license <http://spdx.org/licenses/BSD-2-Clause.html> ;\n\n";
                }
                else if (uplicense == "BSD-3-CLAUSE" ||
                         uplicense == "BSD3" ||
                         uplicense == "BSD-3")
                {
                    pluginString += "    doap:license <http://spdx.org/licenses/BSD-3-Clause.html> ;\n\n";
                }
                else if (uplicense == "GPL-2.0-ONLY" ||
                         uplicense == "GPL2" ||
                         uplicense == "GPLV2")
                {
                    pluginString += "    doap:license <http://spdx.org/licenses/GPL-2.0-only.html> ;\n\n";
                }
                else if (uplicense == "GPL-2.0-OR-LATER" ||
                         uplicense == "GPL2+" ||
                         uplicense == "GPLV2+" ||
                         uplicense == "GPLV2.0+" ||
                         uplicense == "GPL V2+")
                {
                    pluginString += "    doap:license <http://spdx.org/licenses/GPL-2.0-or-later.html> ;\n\n";
                }
                else if (uplicense == "GPL-3.0-ONLY" ||
                         uplicense == "GPL3" ||
                         uplicense == "GPLV3")
                {
                    pluginString += "    doap:license <http://spdx.org/licenses/GPL-3.0-only.html> ;\n\n";
                }
                else if (uplicense == "GPL-3.0-OR-LATER" ||
                         uplicense == "GPL3+" ||
                         uplicense == "GPLV3+" ||
                         uplicense == "GPLV3.0+" ||
                         uplicense == "GPL V3+")
                {
                    pluginString += "    doap:license <http://spdx.org/licenses/GPL-3.0-or-later.html> ;\n\n";
                }
                else if (uplicense == "ISC")
                {
                    pluginString += "    doap:license <http://spdx.org/licenses/ISC.html> ;\n\n";
                }
                else if (uplicense == "LGPL-2.0-ONLY" ||
                         uplicense == "LGPL2" ||
                         uplicense == "LGPLV2")
                {
                    pluginString += "    doap:license <http://spdx.org/licenses/LGPL-2.0-only.html> ;\n\n";
                }
                else if (uplicense == "LGPL-2.0-OR-LATER" ||
                         uplicense == "LGPL2+" ||
                         uplicense == "LGPLV2+")
                {
                    pluginString += "    doap:license <http://spdx.org/licenses/LGPL-2.0-or-later.html> ;\n\n";
                }
                else if (uplicense == "LGPL-2.1-ONLY" ||
                         uplicense == "LGPL2.1" ||
                         uplicense == "LGPLV2.1")
                {
                    pluginString += "    doap:license <http://spdx.org/licenses/LGPL-2.1-only.html> ;\n\n";
                }
                else if (uplicense == "LGPL-2.1-OR-LATER" ||
                         uplicense == "LGPL2.1+" ||
                         uplicense == "LGPLV2.1+")
                {
                    pluginString += "    doap:license <http://spdx.org/licenses/LGPL-2.1-or-later.html> ;\n\n";
                }
                else if (uplicense == "LGPL-3.0-ONLY" ||
                         uplicense == "LGPL3" ||
                         uplicense == "LGPLV3")
                {
                    pluginString += "    doap:license <http://spdx.org/licenses/LGPL-2.0-only.html> ;\n\n";
                }
                else if (uplicense == "LGPL-3.0-OR-LATER" ||
                         uplicense == "LGPL3+" ||
                         uplicense == "LGPLV3+")
                {
                    pluginString += "    doap:license <http://spdx.org/licenses/LGPL-3.0-or-later.html> ;\n\n";
                }
                else if (uplicense == "MIT")
                {
                    pluginString += "    doap:license <http://spdx.org/licenses/MIT.html> ;\n\n";
                }

                // generic fallbacks
                else if (uplicense.startsWith("GPL"))
                {
                    pluginString += "    doap:license <http://opensource.org/licenses/gpl-license> ;\n\n";
                }
                else if (uplicense.startsWith("LGPL"))
                {
                    pluginString += "    doap:license <http://opensource.org/licenses/lgpl-license> ;\n\n";
                }

                // unknown or not handled yet, log a warning
                else
                {
                    d_stderr("Unknown license string '%s'", license.buffer());
                    pluginString += "    doap:license \"" +  license + "\" ;\n\n";
                }
            }
        }

        // developer
        {
            const String homepage(plugin.getHomePage());
            const String maker(plugin.getMaker());

            pluginString += "    doap:maintainer [\n";

            if (maker.contains('"'))
                pluginString += "        foaf:name \"\"\"" + maker + "\"\"\" ;\n";
            else
                pluginString += "        foaf:name \"" + maker + "\" ;\n";

            if (homepage.isNotEmpty())
                pluginString += "        foaf:homepage <" + homepage + "> ;\n";

            pluginString += "    ] ;\n\n";
        }

        {
            const uint32_t version(plugin.getVersion());

            const uint32_t majorVersion = (version & 0xFF0000) >> 16;
            /* */ uint32_t minorVersion = (version & 0x00FF00) >> 8;
            const uint32_t microVersion = (version & 0x0000FF) >> 0;

            // NOTE: LV2 ignores 'major' version and says 0 for minor is pre-release/unstable.
            if (majorVersion > 0)
                minorVersion += 2;

            pluginString += "    lv2:microVersion " + String(microVersion) + " ;\n";
            pluginString += "    lv2:minorVersion " + String(minorVersion) + " .\n";
        }

        // port groups
        if (const uint32_t portGroupCount = plugin.getPortGroupCount())
        {
            bool isInput, isOutput;

            for (uint32_t i = 0; i < portGroupCount; ++i)
            {
                const PortGroupWithId& portGroup(plugin.getPortGroupByIndex(i));
                DISTRHO_SAFE_ASSERT_CONTINUE(portGroup.groupId != kPortGroupNone);
                DISTRHO_SAFE_ASSERT_CONTINUE(portGroup.symbol.isNotEmpty());

                pluginString += "\n<" DISTRHO_PLUGIN_URI "#portGroup_" + portGroup.symbol + ">\n";
                isInput = isOutput = false;

#if DISTRHO_PLUGIN_NUM_INPUTS > 0
                for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_INPUTS && !isInput; ++i)
                    isInput = plugin.getAudioPort(true, i).groupId == portGroup.groupId;
#endif

#if DISTRHO_PLUGIN_NUM_OUTPUTS > 0
                for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS && !isOutput; ++i)
                    isOutput = plugin.getAudioPort(false, i).groupId == portGroup.groupId;
#endif

                for (uint32_t i=0, count=plugin.getParameterCount(); i < count && (!isInput || !isOutput); ++i)
                {
                    if (plugin.getParameterGroupId(i) == portGroup.groupId)
                    {
                        isInput = isInput || plugin.isParameterInput(i);
                        isOutput = isOutput || plugin.isParameterOutput(i);
                    }
                }

                pluginString += "    a ";

                if (isInput && !isOutput)
                    pluginString += "pg:InputGroup";
                else if (isOutput && !isInput)
                    pluginString += "pg:OutputGroup";
                else
                    pluginString += "pg:Group";

                switch (portGroup.groupId)
                {
                case kPortGroupMono:
                    pluginString += " , pg:MonoGroup";
                    break;
                case kPortGroupStereo:
                    pluginString += " , pg:StereoGroup";
                    break;
                }

                pluginString += " ;\n";

                // pluginString += "    rdfs:label \"" + portGroup.name + "\" ;\n";
                pluginString += "    lv2:name \"" + portGroup.name + "\" ;\n";
                pluginString += "    lv2:symbol \"" + portGroup.symbol + "\" .\n";
            }
        }

        pluginFile << pluginString << std::endl;
        pluginFile.close();
        std::cout << " done!" << std::endl;
    }

    // ---------------------------------------------

#if DISTRHO_PLUGIN_HAS_UI && ! DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
    {
        std::cout << "Writing " << uiTTL << "..."; std::cout.flush();
        std::fstream uiFile(uiTTL, std::ios::out);

        String uiString;
        uiString += "@prefix lv2:  <" LV2_CORE_PREFIX "> .\n";
        uiString += "@prefix ui:   <" LV2_UI_PREFIX "> .\n";
        uiString += "@prefix opts: <" LV2_OPTIONS_PREFIX "> .\n";
        uiString += "\n";

        uiString += "<" DISTRHO_UI_URI ">\n";

        addAttribute(uiString, "lv2:extensionData", lv2ManifestUiExtensionData, 4);
        addAttribute(uiString, "lv2:optionalFeature", lv2ManifestUiOptionalFeatures, 4);
        addAttribute(uiString, "lv2:requiredFeature", lv2ManifestUiRequiredFeatures, 4);
        addAttribute(uiString, "opts:supportedOption", lv2ManifestUiSupportedOptions, 4, true);

        uiFile << uiString << std::endl;
        uiFile.close();
        std::cout << " done!" << std::endl;
    }
#endif

    // ---------------------------------------------

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    {
        std::cout << "Writing presets.ttl..."; std::cout.flush();
        std::fstream presetsFile("presets.ttl", std::ios::out);

        String presetsString;
        presetsString += "@prefix lv2:   <" LV2_CORE_PREFIX "> .\n";
        presetsString += "@prefix pset:  <" LV2_PRESETS_PREFIX "> .\n";
# if DISTRHO_PLUGIN_WANT_STATE
        presetsString += "@prefix owl:   <http://www.w3.org/2002/07/owl#> .\n";
        presetsString += "@prefix rdfs:  <http://www.w3.org/2000/01/rdf-schema#> .\n";
        presetsString += "@prefix state: <" LV2_STATE_PREFIX "> .\n";
        presetsString += "@prefix xsd:   <http://www.w3.org/2001/XMLSchema#> .\n";
# endif
        presetsString += "\n";

        const uint32_t numParameters = plugin.getParameterCount();
        const uint32_t numPrograms   = plugin.getProgramCount();
# if DISTRHO_PLUGIN_WANT_FULL_STATE
        const uint32_t numStates     = plugin.getStateCount();
        const bool     valid         = numParameters != 0 || numStates != 0;
# else
        const bool     valid         = numParameters != 0;
# endif

        DISTRHO_CUSTOM_SAFE_ASSERT_RETURN("Programs require parameters or full state", valid, presetsFile.close());

        const String presetSeparator(std::strstr(DISTRHO_PLUGIN_URI, "#") != nullptr ? ":" : "#");

        char strBuf[0xff+1];
        strBuf[0xff] = '\0';

        String presetString;

# if DISTRHO_PLUGIN_WANT_FULL_STATE
        for (uint32_t i=0; i<numStates; ++i)
        {
            if (plugin.getStateHints(i) & kStateIsHostReadable)
                continue;

            // readable states are defined as lv2 parameters.
            // non-readable states have no definition, but one is needed for presets and ttl validation.
            presetString  = "<" DISTRHO_PLUGIN_LV2_STATE_PREFIX + plugin.getStateKey(i) + ">\n";
            presetString += "    a owl:DatatypeProperty ;\n";
            presetString += "    rdfs:label \"Plugin state key-value string pair\" ;\n";
            presetString += "    rdfs:domain state:State ;\n";
            presetString += "    rdfs:range xsd:string .\n\n";
            presetsString += presetString;
        }
# endif

        for (uint32_t i=0; i<numPrograms; ++i)
        {
            std::snprintf(strBuf, 0xff, "%03i", i+1);

            plugin.loadProgram(i);

            presetString = "<" DISTRHO_PLUGIN_URI + presetSeparator + "preset" + strBuf + ">\n";

# if DISTRHO_PLUGIN_WANT_FULL_STATE
            presetString += "    state:state [\n";
            for (uint32_t j=0; j<numStates; ++j)
            {
                const String key   = plugin.getStateKey(j);
                const String value = plugin.getStateValue(key);

                presetString += "        <";

                if (plugin.getStateHints(i) & kStateIsHostReadable)
                    presetString += DISTRHO_PLUGIN_URI "#";
                else
                    presetString += DISTRHO_PLUGIN_LV2_STATE_PREFIX;

                presetString += key + ">";

                if (value.length() < 10)
                    presetString += " \"" + value + "\" ;\n";
                else
                    presetString += "\n\"\"\"" + value + "\"\"\" ;\n";
            }

            if (numParameters > 0)
                presetString += "    ] ;\n\n";
            else
                presetString += "    ] .\n\n";
# endif

            bool firstParameter = true;

            for (uint32_t j=0; j <numParameters; ++j)
            {
                if (plugin.isParameterOutput(j))
                    continue;

                if (firstParameter)
                {
                    presetString += "    lv2:port [\n";
                    firstParameter = false;
                }
                else
                {
                    presetString += "    [\n";
                }

                String parameterSymbol = plugin.getParameterSymbol(j);
                float parameterValue = plugin.getParameterValue(j);

                if (plugin.getParameterDesignation(j) == kParameterDesignationBypass)
                {
                    parameterSymbol = DISTRHO_BYPASS_PARAMETER_NAME;
                    parameterValue = 1.0f - parameterValue;
                }

                presetString += "        lv2:symbol \"" + parameterSymbol + "\" ;\n";

                if (plugin.getParameterHints(j) & kParameterIsInteger)
                    presetString += "        pset:value " + String(int(parameterValue)) + " ;\n";
                else
                    presetString += "        pset:value " + String(parameterValue) + " ;\n";

                if (j+1 == numParameters || plugin.isParameterOutput(j+1))
                    presetString += "    ] .\n\n";
                else
                    presetString += "    ] ,\n";
            }

            presetsString += presetString;
        }

        presetsFile << presetsString << std::endl;
        presetsFile.close();
        std::cout << " done!" << std::endl;
    }
#endif
}
