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

#include "DistrhoUIInternal.hpp"

#include "../extra/String.hpp"

#include "lv2/atom.h"
#include "lv2/atom-util.h"
#include "lv2/data-access.h"
#include "lv2/instance-access.h"
#include "lv2/midi.h"
#include "lv2/options.h"
#include "lv2/parameters.h"
#include "lv2/patch.h"
#include "lv2/ui.h"
#include "lv2/urid.h"
#include "lv2/lv2_kxstudio_properties.h"
#include "lv2/lv2_programs.h"

#ifndef DISTRHO_PLUGIN_LV2_STATE_PREFIX
# define DISTRHO_PLUGIN_LV2_STATE_PREFIX "urn:distrho:"
#endif

START_NAMESPACE_DISTRHO

typedef struct _LV2_Atom_MidiEvent {
    LV2_Atom atom;    /**< Atom header. */
    uint8_t  data[3]; /**< MIDI data (body). */
} LV2_Atom_MidiEvent;

#if ! DISTRHO_PLUGIN_WANT_STATE
static constexpr const setStateFunc setStateCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_MIDI_INPUT
static constexpr const sendNoteFunc sendNoteCallback = nullptr;
#endif

// unwanted in LV2, resize extension is deprecated and hosts can do it without extensions
static constexpr const setSizeFunc setSizeCallback = nullptr;

// -----------------------------------------------------------------------

template <class LV2F>
static const LV2F* getLv2Feature(const LV2_Feature* const* features, const char* const uri)
{
    for (int i=0; features[i] != nullptr; ++i)
    {
        if (std::strcmp(features[i]->URI, uri) == 0)
            return (const LV2F*)features[i]->data;
    }

    return nullptr;
}

class UiLv2
{
public:
    UiLv2(const char* const bundlePath,
          const intptr_t winId,
          const LV2_Options_Option* options,
          const LV2_URID_Map* const uridMap,
          const LV2_Feature* const* const features,
          const LV2UI_Controller controller,
          const LV2UI_Write_Function writeFunc,
          LV2UI_Widget* const widget,
          void* const dspPtr,
          const float sampleRate,
          const float scaleFactor,
          const uint32_t bgColor,
          const uint32_t fgColor,
          const char* const appClassName)
        : fUridMap(uridMap),
          fUridUnmap(getLv2Feature<LV2_URID_Unmap>(features, LV2_URID__unmap)),
          fUiPortMap(getLv2Feature<LV2UI_Port_Map>(features, LV2_UI__portMap)),
          fUiRequestValue(getLv2Feature<LV2UI_Request_Value>(features, LV2_UI__requestValue)),
          fUiTouch(getLv2Feature<LV2UI_Touch>(features, LV2_UI__touch)),
          fController(controller),
          fWriteFunction(writeFunc),
          fURIDs(uridMap),
          fBypassParameterIndex(fUiPortMap != nullptr ? fUiPortMap->port_index(fUiPortMap->handle, ParameterDesignationSymbols::bypass_lv2)
                                                      : LV2UI_INVALID_PORT_INDEX),
          fWinIdWasNull(winId == 0),
          fUI(this, winId, sampleRate,
              editParameterCallback,
              setParameterCallback,
              setStateCallback,
              sendNoteCallback,
              setSizeCallback,
              fileRequestCallback,
              bundlePath, dspPtr, scaleFactor, bgColor, fgColor, appClassName)
    {
        if (widget != nullptr)
            *widget = (LV2UI_Widget)fUI.getNativeWindowHandle();

       #if DISTRHO_PLUGIN_WANT_STATE
        // tell the DSP we're ready to receive msgs
        setState("__dpf_ui_data__", "");
       #endif

        if (winId != 0)
            return;

        // if winId == 0 then options must not be null
        DISTRHO_SAFE_ASSERT_RETURN(options != nullptr,);

       #ifndef __EMSCRIPTEN__
        const LV2_URID uridWindowTitle    = uridMap->map(uridMap->handle, LV2_UI__windowTitle);
        const LV2_URID uridTransientWinId = uridMap->map(uridMap->handle, LV2_KXSTUDIO_PROPERTIES__TransientWindowId);

        const char* windowTitle = nullptr;

        for (int i=0; options[i].key != 0; ++i)
        {
            if (options[i].key == uridTransientWinId)
            {
                if (options[i].type == fURIDs.atomLong)
                {
                    if (const int64_t transientWinId = *(const int64_t*)options[i].value)
                        fUI.setWindowTransientWinId(static_cast<intptr_t>(transientWinId));
                }
                else
                    d_stderr("Host provides transientWinId but has wrong value type");
            }
            else if (options[i].key == uridWindowTitle)
            {
                if (options[i].type == fURIDs.atomString)
                {
                    windowTitle = (const char*)options[i].value;
                }
                else
                    d_stderr("Host provides windowTitle but has wrong value type");
            }
        }

        if (windowTitle == nullptr)
            windowTitle = DISTRHO_PLUGIN_NAME;

        fUI.setWindowTitle(windowTitle);
       #endif
    }

