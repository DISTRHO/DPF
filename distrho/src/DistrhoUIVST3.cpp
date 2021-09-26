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

#include <atomic>

// TESTING awful idea dont reuse
#include "../extra/Thread.hpp"

#if DISTRHO_PLUGIN_HAS_UI && ! DISTRHO_PLUGIN_HAS_EMBED_UI
# undef DISTRHO_PLUGIN_HAS_UI
# define DISTRHO_PLUGIN_HAS_UI 0
#endif

#if DISTRHO_PLUGIN_HAS_UI && ! defined(HAVE_DGL) && ! DISTRHO_PLUGIN_HAS_EXTERNAL_UI
# undef DISTRHO_PLUGIN_HAS_UI
# define DISTRHO_PLUGIN_HAS_UI 0
#endif

// #undef DISTRHO_PLUGIN_HAS_UI
// #define DISTRHO_PLUGIN_HAS_UI 1

// #if DISTRHO_PLUGIN_HAS_UI
# include "DistrhoUIInternal.hpp"
// #endif

#include "travesty/edit_controller.h"
#include "travesty/message.h"
#include "travesty/view.h"

#include <atomic>
#include <map>
#include <string>

/* TODO items:
 * - disable UI if non-embed UI build
 * - parameter change listener
 * - parameter change sender
 * - program change listener
 * - program change sender
 * - state change listener
 * - state change sender
 * - sample rate change listener
 * - call component handler restart with params-changed flag when setting program?
 */

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

#if ! DISTRHO_PLUGIN_WANT_MIDI_INPUT
static constexpr const sendNoteFunc sendNoteCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_STATE
static constexpr const setStateFunc setStateCallback = nullptr;
#endif

// --------------------------------------------------------------------------------------------------------------------
// Utility functions (defined on plugin side)

const char* tuid2str(const v3_tuid iid);
void strncpy_utf16(int16_t* dst, const char* src, size_t length);

// --------------------------------------------------------------------------------------------------------------------
// create message object (needed by the UI class, implementation comes later)

static v3_message** dpf_message_create(const char* id);

// --------------------------------------------------------------------------------------------------------------------
// custom attribute list struct, used for sending utf8 strings

struct v3_attribute_list_utf8 {
    V3_API v3_result (*set_string_utf8)(void* self, const char* id, const char* string);
    V3_API v3_result (*get_string_utf8)(void* self, const char* id, char* string, uint32_t size);
};

static constexpr const v3_tuid v3_attribute_list_utf8_iid =
    V3_ID(d_cconst('D','P','F',' '),
          d_cconst('c','l','a','s'),
          d_cconst('a','t','t','r'),
          d_cconst('u','t','f','8'));

// --------------------------------------------------------------------------------------------------------------------

/**
 * VST3 UI class.
 *
 * All the dynamic things from VST3 get implemented here, free of complex low-level VST3 pointer things.
 * The UI is created during the "attach" view event, and destroyed during "removed".
 *
 * Note that DPF VST3 implementation works over the connection point interface,
 * rather than using edit controller directly.
 * This allows the UI to be running remotely from the DSP.
 *
 * The low-level VST3 stuff comes after.
 */
class UIVst3 : public Thread
{
public:
    UIVst3(v3_plugin_view** const view,
           v3_connection_point** const connection,
           const intptr_t winId,
           const float scaleFactor,
           const double sampleRate,
           void* const instancePointer)
        : fUI(this, winId, sampleRate,
              editParameterCallback,
              setParameterCallback,
              setStateCallback,
              sendNoteCallback,
              setSizeCallback,
              nullptr, // TODO file request
              nullptr,
              instancePointer,
              scaleFactor),
          fView(view),
          fConnection(connection),
          fFrame(nullptr),
          fScaleFactor(scaleFactor)
    {
        // TESTING awful idea dont reuse
        startThread();
    }

    ~UIVst3() override
    {
        stopThread(5000);
    }

