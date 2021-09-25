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

#include "travesty/audio_processor.h"
#include "travesty/component.h"
#include "travesty/edit_controller.h"
#include "travesty/factory.h"
#include "travesty/view.h"

// --------------------------------------------------------------------------------------------------------------------

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
    UIVst3(const intptr_t winId, const float scaleFactor, const double sampleRate,
           void* const instancePointer, v3_plugin_view** const view)
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
          fFrame(nullptr),
          fFrameArg(nullptr),
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

    v3_result setFrame(v3_plugin_frame* const frame, void* const arg) noexcept
    {
        d_stdout("setFrame %p %p", frame, arg);
        fFrame = frame;
        fFrameArg = arg;

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
    v3_plugin_frame* fFrame;
    void* fFrameArg;
    // v3_component_handler_cpp** handler = nullptr;

    // Temporary data
    float fScaleFactor;

    void editParameter(const uint32_t /*index*/, const bool /*started*/) const
    {
//         DISTRHO_SAFE_ASSERT_RETURN(handler != nullptr,);
//
//         v3_component_handler_cpp* const chandler = *handler;
//         DISTRHO_SAFE_ASSERT_RETURN(chandler != nullptr,);
//
//         if (started)
//             chandler->handler.begin_edit(handler, index);
//         else
//             chandler->handler.end_edit(handler, index);
    }

    static void editParameterCallback(void* ptr, uint32_t index, bool started)
    {
        ((UIVst3*)ptr)->editParameter(index, started);
    }

    void setParameterValue(const uint32_t /*index*/, const float /*realValue*/)
    {
//         DISTRHO_SAFE_ASSERT_RETURN(handler != nullptr,);
//
//         v3_component_handler_cpp* const chandler = *handler;
//         DISTRHO_SAFE_ASSERT_RETURN(chandler != nullptr,);

//         const double value = vst3->plainParameterToNormalised(index, realValue);
//         chandler->handler.perform_edit(handler, index, value);

        // TODO send change to DSP side?
    }

    static void setParameterCallback(void* ptr, uint32_t rindex, float value)
    {
        ((UIVst3*)ptr)->setParameterValue(rindex, value);
    }

    void setSize(uint width, uint height)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fView != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fFrame != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fFrameArg != nullptr,);
        d_stdout("from UI setSize %u %u | %p %p %p", width, height, fView, fFrame, fFrameArg);

#ifdef DISTRHO_OS_MAC
        const double scaleFactor = fUI.getScaleFactor();
        width /= scaleFactor;
        height /= scaleFactor;
#endif

        v3_view_rect rect;
        std::memset(&rect, 0, sizeof(rect));
        rect.right = width;
        rect.bottom = height;
        fFrame->resize_view(fFrameArg, fView, &rect);
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

#if DISTRHO_PLUGIN_WANT_STATE
    void setState(const char* const /*key*/, const char* const /*value*/)
    {
        // fUiHelper->setStateFromUI(key, value);
    }

    static void setStateCallback(void* ptr, const char* key, const char* value)
    {
        ((UIVst3*)ptr)->setState(key, value);
    }
#endif
};

// --------------------------------------------------------------------------------------------------------------------
// dpf_plugin_view_content_scale

struct v3_plugin_view_content_scale_steinberg_cpp : v3_funknown {
    v3_plugin_view_content_scale_steinberg scale;
};

struct dpf_plugin_view_scale : v3_plugin_view_content_scale_steinberg_cpp {
    ScopedPointer<UIVst3>& uivst3;
    // cached values
    float scaleFactor;

    dpf_plugin_view_scale(ScopedPointer<UIVst3>& v)
        : uivst3(v),
          scaleFactor(0.0f)
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

        // there is only a single instance of this, so we don't have to care here
        ref = []V3_API(void*) -> uint32_t { return 1; };
        unref = []V3_API(void*) -> uint32_t { return 0; };

        // ------------------------------------------------------------------------------------------------------------
        // v3_plugin_view_content_scale_steinberg

        scale.set_content_scale_factor = []V3_API(void* self, float factor) -> v3_result
        {
            d_stdout("dpf_plugin_view::set_content_scale_factor => %p %f", self, factor);
            dpf_plugin_view_scale* const scale = *(dpf_plugin_view_scale**)self;
            DISTRHO_SAFE_ASSERT_RETURN(scale != nullptr, V3_NOT_INITIALISED);

            scale->scaleFactor = factor;

            if (UIVst3* const uivst3 = scale->uivst3)
                return uivst3->setContentScaleFactor(factor);

            return V3_NOT_INITIALISED;
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
    void* const instancePointer;
    double sampleRate;
//     v3_component_handler_cpp** handler = nullptr;
    v3_plugin_frame** frame = nullptr;

    dpf_plugin_view(ScopedPointer<dpf_plugin_view>* s, void* const instance, const double sr)
        : refcounter(1),
          self(s),
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

            if (v3_tuid_match(v3_plugin_view_content_scale_steinberg_iid, iid))
            {
                if (view->scale == nullptr)
                    view->scale = new dpf_plugin_view_scale(view->uivst3);
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
                    view->uivst3 = new UIVst3((uintptr_t)parent, scaleFactor, view->sampleRate,
                                              view->instancePointer, (v3_plugin_view**)self);

                    // offset struct by sizeof(v3_funknown), because of differences between C and C++
                    v3_plugin_frame* const frameptr
                        = view->frame != nullptr ? (v3_plugin_frame*)((uint8_t*)*(view->frame)+sizeof(v3_funknown))
                                                 : nullptr;

                    view->uivst3->setFrame(frameptr, view->frame);
                    // view->uivst3->setHandler(view->handler);
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

            // special case: allow UI to not be attached yet, as a way to get size before window creation

            if (UIVst3* const uivst3 = view->uivst3)
                return uivst3->getSize(rect);

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
            {
                // offset struct by sizeof(v3_funknown), because of differences between C and C++
                v3_plugin_frame* const frameptr
                    = frame != nullptr ? (v3_plugin_frame*)((uint8_t*)*(frame)+sizeof(v3_funknown))
                                       : nullptr;

                return uivst3->setFrame(frameptr, frame);
            }

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
// dpf_plugin_view_create (called from DSP side)

v3_funknown** dpf_plugin_view_create(void* instancePointer, double sampleRate);

v3_funknown** dpf_plugin_view_create(void* const instancePointer, const double sampleRate)
{
    ScopedPointer<dpf_plugin_view>* const viewptr = new ScopedPointer<dpf_plugin_view>;
    *viewptr = new dpf_plugin_view(viewptr, instancePointer, sampleRate);
    return (v3_funknown**)viewptr;
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