    // -------------------------------------------------------------------

    void lv2ui_port_event(const uint32_t rindex, const uint32_t bufferSize, const uint32_t format, const void* const buffer)
    {
        if (format == 0)
        {
            const uint32_t parameterOffset = fUI.getParameterOffset();

            if (rindex < parameterOffset)
                return;

            DISTRHO_SAFE_ASSERT_RETURN(bufferSize == sizeof(float),)

            float value = *(const float*)buffer;

            if (rindex == fBypassParameterIndex)
                value = 1.0f - value;

            fUI.parameterChanged(rindex-parameterOffset, value);
        }
       #if DISTRHO_PLUGIN_WANT_STATE
        else if (format == fURIDs.atomEventTransfer)
        {
            const LV2_Atom* const atom = (const LV2_Atom*)buffer;

            if (atom->type == fURIDs.dpfKeyValue)
            {
                const char* const key   = (const char*)LV2_ATOM_BODY_CONST(atom);
                const char* const value = key+(std::strlen(key)+1);

                fUI.stateChanged(key, value);
            }
            else if (atom->type == fURIDs.atomObject && fUridUnmap != nullptr)
            {
                const LV2_Atom_Object* const obj = (const LV2_Atom_Object*)atom;

                const LV2_Atom* property = nullptr;
                const LV2_Atom* atomvalue = nullptr;
                lv2_atom_object_get(obj, fURIDs.patchProperty, &property, fURIDs.patchValue, &atomvalue, 0);

                DISTRHO_SAFE_ASSERT_RETURN(property != nullptr,);
                DISTRHO_SAFE_ASSERT_RETURN(atomvalue != nullptr,);

                DISTRHO_SAFE_ASSERT_RETURN(property->type == fURIDs.atomURID,);
                DISTRHO_SAFE_ASSERT_RETURN(atomvalue->type == fURIDs.atomPath || atomvalue->type == fURIDs.atomString,);

                if (property != nullptr && property->type == fURIDs.atomURID &&
                    atomvalue != nullptr && (atomvalue->type == fURIDs.atomPath || atomvalue->type == fURIDs.atomString))
                {
                    const LV2_URID dpf_lv2_urid = ((const LV2_Atom_URID*)property)->body;
                    DISTRHO_SAFE_ASSERT_RETURN(dpf_lv2_urid != 0,);

                    const char* const dpf_lv2_key = fUridUnmap->unmap(fUridUnmap->handle, dpf_lv2_urid);
                    DISTRHO_SAFE_ASSERT_RETURN(dpf_lv2_key != nullptr,);

                    /*constexpr*/ const size_t reqLen = std::strlen(DISTRHO_PLUGIN_URI "#");
                    DISTRHO_SAFE_ASSERT_RETURN(std::strlen(dpf_lv2_key) > reqLen,);

                    const char* const key   = dpf_lv2_key + reqLen;
                    const char* const value = (const char*)LV2_ATOM_BODY_CONST(atomvalue);

                    fUI.stateChanged(key, value);
                }
            }
            else if (atom->type == fURIDs.midiEvent)
            {
                // ignore
            }
            else
            {
                d_stdout("DPF :: received atom not handled :: %s",
                         fUridUnmap != nullptr ? fUridUnmap->unmap(fUridUnmap->handle, atom->type) : "(null)");
            }
        }
       #endif
    }