    // TESTING awful idea dont reuse
    void run() override
    {
        while (! shouldThreadExit())
        {
            fUI.plugin_idle();
            d_msleep(50);
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_plugin_view interface calls

    v3_result onWheel(float /*distance*/)
    {
        // TODO
        return V3_NOT_IMPLEMENTED;
    };

    v3_result onKeyDown(int16_t /*key_char*/, int16_t /*key_code*/, int16_t /*modifiers*/)
    {
        // TODO
        return V3_NOT_IMPLEMENTED;
    };

    v3_result onKeyUp(int16_t /*key_char*/, int16_t /*key_code*/, int16_t /*modifiers*/)
    {
        // TODO
        return V3_NOT_IMPLEMENTED;
    };

    v3_result getSize(v3_view_rect* const rect) const noexcept
    {
        std::memset(rect, 0, sizeof(v3_view_rect));

        rect->right = fUI.getWidth();
        rect->bottom = fUI.getHeight();
#ifdef DISTRHO_OS_MAC
        const double scaleFactor = fUI.getScaleFactor();
        rect->right /= scaleFactor;
        rect->bottom /= scaleFactor;
#endif

        return V3_OK;
    }

    v3_result onSize(v3_view_rect* const /*rect*/)
    {
        // TODO
        return V3_NOT_IMPLEMENTED;
    }

    v3_result onFocus(const bool state)
    {
#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
        fUI.notifyFocusChanged(state);
        return V3_OK;
#else
        return V3_NOT_IMPLEMENTED;
        // unused
        (void)state;
#endif
    }

    v3_result setFrame(v3_plugin_frame** const frame) noexcept
    {
        fFrame = frame;
        return V3_OK;
    }

    v3_result checkSizeConstraint(v3_view_rect* const /*rect*/)
    {
        // TODO
        return V3_NOT_IMPLEMENTED;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_plugin_view_content_scale_steinberg interface calls

    v3_result setContentScaleFactor(const float factor)
    {
        if (d_isEqual(fScaleFactor, factor))
            return V3_OK;

        fScaleFactor = factor;
        fUI.notifyScaleFactorChanged(factor);
        return V3_OK;
    }

    // ----------------------------------------------------------------------------------------------------------------

private:
    // Plugin UI
    UIExporter fUI;

    // VST3 stuff
    v3_plugin_view** const fView;
    v3_connection_point** const fConnection;
    v3_plugin_frame** fFrame;

    // Temporary data
    float fScaleFactor;

    // ----------------------------------------------------------------------------------------------------------------
    // DPF callbacks

    void editParameter(const uint32_t rindex, const bool started) const
    {
        DISTRHO_SAFE_ASSERT_RETURN(fConnection != nullptr,);

        v3_message** const message = dpf_message_create("parameter-edit");
        DISTRHO_SAFE_ASSERT_RETURN(message != nullptr,);

        v3_attribute_list** const attrs = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrs != nullptr,);

        v3_cpp_obj(attrs)->set_int(attrs, "rindex", rindex);
        v3_cpp_obj(attrs)->set_int(attrs, "started", started ? 1 : 0);
        v3_cpp_obj(fConnection)->notify(fConnection, message);

        v3_cpp_obj_unref(message);
    }

    static void editParameterCallback(void* ptr, uint32_t rindex, bool started)
    {
        ((UIVst3*)ptr)->editParameter(rindex, started);
    }

    void setParameterValue(const uint32_t rindex, const float realValue)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fConnection != nullptr,);

        v3_message** const message = dpf_message_create("parameter-set");
        DISTRHO_SAFE_ASSERT_RETURN(message != nullptr,);

        v3_attribute_list** const attrs = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrs != nullptr,);

        v3_cpp_obj(attrs)->set_int(attrs, "rindex", rindex);
        v3_cpp_obj(attrs)->set_float(attrs, "value", realValue);
        v3_cpp_obj(fConnection)->notify(fConnection, message);

        v3_cpp_obj_unref(message);
    }

    static void setParameterCallback(void* ptr, uint32_t rindex, float value)
    {
        ((UIVst3*)ptr)->setParameterValue(rindex, value);
    }

    void setSize(uint width, uint height)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fView != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fFrame != nullptr,);
        d_stdout("from UI setSize %u %u | %p %p", width, height, fView, fFrame);

// #ifdef DISTRHO_OS_MAC
//         const double scaleFactor = fUI.getScaleFactor();
//         width /= scaleFactor;
//         height /= scaleFactor;
// #endif
//
//         v3_view_rect rect;
//         std::memset(&rect, 0, sizeof(rect));
//         rect.right = width;
//         rect.bottom = height;
//         v3_cpp_obj(fFrame)->resize_view(fFrame, fView, &rect);
    }

    static void setSizeCallback(void* ptr, uint width, uint height)
    {
        ((UIVst3*)ptr)->setSize(width, height);
    }

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    void sendNote(const uint8_t /*channel*/, const uint8_t /*note*/, const uint8_t /*velocity*/)
    {
//         uint8_t midiData[3];
//         midiData[0] = (velocity != 0 ? 0x90 : 0x80) | channel;
//         midiData[1] = note;
//         midiData[2] = velocity;
//         fNotesRingBuffer.writeCustomData(midiData, 3);
//         fNotesRingBuffer.commitWrite();
    }

    static void sendNoteCallback(void* ptr, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        ((UIVst3*)ptr)->sendNote(channel, note, velocity);
    }
