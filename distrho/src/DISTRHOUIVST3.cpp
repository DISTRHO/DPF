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

#include "travesty/audio_processor.h"
#include "travesty/component.h"
#include "travesty/edit_controller.h"
#include "travesty/factory.h"
#include "travesty/view.h"

#include <atomic>

// TESTING awful idea dont reuse
#include "../extra/Thread.hpp"

// #if DISTRHO_PLUGIN_HAS_UI && ! DISTRHO_PLUGIN_HAS_EMBED_UI
// # undef DISTRHO_PLUGIN_HAS_UI
// # define DISTRHO_PLUGIN_HAS_UI 0
// #endif
//
// #if DISTRHO_PLUGIN_HAS_UI && ! defined(HAVE_DGL) && ! DISTRHO_PLUGIN_HAS_EXTERNAL_UI
// # undef DISTRHO_PLUGIN_HAS_UI
// # define DISTRHO_PLUGIN_HAS_UI 0
// #endif
//
// #undef DISTRHO_PLUGIN_HAS_UI
// #define DISTRHO_PLUGIN_HAS_UI 1

// #if DISTRHO_PLUGIN_HAS_UI
# include "DistrhoUIInternal.hpp"
// #endif

// --------------------------------------------------------------------------------------------------------------------

struct v3_component_handler_cpp : v3_funknown {
    v3_component_handler handler;
};

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

#if ! DISTRHO_PLUGIN_WANT_MIDI_INPUT
static const sendNoteFunc sendNoteCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_STATE
static const setStateFunc setStateCallback = nullptr;
#endif

// --------------------------------------------------------------------------------------------------------------------
// custom v3_tuid compatible type

typedef uint32_t dpf_tuid[4];
static_assert(sizeof(v3_tuid) == sizeof(dpf_tuid), "uid size mismatch");

// --------------------------------------------------------------------------------------------------------------------
// Utility functions

const char* tuid2str(const v3_tuid iid)
{
    if (v3_tuid_match(iid, v3_funknown_iid))
        return "{v3_funknown}";
    if (v3_tuid_match(iid, v3_plugin_base_iid))
        return "{v3_plugin_base}";
    if (v3_tuid_match(iid, v3_plugin_factory_iid))
        return "{v3_plugin_factory}";
    if (v3_tuid_match(iid, v3_plugin_factory_2_iid))
        return "{v3_plugin_factory_2}";
    if (v3_tuid_match(iid, v3_plugin_factory_3_iid))
        return "{v3_plugin_factory_3}";
    if (v3_tuid_match(iid, v3_component_iid))
        return "{v3_component}";
    if (v3_tuid_match(iid, v3_bstream_iid))
        return "{v3_bstream}";
    if (v3_tuid_match(iid, v3_event_list_iid))
        return "{v3_event_list}";
    if (v3_tuid_match(iid, v3_param_value_queue_iid))
        return "{v3_param_value_queue}";
    if (v3_tuid_match(iid, v3_param_changes_iid))
        return "{v3_param_changes}";
    if (v3_tuid_match(iid, v3_process_context_requirements_iid))
        return "{v3_process_context_requirements}";
    if (v3_tuid_match(iid, v3_audio_processor_iid))
        return "{v3_audio_processor}";
    if (v3_tuid_match(iid, v3_component_handler_iid))
        return "{v3_component_handler}";
    if (v3_tuid_match(iid, v3_edit_controller_iid))
        return "{v3_edit_controller}";
    if (v3_tuid_match(iid, v3_plugin_view_iid))
        return "{v3_plugin_view}";
    if (v3_tuid_match(iid, v3_plugin_frame_iid))
        return "{v3_plugin_frame}";
    if (v3_tuid_match(iid, v3_plugin_view_content_scale_steinberg_iid))
        return "{v3_plugin_view_content_scale_steinberg}";
    if (v3_tuid_match(iid, v3_plugin_view_parameter_finder_iid))
        return "{v3_plugin_view_parameter_finder}";
    /*
    if (std::memcmp(iid, dpf_tuid_class, sizeof(dpf_tuid)) == 0)
        return "{dpf_tuid_class}";
    if (std::memcmp(iid, dpf_tuid_component, sizeof(dpf_tuid)) == 0)
        return "{dpf_tuid_component}";
    if (std::memcmp(iid, dpf_tuid_controller, sizeof(dpf_tuid)) == 0)
        return "{dpf_tuid_controller}";
    if (std::memcmp(iid, dpf_tuid_processor, sizeof(dpf_tuid)) == 0)
        return "{dpf_tuid_processor}";
    if (std::memcmp(iid, dpf_tuid_view, sizeof(dpf_tuid)) == 0)
        return "{dpf_tuid_view}";
    */

    static char buf[46];
    std::snprintf(buf, sizeof(buf), "{0x%08X,0x%08X,0x%08X,0x%08X}",
                  (uint32_t)d_cconst(iid[ 0], iid[ 1], iid[ 2], iid[ 3]),
                  (uint32_t)d_cconst(iid[ 4], iid[ 5], iid[ 6], iid[ 7]),
                  (uint32_t)d_cconst(iid[ 8], iid[ 9], iid[10], iid[11]),
                  (uint32_t)d_cconst(iid[12], iid[13], iid[14], iid[15]));
    return buf;
}