    // -------------------------------------------------------------------

    int lv2ui_idle()
    {
        if (fWinIdWasNull)
            return (fUI.plugin_idle() && fUI.isVisible()) ? 0 : 1;

        return fUI.plugin_idle() ? 0 : 1;
    }

    int lv2ui_show()
    {
        return fUI.setWindowVisible(true) ? 0 : 1;
    }

    int lv2ui_hide()
    {
        return fUI.setWindowVisible(false) ? 0 : 1;
    }

    // -------------------------------------------------------------------

    uint32_t lv2_get_options(LV2_Options_Option* const /*options*/)
    {
        // currently unused
        return LV2_OPTIONS_ERR_UNKNOWN;
    }

    uint32_t lv2_set_options(const LV2_Options_Option* const options)
    {
        for (int i=0; options[i].key != 0; ++i)
        {
            if (options[i].key == fURIDs.paramSampleRate)
            {
                if (options[i].type == fURIDs.atomFloat)
                {
                    const float sampleRate = *(const float*)options[i].value;
                    fUI.setSampleRate(sampleRate, true);
                    continue;
                }
                else
                {
                    d_stderr("Host changed UI sample-rate but with wrong value type");
                    continue;
                }
            }
        }

        return LV2_OPTIONS_SUCCESS;
    }

    // -------------------------------------------------------------------

   #if DISTRHO_PLUGIN_WANT_PROGRAMS
    void lv2ui_select_program(const uint32_t bank, const uint32_t program)
    {
        const uint32_t realProgram = bank * 128 + program;

        fUI.programLoaded(realProgram);
    }
   #endif

    // -------------------------------------------------------------------

private:
    // LV2 features
    const LV2_URID_Map*        const fUridMap;
    const LV2_URID_Unmap*      const fUridUnmap;
    const LV2UI_Port_Map*      const fUiPortMap;
    const LV2UI_Request_Value* const fUiRequestValue;
    const LV2UI_Touch*         const fUiTouch;

    // LV2 UI stuff
    const LV2UI_Controller     fController;
    const LV2UI_Write_Function fWriteFunction;

    // LV2 URIDs
    const struct URIDs {
        const LV2_URID_Map* _uridMap;
        const LV2_URID dpfKeyValue;
        const LV2_URID atomEventTransfer;
        const LV2_URID atomFloat;
        const LV2_URID atomLong;
        const LV2_URID atomObject;
        const LV2_URID atomPath;
        const LV2_URID atomString;
        const LV2_URID atomURID;
        const LV2_URID midiEvent;
        const LV2_URID paramSampleRate;
        const LV2_URID patchProperty;
        const LV2_URID patchSet;
        const LV2_URID patchValue;

        URIDs(const LV2_URID_Map* const uridMap)
            : _uridMap(uridMap),
              dpfKeyValue(map(DISTRHO_PLUGIN_LV2_STATE_PREFIX "KeyValueState")),
              atomEventTransfer(map(LV2_ATOM__eventTransfer)),
              atomFloat(map(LV2_ATOM__Float)),
              atomLong(map(LV2_ATOM__Long)),
              atomObject(map(LV2_ATOM__Object)),
              atomPath(map(LV2_ATOM__Path)),
              atomString(map(LV2_ATOM__String)),
              atomURID(map(LV2_ATOM__URID)),
              midiEvent(map(LV2_MIDI__MidiEvent)),
              paramSampleRate(map(LV2_PARAMETERS__sampleRate)),
              patchProperty(map(LV2_PATCH__property)),
              patchSet(map(LV2_PATCH__Set)),
              patchValue(map(LV2_PATCH__value)) {}

        inline LV2_URID map(const char* const uri) const
        {
            return _uridMap->map(_uridMap->handle, uri);
        }
    } fURIDs;

    // index of bypass parameter, if present
    const uint32_t fBypassParameterIndex;

