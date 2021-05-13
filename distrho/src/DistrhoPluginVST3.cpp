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

#include "DistrhoPluginInternal.hpp"
#include "../extra/ScopedPointer.hpp"

#include "travesty/audio_processor.h"
#include "travesty/component.h"
#include "travesty/edit_controller.h"
#include "travesty/factory.h"

START_NAMESPACE_DISTRHO

#if ! DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
static const writeMidiFunc writeMidiCallback = nullptr;
#endif

// custom v3_tuid compatible type
typedef uint32_t dpf_tuid[4];
static_assert(sizeof(v3_tuid) == sizeof(dpf_tuid), "uid size mismatch");

// custom uids, fully created during module init
static constexpr const uint32_t dpf_id_entry = d_cconst('D', 'P', 'F', ' ');
static constexpr const uint32_t dpf_id_clas  = d_cconst('c', 'l', 'a', 's');
static constexpr const uint32_t dpf_id_comp  = d_cconst('c', 'o', 'm', 'p');
static constexpr const uint32_t dpf_id_ctrl  = d_cconst('c', 't', 'r', 'l');
static constexpr const uint32_t dpf_id_proc  = d_cconst('p', 'r', 'o', 'c');
static constexpr const uint32_t dpf_id_view  = d_cconst('v', 'i', 'e', 'w');

static dpf_tuid dpf_tuid_class = { dpf_id_entry, dpf_id_clas, 0, 0 };
static dpf_tuid dpf_tuid_component = { dpf_id_entry, dpf_id_comp, 0, 0 };
static dpf_tuid dpf_tuid_controller = { dpf_id_entry, dpf_id_ctrl, 0, 0 };
static dpf_tuid dpf_tuid_processor = { dpf_id_entry, dpf_id_proc, 0, 0 };
static dpf_tuid dpf_tuid_view = { dpf_id_entry, dpf_id_view, 0, 0 };

// -----------------------------------------------------------------------

void strncpy(char* const dst, const char* const src, const size_t size)
{
    DISTRHO_SAFE_ASSERT_RETURN(size > 0,);

    if (const size_t len = std::min(std::strlen(src), size-1U))
    {
        std::memcpy(dst, src, len);
        dst[len] = '\0';
    }
    else
    {
        dst[0] = '\0';
    }
}

// -----------------------------------------------------------------------

// v3_funknown, v3_plugin_base, v3_component

// audio_processor

class PluginVst3
{
public:
    PluginVst3()
        : fPlugin(this, writeMidiCallback)
    {
    }

private:
    // Plugin
    PluginExporter fPlugin;

    // VST3 stuff
    // TODO

#if DISTRHO_PLUGIN_WANT_MIDI_OUTPUT
    bool writeMidi(const MidiEvent& midiEvent)
    {
        // TODO
        return true;
    }

    static bool writeMidiCallback(void* ptr, const MidiEvent& midiEvent)
    {
        return ((PluginVst*)ptr)->writeMidi(midiEvent);
    }
#endif

};

// -----------------------------------------------------------------------
// WIP this whole section still TODO

struct ControllerComponent;
struct ProcessorComponent;

struct ComponentAdapter : v3_funknown, v3_plugin_base
{
    // needs atomic refcount, starts at 1

    ComponentAdapter()
    {
        static const uint8_t* kSupportedFactories[] = {
            v3_funknown_iid,
            v3_plugin_base_iid,
            /*
            v3_component_iid,
            v3_edit_controller_iid,
            v3_audio_processor_iid
            */
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            d_stdout("ComponentAdapter::query_interface %p %p %p", self, iid, iface);

            *iface = NULL;
            DISTRHO_SAFE_ASSERT_RETURN(self != nullptr, V3_NO_INTERFACE);

            for (const uint8_t* factory_iid : kSupportedFactories)
            {
                if (v3_tuid_match(factory_iid, iid))
                {
                    *iface = self;
                    return V3_OK;
                }
            }

            return V3_NO_INTERFACE;
        };

        // TODO use atomic counter
        ref = []V3_API(void*) -> uint32_t { return 1; };
        unref = []V3_API(void*) -> uint32_t { return 0; };
    }
};

struct ControllerComponent : ComponentAdapter
{
};

struct ProcessorComponent : ComponentAdapter
{
};

// --------------------------------------------------------------------------------------------------------------------
// Dummy plugin to get data from

static ScopedPointer<PluginExporter> gPluginInfo;

static void gPluginInit()
{
    if (gPluginInfo != nullptr)
        return;

    d_lastBufferSize = 512;
    d_lastSampleRate = 44100.0;
    gPluginInfo = new PluginExporter(nullptr, nullptr);
    d_lastBufferSize = 0;
    d_lastSampleRate = 0.0;

    dpf_tuid_class[3] = dpf_tuid_component[3] = dpf_tuid_controller[3]
        = dpf_tuid_processor[3] = dpf_tuid_view[3] = gPluginInfo->getUniqueId();
}

// --------------------------------------------------------------------------------------------------------------------
// dpf_factory

struct v3_plugin_factory_cpp : v3_funknown, v3_plugin_factory {
    v3_plugin_factory_2 v2;
    v3_plugin_factory_3 v3;
};