// --------------------------------------------------------------------------------------------------------------------

class UIVst3 : public Thread
{
public:
    UIVst3(const intptr_t winId, const float scaleFactor, const double sampleRate, void* const instancePointer)
        : fUI(this, winId, sampleRate,
              editParameterCallback,
              setParameterCallback,
              setStateCallback,
              sendNoteCallback,
              setSizeCallback,
              nullptr, // TODO file request
              nullptr,
              instancePointer,
              scaleFactor)
    {
        // TESTING awful idea dont reuse
        startThread();
    }

    ~UIVst3() override
    {
        stopThread(5000);
    }

    // -------------------------------------------------------------------

    // TESTING awful idea dont reuse
    void run() override
    {
        while (! shouldThreadExit())
        {
            idle();
            d_msleep(50);
        }
    }

    void idle()
    {
        /*
        for (uint32_t i=0, count = fPlugin->getParameterCount(); i < count; ++i)
        {
            if (fUiHelper->parameterChecks[i])
            {
                fUiHelper->parameterChecks[i] = false;
                fUI.parameterChanged(i, fUiHelper->parameterValues[i]);
            }
        }
        */

        fUI.plugin_idle();
    }

    int16_t getWidth() const
    {
        return fUI.getWidth();
    }

    int16_t getHeight() const
    {
        return fUI.getHeight();
    }

    double getScaleFactor() const
    {
        return fUI.getScaleFactor();
    }

    void notifyScaleFactorChanged(const double scaleFactor)
    {
        fUI.notifyScaleFactorChanged(scaleFactor);
    }

    // TODO dont use this
    void setParameterValueFromDSP(const uint32_t index, const float value)
    {
        fUI.parameterChanged(index, value);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_plugin_view interface calls

    void setFrame(v3_plugin_frame* const f) noexcept
    {
        frame = f;
    }

    void setHandler(v3_component_handler_cpp** const h) noexcept
    {
        handler = h;
    }

    // ----------------------------------------------------------------------------------------------------------------

protected:
    void editParameter(const uint32_t index, const bool started) const
    {
        DISTRHO_SAFE_ASSERT_RETURN(handler != nullptr,);

        v3_component_handler_cpp* const chandler = *handler;
        DISTRHO_SAFE_ASSERT_RETURN(chandler != nullptr,);

        if (started)
            chandler->handler.begin_edit(handler, index);
        else
            chandler->handler.end_edit(handler, index);
    }

    void setParameterValue(const uint32_t index, const float realValue)
    {
        DISTRHO_SAFE_ASSERT_RETURN(handler != nullptr,);

        v3_component_handler_cpp* const chandler = *handler;
        DISTRHO_SAFE_ASSERT_RETURN(chandler != nullptr,);

        const double value = vst3->plainParameterToNormalised(index, realValue);
        chandler->handler.perform_edit(handler, index, value);

        // TODO send change to DSP side?
    }

    void setSize(uint width, uint height)
    {
# ifdef DISTRHO_OS_MAC
        const double scaleFactor = fUI.getScaleFactor();
        width /= scaleFactor;
        height /= scaleFactor;
# endif
        if (frame == nullptr)
            return;

        v3_view_rect rect = {};
        rect.right = width;
        rect.bottom = height;
        (void)rect;
        // frame->resize_view(nullptr, uivst3, &rect);
    }

# if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    void sendNote(const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        uint8_t midiData[3];
        midiData[0] = (velocity != 0 ? 0x90 : 0x80) | channel;
        midiData[1] = note;
        midiData[2] = velocity;
        fNotesRingBuffer.writeCustomData(midiData, 3);
        fNotesRingBuffer.commitWrite();
    }
# endif

# if DISTRHO_PLUGIN_WANT_STATE
    void setState(const char* const key, const char* const value)
    {
        // fUiHelper->setStateFromUI(key, value);
    }
# endif

private:
    // VST3 stuff
    v3_plugin_frame* frame;
    v3_component_handler_cpp** handler = nullptr;

    // Plugin UI
    UIExporter fUI;

    // -------------------------------------------------------------------
    // Callbacks

    #define handlePtr ((UIVst3*)ptr)

    static void editParameterCallback(void* ptr, uint32_t index, bool started)
    {
        handlePtr->editParameter(index, started);
    }

