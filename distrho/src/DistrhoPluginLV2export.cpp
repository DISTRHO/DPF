/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2023 Filipe Coelho <falktx@falktx.com>
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
#include "lv2/control-input-port-change-request.h"

#ifdef DISTRHO_PLUGIN_LICENSED_FOR_MOD
# include "mod-license.h"
#endif

#ifdef DISTRHO_OS_WINDOWS
# include <direct.h>
#else
# include <sys/stat.h>
# include <sys/types.h>
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

#ifndef DISTRHO_PLUGIN_USES_CUSTOM_MODGUI
# define DISTRHO_PLUGIN_USES_CUSTOM_MODGUI 0
#endif

#if DISTRHO_PLUGIN_HAS_UI
# if defined(DISTRHO_OS_HAIKU)
#  define DISTRHO_LV2_UI_TYPE "BeUI"
# elif defined(DISTRHO_OS_MAC)
#  define DISTRHO_LV2_UI_TYPE "CocoaUI"
# elif defined(DISTRHO_OS_WINDOWS)
#  define DISTRHO_LV2_UI_TYPE "WindowsUI"
# else
#  define DISTRHO_LV2_UI_TYPE "X11UI"
# endif
#else
# define DISTRHO_LV2_UI_TYPE "UI"
#endif

#define DISTRHO_LV2_USE_EVENTS_IN  (DISTRHO_PLUGIN_WANT_MIDI_INPUT || DISTRHO_PLUGIN_WANT_TIMEPOS || DISTRHO_PLUGIN_WANT_STATE)
#define DISTRHO_LV2_USE_EVENTS_OUT (DISTRHO_PLUGIN_WANT_MIDI_OUTPUT || DISTRHO_PLUGIN_WANT_STATE)

// --------------------------------------------------------------------------------------------------------------------