    // using ui:showInterface if true
    const bool fWinIdWasNull;

    // Plugin UI (after LV2 stuff so the UI can call into us during its constructor)
    UIExporter fUI;

    // ----------------------------------------------------------------------------------------------------------------
    // DPF callbacks

    void editParameterValue(const uint32_t rindex, const bool started)
    {
        if (fUiTouch != nullptr && fUiTouch->touch != nullptr)
            fUiTouch->touch(fUiTouch->handle, rindex, started);
    }

    static void editParameterCallback(void* const ptr, const uint32_t rindex, const bool started)
    {
        static_cast<UiLv2*>(ptr)->editParameterValue(rindex, started);
    }

    void setParameterValue(const uint32_t rindex, float value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fWriteFunction != nullptr,);

        if (rindex == fBypassParameterIndex)
            value = 1.0f - value;

        fWriteFunction(fController, rindex, sizeof(float), 0, &value);
    }

    static void setParameterCallback(void* const ptr, const uint32_t rindex, const float value)
    {
        static_cast<UiLv2*>(ptr)->setParameterValue(rindex, value);
    }

   #if DISTRHO_PLUGIN_WANT_STATE
    void setState(const char* const key, const char* const value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fWriteFunction != nullptr,);

        const uint32_t eventInPortIndex = DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS;

        // join key and value
        String tmpStr;
        tmpStr += key;
        tmpStr += "\xff";
        tmpStr += value;

        tmpStr[std::strlen(key)] = '\0';

        // set msg size (key + separator + value + null terminator)
        const uint32_t msgSize = static_cast<uint32_t>(tmpStr.length()) + 1U;

        // reserve atom space
        const uint32_t atomSize = sizeof(LV2_Atom) + msgSize;
        char* const  atomBuf = (char*)malloc(atomSize);
        DISTRHO_SAFE_ASSERT_RETURN(atomBuf != nullptr,);

        std::memset(atomBuf, 0, atomSize);

        // set atom info
        LV2_Atom* const atom = (LV2_Atom*)atomBuf;
        atom->size = msgSize;
        atom->type = fURIDs.dpfKeyValue;

        // set atom data
        std::memcpy(atomBuf + sizeof(LV2_Atom), tmpStr.buffer(), msgSize);

        // send to DSP side
        fWriteFunction(fController, eventInPortIndex, atomSize, fURIDs.atomEventTransfer, atom);

        // free atom space
        free(atomBuf);
    }

    static void setStateCallback(void* const ptr, const char* const key, const char* const value)
    {
        static_cast<UiLv2*>(ptr)->setState(key, value);
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    void sendNote(const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fWriteFunction != nullptr,);

        if (channel > 0xF)
            return;

        const uint32_t eventInPortIndex = DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS;

        LV2_Atom_MidiEvent atomMidiEvent;
        atomMidiEvent.atom.size = 3;
        atomMidiEvent.atom.type = fURIDs.midiEvent;

        atomMidiEvent.data[0] = channel + (velocity != 0 ? 0x90 : 0x80);
        atomMidiEvent.data[1] = note;
        atomMidiEvent.data[2] = velocity;

        // send to DSP side
        fWriteFunction(fController, eventInPortIndex, lv2_atom_total_size(&atomMidiEvent.atom),
                       fURIDs.atomEventTransfer, &atomMidiEvent);
    }

    static void sendNoteCallback(void* const ptr, const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        static_cast<UiLv2*>(ptr)->sendNote(channel, note, velocity);
    }
   #endif

    bool fileRequest(const char* const key)
    {
        d_stdout("UI file request %s %p", key, fUiRequestValue);

        if (fUiRequestValue == nullptr)
            return false;

        String dpf_lv2_key(DISTRHO_PLUGIN_URI "#");
        dpf_lv2_key += key;

        const int r = fUiRequestValue->request(fUiRequestValue->handle,
                                        fUridMap->map(fUridMap->handle, dpf_lv2_key.buffer()),
                                        fURIDs.atomPath,
                                        nullptr);

        d_stdout("UI file request %s %p => %s %i", key, fUiRequestValue, dpf_lv2_key.buffer(), r);
        return r == LV2UI_REQUEST_VALUE_SUCCESS;
    }

    static bool fileRequestCallback(void* ptr, const char* key)
    {
        return static_cast<UiLv2*>(ptr)->fileRequest(key);
    }
};