#endif

    void setState(const char* const key, const char* const value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fConnection != nullptr,);

        v3_message** const message = dpf_message_create("state-set");
        DISTRHO_SAFE_ASSERT_RETURN(message != nullptr,);

        v3_attribute_list** const attrs = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrs != nullptr,);

        v3_attribute_list_utf8** utf8attrs = nullptr;
        DISTRHO_SAFE_ASSERT_RETURN(v3_cpp_obj_query_interface(attrs, v3_attribute_list_utf8_iid, &utf8attrs) == V3_OK,);
        DISTRHO_SAFE_ASSERT_RETURN(utf8attrs != nullptr,);

        v3_cpp_obj(utf8attrs)->set_string_utf8(utf8attrs, "key", key);
        v3_cpp_obj(utf8attrs)->set_string_utf8(utf8attrs, "value", value);
        v3_cpp_obj(fConnection)->notify(fConnection, message);

        v3_cpp_obj_unref(message);
    }

#if DISTRHO_PLUGIN_WANT_STATE
    static void setStateCallback(void* ptr, const char* key, const char* value)
    {
        ((UIVst3*)ptr)->setState(key, value);
    }
#endif
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * VST3 low-level pointer thingies follow, proceed with care.
 */

// --------------------------------------------------------------------------------------------------------------------

struct dpf_attribute_value {
    char type; // one of: i, f, s, b
    union {
        int64_t integer;
        double v_float;
        int16_t* string;
        struct {
            void* ptr;
            uint32_t size;
        } binary;
    };
};