static constexpr const char* const lv2ManifestPluginExtensionData[] = {
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

static constexpr const char* const lv2ManifestPluginOptionalFeatures[] = {
   #if DISTRHO_PLUGIN_IS_RT_SAFE
    LV2_CORE__hardRTCapable,
   #endif
    LV2_BUF_SIZE__boundedBlockLength,
   #if DISTRHO_PLUGIN_WANT_STATE
    LV2_STATE__mapPath,
    LV2_STATE__freePath,
   #endif
   #if DISTRHO_PLUGIN_WANT_PARAMETER_VALUE_CHANGE_REQUEST
    LV2_CONTROL_INPUT_PORT_CHANGE_REQUEST_URI,
   #endif
    nullptr
};

static constexpr const char* const lv2ManifestPluginRequiredFeatures[] = {
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

static constexpr const char* const lv2ManifestPluginSupportedOptions[] =
{
    LV2_BUF_SIZE__nominalBlockLength,
    LV2_BUF_SIZE__maxBlockLength,
    LV2_PARAMETERS__sampleRate,
    nullptr
};

#if DISTRHO_PLUGIN_HAS_UI
static constexpr const char* const lv2ManifestUiExtensionData[] = {
    "opts:interface",
    "ui:idleInterface",
    "ui:showInterface",
   #if DISTRHO_PLUGIN_WANT_PROGRAMS
    LV2_PROGRAMS__UIInterface,
   #endif
    nullptr
};

static constexpr const char* const lv2ManifestUiOptionalFeatures[] = {
  #if DISTRHO_PLUGIN_HAS_UI
   #if !DISTRHO_UI_USER_RESIZABLE
    "ui:noUserResize",
   #endif
    "ui:parent",
    "ui:touch",
  #endif
    "ui:requestValue",
    nullptr
};

static constexpr const char* const lv2ManifestUiRequiredFeatures[] = {
    "opts:options",
    "ui:idleInterface",
   #if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
    LV2_DATA_ACCESS_URI,
    LV2_INSTANCE_ACCESS_URI,
   #endif
    LV2_URID__map,
    nullptr
};

static constexpr const char* const lv2ManifestUiSupportedOptions[] = {
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

// --------------------------------------------------------------------------------------------------------------------

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

            const String& programName(plugin.getProgramName(i));

            presetString  = "<" DISTRHO_PLUGIN_URI + presetSeparator + "preset" + strBuf + ">\n";
            presetString += "    a pset:Preset ;\n";
            presetString += "    lv2:appliesTo <" DISTRHO_PLUGIN_URI "> ;\n";

            if (programName.contains('"'))
                presetString += "    rdfs:label\"\"\"" + programName + "\"\"\" ;\n";
            else
                presetString += "    rdfs:label \"" + programName + "\" ;\n";

            presetString += "    rdfs:seeAlso <presets.ttl> .\n";
            presetString += "\n";

            manifestString += presetString;
        }
#endif

        manifestFile << manifestString;
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
            {
               #ifdef __MOD_DEVICES__
                const String& fileTypes(plugin.getStateFileTypes(i));
                if (fileTypes.isNotEmpty())
                    pluginString += "    mod:fileTypes \"" + fileTypes + "\" ; \n";
               #endif
                pluginString += "    rdfs:range atom:Path .\n\n";
            }
            else
            {
                pluginString += "    rdfs:range atom:String .\n\n";
            }

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
                        pluginString += "        lv2:symbol \"" + String(ParameterDesignationSymbols::bypass_lv2) + "\" ;\n";
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
                    const uint32_t hints = plugin.getParameterHints(i);

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

                    if (hints & kParameterIsInteger)
                    {
                        if (plugin.isParameterInput(i))
                            pluginString += "        lv2:default " + String(int(ranges.def)) + " ;\n";
                        pluginString += "        lv2:minimum " + String(int(ranges.min)) + " ;\n";
                        pluginString += "        lv2:maximum " + String(int(ranges.max)) + " ;\n";
                    }
                    else if (hints & kParameterIsLogarithmic)
                    {
                        if (plugin.isParameterInput(i))
                        {
                            if (d_isNotZero(ranges.def))
                                pluginString += "        lv2:default " + String(ranges.def) + " ;\n";
                            else if (d_isEqual(ranges.def, ranges.max))
                                pluginString += "        lv2:default -0.0001 ;\n";
                            else
                                pluginString += "        lv2:default 0.0001 ;\n";
                        }

                        if (d_isNotZero(ranges.min))
                            pluginString += "        lv2:minimum " + String(ranges.min) + " ;\n";
                        else
                            pluginString += "        lv2:minimum 0.0001 ;\n";

                        if (d_isNotZero(ranges.max))
                            pluginString += "        lv2:maximum " + String(ranges.max) + " ;\n";
                        else
                            pluginString += "        lv2:maximum -0.0001 ;\n";
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

                            if (hints & kParameterIsInteger)
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
                            if (hints & kParameterIsInteger)
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
                    if (hints & kParameterIsHidden)
                        pluginString += "        lv2:portProperty <" LV2_PORT_PROPS__notOnGUI "> ;\n";
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

            if (license.isEmpty())
            {}
            // Using URL as license
            else if (license.contains("://"))
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

        pluginFile << pluginString;
        pluginFile.close();
        std::cout << " done!" << std::endl;
    }

   #if DISTRHO_PLUGIN_USES_MODGUI && !DISTRHO_PLUGIN_USES_CUSTOM_MODGUI
    {
        std::cout << "Writing modgui.ttl..."; std::cout.flush();
        std::fstream modguiFile("modgui.ttl", std::ios::out);

        String modguiString;
        modguiString += "@prefix lv2:    <" LV2_CORE_PREFIX "> .\n";
        modguiString += "@prefix modgui: <http://moddevices.com/ns/modgui#> .\n";
        modguiString += "\n";

        modguiString += "<" DISTRHO_PLUGIN_URI ">\n";
        modguiString += "    modgui:gui [\n";
       #ifdef DISTRHO_PLUGIN_BRAND
        modguiString += "        modgui:brand \"" DISTRHO_PLUGIN_BRAND "\" ;\n";
       #endif
        modguiString += "        modgui:label \"" DISTRHO_PLUGIN_NAME "\" ;\n";
        modguiString += "        modgui:resourcesDirectory <modgui> ;\n";
        modguiString += "        modgui:iconTemplate <modgui/icon.html> ;\n";
        modguiString += "        modgui:javascript <modgui/javascript.js> ;\n";
        modguiString += "        modgui:stylesheet <modgui/stylesheet.css> ;\n";
        modguiString += "        modgui:screenshot <modgui/screenshot.png> ;\n";
        modguiString += "        modgui:thumbnail <modgui/thumbnail.png> ;\n";

        uint32_t numParametersOutputs = 0;
        for (uint32_t i=0, count=plugin.getParameterCount(); i < count; ++i)
        {
            if (plugin.isParameterOutput(i))
                ++numParametersOutputs;
        }
        if (numParametersOutputs != 0)
        {
            modguiString += "        modgui:monitoredOutputs [\n";
            for (uint32_t i=0, j=0, count=plugin.getParameterCount(); i < count; ++i)
            {
                if (!plugin.isParameterOutput(i))
                    continue;
                modguiString += "            lv2:symbol \"" + plugin.getParameterSymbol(i) + "\" ;\n";
                if (++j != numParametersOutputs)
                    modguiString += "        ] , [\n";
            }
            modguiString += "        ] ;\n";
        }

        modguiString += "    ] .\n";

        modguiFile << modguiString;
        modguiFile.close();
        std::cout << " done!" << std::endl;
    }

   #ifdef DISTRHO_OS_WINDOWS
    ::_mkdir("modgui");
   #else
    ::mkdir("modgui", 0755);
   #endif

    {
        std::cout << "Writing modgui/javascript.js..."; std::cout.flush();
        std::fstream jsFile("modgui/javascript.js", std::ios::out);

        String jsString;
        jsString += "function(e,f){\n";
        jsString += "'use strict';\nvar ps=[";

        for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_INPUTS; ++i)
            jsString += "'lv2_" + plugin.getAudioPort(false, i).symbol + "',";
        for (uint32_t i=0; i < DISTRHO_PLUGIN_NUM_OUTPUTS; ++i)
            jsString += "'lv2_" + plugin.getAudioPort(true, i).symbol + "',";
       #if DISTRHO_LV2_USE_EVENTS_IN
        jsString += "'lv2_events_in',";
       #endif
       #if DISTRHO_LV2_USE_EVENTS_OUT
        jsString += "'lv2_events_out',";
       #endif
       #if DISTRHO_PLUGIN_WANT_LATENCY
        jsString += "'lv2_latency',";
       #endif

        int32_t enabledIndex = INT32_MAX;
        for (uint32_t i=0, count=plugin.getParameterCount(); i < count; ++i)
        {
            jsString += "'" + plugin.getParameterSymbol(i) + "',";
            if (plugin.getParameterDesignation(i) == kParameterDesignationBypass)
                enabledIndex = i;
        }
        jsString += "];\n";
        jsString += "var ei=" + String(enabledIndex != INT32_MAX ? enabledIndex : -1) + ";\n\n";
        jsString += "if(e.type==='start'){\n";
        jsString += "e.data.p={p:{},c:{},};\n\n";
        jsString += "var err=[];\n";
        jsString += "if(typeof(WebAssembly)==='undefined'){err.push('WebAssembly unsupported');}\n";
        jsString += "else{\n";
        jsString += "if(!WebAssembly.validate(new Uint8Array([0,97,115,109,1,0,0,0,1,4,1,96,0,0,3,2,1,0,5,3,1,0,1,10,14,1,12,0,65,0,65,0,65,0,252,10,0,0,11])))";
        jsString += "err.push('Bulk Memory Operations unsupported');\n";
        jsString += "if(!WebAssembly.validate(new Uint8Array([0,97,115,109,1,0,0,0,2,8,1,1,97,1,98,3,127,1,6,6,1,127,1,65,0,11,7,5,1,1,97,3,1])))";
        jsString += "err.push('Importable/Exportable mutable globals unsupported');\n";
        jsString += "}\n";
        jsString += "if(err.length!==0){e.icon.find('.canvas_wrapper').html('<h2>'+err.join('<br>')+'</h2>');return;}\n\n";
        jsString += "var s=document.createElement('script');\n";
        jsString += "s.setAttribute('async',true);\n";
        jsString += "s.setAttribute('src',e.api_version>=3?f.get_custom_resource_filename('module.js'):('/resources/module.js?uri='+escape(\"" DISTRHO_PLUGIN_URI "\")+'&r='+VERSION));\n";
        jsString += "s.setAttribute('type','text/javascript');\n";
        jsString += "s.onload=function(){\n";
        jsString += " Module_" DISTRHO_PLUGIN_MODGUI_CLASS_NAME "({\n";
        jsString += " locateFile: function(p,_){return e.api_version>=3?f.get_custom_resource_filename(p):('/resources/'+p+'?uri='+escape(\"" DISTRHO_PLUGIN_URI "\")+'&r='+VERSION)},\n";
        jsString += " postRun:function(m){\n";
        jsString += " var cn=e.icon.attr('mod-instance').replaceAll('/','_');\n";
        jsString += " var cnl=m.lengthBytesUTF8(cn) + 1;\n";
        jsString += " var cna=m._malloc(cnl);\n";
        jsString += " m.stringToUTF8(cn, cna, cnl);\n";
        jsString += " e.icon.find('canvas')[0].id=cn;\n";
        jsString += " var a=m.addFunction(function(i,v){f.set_port_value(ps[i],v);},'vif');\n";
        jsString += " var b=m.addFunction(function(u,v){f.patch_set(m.UTF8ToString(u),'s',m.UTF8ToString(v));},'vpp');\n";
        jsString += " var h=m._modgui_init(cna,a,b);\n";
        jsString += " m._free(cna);\n";
        jsString += " e.data.h=h;\n";
        jsString += " e.data.m=m;\n";
        jsString += " for(var u in e.data.p.p){\n";
        jsString += " var ul=m.lengthBytesUTF8(u)+1,ua=m._malloc(ul),v=e.data.p.p[u],vl=m.lengthBytesUTF8(v)+1,va=m._malloc(vl);\n";
        jsString += " m.stringToUTF8(u,ua,ul);\n";
        jsString += " m.stringToUTF8(v,va,vl);\n";
        jsString += " m._modgui_patch_set(h, ua, va);\n";
        jsString += " m._free(ua);\n";
        jsString += " m._free(va);\n";
        jsString += " }\n";
        jsString += " for(var symbol in e.data.p.c){m._modgui_param_set(h,ps.indexOf(symbol),e.data.p.c[symbol]);}\n";
        jsString += " delete e.data.p;\n";
        jsString += " window.dispatchEvent(new Event('resize'));\n";
        jsString += " },\n";
        jsString += " canvas:(function(){var c=e.icon.find('canvas')[0];c.addEventListener('webglcontextlost',function(e2){alert('WebGL context lost. You will need to reload the page.');e2.preventDefault();},false);return c;})(),\n";
        jsString += " });\n";
        jsString += "};\n";
        jsString += "document.head.appendChild(s);\n\n";
        jsString += "}else if(e.type==='change'){\n\n";
        jsString += "if(e.data.h && e.data.m){\n";
        jsString += " var m=e.data.m;\n";
        jsString += " if(e.uri){\n";
        jsString += "  var ul=m.lengthBytesUTF8(e.uri)+1,ua=m._malloc(ul),vl=m.lengthBytesUTF8(e.value)+1,va=m._malloc(vl);\n";
        jsString += "  m.stringToUTF8(e.uri,ua,ul);\n";
        jsString += "  m.stringToUTF8(e.value,va,vl);\n";
        jsString += "  m._modgui_patch_set(e.data.h,ua,va);\n";
        jsString += "  m._free(ua);\n";
        jsString += "  m._free(va);\n";
        jsString += " }else if(e.symbol===':bypass'){return;\n";
        jsString += " }else{m._modgui_param_set(e.data.h,ps.indexOf(e.symbol),e.value);}\n";
        jsString += "}else{\n";
        jsString += " if(e.symbol===':bypass')return;\n";
        jsString += " if(e.uri){e.data.p.p[e.uri]=e.value;}else{e.data.p.c[e.symbol]=e.value;}\n";
        jsString += "}\n\n";
        jsString += "}else if(e.type==='end'){\n";
        jsString += " if(e.data.h && e.data.m){\n";
        jsString += "  var h = e.data.h;\n";
        jsString += "  var m = e.data.m;\n";
        jsString += "  e.data.h = e.data.m = null;\n";
        jsString += "  m._modgui_cleanup(h);\n";
        jsString += "}\n\n";
        jsString += "}\n}\n";
        jsFile << jsString;
        jsFile.close();
        std::cout << " done!" << std::endl;
    }

    {
        std::cout << "Writing modgui/icon.html..."; std::cout.flush();
        std::fstream iconFile("modgui/icon.html", std::ios::out);

        iconFile << "<div class='" DISTRHO_PLUGIN_MODGUI_CLASS_NAME " mod-pedal'>" << std::endl;
        iconFile << "    <div mod-role='drag-handle' class='mod-drag-handle'></div>" << std::endl;
        iconFile << "    <div class='mod-plugin-title'><h1>{{#brand}}{{brand}} | {{/brand}}{{label}}</h1></div>" << std::endl;
        iconFile << "    <div class='mod-light on' mod-role='bypass-light'></div>" << std::endl;
        iconFile << "    <div class='mod-control-group mod-switch'>" << std::endl;
        iconFile << "        <div class='mod-control-group mod-switch-image mod-port transport' mod-role='bypass' mod-widget='film'></div>" << std::endl;
        iconFile << "    </div>" << std::endl;
        iconFile << "    <div class='canvas_wrapper'>" << std::endl;
        iconFile << "        <canvas oncontextmenu='event.preventDefault()' tabindex=-1></canvas>" << std::endl;
        iconFile << "    </div>" << std::endl;
        iconFile << "    <div class='mod-pedal-input'>" << std::endl;
        iconFile << "        {{#effect.ports.audio.input}}" << std::endl;
        iconFile << "        <div class='mod-input mod-input-disconnected' title='{{name}}' mod-role='input-audio-port' mod-port-symbol='{{symbol}}'>" << std::endl;
        iconFile << "            <div class='mod-pedal-input-image'></div>" << std::endl;
        iconFile << "        </div>" << std::endl;
        iconFile << "        {{/effect.ports.audio.input}}" << std::endl;
        iconFile << "        {{#effect.ports.midi.input}}" << std::endl;
        iconFile << "        <div class='mod-input mod-input-disconnected' title='{{name}}' mod-role='input-midi-port' mod-port-symbol='{{symbol}}'>" << std::endl;
        iconFile << "            <div class='mod-pedal-input-image'></div>" << std::endl;
        iconFile << "        </div>" << std::endl;
        iconFile << "        {{/effect.ports.midi.input}}" << std::endl;
        iconFile << "        {{#effect.ports.cv.input}}" << std::endl;
        iconFile << "        <div class='mod-input mod-input-disconnected' title='{{name}}' mod-role='input-cv-port' mod-port-symbol='{{symbol}}'>" << std::endl;
        iconFile << "            <div class='mod-pedal-input-image'></div>" << std::endl;
        iconFile << "        </div>" << std::endl;
        iconFile << "        {{/effect.ports.cv.input}}" << std::endl;
        iconFile << "    </div>" << std::endl;
        iconFile << "    <div class='mod-pedal-output'>" << std::endl;
        iconFile << "        {{#effect.ports.audio.output}}" << std::endl;
        iconFile << "        <div class='mod-output mod-output-disconnected' title='{{name}}' mod-role='output-audio-port' mod-port-symbol='{{symbol}}'>" << std::endl;
        iconFile << "            <div class='mod-pedal-output-image'></div>" << std::endl;
        iconFile << "        </div>" << std::endl;
        iconFile << "        {{/effect.ports.audio.output}}" << std::endl;
        iconFile << "        {{#effect.ports.midi.output}}" << std::endl;
        iconFile << "        <div class='mod-output mod-output-disconnected' title='{{name}}' mod-role='output-midi-port' mod-port-symbol='{{symbol}}'>" << std::endl;
        iconFile << "            <div class='mod-pedal-output-image'></div>" << std::endl;
        iconFile << "        </div>" << std::endl;
        iconFile << "        {{/effect.ports.midi.output}}" << std::endl;
        iconFile << "        {{#effect.ports.cv.output}}" << std::endl;
        iconFile << "        <div class='mod-output mod-output-disconnected' title='{{name}}' mod-role='output-cv-port' mod-port-symbol='{{symbol}}'>" << std::endl;
        iconFile << "            <div class='mod-pedal-output-image'></div>" << std::endl;
        iconFile << "        </div>" << std::endl;
        iconFile << "        {{/effect.ports.cv.output}}" << std::endl;
        iconFile << "    </div>" << std::endl;
        iconFile << "</div>" << std::endl;

        iconFile.close();
        std::cout << " done!" << std::endl;
    }

    {
        std::cout << "Writing modgui/stylesheet.css..."; std::cout.flush();
        std::fstream stylesheetFile("modgui/stylesheet.css", std::ios::out);

        stylesheetFile << "." DISTRHO_PLUGIN_MODGUI_CLASS_NAME ".mod-pedal{" << std::endl;
        stylesheetFile << " padding:0;" << std::endl;
        stylesheetFile << " margin:0;" << std::endl;
        stylesheetFile << " width:" + String(DISTRHO_UI_DEFAULT_WIDTH) + "px;" << std::endl;
        stylesheetFile << " height:" + String(DISTRHO_UI_DEFAULT_HEIGHT + 50) + "px;" << std::endl;
        stylesheetFile << " background:#2a2e32;" << std::endl;
        stylesheetFile << " border-radius:20px 20px 0 0;" << std::endl;
        stylesheetFile << " color:#fff;" << std::endl;
        stylesheetFile << "}" << std::endl;
        stylesheetFile << "." DISTRHO_PLUGIN_MODGUI_CLASS_NAME ".mod-pedal .canvas_wrapper{" << std::endl;
        stylesheetFile << " --device-pixel-ratio:1;" << std::endl;
        stylesheetFile << " /*image-rendering:pixelated;*/" << std::endl;
        stylesheetFile << " /*image-rendering:crisp-edges;*/" << std::endl;
        stylesheetFile << " background:#000;" << std::endl;
        stylesheetFile << " position:absolute;" << std::endl;
        stylesheetFile << " top:50px;" << std::endl;
        stylesheetFile << " transform-origin:0 0 0;" << std::endl;
        stylesheetFile << " transform:scale(calc(1/var(--device-pixel-ratio)));" << std::endl;
        stylesheetFile << " width:" + String(DISTRHO_UI_DEFAULT_WIDTH) + "px;" << std::endl;
        stylesheetFile << " height:" + String(DISTRHO_UI_DEFAULT_HEIGHT) + "px;" << std::endl;
        stylesheetFile << " text-align:center;" << std::endl;
        stylesheetFile << " z-index:21;" << std::endl;
        stylesheetFile << "}" << std::endl;
        stylesheetFile << "/*" << std::endl;
        stylesheetFile << "." DISTRHO_PLUGIN_MODGUI_CLASS_NAME ".mod-pedal .canvas_wrapper:focus-within{" << std::endl;
        stylesheetFile << " z-index:21;" << std::endl;
        stylesheetFile << "}" << std::endl;
        stylesheetFile << "*/" << std::endl;
        stylesheetFile << "." DISTRHO_PLUGIN_MODGUI_CLASS_NAME ".mod-pedal .mod-plugin-title{" << std::endl;
        stylesheetFile << " position:absolute;" << std::endl;
        stylesheetFile << " text-align:center;" << std::endl;
        stylesheetFile << " width:100%;" << std::endl;
        stylesheetFile << "}" << std::endl;
        stylesheetFile << "." DISTRHO_PLUGIN_MODGUI_CLASS_NAME ".mod-pedal h1{" << std::endl;
        stylesheetFile << " font-size:20px;" << std::endl;
        stylesheetFile << " font-weight:bold;" << std::endl;
        stylesheetFile << " line-height:50px;" << std::endl;
        stylesheetFile << " margin:0;" << std::endl;
        stylesheetFile << "}" << std::endl;
        stylesheetFile << "." DISTRHO_PLUGIN_MODGUI_CLASS_NAME ".mod-pedal .mod-control-group{" << std::endl;
        stylesheetFile << " position:absolute;" << std::endl;
        stylesheetFile << " left:5px;" << std::endl;
        stylesheetFile << " z-index:35;" << std::endl;
        stylesheetFile << "}" << std::endl;
        stylesheetFile << "." DISTRHO_PLUGIN_MODGUI_CLASS_NAME ".mod-pedal .mod-pedal-input," << std::endl;
        stylesheetFile << "." DISTRHO_PLUGIN_MODGUI_CLASS_NAME ".mod-pedal .mod-pedal-output{" << std::endl;
        stylesheetFile << " top:75px;" << std::endl;
        stylesheetFile << "}" << std::endl;
        stylesheetFile << "." DISTRHO_PLUGIN_MODGUI_CLASS_NAME ".mod-pedal .mod-audio-input," << std::endl;
        stylesheetFile << "." DISTRHO_PLUGIN_MODGUI_CLASS_NAME ".mod-pedal .mod-audio-output{" << std::endl;
        stylesheetFile << " margin-bottom:25px;" << std::endl;
        stylesheetFile << "}" << std::endl;
        stylesheetFile << "." DISTRHO_PLUGIN_MODGUI_CLASS_NAME ".mod-pedal .jack-disconnected{" << std::endl;
        stylesheetFile << " top:0px!important;" << std::endl;
        stylesheetFile << "}" << std::endl;
        stylesheetFile << "." DISTRHO_PLUGIN_MODGUI_CLASS_NAME ".mod-pedal .mod-switch-image{" << std::endl;
        stylesheetFile << " background-image: url(/img/switch.png);" << std::endl;
        stylesheetFile << " background-position: left center;" << std::endl;
        stylesheetFile << " background-repeat: no-repeat;" << std::endl;
        stylesheetFile << " background-size: auto 50px;" << std::endl;
        stylesheetFile << " font-weight: bold;" << std::endl;
        stylesheetFile << " width: 100px;" << std::endl;
        stylesheetFile << " height: 50px;" << std::endl;
        stylesheetFile << " cursor: pointer;" << std::endl;
        stylesheetFile << "}" << std::endl;
        stylesheetFile << "." DISTRHO_PLUGIN_MODGUI_CLASS_NAME ".mod-pedal .mod-switch-image.off{" << std::endl;
        stylesheetFile << " background-position: right center !important;" << std::endl;
        stylesheetFile << "}" << std::endl;
        stylesheetFile << "." DISTRHO_PLUGIN_MODGUI_CLASS_NAME ".mod-pedal .mod-switch-image.on{" << std::endl;
        stylesheetFile << " background-position: left center !important;" << std::endl;
        stylesheetFile << "}" << std::endl;

        stylesheetFile.close();
        std::cout << " done!" << std::endl;
    }
   #endif // DISTRHO_PLUGIN_USES_MODGUI && !DISTRHO_PLUGIN_USES_CUSTOM_MODGUI

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

        uiFile << uiString;
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

                if (plugin.getStateHints(j) & kStateIsHostReadable)
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
                    parameterSymbol = ParameterDesignationSymbols::bypass_lv2;
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

        presetsFile << presetsString;
        presetsFile.close();
        std::cout << " done!" << std::endl;
    }
#endif
}