// -----------------------------------------------------------------------

static LV2UI_Handle lv2ui_instantiate(const LV2UI_Descriptor*,
                                      const char* const uri,
                                      const char* const bundlePath,
                                      const LV2UI_Write_Function writeFunction,
                                      const LV2UI_Controller controller,
                                      LV2UI_Widget* const widget,
                                      const LV2_Feature* const* const features)
{
    if (uri == nullptr || std::strcmp(uri, DISTRHO_PLUGIN_URI) != 0)
    {
        d_stderr("Invalid plugin URI");
        return nullptr;
    }

    const LV2_Options_Option* options   = nullptr;
    const LV2_URID_Map*       uridMap   = nullptr;
    void*                     parentId  = nullptr;
    void*                     instance  = nullptr;

#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
    struct LV2_DirectAccess_Interface {
        void* (*get_instance_pointer)(LV2_Handle handle);
    };
    const LV2_Extension_Data_Feature* extData = nullptr;
#endif

    for (int i=0; features[i] != nullptr; ++i)
    {
        /**/ if (std::strcmp(features[i]->URI, LV2_OPTIONS__options) == 0)
            options = (const LV2_Options_Option*)features[i]->data;
        else if (std::strcmp(features[i]->URI, LV2_URID__map) == 0)
            uridMap = (const LV2_URID_Map*)features[i]->data;
        else if (std::strcmp(features[i]->URI, LV2_UI__parent) == 0)
            parentId = features[i]->data;
#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
        else if (std::strcmp(features[i]->URI, LV2_DATA_ACCESS_URI) == 0)
            extData = (const LV2_Extension_Data_Feature*)features[i]->data;
        else if (std::strcmp(features[i]->URI, LV2_INSTANCE_ACCESS_URI) == 0)
            instance = features[i]->data;
#endif
    }

    if (options == nullptr && parentId == nullptr)
    {
        d_stderr("Options feature missing (needed for show-interface), cannot continue!");
        return nullptr;
    }

    if (uridMap == nullptr)
    {
        d_stderr("URID Map feature missing, cannot continue!");
        return nullptr;
    }

    if (parentId == nullptr)
    {
        d_stdout("Parent Window Id missing, host should be using ui:showInterface...");
    }

#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
    if (extData == nullptr || instance == nullptr)
    {
        d_stderr("Data or instance access missing, cannot continue!");
        return nullptr;
    }

    if (const LV2_DirectAccess_Interface* const directAccess = (const LV2_DirectAccess_Interface*)extData->data_access(DISTRHO_PLUGIN_LV2_STATE_PREFIX "direct-access"))
        instance = directAccess->get_instance_pointer(instance);
    else
        instance = nullptr;

    if (instance == nullptr)
    {
        d_stderr("Failed to get direct access, cannot continue!");
        return nullptr;
    }
#endif

    const intptr_t winId = (intptr_t)parentId;
    float sampleRate = 0.0f;
    float scaleFactor = 0.0f;
    uint32_t bgColor = 0;
    uint32_t fgColor = 0xffffffff;
    const char* appClassName = nullptr;

    if (options != nullptr)
    {
        const LV2_URID uridAtomInt     = uridMap->map(uridMap->handle, LV2_ATOM__Int);
        const LV2_URID uridAtomFloat   = uridMap->map(uridMap->handle, LV2_ATOM__Float);
        const LV2_URID uridAtomString  = uridMap->map(uridMap->handle, LV2_ATOM__String);
        const LV2_URID uridSampleRate  = uridMap->map(uridMap->handle, LV2_PARAMETERS__sampleRate);
        const LV2_URID uridBgColor     = uridMap->map(uridMap->handle, LV2_UI__backgroundColor);
        const LV2_URID uridFgColor     = uridMap->map(uridMap->handle, LV2_UI__foregroundColor);
       #ifndef DISTRHO_OS_MAC
        const LV2_URID uridScaleFactor = uridMap->map(uridMap->handle, LV2_UI__scaleFactor);
       #endif
        const LV2_URID uridClassName   = uridMap->map(uridMap->handle, "urn:distrho:className");

        for (int i=0; options[i].key != 0; ++i)
        {
            /**/ if (options[i].key == uridSampleRate)
            {
                if (options[i].type == uridAtomFloat)
                    sampleRate = *(const float*)options[i].value;
                else
                    d_stderr("Host provides UI sample-rate but has wrong value type");
            }
            else if (options[i].key == uridBgColor)
            {
                if (options[i].type == uridAtomInt)
                    bgColor = (uint32_t)*(const int32_t*)options[i].value;
                else
                    d_stderr("Host provides UI background color but has wrong value type");
            }
            else if (options[i].key == uridFgColor)
            {
                if (options[i].type == uridAtomInt)
                    fgColor = (uint32_t)*(const int32_t*)options[i].value;
                else
                    d_stderr("Host provides UI foreground color but has wrong value type");
            }
           #ifndef DISTRHO_OS_MAC
            else if (options[i].key == uridScaleFactor)
            {
                if (options[i].type == uridAtomFloat)
                    scaleFactor = *(const float*)options[i].value;
                else
                    d_stderr("Host provides UI scale factor but has wrong value type");
            }
           #endif
            else if (options[i].key == uridClassName)
            {
                if (options[i].type == uridAtomString)
                    appClassName = (const char*)options[i].value;
                else
                    d_stderr("Host provides UI scale factor but has wrong value type");
            }
        }
    }

    if (sampleRate < 1.0)
    {
        d_stdout("WARNING: this host does not send sample-rate information for LV2 UIs, using 44100 as fallback (this could be wrong)");
        sampleRate = 44100.0;
    }

    return new UiLv2(bundlePath, winId, options, uridMap, features,
                     controller, writeFunction, widget, instance,
                     sampleRate, scaleFactor, bgColor, fgColor, appClassName);
}