    static void setParameterCallback(void* ptr, uint32_t rindex, float value)
    {
        handlePtr->setParameterValue(rindex, value);
    }

    static void setSizeCallback(void* ptr, uint width, uint height)
    {
        handlePtr->setSize(width, height);
    }

# if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    static void sendNoteCallback(void* ptr, uint8_t channel, uint8_t note, uint8_t velocity)
    {
        handlePtr->sendNote(channel, note, velocity);
    }
# endif

# if DISTRHO_PLUGIN_WANT_STATE
    static void setStateCallback(void* ptr, const char* key, const char* value)
    {
        handlePtr->setState(key, value);
    }
# endif

    #undef handlePtr
};

// --------------------------------------------------------------------------------------------------------------------
// dpf_plugin_view_content_scale

struct v3_plugin_view_content_scale_steinberg_cpp : v3_funknown {
    v3_plugin_view_content_scale_steinberg scale;
};

struct dpf_plugin_view_scale : v3_plugin_view_content_scale_steinberg_cpp {
    std::atomic<int> refcounter;
    ScopedPointer<dpf_plugin_view_scale>* self;
    ScopedPointer<UIVst3>& uivst3;
    float lastScaleFactor = 0.0f;

    dpf_plugin_view_scale(ScopedPointer<dpf_plugin_view_scale>* s, ScopedPointer<UIVst3>& v)
        : refcounter(1),
          self(s),
          uivst3(v)
    {
        static const uint8_t* kSupportedInterfaces[] = {
            v3_funknown_iid,
            v3_plugin_view_content_scale_steinberg_iid
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_funknown

        query_interface = []V3_API(void* self, const v3_tuid iid, void** iface) -> v3_result
        {
            d_stdout("dpf_plugin_view_scale::query_interface    => %p %s %p", self, tuid2str(iid), iface);
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

        ref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_plugin_view_scale::ref                => %p", self);
            dpf_plugin_view_scale* const scale = *(dpf_plugin_view_scale**)self;
            DISTRHO_SAFE_ASSERT_RETURN(scale != nullptr, 0);

            return ++scale->refcounter;
        };

        unref = []V3_API(void* self) -> uint32_t
        {
            d_stdout("dpf_plugin_view_scale::unref              => %p", self);
            dpf_plugin_view_scale* const scale = *(dpf_plugin_view_scale**)self;
            DISTRHO_SAFE_ASSERT_RETURN(scale != nullptr, 0);

            if (const int refcounter = --scale->refcounter)
                return refcounter;

            *(dpf_plugin_view_scale**)self = nullptr;
            *scale->self = nullptr;
            return 0;
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_plugin_view_content_scale_steinberg

        scale.set_content_scale_factor = []V3_API(void* self, float factor) -> v3_result
        {
            d_stdout("dpf_plugin_view::set_content_scale_factor => %p %f", self, factor);
            dpf_plugin_view_scale* const scale = *(dpf_plugin_view_scale**)self;
            DISTRHO_SAFE_ASSERT_RETURN(scale != nullptr, V3_NOT_INITIALISED);

            if (UIVst3* const uivst3 = scale->uivst3)
                if (d_isNotZero(scale->lastScaleFactor) && d_isNotEqual(scale->lastScaleFactor, factor))
                    uivst3->notifyScaleFactorChanged(factor);

            scale->lastScaleFactor = factor;
            return V3_OK;
        };
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
    ScopedPointer<dpf_plugin_view_scale> scale;
    ScopedPointer<UIVst3> uivst3;
    // cached values
    v3_component_handler_cpp** handler = nullptr;
    v3_plugin_frame* hostframe = nullptr;

    dpf_plugin_view(ScopedPointer<dpf_plugin_view>* s)
        : refcounter(1),
          self(s)
    {
        static const uint8_t* kSupportedInterfacesBase[] = {
            v3_funknown_iid,
            v3_plugin_view_iid
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

            if (v3_tuid_match(v3_plugin_view_content_scale_steinberg_iid, iid))
            {
                if (view->scale == nullptr)
                    view->scale = new dpf_plugin_view_scale(&view->scale, view->uivst3);
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

            *(dpf_plugin_view**)self = nullptr;
            *view->self = nullptr;
            return 0;
        };

        // ------------------------------------------------------------------------------------------------------------
        // v3_plugin_view

        view.is_platform_type_supported = []V3_API(void* self, const char* platform_type) -> v3_result
        {
            d_stdout("dpf_plugin_view::is_platform_type_supported => %p %s", self, platform_type);
            const char* const supported[] = {
#ifdef _WIN32
                V3_VIEW_PLATFORM_TYPE_HWND,
#elif defined(__APPLE__)
                V3_VIEW_PLATFORM_TYPE_NSVIEW,
#else
                V3_VIEW_PLATFORM_TYPE_X11,
#endif
            };

            for (size_t i=0; i<sizeof(supported)/sizeof(supported[0]); ++i)
            {
                if (std::strcmp(supported[i], platform_type))
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

            const double scaleFactor = view->scale != nullptr ? view->scale->lastScaleFactor : 0.0;
            view->uivst3 = new UIVst3(view->vst3, view->hostframe, (uintptr_t)parent, scaleFactor);
            view->uivst3->setHandler(view->handler);
            return V3_OK;
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
            DISTRHO_SAFE_ASSERT_RETURN(view->uivst3 != nullptr, V3_NOT_INITIALISED);
            return V3_NOT_IMPLEMENTED;
        };

        view.on_key_down = []V3_API(void* self, int16_t key_char, int16_t key_code, int16_t modifiers) -> v3_result
        {
            d_stdout("dpf_plugin_view::on_key_down                => %p %i %i %i", self, key_char, key_code, modifiers);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);
            DISTRHO_SAFE_ASSERT_RETURN(view->uivst3 != nullptr, V3_NOT_INITIALISED);
            return V3_NOT_IMPLEMENTED;
        };

        view.on_key_up = []V3_API(void* self, int16_t key_char, int16_t key_code, int16_t modifiers) -> v3_result
        {
            d_stdout("dpf_plugin_view::on_key_up                  => %p %i %i %i", self, key_char, key_code, modifiers);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);
            DISTRHO_SAFE_ASSERT_RETURN(view->uivst3 != nullptr, V3_NOT_INITIALISED);
            return V3_NOT_IMPLEMENTED;
        };

        view.get_size = []V3_API(void* self, v3_view_rect* rect) -> v3_result
        {
            d_stdout("dpf_plugin_view::get_size                   => %p", self);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);

            std::memset(rect, 0, sizeof(v3_view_rect));

            if (view->uivst3 != nullptr)
            {
                rect->right  = view->uivst3->getWidth();
                rect->bottom = view->uivst3->getHeight();
# ifdef DISTRHO_OS_MAC
                const double scaleFactor = view->uivst3->getScaleFactor();
                rect->right /= scaleFactor;
                rect->bottom /= scaleFactor;
# endif
            }
            else
            {
                const double scaleFactor = view->scale != nullptr ? view->scale->lastScaleFactor : 0.0;
                UIExporter tmpUI(nullptr, 0, view->vst3->getSampleRate(),
                                 nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                 view->vst3->getInstancePointer(), scaleFactor);
                rect->right  = tmpUI.getWidth();
                rect->bottom = tmpUI.getHeight();
# ifdef DISTRHO_OS_MAC
                const double scaleFactor = tmpUI.getScaleFactor();
                rect->right /= scaleFactor;
                rect->bottom /= scaleFactor;
# endif
                tmpUI.quit();
            }

            return V3_NOT_IMPLEMENTED;
        };

        view.set_size = []V3_API(void* self, v3_view_rect*) -> v3_result
        {
            d_stdout("dpf_plugin_view::set_size                   => %p", self);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);
            DISTRHO_SAFE_ASSERT_RETURN(view->uivst3 != nullptr, V3_NOT_INITIALISED);
            return V3_NOT_IMPLEMENTED;
        };

        view.on_focus = []V3_API(void* self, v3_bool state) -> v3_result
        {
            d_stdout("dpf_plugin_view::on_focus                   => %p %u", self, state);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);
            DISTRHO_SAFE_ASSERT_RETURN(view->uivst3 != nullptr, V3_NOT_INITIALISED);
            return V3_NOT_IMPLEMENTED;
        };

        view.set_frame = []V3_API(void* self, v3_plugin_frame* frame) -> v3_result
        {
            d_stdout("dpf_plugin_view::set_frame                  => %p", self);
            dpf_plugin_view* const view = *(dpf_plugin_view**)self;
            DISTRHO_SAFE_ASSERT_RETURN(view != nullptr, V3_NOT_INITIALISED);

            view->hostframe = frame;

            if (view->uivst3 != nullptr)
                view->uivst3->setFrame(frame);

            return V3_OK;
        };

        view.can_resize = []V3_API(void* self) -> v3_result
        {
            d_stdout("dpf_plugin_view::can_resize                 => %p", self);
#if DISTRHO_UI_USER_RESIZABLE
            return V3_OK;
#else
            return V3_NOT_IMPLEMENTED;
#endif
        };

        view.check_size_constraint = []V3_API(void* self, v3_view_rect*) -> v3_result
        {
            d_stdout("dpf_plugin_view::check_size_constraint      => %p", self);
            return V3_NOT_IMPLEMENTED;
        };
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