struct dpf_factory : v3_plugin_factory_cpp {
    dpf_factory()
    {
        static const uint8_t* kSupportedFactories[] = {
            v3_funknown_iid,
            v3_plugin_factory_iid,
            v3_plugin_factory_2_iid
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            *iface = NULL;
            DISTRHO_SAFE_ASSERT_RETURN(self != nullptr, V3_NO_INTERFACE);

            for (const uint8_t* factory_iid : kSupportedFactories)
            {
                if (v3_tuid_match(factory_iid, iid))
                {
                    *iface = self;
                    return V3_OK;
                }
            }

            return V3_NO_INTERFACE;
        };

        // we only support 1 plugin per binary, so don't have to care here
        ref = []V3_API(void*) -> uint32_t { return 1; };
        unref = []V3_API(void*) -> uint32_t { return 0; };

        // ------------------------------------------------------------------------------------------------------------
        // v3_plugin_factory

        get_factory_info = []V3_API(void*, struct v3_factory_info* const info) -> v3_result
        {
            DISTRHO_NAMESPACE::strncpy(info->vendor, gPluginInfo->getMaker(), sizeof(info->vendor));
            DISTRHO_NAMESPACE::strncpy(info->url, gPluginInfo->getHomePage(), sizeof(info->url));
            DISTRHO_NAMESPACE::strncpy(info->email, "", sizeof(info->email)); // TODO
            return V3_OK;
        };

        num_classes = []V3_API(void*) -> int32_t
        {
            return 1;
        };

        get_class_info = []V3_API(void*, int32_t /*idx*/, struct v3_class_info* const info) -> v3_result
        {
            memcpy(info->class_id, dpf_tuid_class, sizeof(v3_tuid));
            info->cardinality = 0x7FFFFFFF;
            DISTRHO_NAMESPACE::strncpy(info->category, "Audio Module Class", sizeof(info->category));
            DISTRHO_NAMESPACE::strncpy(info->name, gPluginInfo->getName(), sizeof(info->name));
            return V3_OK;
        };

        create_instance = []V3_API(void* self, const v3_tuid class_id, const v3_tuid iid, void** instance) -> v3_result
        {
            d_stdout("%s %i %p %p %p %p", __PRETTY_FUNCTION__, __LINE__, self, class_id, iid, instance);
            DISTRHO_SAFE_ASSERT_RETURN(v3_tuid_match(class_id, *(v3_tuid*)&dpf_tuid_class) &&
                                       v3_tuid_match(iid, v3_component_iid), V3_NO_INTERFACE);

            *instance = nullptr; // new ComponentAdapter();
            return V3_INTERNAL_ERR;
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_plugin_factory_2

        v2.get_class_info_2 = []V3_API(void*, int32_t /*idx*/, struct v3_class_info_2 *info) -> v3_result
        {
            // get_class_info
            memcpy(info->class_id, dpf_tuid_class, sizeof(v3_tuid));
            info->cardinality = 0x7FFFFFFF;
            DISTRHO_NAMESPACE::strncpy(info->category, "Audio Module Class", sizeof(info->category));
            DISTRHO_NAMESPACE::strncpy(info->name, gPluginInfo->getName(), sizeof(info->name));
            // get_class_info_2
            info->class_flags = 0;
            DISTRHO_NAMESPACE::strncpy(info->sub_categories, "", sizeof(info->sub_categories)); // TODO
            DISTRHO_NAMESPACE::strncpy(info->vendor, gPluginInfo->getMaker(), sizeof(info->vendor));
            DISTRHO_NAMESPACE::strncpy(info->version, "", sizeof(info->version)); // TODO
            DISTRHO_NAMESPACE::strncpy(info->sdk_version, "Travesty", sizeof(info->sdk_version)); // TESTING use "VST 3.7" ?
            return V3_OK;
        };
    }
};

static const dpf_factory dpf_factory;

END_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------
// VST3 entry point

DISTRHO_PLUGIN_EXPORT
const void* GetPluginFactory(void);

const void* GetPluginFactory(void)
{
    static const struct v3_plugin_factory_2* const factory = (v3_plugin_factory_2*)&dpf_factory;
    return &factory;
}

// --------------------------------------------------------------------------------------------------------------------
// OS specific module load

#ifdef DISTRHO_OS_MAC
DISTRHO_PLUGIN_EXPORT bool bundleEntry(CFBundleRef);
DISTRHO_PLUGIN_EXPORT bool bundleExit(void);
bool bundleEntry(CFBundleRef)
{
    gPluginInit();
    return true;
}
bool bundleExit(void)
{
    gPluginInfo = nullptr;
    return true;
}
#else
# ifdef DISTRHO_OS_WINDOWS
#  define ENTRYFNNAME InitDll
#  define EXITFNNAME ExitDll
# else
#  define ENTRYFNNAME ModuleEntry
#  define EXITFNNAME ModuleExit
# endif
DISTRHO_PLUGIN_EXPORT bool ENTRYFNNAME(void*);
DISTRHO_PLUGIN_EXPORT bool EXITFNNAME(void);
bool ENTRYFNNAME(void*)
{
    gPluginInit();
    return true;
}
bool EXITFNNAME(void)
{
    gPluginInfo = nullptr;
    return true;
}
# undef ENTRYFNNAME
# undef EXITFNNAME
#endif

// --------------------------------------------------------------------------------------------------------------------