#define uiPtr ((UiLv2*)ui)

static void lv2ui_cleanup(LV2UI_Handle ui)
{
    delete uiPtr;
}

static void lv2ui_port_event(LV2UI_Handle ui, uint32_t portIndex, uint32_t bufferSize, uint32_t format, const void* buffer)
{
    uiPtr->lv2ui_port_event(portIndex, bufferSize, format, buffer);
}

// -----------------------------------------------------------------------

static int lv2ui_idle(LV2UI_Handle ui)
{
    return uiPtr->lv2ui_idle();
}

static int lv2ui_show(LV2UI_Handle ui)
{
    return uiPtr->lv2ui_show();
}

static int lv2ui_hide(LV2UI_Handle ui)
{
    return uiPtr->lv2ui_hide();
}

// -----------------------------------------------------------------------

static uint32_t lv2_get_options(LV2UI_Handle ui, LV2_Options_Option* options)
{
    return uiPtr->lv2_get_options(options);
}

static uint32_t lv2_set_options(LV2UI_Handle ui, const LV2_Options_Option* options)
{
    return uiPtr->lv2_set_options(options);
}

// -----------------------------------------------------------------------

#if DISTRHO_PLUGIN_WANT_PROGRAMS
static void lv2ui_select_program(LV2UI_Handle ui, uint32_t bank, uint32_t program)
{
    uiPtr->lv2ui_select_program(bank, program);
}
#endif

// -----------------------------------------------------------------------

static const void* lv2ui_extension_data(const char* uri)
{
    static const LV2_Options_Interface options = { lv2_get_options, lv2_set_options };
    static const LV2UI_Idle_Interface  uiIdle  = { lv2ui_idle };
    static const LV2UI_Show_Interface  uiShow  = { lv2ui_show, lv2ui_hide };

    if (std::strcmp(uri, LV2_OPTIONS__interface) == 0)
        return &options;
    if (std::strcmp(uri, LV2_UI__idleInterface) == 0)
        return &uiIdle;
    if (std::strcmp(uri, LV2_UI__showInterface) == 0)
        return &uiShow;

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    static const LV2_Programs_UI_Interface uiPrograms = { lv2ui_select_program };

    if (std::strcmp(uri, LV2_PROGRAMS__UIInterface) == 0)
        return &uiPrograms;
#endif

    return nullptr;
}