static void dpf_attribute_list_free(std::map<std::string, dpf_attribute_value>& attrs)
{
    for (auto& it : attrs)
    {
        dpf_attribute_value& v(it.second);

        switch (v.type)
        {
        case 's':
        case 'b':
            std::free(v.binary.ptr);
            break;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
// dpf_attribute_list_dpf (the custom utf8 variant)

struct v3_attribute_list_utf8_cpp : v3_funknown {
    v3_attribute_list_utf8 attr;
};

struct dpf_attribute_list_dpf : v3_attribute_list_utf8_cpp {
    std::map<std::string, dpf_attribute_value>& attrs;

    dpf_attribute_list_dpf(std::map<std::string, dpf_attribute_value>& a)
        : attrs(a)
    {
        static const uint8_t* kSupportedInterfaces[] = {
            v3_funknown_iid,
            v3_attribute_list_utf8_iid
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            d_stdout("dpf_attribute_list_dpf::query_interface => %p %s %p", self, tuid2str(iid), iface);
            *iface = NULL;
            DISTRHO_SAFE_ASSERT_RETURN(self != nullptr, V3_NO_INTERFACE);

            for (const uint8_t* interface_iid : kSupportedInterfaces)
            {
                if (v3_tuid_match(interface_iid, iid))
                {
                    *iface = self;
                    return V3_OK;
                }
            }

            return V3_NO_INTERFACE;
        };

        // there is only a single instance of this, so we don't have to care here
        ref = []V3_API(void*) -> uint32_t { return 1; };
        unref = []V3_API(void*) -> uint32_t { return 0; };

        // ------------------------------------------------------------------------------------------------------------
        // v3_attribute_list_utf8

        attr.set_string_utf8 = []V3_API(void* self, const char* id, const char* string) -> v3_result
        {
            dpf_attribute_list_dpf* const attr = *(dpf_attribute_list_dpf**)self;
            DISTRHO_SAFE_ASSERT_RETURN(attr != nullptr, V3_NOT_INITIALISED);

            const uint32_t size = std::strlen(string) + 1;
            int16_t* const copy = (int16_t*)std::malloc(sizeof(int16_t) * size);
            DISTRHO_SAFE_ASSERT_RETURN(copy != nullptr, V3_NOMEM);

            DISTRHO_NAMESPACE::strncpy_utf16(copy, string, size);

            dpf_attribute_value& attrval(attr->attrs[id]);
            attrval.type = 's';
            attrval.binary.ptr = copy;
            attrval.binary.size = sizeof(int16_t) * size;
            return V3_OK;
        };

        attr.get_string_utf8 = []V3_API(void* self, const char* id, char*, uint32_t) -> v3_result
        {
            dpf_attribute_list_dpf* const attr = *(dpf_attribute_list_dpf**)self;
            DISTRHO_SAFE_ASSERT_RETURN(attr != nullptr, V3_NOT_INITIALISED);

            if (attr->attrs.find(id) == attr->attrs.end())
                return V3_INVALID_ARG;

            return V3_NOT_IMPLEMENTED;
        };
    }
};

// --------------------------------------------------------------------------------------------------------------------
// dpf_attribute_list

struct v3_attribute_list_cpp : v3_funknown {
    v3_attribute_list attr;
};

struct dpf_attribute_list : v3_attribute_list_cpp {
    ScopedPointer<dpf_attribute_list_dpf> attrdpf;
    std::map<std::string, dpf_attribute_value> attrs;

    dpf_attribute_list()
    {
        static const uint8_t* kSupportedInterfacesBase[] = {
            v3_funknown_iid,
            v3_attribute_list_iid
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            d_stdout("dpf_attribute_list::query_interface => %p %s %p", self, tuid2str(iid), iface);
            *iface = NULL;
            DISTRHO_SAFE_ASSERT_RETURN(self != nullptr, V3_NO_INTERFACE);

            for (const uint8_t* interface_iid : kSupportedInterfacesBase)
            {
                if (v3_tuid_match(interface_iid, iid))
                {
                    *iface = self;
                    return V3_OK;
                }
            }

            dpf_attribute_list* const attr = *(dpf_attribute_list**)self;
            DISTRHO_SAFE_ASSERT_RETURN(attr != nullptr, V3_NO_INTERFACE);

            if (v3_tuid_match(v3_attribute_list_utf8_iid, iid))
            {
                if (attr->attrdpf == nullptr)
                    attr->attrdpf = new dpf_attribute_list_dpf(attr->attrs);
                *iface = &attr->attrdpf;
                return V3_OK;
            }

            return V3_NO_INTERFACE;
        };

        // there is only a single instance of this, so we don't have to care here
        ref = []V3_API(void*) -> uint32_t { return 1; };
        unref = []V3_API(void*) -> uint32_t { return 0; };

        // ------------------------------------------------------------------------------------------------------------
        // v3_attribute_list

        attr.set_int = []V3_API(void* self, const char* id, int64_t value) -> v3_result
        {
            dpf_attribute_list* const attr = *(dpf_attribute_list**)self;
            DISTRHO_SAFE_ASSERT_RETURN(attr != nullptr, V3_NOT_INITIALISED);

            dpf_attribute_value& attrval(attr->attrs[id]);
            attrval.type = 'i';
            attrval.integer = value;
            return V3_OK;
        };

        attr.get_int = []V3_API(void* self, const char* id, int64_t* value) -> v3_result
        {
            dpf_attribute_list* const attr = *(dpf_attribute_list**)self;
            DISTRHO_SAFE_ASSERT_RETURN(attr != nullptr, V3_NOT_INITIALISED);

            if (attr->attrs.find(id) == attr->attrs.end())
                return V3_INVALID_ARG;

            const dpf_attribute_value& attrval(attr->attrs[id]);
            DISTRHO_SAFE_ASSERT_RETURN(attrval.type == 'i', V3_INVALID_ARG);

            *value = attrval.integer;
            return V3_OK;
        };

        attr.set_float = []V3_API(void* self, const char* id, double value) -> v3_result
        {
            dpf_attribute_list* const attr = *(dpf_attribute_list**)self;
            DISTRHO_SAFE_ASSERT_RETURN(attr != nullptr, V3_NOT_INITIALISED);

            dpf_attribute_value& attrval(attr->attrs[id]);
            attrval.type = 'f';
            attrval.v_float = value;
            return V3_OK;
        };

        attr.get_float = []V3_API(void* self, const char* id, double* value) -> v3_result
        {
            dpf_attribute_list* const attr = *(dpf_attribute_list**)self;
            DISTRHO_SAFE_ASSERT_RETURN(attr != nullptr, V3_NOT_INITIALISED);

            if (attr->attrs.find(id) == attr->attrs.end())
                return V3_INVALID_ARG;

            const dpf_attribute_value& attrval(attr->attrs[id]);
            DISTRHO_SAFE_ASSERT_RETURN(attrval.type == 'f', V3_INVALID_ARG);

            *value = attrval.v_float;
            return V3_OK;
        };

        attr.set_string = []V3_API(void* self, const char* id, const int16_t* /*string*/) -> v3_result
        {
            dpf_attribute_list* const attr = *(dpf_attribute_list**)self;
            DISTRHO_SAFE_ASSERT_RETURN(attr != nullptr, V3_NOT_INITIALISED);

            dpf_attribute_value& attrval(attr->attrs[id]);
            attrval.type = 's';
            attrval.binary.ptr = nullptr;
            attrval.binary.size = 0;
            return V3_NOT_IMPLEMENTED;
        };

        attr.get_string = []V3_API(void* self, const char* id, int16_t* /*string*/, uint32_t /*size*/) -> v3_result
        {
            dpf_attribute_list* const attr = *(dpf_attribute_list**)self;
            DISTRHO_SAFE_ASSERT_RETURN(attr != nullptr, V3_NOT_INITIALISED);

            if (attr->attrs.find(id) == attr->attrs.end())
                return V3_INVALID_ARG;

            const dpf_attribute_value& attrval(attr->attrs[id]);
            DISTRHO_SAFE_ASSERT_RETURN(attrval.type == 's', V3_INVALID_ARG);

            return V3_NOT_IMPLEMENTED;
        };

        attr.set_binary = []V3_API(void* self, const char* id, const void* data, uint32_t size) -> v3_result
        {
            dpf_attribute_list* const attr = *(dpf_attribute_list**)self;
            DISTRHO_SAFE_ASSERT_RETURN(attr != nullptr, V3_NOT_INITIALISED);

            void* const copy = std::malloc(size);
            DISTRHO_SAFE_ASSERT_RETURN(copy != nullptr, V3_NOMEM);

            std::memcpy(copy, data, size);

            dpf_attribute_value& attrval(attr->attrs[id]);
            attrval.type = 'b';
            attrval.binary.ptr = copy;
            attrval.binary.size = size;
            return V3_NOT_IMPLEMENTED;
        };

        attr.get_binary = []V3_API(void* self, const char* id, const void** data, uint32_t* size) -> v3_result
        {
            dpf_attribute_list* const attr = *(dpf_attribute_list**)self;
            DISTRHO_SAFE_ASSERT_RETURN(attr != nullptr, V3_NOT_INITIALISED);

            if (attr->attrs.find(id) == attr->attrs.end())
                return V3_INVALID_ARG;

            const dpf_attribute_value& attrval(attr->attrs[id]);
            DISTRHO_SAFE_ASSERT_RETURN(attrval.type == 's' || attrval.type == 'b', V3_INVALID_ARG);

            *data = attrval.binary.ptr;
            *size = attrval.binary.size;
            return V3_OK;
        };
    }
};

// --------------------------------------------------------------------------------------------------------------------
// dpf_message

struct v3_message_cpp : v3_funknown {
    v3_message msg;
};

struct dpf_message : v3_message_cpp {
    std::atomic<int> refcounter;
    ScopedPointer<dpf_message>* self;
    ScopedPointer<dpf_attribute_list> attrlist;
    String id;

    dpf_message(ScopedPointer<dpf_message>* const s, const char* const id2)
        : self(s),
          id(id2)
    {
        static const uint8_t* kSupportedInterfaces[] = {
            v3_funknown_iid,
            v3_message_iid
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            d_stdout("dpf_plugin_view::query_interface         => %p %s %p", self, tuid2str(iid), iface);
            *iface = NULL;
            DISTRHO_SAFE_ASSERT_RETURN(self != nullptr, V3_NO_INTERFACE);

            for (const uint8_t* interface_iid : kSupportedInterfaces)
            {
                if (v3_tuid_match(interface_iid, iid))
                {
                    *iface = self;
                    return V3_OK;
                }
            }

            return V3_NO_INTERFACE;
        };

        ref = []V3_API(void* const self) -> uint32_t
        {
            d_stdout("dpf_message::ref                     => %p", self);
            dpf_message** const messageptr = static_cast<dpf_message**>(self);
            DISTRHO_SAFE_ASSERT_RETURN(messageptr != nullptr, 0);
            dpf_message* const message = *messageptr;
            DISTRHO_SAFE_ASSERT_RETURN(message != nullptr, 0);

            return ++message->refcounter;
        };

        unref = []V3_API(void* const self) -> uint32_t
        {
            d_stdout("dpf_message::unref                   => %p", self);
            dpf_message** const messageptr = static_cast<dpf_message**>(self);
            DISTRHO_SAFE_ASSERT_RETURN(messageptr != nullptr, 0);
            dpf_message* const message = *messageptr;
            DISTRHO_SAFE_ASSERT_RETURN(message != nullptr, 0);

            if (const int refcounter = --message->refcounter)
                return refcounter;

            if (message->attrlist != nullptr)
                dpf_attribute_list_free(message->attrlist->attrs);

            *message->self = nullptr;
            delete messageptr;
            return 0;
        };

        msg.get_message_id = []V3_API(void* const self) -> const char*
        {
            d_stdout("dpf_message::get_message_id => %p", self);
            dpf_message* const message = *(dpf_message**)self;
            DISTRHO_SAFE_ASSERT_RETURN(message != nullptr, nullptr);

            return message->id;
        };

        msg.set_message_id = []V3_API(void* const self, const char* const id) -> void
        {
            d_stdout("dpf_message::set_message_id => %p %s", self, id);
            dpf_message* const message = *(dpf_message**)self;
            DISTRHO_SAFE_ASSERT_RETURN(message != nullptr,);

            message->id = id;
        };

        msg.get_attributes = []V3_API(void* const self) -> v3_attribute_list**
        {
            d_stdout("dpf_message::get_attributes => %p", self);
            dpf_message* const message = *(dpf_message**)self;
            DISTRHO_SAFE_ASSERT_RETURN(message != nullptr, nullptr);

            if (message->attrlist == nullptr)
                message->attrlist = new dpf_attribute_list();

            return (v3_attribute_list**)&message->attrlist;
        };
    }
};

static v3_message** dpf_message_create(const char* const id)
{
    ScopedPointer<dpf_message>* const messageptr = new ScopedPointer<dpf_message>;
    *messageptr = new dpf_message(messageptr, id);
    return static_cast<v3_message**>(static_cast<void*>(messageptr));
}

// --------------------------------------------------------------------------------------------------------------------
// dpf_plugin_view_content_scale

struct v3_plugin_view_content_scale_cpp : v3_funknown {
    v3_plugin_view_content_scale scale;
};

struct dpf_plugin_view_content_scale : v3_plugin_view_content_scale_cpp {
    ScopedPointer<UIVst3>& uivst3;
    // cached values
    float scaleFactor;

    dpf_plugin_view_content_scale(ScopedPointer<UIVst3>& v)
        : uivst3(v),
          scaleFactor(0.0f)
    {
        query_interface = query_interface_fn;
        ref = ref_fn;
        unref = unref_fn;
        scale.set_content_scale_factor = set_content_scale_factor_fn;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_funknown

    static V3_API v3_result query_interface_fn(void* self, const v3_tuid iid, void** iface)
    {
        d_stdout("dpf_plugin_view_content_scale::query_interface    => %p %s %p", self, tuid2str(iid), iface);
        *iface = NULL;
        DISTRHO_SAFE_ASSERT_RETURN(self != nullptr, V3_NO_INTERFACE);

        static const uint8_t* kSupportedInterfaces[] = {
            v3_funknown_iid,
            v3_plugin_view_content_scale_iid
        };

        for (const uint8_t* interface_iid : kSupportedInterfaces)
        {
            if (v3_tuid_match(interface_iid, iid))
            {
                *iface = self;
                return V3_OK;
            }
        }

        return V3_NO_INTERFACE;
    }

    // there is only a single instance of this, so we don't have to care here
    static V3_API uint32_t ref_fn(void*) { return 1; };
    static V3_API uint32_t unref_fn(void*) { return 0; };

    // ----------------------------------------------------------------------------------------------------------------
    // v3_plugin_view_content_scale_steinberg

    static V3_API v3_result set_content_scale_factor_fn(void* self, float factor)
    {
        d_stdout("dpf_plugin_view::set_content_scale_factor => %p %f", self, factor);
        dpf_plugin_view_content_scale* const scale = *(dpf_plugin_view_content_scale**)self;
        DISTRHO_SAFE_ASSERT_RETURN(scale != nullptr, V3_NOT_INITIALISED);

        scale->scaleFactor = factor;

        if (UIVst3* const uivst3 = scale->uivst3)
            return uivst3->setContentScaleFactor(factor);

        return V3_NOT_INITIALISED;
    }
};

// --------------------------------------------------------------------------------------------------------------------
// dpf_plugin_view

struct v3_plugin_view_cpp : v3_funknown {
    v3_plugin_view view;
};

struct dpf_plugin_view : v3_plugin_view_cpp {
    std::atomic<int> refcounter;
    ScopedPointer<dpf_plugin_view>* self;
    ScopedPointer<dpf_plugin_view_content_scale> scale;
    ScopedPointer<UIVst3> uivst3;
    // cached values
    v3_connection_point** const connection;
    void* const instancePointer;
    double sampleRate;
    v3_plugin_frame** frame = nullptr;

    dpf_plugin_view(ScopedPointer<dpf_plugin_view>* selfptr,
                    v3_connection_point** const connectionptr,
                    void* const instance,
                    const double sr)
        : refcounter(1),
          self(selfptr),
          connection(connectionptr),
          instancePointer(instance),
          sampleRate(sr)
    {
        static const uint8_t* kSupportedInterfacesBase[] = {
            v3_funknown_iid,
            v3_plugin_view_iid
        };

        static const char* const kSupportedPlatforms[] = {
#ifdef _WIN32
            V3_VIEW_PLATFORM_TYPE_HWND,
#elif defined(__APPLE__)
            V3_VIEW_PLATFORM_TYPE_NSVIEW,
#else
            V3_VIEW_PLATFORM_TYPE_X11,
#endif
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            d_stdout("dpf_plugin_view::query_interface         => %p %s %p", self, tuid2str(iid), iface);
            *iface = NULL;
            DISTRHO_SAFE_ASSERT_RETURN(self != nullptr, V3_NO_INTERFACE);

            for (const uint8_t* interface_iid : kSupportedInterfacesBase)
            {
                if (v3_tuid_match(interface_iid, iid))
                {
                    *iface = self;
                    return V3_OK;
                }
            }

            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NO_INTERFACE);

            if (v3_tuid_match(v3_plugin_view_content_scale_iid, iid))
            {
                if (view->scale == nullptr)
                    view->scale = new dpf_plugin_view_content_scale(view->uivst3);
                *iface = &view->scale;
                return V3_OK;
            }

            return V3_NO_INTERFACE;
        };

        ref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_plugin_view::ref                     => %p", self);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, 0);

            return ++view->refcounter;
        };

        unref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_plugin_view::unref                   => %p", self);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, 0);

            if (const int refcounter = --view->refcounter)
                return refcounter;

            *view->self = nullptr;
            delete (dpf_plugin_view**)self;
            return 0;
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_plugin_view

        view.is_platform_type_supported = []V3_API(void* self, const char* platform_type) -> v3_result
        {
            d_stdout("dpf_plugin_view::is_platform_type_supported => %p %s", self, platform_type);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);

            for (size_t i=0; i<ARRAY_SIZE(kSupportedPlatforms); ++i)
            {
                if (std::strcmp(kSupportedPlatforms[i], platform_type) == 0)
                    return V3_OK;
            }

            return V3_NOT_IMPLEMENTED;
        };

        view.attached = []V3_API(void* self, void* parent, const char* platform_type) -> v3_result
        {
            d_stdout("dpf_plugin_view::attached                   => %p %p %s", self, parent, platform_type);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);
            DISTRHO_SAFE_ASSERT_RETURN(view->uivst3 == nullptr, V3_INVALID_ARG);

            for (size_t i=0; i<ARRAY_SIZE(kSupportedPlatforms); ++i)
            {
                if (std::strcmp(kSupportedPlatforms[i], platform_type) == 0)
                {
                    const float scaleFactor = view->scale != nullptr ? view->scale->scaleFactor : 0.0f;
                    view->uivst3 = new UIVst3((v3_plugin_view**)view->self,
                                              view->connection,
                                              (uintptr_t)parent,
                                              scaleFactor,
                                              view->sampleRate,
                                              view->instancePointer);
                    view->uivst3->setFrame(view->frame);
                    return V3_OK;
                }
            }

            return V3_NOT_IMPLEMENTED;
        };

        view.removed = []V3_API(void* self) -> v3_result
        {
            d_stdout("dpf_plugin_view::removed                    => %p", self);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);
            DISTRHO_SAFE_ASSERT_RETURN(view->uivst3 != nullptr, V3_INVALID_ARG);
            view->uivst3 = nullptr;
            return V3_OK;
        };

        view.on_wheel = []V3_API(void* self, float distance) -> v3_result
        {
            d_stdout("dpf_plugin_view::on_wheel                   => %p %f", self, distance);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);

            UIVst3* const uivst3 = view->uivst3;
            DISTRHO_SAFE_ASSERT_RETURN(uivst3 != nullptr, V3_NOT_INITIALISED);

            return uivst3->onWheel(distance);
        };

        view.on_key_down = []V3_API(void* self, int16_t key_char, int16_t key_code, int16_t modifiers) -> v3_result
        {
            d_stdout("dpf_plugin_view::on_key_down                => %p %i %i %i", self, key_char, key_code, modifiers);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);

            UIVst3* const uivst3 = view->uivst3;
            DISTRHO_SAFE_ASSERT_RETURN(uivst3 != nullptr, V3_NOT_INITIALISED);

            return uivst3->onKeyDown(key_char, key_code, modifiers);
        };

        view.on_key_up = []V3_API(void* self, int16_t key_char, int16_t key_code, int16_t modifiers) -> v3_result
        {
            d_stdout("dpf_plugin_view::on_key_up                  => %p %i %i %i", self, key_char, key_code, modifiers);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);

            UIVst3* const uivst3 = view->uivst3;
            DISTRHO_SAFE_ASSERT_RETURN(uivst3 != nullptr, V3_NOT_INITIALISED);

            return uivst3->onKeyUp(key_char, key_code, modifiers);
        };

        view.get_size = []V3_API(void* self, v3_view_rect* rect) -> v3_result
        {
            d_stdout("dpf_plugin_view::get_size                   => %p", self);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);

            if (UIVst3* const uivst3 = view->uivst3)
                return uivst3->getSize(rect);

            // special case: allow UI to not be attached yet, as a way to get size before window creation

            const float scaleFactor = view->scale != nullptr ? view->scale->scaleFactor : 0.0f;
            UIExporter tmpUI(nullptr, 0, view->sampleRate,
                             nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                             view->instancePointer, scaleFactor);
            rect->right = tmpUI.getWidth();
            rect->bottom = tmpUI.getHeight();
            return V3_OK;
        };

        view.on_size = []V3_API(void* self, v3_view_rect* rect) -> v3_result
        {
            d_stdout("dpf_plugin_view::on_size                    => %p %p", self, rect);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);

            UIVst3* const uivst3 = view->uivst3;
            DISTRHO_SAFE_ASSERT_RETURN(uivst3 != nullptr, V3_NOT_INITIALISED);

            return uivst3->onSize(rect);
        };

        view.on_focus = []V3_API(void* self, v3_bool state) -> v3_result
        {
            d_stdout("dpf_plugin_view::on_focus                   => %p %u", self, state);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);

            UIVst3* const uivst3 = view->uivst3;
            DISTRHO_SAFE_ASSERT_RETURN(uivst3 != nullptr, V3_NOT_INITIALISED);

            return uivst3->onFocus(state);
        };

        view.set_frame = []V3_API(void* self, v3_plugin_frame** frame) -> v3_result
        {
            d_stdout("dpf_plugin_view::set_frame                  => %p %p", self, frame);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);

            view->frame = frame;

            if (UIVst3* const uivst3 = view->uivst3)
                return uivst3->setFrame(frame);

            return V3_NOT_INITIALISED;
        };

        view.can_resize = []V3_API(void* self) -> v3_result
        {
            d_stdout("dpf_plugin_view::can_resize                 => %p", self);
// #if DISTRHO_UI_USER_RESIZABLE
//             return V3_OK;
// #else
            return V3_NOT_IMPLEMENTED;
// #endif
        };

        view.check_size_constraint = []V3_API(void* self, v3_view_rect* rect) -> v3_result
        {
            d_stdout("dpf_plugin_view::check_size_constraint      => %p %p", self, rect);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);

            UIVst3* const uivst3 = view->uivst3;
            DISTRHO_SAFE_ASSERT_RETURN(uivst3 != nullptr, V3_NOT_INITIALISED);

            return uivst3->checkSizeConstraint(rect);
        };
    }
};

// --------------------------------------------------------------------------------------------------------------------
// dpf_plugin_view_create (called from plugin side)

v3_funknown** dpf_plugin_view_create(v3_connection_point** connection, void* instancePointer, double sampleRate);

v3_funknown** dpf_plugin_view_create(v3_connection_point** const connection,
                                     void* const instancePointer, const double sampleRate)
{
    ScopedPointer<dpf_plugin_view>* const viewptr = new ScopedPointer<dpf_plugin_view>;
    *viewptr = new dpf_plugin_view(viewptr, connection, instancePointer, sampleRate);
    return (v3_funknown**)viewptr;
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