#undef instancePtr

// -----------------------------------------------------------------------

static const LV2UI_Descriptor sLv2UiDescriptor = {
    DISTRHO_UI_URI,
    lv2ui_instantiate,
    lv2ui_cleanup,
    lv2ui_port_event,
    lv2ui_extension_data
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

DISTRHO_PLUGIN_EXPORT
const LV2UI_Descriptor* lv2ui_descriptor(uint32_t index)
{
    USE_NAMESPACE_DISTRHO
    return (index == 0) ? &sLv2UiDescriptor : nullptr;
}

#if defined(__MOD_DEVICES__) && defined(__EMSCRIPTEN__)
#include <emscripten/html5.h>
#include <string>

typedef void (*_custom_param_set)(uint32_t port_index, float value);
typedef void (*_custom_patch_set)(const char* uri, const char* value);

struct ModguiHandle {
    LV2UI_Handle handle;
    long loop_id;
    _custom_param_set param_set;
    _custom_patch_set patch_set;
};

enum URIs {
    kUriNull,
    kUriAtomEventTransfer,
    kUriDpfKeyValue,
};

static std::vector<std::string> kURIs;

static LV2_URID lv2_urid_map(LV2_URID_Map_Handle, const char* const uri)
{
    for (size_t i=0, size=kURIs.size(); i<size; ++i)
    {
        if (kURIs[i] == uri)
            return i;
    }

    kURIs.push_back(uri);
    return kURIs.size() - 1u;
}

static const char* lv2_urid_unmap(LV2_URID_Map_Handle, const LV2_URID urid)
{
    return kURIs[urid].c_str();
}

static void lv2ui_write_function(LV2UI_Controller controller,
                                 uint32_t         port_index,
                                 uint32_t         buffer_size,
                                 uint32_t         port_protocol,
                                 const void*      buffer)
{
    DISTRHO_SAFE_ASSERT_RETURN(buffer_size >= 1,);

    // d_stdout("lv2ui_write_function %p %u %u %u %p", controller, port_index, buffer_size, port_protocol, buffer);
    ModguiHandle* const mhandle = static_cast<ModguiHandle*>(controller);

    switch (port_protocol)
    {
    case kUriNull:
        mhandle->param_set(port_index, *static_cast<const float*>(buffer));
        break;
    case kUriAtomEventTransfer:
        if (const LV2_Atom* const atom = static_cast<const LV2_Atom*>(buffer))
        {
            // d_stdout("lv2ui_write_function %u %u:%s", atom->size, atom->type, kURIs[atom->type].c_str());

            // if (kURIs[atom->type] == "urn:distrho:KeyValueState")
            {
                const char* const key   = (const char*)(atom + 1);
                const char* const value = key + (std::strlen(key) + 1U);
                // d_stdout("lv2ui_write_function %s %s", key, value);

                String urikey;
                urikey  = DISTRHO_PLUGIN_URI "#";
                urikey += key;

                mhandle->patch_set(urikey, value);
            }
        }
        break;
    }
}

static void app_idle(void* const handle)
{
    static_cast<UiLv2*>(handle)->lv2ui_idle();
}

DISTRHO_PLUGIN_EXPORT
LV2UI_Handle modgui_init(const char* const className, _custom_param_set param_set, _custom_patch_set patch_set)
{
    d_stdout("init \"%s\"", className);
    DISTRHO_SAFE_ASSERT_RETURN(className != nullptr, nullptr);

    static LV2_URID_Map uridMap = { nullptr, lv2_urid_map };
    static LV2_URID_Unmap uridUnmap = { nullptr, lv2_urid_unmap };

    // known first URIDs, matching URIs
    if (kURIs.empty())
    {
        kURIs.push_back("");
        kURIs.push_back("http://lv2plug.in/ns/ext/atom#eventTransfer");
        kURIs.push_back(DISTRHO_PLUGIN_LV2_STATE_PREFIX "KeyValueState");
    }

    static float sampleRateValue = 48000.f;
    static LV2_Options_Option options[3] = {
        {
            LV2_OPTIONS_INSTANCE,
            0,
            uridMap.map(uridMap.handle, LV2_PARAMETERS__sampleRate),
            sizeof(float),
            uridMap.map(uridMap.handle, LV2_ATOM__Float),
            &sampleRateValue
        },
        {
            LV2_OPTIONS_INSTANCE,
            0,
            uridMap.map(uridMap.handle, "urn:distrho:className"),
            std::strlen(className) + 1,
            uridMap.map(uridMap.handle, LV2_ATOM__String),
            className
        },
        {}
    };

    static const LV2_Feature optionsFt = { LV2_OPTIONS__options, static_cast<void*>(options) };
    static const LV2_Feature uridMapFt = { LV2_URID__map, static_cast<void*>(&uridMap) };
    static const LV2_Feature uridUnmapFt = { LV2_URID__unmap, static_cast<void*>(&uridUnmap) };

    static const LV2_Feature* features[] = {
        &optionsFt,
        &uridMapFt,
        &uridUnmapFt,
        nullptr
    };

    ModguiHandle* const mhandle = new ModguiHandle;
    mhandle->handle = nullptr;
    mhandle->loop_id = 0;
    mhandle->param_set = param_set;
    mhandle->patch_set = patch_set;

    LV2UI_Widget widget;
    const LV2UI_Handle handle = lv2ui_instantiate(&sLv2UiDescriptor,
                                                  DISTRHO_PLUGIN_URI,
                                                  "", // bundlePath
                                                  lv2ui_write_function,
                                                  mhandle,
                                                  &widget,
                                                  features);
    mhandle->handle = handle;

    static_cast<UiLv2*>(handle)->lv2ui_show();
    mhandle->loop_id = emscripten_set_interval(app_idle, 1000.0/60, handle);

    return mhandle;
}

DISTRHO_PLUGIN_EXPORT
void modgui_param_set(const LV2UI_Handle handle, const uint32_t index, const float value)
{
    lv2ui_port_event(static_cast<ModguiHandle*>(handle)->handle, index, sizeof(float), kUriNull, &value);
}

DISTRHO_PLUGIN_EXPORT
void modgui_patch_set(const LV2UI_Handle handle, const char* const uri, const char* const value)
{
    static const constexpr uint32_t URI_PREFIX_LEN = sizeof(DISTRHO_PLUGIN_URI);
    DISTRHO_SAFE_ASSERT_RETURN(std::strncmp(uri, DISTRHO_PLUGIN_URI "#", URI_PREFIX_LEN) == 0,);

    const uint32_t keySize = std::strlen(uri + URI_PREFIX_LEN) + 1;
    const uint32_t valueSize = std::strlen(value) + 1;
    const uint32_t atomSize = sizeof(LV2_Atom) + keySize + valueSize;

    LV2_Atom* const atom = static_cast<LV2_Atom*>(std::malloc(atomSize));
    atom->size = atomSize;
    atom->type = kUriDpfKeyValue;

    std::memcpy(static_cast<uint8_t*>(static_cast<void*>(atom + 1)), uri + URI_PREFIX_LEN, keySize);
    std::memcpy(static_cast<uint8_t*>(static_cast<void*>(atom + 1)) + keySize, value, valueSize);

    lv2ui_port_event(static_cast<ModguiHandle*>(handle)->handle,
                     DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS, // events input port
                     atomSize, kUriAtomEventTransfer, atom);

    std::free(atom);
}

DISTRHO_PLUGIN_EXPORT
void modgui_cleanup(const LV2UI_Handle handle)
{
    d_stdout("cleanup");
    ModguiHandle* const mhandle = static_cast<ModguiHandle*>(handle);
    if (mhandle->loop_id != 0)
        emscripten_clear_interval(mhandle->loop_id);
    lv2ui_cleanup(mhandle->handle);
    delete mhandle;
}
#endif

// -----------------------------------------------------------------------
