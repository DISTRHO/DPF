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

#include "DistrhoUIInternal.hpp"

#include "travesty/base.h"
#include "travesty/edit_controller.h"
#include "travesty/host.h"
#include "travesty/view.h"

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
# if defined(DISTRHO_OS_MAC)
#  include <CoreFoundation/CoreFoundation.h>
# elif defined(DISTRHO_OS_WINDOWS)
#  include <winuser.h>
#  define DPF_VST3_WIN32_TIMER_ID 1
# endif
#endif

/* TODO items:
 * - mousewheel event
 * - file request?
 */

#if !(defined(DISTRHO_OS_MAC) || defined(DISTRHO_OS_WINDOWS))
# define DPF_VST3_USING_HOST_RUN_LOOP 1
#else
# define DPF_VST3_USING_HOST_RUN_LOOP 0
#endif

#ifndef DPF_VST3_TIMER_INTERVAL
# define DPF_VST3_TIMER_INTERVAL 16 /* ~60 fps */
#endif

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

#if ! DISTRHO_PLUGIN_WANT_STATE
static constexpr const setStateFunc setStateCallback = nullptr;
#endif
#if ! DISTRHO_PLUGIN_WANT_MIDI_INPUT
static constexpr const sendNoteFunc sendNoteCallback = nullptr;
#endif

// --------------------------------------------------------------------------------------------------------------------
// Static data, see DistrhoPlugin.cpp

extern const char* d_nextBundlePath;

// --------------------------------------------------------------------------------------------------------------------
// Utility functions (defined on plugin side)

const char* tuid2str(const v3_tuid iid);

// --------------------------------------------------------------------------------------------------------------------

static void applyGeometryConstraints(const uint minimumWidth,
                                     const uint minimumHeight,
                                     const bool keepAspectRatio,
                                     v3_view_rect* const rect)
{
    d_debug("applyGeometryConstraints %u %u %d {%d,%d,%d,%d} | BEFORE",
            minimumWidth, minimumHeight, keepAspectRatio, rect->top, rect->left, rect->right, rect->bottom);
    const int32_t minWidth = static_cast<int32_t>(minimumWidth);
    const int32_t minHeight = static_cast<int32_t>(minimumHeight);

    if (keepAspectRatio)
    {
        if (rect->right < 1)
            rect->right = 1;
        if (rect->bottom < 1)
            rect->bottom = 1;

        const double ratio = static_cast<double>(minWidth) / static_cast<double>(minHeight);
        const double reqRatio = static_cast<double>(rect->right) / static_cast<double>(rect->bottom);

        if (d_isNotEqual(ratio, reqRatio))
        {
            // fix width
            if (reqRatio > ratio)
                rect->right = static_cast<int32_t>(rect->bottom * ratio + 0.5);
            // fix height
            else
                rect->bottom = static_cast<int32_t>(static_cast<double>(rect->right) / ratio + 0.5);
        }
    }

    if (minWidth > rect->right)
        rect->right = minWidth;
    if (minHeight > rect->bottom)
        rect->bottom = minHeight;

    d_debug("applyGeometryConstraints %u %u %d {%d,%d,%d,%d} | AFTER",
            minimumWidth, minimumHeight, keepAspectRatio, rect->top, rect->left, rect->right, rect->bottom);
}

// --------------------------------------------------------------------------------------------------------------------

#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
static uint translateVST3Modifiers(const int64_t modifiers) noexcept
{
    using namespace DGL_NAMESPACE;

    uint dglmods = 0;
    if (modifiers & (1 << 0))
        dglmods |= kModifierShift;
    if (modifiers & (1 << 1))
        dglmods |= kModifierAlt;
   #ifdef DISTRHO_OS_MAC
    if (modifiers & (1 << 2))
        dglmods |= kModifierSuper;
    if (modifiers & (1 << 3))
        dglmods |= kModifierControl;
   #else
    if (modifiers & (1 << 2))
        dglmods |= kModifierControl;
    if (modifiers & (1 << 3))
        dglmods |= kModifierSuper;
   #endif

    return dglmods;
}
#endif

// --------------------------------------------------------------------------------------------------------------------

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI && !DPF_VST3_USING_HOST_RUN_LOOP
/**
 * Helper class for getting a native idle timer via native APIs.
 */
class NativeIdleHelper
{
public:
    NativeIdleHelper(IdleCallback* const callback)
        : fCallback(callback),
       #ifdef DISTRHO_OS_MAC
          fTimerRef(nullptr)
       #else
          fTimerWindow(nullptr),
          fTimerWindowClassName()
       #endif
    {
    }

    void registerNativeIdleCallback()
    {
       #ifdef DISTRHO_OS_MAC
        constexpr const CFTimeInterval interval = DPF_VST3_TIMER_INTERVAL * 0.0001;

        CFRunLoopTimerContext context = {};
        context.info = this;
        fTimerRef = CFRunLoopTimerCreate(kCFAllocatorDefault, CFAbsoluteTimeGetCurrent() + interval, interval, 0, 0,
                                         platformIdleTimerCallback, &context);
        DISTRHO_SAFE_ASSERT_RETURN(fTimerRef != nullptr,);

        CFRunLoopAddTimer(CFRunLoopGetCurrent(), fTimerRef, kCFRunLoopCommonModes);
       #else
        /* 
         * Create an invisible window to handle a timer.
         * There is no need for implementing a window proc because DefWindowProc already calls the
         * callback function when processing WM_TIMER messages.
         */
        fTimerWindowClassName = (
           #ifdef DISTRHO_PLUGIN_BRAND
            DISTRHO_PLUGIN_BRAND
           #else
            DISTRHO_MACRO_AS_STRING(DISTRHO_NAMESPACE)
           #endif
            "-" DISTRHO_PLUGIN_NAME "-"
        );

        char suffix[9];
        std::snprintf(suffix, sizeof(suffix), "%08x", std::rand());
        suffix[sizeof(suffix)-1] = '\0';
        fTimerWindowClassName += suffix;

        WNDCLASSEX cls;
        ZeroMemory(&cls, sizeof(cls));
        cls.cbSize = sizeof(WNDCLASSEX);
        cls.cbWndExtra = sizeof(LONG_PTR);
        cls.lpszClassName = fTimerWindowClassName.buffer();
        cls.lpfnWndProc = DefWindowProc;
        RegisterClassEx(&cls);

        fTimerWindow = CreateWindowEx(0, cls.lpszClassName, "DPF Timer Helper",
                                      0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, nullptr, nullptr);
        DISTRHO_SAFE_ASSERT_RETURN(fTimerWindow != nullptr,);

        SetWindowLongPtr(fTimerWindow, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(static_cast<void*>(this)));
        SetTimer(fTimerWindow, DPF_VST3_WIN32_TIMER_ID, DPF_VST3_TIMER_INTERVAL,
                 static_cast<TIMERPROC>(platformIdleTimerCallback));
       #endif
    }

    void unregisterNativeIdleCallback()
    {
       #ifdef DISTRHO_OS_MAC
        CFRunLoopRemoveTimer(CFRunLoopGetCurrent(), fTimerRef, kCFRunLoopCommonModes);
        CFRelease(fTimerRef);
       #else
        DISTRHO_SAFE_ASSERT_RETURN(fTimerWindow != nullptr,);
        KillTimer(fTimerWindow, DPF_VST3_WIN32_TIMER_ID);
        DestroyWindow(fTimerWindow);
        UnregisterClass(fTimerWindowClassName, nullptr);
       #endif
    }

private:
    IdleCallback* const fCallback;

   #ifdef DISTRHO_OS_MAC
    CFRunLoopTimerRef fTimerRef;

    static void platformIdleTimerCallback(CFRunLoopTimerRef, void* const info)
    {
        static_cast<NativeIdleHelper*>(info)->fCallback->idleCallback();
    }
   #else
    HWND fTimerWindow;
    String fTimerWindowClassName;

    static void WINAPI platformIdleTimerCallback(const HWND hwnd, UINT, UINT_PTR, DWORD)
    {
        reinterpret_cast<NativeIdleHelper*>(GetWindowLongPtr(hwnd, GWLP_USERDATA))->fCallback->idleCallback();
    }
   #endif
};
#endif

/**
 * Helper class for getting a native idle timer, either through pugl or via native APIs.
 */
#if !DPF_VST3_USING_HOST_RUN_LOOP
class NativeIdleCallback : public IdleCallback
{
public:
    NativeIdleCallback(UIExporter& ui)
        : fCallbackRegistered(false),
         #if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
          fIdleHelper(this)
         #else
          fUI(ui)
         #endif
    {
       #if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
        // unused
        (void)ui;
       #endif
    }

    void registerNativeIdleCallback()
    {
        DISTRHO_SAFE_ASSERT_RETURN(!fCallbackRegistered,);
        fCallbackRegistered = true;

       #if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
        fIdleHelper.registerNativeIdleCallback();
       #else
        fUI.addIdleCallbackForNativeIdle(this, DPF_VST3_TIMER_INTERVAL);
       #endif
    }

    void unregisterNativeIdleCallback()
    {
        DISTRHO_SAFE_ASSERT_RETURN(fCallbackRegistered,);
        fCallbackRegistered = false;

       #if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
        fIdleHelper.unregisterNativeIdleCallback();
       #else
        fUI.removeIdleCallbackForNativeIdle(this);
       #endif
    }

private:
    bool fCallbackRegistered;
   #if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
    NativeIdleHelper fIdleHelper;
   #else
    UIExporter& fUI;
   #endif
};
#endif

// --------------------------------------------------------------------------------------------------------------------

/**
 * VST3 UI class.
 *
 * All the dynamic things from VST3 get implemented here, free of complex low-level VST3 pointer things.
 * The UI is created during the "attach" view event, and destroyed during "removed".
 *
 * The low-level VST3 stuff comes after.
 */
class UIVst3
#if !DPF_VST3_USING_HOST_RUN_LOOP
  : public NativeIdleCallback
#endif
{
public:
    UIVst3(v3_plugin_view** const view,
           v3_host_application** const host,
           v3_connection_point** const connection,
           v3_plugin_frame** const frame,
           const intptr_t winId,
           const float scaleFactor,
           const double sampleRate,
           void* const instancePointer,
           const bool willResizeFromHost,
           const bool needsResizeFromPlugin)
        :
#if !DPF_VST3_USING_HOST_RUN_LOOP
          NativeIdleCallback(fUI),
#endif
          fView(view),
          fHostApplication(host),
          fConnection(connection),
          fFrame(frame),
          fScaleFactor(scaleFactor),
          fReadyForPluginData(false),
          fIsResizingFromPlugin(false),
          fIsResizingFromHost(willResizeFromHost),
          fNeedsResizeFromPlugin(needsResizeFromPlugin),
          fNextPluginRect(),
          fUI(this, winId, sampleRate,
              editParameterCallback,
              setParameterCallback,
              setStateCallback,
              sendNoteCallback,
              setSizeCallback,
              nullptr, // TODO file request
              d_nextBundlePath,
              instancePointer,
              scaleFactor)
    {
    }

    ~UIVst3()
    {
       #if !DPF_VST3_USING_HOST_RUN_LOOP
        unregisterNativeIdleCallback();
       #endif

        if (fConnection != nullptr)
            disconnect();
    }

    void postInit(uint32_t nextWidth, uint32_t nextHeight)
    {
        if (fIsResizingFromHost && nextWidth > 0 && nextHeight > 0)
        {
           #ifdef DISTRHO_OS_MAC
            const double scaleFactor = fUI.getScaleFactor();
            nextWidth *= scaleFactor;
            nextHeight *= scaleFactor;
           #endif

            if (fUI.getWidth() != nextWidth || fUI.getHeight() != nextHeight)
            {
                d_debug("postInit sets new size as %u %u", nextWidth, nextHeight);
                fUI.setWindowSizeFromHost(nextWidth, nextHeight);
            }
        }
        else if (fNeedsResizeFromPlugin)
        {
            d_debug("postInit forcely sets size from plugin as %u %u", fUI.getWidth(), fUI.getHeight());
            setSize(fUI.getWidth(), fUI.getHeight());
        }

        if (fConnection != nullptr)
            connect(fConnection);

       #if !DPF_VST3_USING_HOST_RUN_LOOP
        registerNativeIdleCallback();
       #endif
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_plugin_view interface calls

#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
    v3_result onWheel(float /*distance*/)
    {
        // TODO
        return V3_NOT_IMPLEMENTED;
    }

    v3_result onKeyDown(const int16_t keychar, const int16_t keycode, const int16_t modifiers)
    {
        DISTRHO_SAFE_ASSERT_INT_RETURN(keychar >= 0 && keychar < 0x7f, keychar, V3_FALSE);

        bool special;
        const uint key = translateVstKeyCode(special, keychar, keycode);
        d_debug("onKeyDown %d %d %x -> %d %d", keychar, keycode, modifiers, special, key);

        return fUI.handlePluginKeyboardVST(true, special, key,
                                           keycode >= 0 ? static_cast<uint>(keycode) : 0,
                                           translateVST3Modifiers(modifiers)) ? V3_TRUE : V3_FALSE;
    }

    v3_result onKeyUp(const int16_t keychar, const int16_t keycode, const int16_t modifiers)
    {
        DISTRHO_SAFE_ASSERT_INT_RETURN(keychar >= 0 && keychar < 0x7f, keychar, V3_FALSE);

        bool special;
        const uint key = translateVstKeyCode(special, keychar, keycode);
        d_debug("onKeyUp %d %d %x -> %d %d", keychar, keycode, modifiers, special, key);

        return fUI.handlePluginKeyboardVST(false, special, key,
                                           keycode >= 0 ? static_cast<uint>(keycode) : 0,
                                           translateVST3Modifiers(modifiers)) ? V3_TRUE : V3_FALSE;
    }

    v3_result onFocus(const bool state)
    {
        if (state)
            fUI.focus();
        fUI.notifyFocusChanged(state);
        return V3_OK;
    }
#endif

    v3_result getSize(v3_view_rect* const rect) const noexcept
    {
        if (fIsResizingFromPlugin)
        {
            *rect = fNextPluginRect;
        }
        else
        {
            rect->left = rect->top = 0;
            rect->right = fUI.getWidth();
            rect->bottom = fUI.getHeight();
           #ifdef DISTRHO_OS_MAC
            const double scaleFactor = fUI.getScaleFactor();
            rect->right /= scaleFactor;
            rect->bottom /= scaleFactor;
           #endif
        }

        d_debug("getSize request returning %i %i", rect->right, rect->bottom);
        return V3_OK;
    }

    v3_result onSize(v3_view_rect* const orect)
    {
        v3_view_rect rect = *orect;

       #ifdef DISTRHO_OS_MAC
        const double scaleFactor = fUI.getScaleFactor();
        rect.top *= scaleFactor;
        rect.left *= scaleFactor;
        rect.right *= scaleFactor;
        rect.bottom *= scaleFactor;
       #endif

        if (fIsResizingFromPlugin)
        {
            d_debug("host->plugin onSize request %i %i (plugin resize was active, unsetting now)",
                    rect.right - rect.left, rect.bottom - rect.top);
            fIsResizingFromPlugin = false;
        }
        else
        {
            d_debug("host->plugin onSize request %i %i (OK)", rect.right - rect.left, rect.bottom - rect.top);
        }

        fIsResizingFromHost = true;
        fUI.setWindowSizeFromHost(rect.right - rect.left, rect.bottom - rect.top);
        return V3_OK;
    }

    v3_result setFrame(v3_plugin_frame** const frame) noexcept
    {
        fFrame = frame;
        return V3_OK;
    }

    v3_result canResize() noexcept
    {
        return fUI.isResizable() ? V3_TRUE : V3_FALSE;
    }

    v3_result checkSizeConstraint(v3_view_rect* const rect)
    {
        uint minimumWidth, minimumHeight;
        bool keepAspectRatio;
        fUI.getGeometryConstraints(minimumWidth, minimumHeight, keepAspectRatio);

       #ifdef DISTRHO_OS_MAC
        const double scaleFactor = fUI.getScaleFactor();
        minimumWidth /= scaleFactor;
        minimumHeight /= scaleFactor;
       #endif

        applyGeometryConstraints(minimumWidth, minimumHeight, keepAspectRatio, rect);
        return V3_TRUE;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_connection_point interface calls

    void connect(v3_connection_point** const point) noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(point != nullptr,);

        fConnection = point;

        d_debug("requesting current plugin state");

        v3_message** const message = createMessage("init");
        DISTRHO_SAFE_ASSERT_RETURN(message != nullptr,);

        v3_attribute_list** const attrlist = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrlist != nullptr,);

        v3_cpp_obj(attrlist)->set_int(attrlist, "__dpf_msg_target__", 1);
        v3_cpp_obj(fConnection)->notify(fConnection, message);

        v3_cpp_obj_unref(message);
    }

    void disconnect() noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fConnection != nullptr,);

        d_debug("reporting UI closed");
        fReadyForPluginData = false;

        v3_message** const message = createMessage("close");
        DISTRHO_SAFE_ASSERT_RETURN(message != nullptr,);

        v3_attribute_list** const attrlist = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrlist != nullptr,);

        v3_cpp_obj(attrlist)->set_int(attrlist, "__dpf_msg_target__", 1);
        v3_cpp_obj(fConnection)->notify(fConnection, message);

        v3_cpp_obj_unref(message);

        fConnection = nullptr;
    }

    v3_result notify(v3_message** const message)
    {
        const char* const msgid = v3_cpp_obj(message)->get_message_id(message);
        DISTRHO_SAFE_ASSERT_RETURN(msgid != nullptr, V3_INVALID_ARG);

        v3_attribute_list** const attrs = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrs != nullptr, V3_INVALID_ARG);

        if (std::strcmp(msgid, "ready") == 0)
        {
            DISTRHO_SAFE_ASSERT_RETURN(! fReadyForPluginData, V3_INTERNAL_ERR);
            fReadyForPluginData = true;
            return V3_OK;
        }

        if (std::strcmp(msgid, "parameter-set") == 0)
        {
            int64_t rindex;
            double value;
            v3_result res;

            res = v3_cpp_obj(attrs)->get_int(attrs, "rindex", &rindex);
            DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);

            res = v3_cpp_obj(attrs)->get_float(attrs, "value", &value);
            DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);

            if (rindex < kVst3InternalParameterBaseCount)
            {
                switch (rindex)
                {
               #if DPF_VST3_USES_SEPARATE_CONTROLLER
                case kVst3InternalParameterSampleRate:
                    DISTRHO_SAFE_ASSERT_RETURN(value >= 0.0, V3_INVALID_ARG);
                    fUI.setSampleRate(value, true);
                    break;
               #endif
               #if DISTRHO_PLUGIN_WANT_PROGRAMS
                case kVst3InternalParameterProgram:
                    DISTRHO_SAFE_ASSERT_RETURN(value >= 0.0, V3_INVALID_ARG);
                    fUI.programLoaded(static_cast<uint32_t>(value + 0.5));
                    break;
               #endif
                }

                // others like latency and buffer-size do not matter on UI side
                return V3_OK;
            }

            DISTRHO_SAFE_ASSERT_UINT2_RETURN(rindex >= kVst3InternalParameterCount, rindex, kVst3InternalParameterCount, V3_INVALID_ARG);
            const uint32_t index = static_cast<uint32_t>(rindex - kVst3InternalParameterCount);

            fUI.parameterChanged(index, value);
            return V3_OK;
        }

       #if DISTRHO_PLUGIN_WANT_STATE
        if (std::strcmp(msgid, "state-set") == 0)
        {
            int64_t keyLength = -1;
            int64_t valueLength = -1;
            v3_result res;

            res = v3_cpp_obj(attrs)->get_int(attrs, "key:length", &keyLength);
            DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);
            DISTRHO_SAFE_ASSERT_INT_RETURN(keyLength >= 0, keyLength, V3_INTERNAL_ERR);

            res = v3_cpp_obj(attrs)->get_int(attrs, "value:length", &valueLength);
            DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_OK, res, res);
            DISTRHO_SAFE_ASSERT_INT_RETURN(valueLength >= 0, valueLength, V3_INTERNAL_ERR);

            int16_t* const key16 = (int16_t*)std::malloc(sizeof(int16_t)*(keyLength + 1));
            DISTRHO_SAFE_ASSERT_RETURN(key16 != nullptr, V3_NOMEM);

            int16_t* const value16 = (int16_t*)std::malloc(sizeof(int16_t)*(valueLength + 1));
            DISTRHO_SAFE_ASSERT_RETURN(value16 != nullptr, V3_NOMEM);

            res = v3_cpp_obj(attrs)->get_string(attrs, "key", key16, sizeof(int16_t)*(keyLength+1));
            DISTRHO_SAFE_ASSERT_INT2_RETURN(res == V3_OK, res, keyLength, res);

            if (valueLength != 0)
            {
                res = v3_cpp_obj(attrs)->get_string(attrs, "value", value16, sizeof(int16_t)*(valueLength+1));
                DISTRHO_SAFE_ASSERT_INT2_RETURN(res == V3_OK, res, valueLength, res);
            }

            // do cheap inline conversion
            char* const key = (char*)key16;
            char* const value = (char*)value16;

            for (int64_t i=0; i<keyLength; ++i)
                key[i] = key16[i];
            for (int64_t i=0; i<valueLength; ++i)
                value[i] = value16[i];

            key[keyLength] = '\0';
            value[valueLength] = '\0';

            fUI.stateChanged(key, value);

            std::free(key16);
            std::free(value16);
            return V3_OK;
        }
       #endif

        d_stderr("UIVst3 received unknown msg '%s'", msgid);

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

   #if DPF_VST3_USING_HOST_RUN_LOOP
    // ----------------------------------------------------------------------------------------------------------------
    // v3_timer_handler interface calls

    void onTimer()
    {
        fUI.plugin_idle();
        doIdleStuff();
    }
   #else
    // ----------------------------------------------------------------------------------------------------------------
    // special idle callback without v3_timer_handler

    void idleCallback() override
    {
        fUI.idleFromNativeIdle();
        doIdleStuff();
    }
   #endif

    void doIdleStuff()
    {
        if (fReadyForPluginData)
        {
            fReadyForPluginData = false;
            requestMorePluginData();
        }

        if (fNeedsResizeFromPlugin)
        {
            fNeedsResizeFromPlugin = false;
            d_debug("first resize forced behaviour is now stopped");
        }

        if (fIsResizingFromHost)
        {
            fIsResizingFromHost = false;
            d_debug("was resizing from host, now stopped");
        }

        if (fIsResizingFromPlugin)
        {
            fIsResizingFromPlugin = false;
            d_debug("was resizing from plugin, now stopped");
        }
    }

    // ----------------------------------------------------------------------------------------------------------------

private:
    // VST3 stuff
    v3_plugin_view** const fView;
    v3_host_application** const fHostApplication;
    v3_connection_point** fConnection;
    v3_plugin_frame** fFrame;

    // Temporary data
    float fScaleFactor;
    bool fReadyForPluginData;
    bool fIsResizingFromPlugin;
    bool fIsResizingFromHost;
    bool fNeedsResizeFromPlugin;
    v3_view_rect fNextPluginRect; // for when plugin requests a new size

    // Plugin UI (after VST3 stuff so the UI can call into us during its constructor)
    UIExporter fUI;

    // ----------------------------------------------------------------------------------------------------------------
    // helper functions called during message passing

    v3_message** createMessage(const char* const id) const
    {
        DISTRHO_SAFE_ASSERT_RETURN(fHostApplication != nullptr, nullptr);

        v3_tuid iid;
        std::memcpy(iid, v3_message_iid, sizeof(v3_tuid));
        v3_message** msg = nullptr;
        const v3_result res = v3_cpp_obj(fHostApplication)->create_instance(fHostApplication, iid, iid, (void**)&msg);
        DISTRHO_SAFE_ASSERT_INT_RETURN(res == V3_TRUE, res, nullptr);
        DISTRHO_SAFE_ASSERT_RETURN(msg != nullptr, nullptr);

        v3_cpp_obj(msg)->set_message_id(msg, id);
        return msg;
    }

    void requestMorePluginData() const
    {
        DISTRHO_SAFE_ASSERT_RETURN(fConnection != nullptr,);

        v3_message** const message = createMessage("idle");
        DISTRHO_SAFE_ASSERT_RETURN(message != nullptr,);

        v3_attribute_list** const attrlist = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrlist != nullptr,);

        v3_cpp_obj(attrlist)->set_int(attrlist, "__dpf_msg_target__", 1);
        v3_cpp_obj(fConnection)->notify(fConnection, message);

        v3_cpp_obj_unref(message);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // DPF callbacks

    void editParameter(const uint32_t rindex, const bool started) const
    {
        DISTRHO_SAFE_ASSERT_RETURN(fConnection != nullptr,);

        v3_message** const message = createMessage("parameter-edit");
        DISTRHO_SAFE_ASSERT_RETURN(message != nullptr,);

        v3_attribute_list** const attrlist = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrlist != nullptr,);

        v3_cpp_obj(attrlist)->set_int(attrlist, "__dpf_msg_target__", 1);
        v3_cpp_obj(attrlist)->set_int(attrlist, "rindex", rindex);
        v3_cpp_obj(attrlist)->set_int(attrlist, "started", started ? 1 : 0);
        v3_cpp_obj(fConnection)->notify(fConnection, message);

        v3_cpp_obj_unref(message);
    }

    static void editParameterCallback(void* const ptr, const uint32_t rindex, const bool started)
    {
        static_cast<UIVst3*>(ptr)->editParameter(rindex, started);
    }

    void setParameterValue(const uint32_t rindex, const float realValue)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fConnection != nullptr,);

        v3_message** const message = createMessage("parameter-set");
        DISTRHO_SAFE_ASSERT_RETURN(message != nullptr,);

        v3_attribute_list** const attrlist = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrlist != nullptr,);

        v3_cpp_obj(attrlist)->set_int(attrlist, "__dpf_msg_target__", 1);
        v3_cpp_obj(attrlist)->set_int(attrlist, "rindex", rindex);
        v3_cpp_obj(attrlist)->set_float(attrlist, "value", realValue);
        v3_cpp_obj(fConnection)->notify(fConnection, message);

        v3_cpp_obj_unref(message);
    }

    static void setParameterCallback(void* const ptr, const uint32_t rindex, const float value)
    {
        static_cast<UIVst3*>(ptr)->setParameterValue(rindex, value);
    }

   #if DISTRHO_PLUGIN_WANT_STATE
    void setState(const char* const key, const char* const value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fConnection != nullptr,);

        v3_message** const message = createMessage("state-set");
        DISTRHO_SAFE_ASSERT_RETURN(message != nullptr,);

        v3_attribute_list** const attrlist = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrlist != nullptr,);

        v3_cpp_obj(attrlist)->set_int(attrlist, "__dpf_msg_target__", 1);
        v3_cpp_obj(attrlist)->set_int(attrlist, "key:length", std::strlen(key));
        v3_cpp_obj(attrlist)->set_int(attrlist, "value:length", std::strlen(value));
        v3_cpp_obj(attrlist)->set_string(attrlist, "key", ScopedUTF16String(key));
        v3_cpp_obj(attrlist)->set_string(attrlist, "value", ScopedUTF16String(value));
        v3_cpp_obj(fConnection)->notify(fConnection, message);

        v3_cpp_obj_unref(message);
    }

    static void setStateCallback(void* const ptr, const char* const key, const char* const value)
    {
        static_cast<UIVst3*>(ptr)->setState(key, value);
    }
   #endif

   #if DISTRHO_PLUGIN_WANT_MIDI_INPUT
    void sendNote(const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fConnection != nullptr,);

        v3_message** const message = createMessage("midi");
        DISTRHO_SAFE_ASSERT_RETURN(message != nullptr,);

        v3_attribute_list** const attrlist = v3_cpp_obj(message)->get_attributes(message);
        DISTRHO_SAFE_ASSERT_RETURN(attrlist != nullptr,);

        uint8_t midiData[3];
        midiData[0] = (velocity != 0 ? 0x90 : 0x80) | channel;
        midiData[1] = note;
        midiData[2] = velocity;

        v3_cpp_obj(attrlist)->set_int(attrlist, "__dpf_msg_target__", 1);
        v3_cpp_obj(attrlist)->set_binary(attrlist, "data", midiData, sizeof(midiData));
        v3_cpp_obj(fConnection)->notify(fConnection, message);

        v3_cpp_obj_unref(message);
    }

    static void sendNoteCallback(void* const ptr, const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        static_cast<UIVst3*>(ptr)->sendNote(channel, note, velocity);
    }
   #endif

    void setSize(uint width, uint height)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fView != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fFrame != nullptr,);

       #ifdef DISTRHO_OS_MAC
        const double scaleFactor = fUI.getScaleFactor();
        width /= scaleFactor;
        height /= scaleFactor;
       #endif

        if (fIsResizingFromHost)
        {
            if (fNeedsResizeFromPlugin)
            {
                d_debug("plugin->host setSize %u %u (FORCED, exception for first resize)", width, height);
            }
            else
            {
                d_debug("plugin->host setSize %u %u (IGNORED, host resize active)", width, height);
                return;
            }
        }
        else
        {
            d_debug("plugin->host setSize %u %u (OK)", width, height);
        }

        fIsResizingFromPlugin = true;

        v3_view_rect rect;
        rect.left = rect.top = 0;
        rect.right = width;
        rect.bottom = height;
        fNextPluginRect = rect;
        v3_cpp_obj(fFrame)->resize_view(fFrame, fView, &rect);
    }

    static void setSizeCallback(void* const ptr, const uint width, const uint height)
    {
        static_cast<UIVst3*>(ptr)->setSize(width, height);
    }
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * VST3 low-level pointer thingies follow, proceed with care.
 */

// --------------------------------------------------------------------------------------------------------------------
// v3_funknown for classes with a single instance

template<class T>
static uint32_t V3_API dpf_single_instance_ref(void* const self)
{
    return ++(*static_cast<T**>(self))->refcounter;
}

template<class T>
static uint32_t V3_API dpf_single_instance_unref(void* const self)
{
    return --(*static_cast<T**>(self))->refcounter;
}

// --------------------------------------------------------------------------------------------------------------------
// dpf_ui_connection_point

struct dpf_ui_connection_point : v3_connection_point_cpp {
    std::atomic_int refcounter;
    ScopedPointer<UIVst3>& uivst3;
    v3_connection_point** other;

    dpf_ui_connection_point(ScopedPointer<UIVst3>& v)
        : refcounter(1),
          uivst3(v),
          other(nullptr)
    {
        // v3_funknown, single instance
        query_interface = query_interface_connection_point;
        ref = dpf_single_instance_ref<dpf_ui_connection_point>;
        unref = dpf_single_instance_unref<dpf_ui_connection_point>;

        // v3_connection_point
        point.connect = connect;
        point.disconnect = disconnect;
        point.notify = notify;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_funknown

    static v3_result V3_API query_interface_connection_point(void* const self, const v3_tuid iid, void** const iface)
    {
        dpf_ui_connection_point* const point = *static_cast<dpf_ui_connection_point**>(self);

        if (v3_tuid_match(iid, v3_funknown_iid) ||
            v3_tuid_match(iid, v3_connection_point_iid))
        {
            d_debug("UI|query_interface_connection_point => %p %s %p | OK", self, tuid2str(iid), iface);
            ++point->refcounter;
            *iface = self;
            return V3_OK;
        }

        d_debug("DSP|query_interface_connection_point => %p %s %p | WARNING UNSUPPORTED", self, tuid2str(iid), iface);

        *iface = NULL;
        return V3_NO_INTERFACE;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_connection_point

    static v3_result V3_API connect(void* const self, v3_connection_point** const other)
    {
        dpf_ui_connection_point* const point = *static_cast<dpf_ui_connection_point**>(self);
        d_debug("UI|dpf_ui_connection_point::connect => %p %p", self, other);

        DISTRHO_SAFE_ASSERT_RETURN(point->other == nullptr, V3_INVALID_ARG);

        point->other = other;

        if (UIVst3* const uivst3 = point->uivst3)
            uivst3->connect(other);

        return V3_OK;
    };

    static v3_result V3_API disconnect(void* const self, v3_connection_point** const other)
    {
        d_debug("UI|dpf_ui_connection_point::disconnect => %p %p", self, other);
        dpf_ui_connection_point* const point = *static_cast<dpf_ui_connection_point**>(self);

        DISTRHO_SAFE_ASSERT_RETURN(point->other != nullptr, V3_INVALID_ARG);
        DISTRHO_SAFE_ASSERT(point->other == other);

        point->other = nullptr;

        if (UIVst3* const uivst3 = point->uivst3)
            uivst3->disconnect();

        return V3_OK;
    };

    static v3_result V3_API notify(void* const self, v3_message** const message)
    {
        dpf_ui_connection_point* const point = *static_cast<dpf_ui_connection_point**>(self);

        UIVst3* const uivst3 = point->uivst3;
        DISTRHO_SAFE_ASSERT_RETURN(uivst3 != nullptr, V3_NOT_INITIALIZED);

        return uivst3->notify(message);
    }
};

// --------------------------------------------------------------------------------------------------------------------
// dpf_plugin_view_content_scale

struct dpf_plugin_view_content_scale : v3_plugin_view_content_scale_cpp {
    std::atomic_int refcounter;
    ScopedPointer<UIVst3>& uivst3;
    // cached values
    float scaleFactor;

    dpf_plugin_view_content_scale(ScopedPointer<UIVst3>& v)
        : refcounter(1),
          uivst3(v),
          scaleFactor(0.0f)
    {
        // v3_funknown, single instance
        query_interface = query_interface_view_content_scale;
        ref = dpf_single_instance_ref<dpf_plugin_view_content_scale>;
        unref = dpf_single_instance_unref<dpf_plugin_view_content_scale>;

        // v3_plugin_view_content_scale
        scale.set_content_scale_factor = set_content_scale_factor;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_funknown

    static v3_result V3_API query_interface_view_content_scale(void* const self, const v3_tuid iid, void** const iface)
    {
        dpf_plugin_view_content_scale* const scale = *static_cast<dpf_plugin_view_content_scale**>(self);

        if (v3_tuid_match(iid, v3_funknown_iid) ||
            v3_tuid_match(iid, v3_plugin_view_content_scale_iid))
        {
            d_debug("query_interface_view_content_scale => %p %s %p | OK", self, tuid2str(iid), iface);
            ++scale->refcounter;
            *iface = self;
            return V3_OK;
        }

        d_debug("query_interface_view_content_scale => %p %s %p | WARNING UNSUPPORTED", self, tuid2str(iid), iface);

        *iface = NULL;
        return V3_NO_INTERFACE;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_plugin_view_content_scale

    static v3_result V3_API set_content_scale_factor(void* const self, const float factor)
    {
        dpf_plugin_view_content_scale* const scale = *static_cast<dpf_plugin_view_content_scale**>(self);
        d_debug("dpf_plugin_view::set_content_scale_factor => %p %f", self, factor);

        scale->scaleFactor = factor;

        if (UIVst3* const uivst3 = scale->uivst3)
            return uivst3->setContentScaleFactor(factor);

        return V3_NOT_INITIALIZED;
    }
};

#if DPF_VST3_USING_HOST_RUN_LOOP
// --------------------------------------------------------------------------------------------------------------------
// dpf_timer_handler

struct dpf_timer_handler : v3_timer_handler_cpp {
    std::atomic_int refcounter;
    ScopedPointer<UIVst3>& uivst3;
    bool valid;

    dpf_timer_handler(ScopedPointer<UIVst3>& v)
        : refcounter(1),
          uivst3(v),
          valid(true)
    {
        // v3_funknown, single instance
        query_interface = query_interface_timer_handler;
        ref = dpf_single_instance_ref<dpf_timer_handler>;
        unref = dpf_single_instance_unref<dpf_timer_handler>;

        // v3_timer_handler
        timer.on_timer = on_timer;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_funknown

    static v3_result V3_API query_interface_timer_handler(void* self, const v3_tuid iid, void** iface)
    {
        dpf_timer_handler* const timer = *static_cast<dpf_timer_handler**>(self);

        if (v3_tuid_match(iid, v3_funknown_iid) ||
            v3_tuid_match(iid, v3_timer_handler_iid))
        {
            d_debug("query_interface_timer_handler => %p %s %p | OK", self, tuid2str(iid), iface);
            ++timer->refcounter;
            *iface = self;
            return V3_OK;
        }

        d_debug("query_interface_timer_handler => %p %s %p | WARNING UNSUPPORTED", self, tuid2str(iid), iface);

        *iface = NULL;
        return V3_NO_INTERFACE;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_timer_handler

    static void V3_API on_timer(void* self)
    {
        dpf_timer_handler* const timer = *static_cast<dpf_timer_handler**>(self);

        DISTRHO_SAFE_ASSERT_RETURN(timer->valid,);

        timer->uivst3->onTimer();
    }
};
#endif

// --------------------------------------------------------------------------------------------------------------------
// dpf_plugin_view

static const char* const kSupportedPlatforms[] = {
#if defined(DISTRHO_OS_WINDOWS)
    V3_VIEW_PLATFORM_TYPE_HWND,
#elif defined(DISTRHO_OS_MAC)
    V3_VIEW_PLATFORM_TYPE_NSVIEW,
#else
    V3_VIEW_PLATFORM_TYPE_X11,
#endif
};

struct dpf_plugin_view : v3_plugin_view_cpp {
    std::atomic_int refcounter;
    ScopedPointer<dpf_ui_connection_point> connection;
    ScopedPointer<dpf_plugin_view_content_scale> scale;
   #if DPF_VST3_USING_HOST_RUN_LOOP
    ScopedPointer<dpf_timer_handler> timer;
   #endif
    ScopedPointer<UIVst3> uivst3;
    // cached values
    v3_host_application** const hostApplication;
    void* const instancePointer;
    double sampleRate;
    v3_plugin_frame** frame;
    v3_run_loop** runloop;
    uint32_t nextWidth, nextHeight;
    bool sizeRequestedBeforeBeingAttached;

    dpf_plugin_view(v3_host_application** const host, void* const instance, const double sr)
        : refcounter(1),
          hostApplication(host),
          instancePointer(instance),
          sampleRate(sr),
          frame(nullptr),
          runloop(nullptr),
          nextWidth(0),
          nextHeight(0),
          sizeRequestedBeforeBeingAttached(false)
    {
        d_debug("dpf_plugin_view() with hostApplication %p", hostApplication);

        // make sure host application is valid through out this view lifetime
        if (hostApplication != nullptr)
            v3_cpp_obj_ref(hostApplication);

        // v3_funknown, everything custom
        query_interface = query_interface_view;
        ref = ref_view;
        unref = unref_view;

        // v3_plugin_view
        view.is_platform_type_supported = is_platform_type_supported;
        view.attached = attached;
        view.removed = removed;
        view.on_wheel = on_wheel;
        view.on_key_down = on_key_down;
        view.on_key_up = on_key_up;
        view.get_size = get_size;
        view.on_size = on_size;
        view.on_focus = on_focus;
        view.set_frame = set_frame;
        view.can_resize = can_resize;
        view.check_size_constraint = check_size_constraint;
    }

    ~dpf_plugin_view()
    {
        d_debug("~dpf_plugin_view()");

        connection = nullptr;
        scale = nullptr;
       #if DPF_VST3_USING_HOST_RUN_LOOP
        timer = nullptr;
       #endif
        uivst3 = nullptr;

        if (hostApplication != nullptr)
            v3_cpp_obj_unref(hostApplication);
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_funknown

    static v3_result V3_API query_interface_view(void* self, const v3_tuid iid, void** iface)
    {
        dpf_plugin_view* const view = *static_cast<dpf_plugin_view**>(self);

        if (v3_tuid_match(iid, v3_funknown_iid) ||
            v3_tuid_match(iid, v3_plugin_view_iid))
        {
            d_debug("query_interface_view => %p %s %p | OK", self, tuid2str(iid), iface);
            ++view->refcounter;
            *iface = self;
            return V3_OK;
        }

        if (v3_tuid_match(v3_connection_point_iid, iid))
        {
            d_debug("query_interface_view => %p %s %p | OK convert %p",
                    self, tuid2str(iid), iface, view->connection.get());

            if (view->connection == nullptr)
                view->connection = new dpf_ui_connection_point(view->uivst3);
            else
                ++view->connection->refcounter;
            *iface = &view->connection;
            return V3_OK;
        }

       #ifndef DISTRHO_OS_MAC
        if (v3_tuid_match(v3_plugin_view_content_scale_iid, iid))
        {
            d_debug("query_interface_view => %p %s %p | OK convert %p",
                    self, tuid2str(iid), iface, view->scale.get());

            if (view->scale == nullptr)
                view->scale = new dpf_plugin_view_content_scale(view->uivst3);
            else
                ++view->scale->refcounter;
            *iface = &view->scale;
            return V3_OK;
        }
       #endif

        d_debug("query_interface_view => %p %s %p | WARNING UNSUPPORTED", self, tuid2str(iid), iface);

        *iface = nullptr;
        return V3_NO_INTERFACE;
    }

    static uint32_t V3_API ref_view(void* self)
    {
        dpf_plugin_view* const view = *static_cast<dpf_plugin_view**>(self);
        const int refcount = ++view->refcounter;
        d_debug("dpf_plugin_view::ref => %p | refcount %i", self, refcount);
        return refcount;
    }

    static uint32_t V3_API unref_view(void* self)
    {
        dpf_plugin_view** const viewptr = static_cast<dpf_plugin_view**>(self);
        dpf_plugin_view* const view = *viewptr;

        if (const int refcount = --view->refcounter)
        {
            d_debug("dpf_plugin_view::unref => %p | refcount %i", self, refcount);
            return refcount;
        }

        if (view->connection != nullptr && view->connection->other)
            v3_cpp_obj(view->connection->other)->disconnect(view->connection->other,
                                                            (v3_connection_point**)&view->connection);

        /**
         * Some hosts will have unclean instances of a few of the view child classes at this point.
         * We check for those here, going through the whole possible chain to see if it is safe to delete.
         * TODO cleanup.
         */

        bool unclean = false;

        if (dpf_ui_connection_point* const conn = view->connection)
        {
            if (const int refcount = conn->refcounter)
            {
                unclean = true;
                d_stderr("DPF warning: asked to delete view while connection point still active (refcount %d)", refcount);
            }
        }

       #ifndef DISTRHO_OS_MAC
        if (dpf_plugin_view_content_scale* const scale = view->scale)
        {
            if (const int refcount = scale->refcounter)
            {
                unclean = true;
                d_stderr("DPF warning: asked to delete view while content scale still active (refcount %d)", refcount);
            }
        }
       #endif

        if (unclean)
            return 0;

        d_debug("dpf_plugin_view::unref => %p | refcount is zero, deleting everything now!", self);

        delete view;
        delete viewptr;
        return 0;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // v3_plugin_view

    static v3_result V3_API is_platform_type_supported(void* const self, const char* const platform_type)
    {
        d_debug("dpf_plugin_view::is_platform_type_supported => %p %s", self, platform_type);

        for (size_t i=0; i<ARRAY_SIZE(kSupportedPlatforms); ++i)
        {
            if (std::strcmp(kSupportedPlatforms[i], platform_type) == 0)
                return V3_OK;
        }

        return V3_NOT_IMPLEMENTED;

        // unused unless debug
        (void)self;
    }

    static v3_result V3_API attached(void* const self, void* const parent, const char* const platform_type)
    {
        d_debug("dpf_plugin_view::attached => %p %p %s", self, parent, platform_type);
        dpf_plugin_view* const view = *static_cast<dpf_plugin_view**>(self);
        DISTRHO_SAFE_ASSERT_RETURN(view->uivst3 == nullptr, V3_INVALID_ARG);

        for (size_t i=0; i<ARRAY_SIZE(kSupportedPlatforms); ++i)
        {
            if (std::strcmp(kSupportedPlatforms[i], platform_type) == 0)
            {
               #if DPF_VST3_USING_HOST_RUN_LOOP
                // find host run loop to plug ourselves into (required on some systems)
                DISTRHO_SAFE_ASSERT_RETURN(view->frame != nullptr, V3_INVALID_ARG);

                v3_run_loop** runloop = nullptr;
                v3_cpp_obj_query_interface(view->frame, v3_run_loop_iid, &runloop);
                DISTRHO_SAFE_ASSERT_RETURN(runloop != nullptr, V3_INVALID_ARG);

                view->runloop = runloop;
               #endif

                const float lastScaleFactor = view->scale != nullptr ? view->scale->scaleFactor : 0.0f;
                view->uivst3 = new UIVst3((v3_plugin_view**)self,
                                          view->hostApplication,
                                          view->connection != nullptr ? view->connection->other : nullptr,
                                          view->frame,
                                          (uintptr_t)parent,
                                          lastScaleFactor,
                                          view->sampleRate,
                                          view->instancePointer,
                                          view->nextWidth > 0 && view->nextHeight > 0,
                                          view->sizeRequestedBeforeBeingAttached);

                view->uivst3->postInit(view->nextWidth, view->nextHeight);
                view->nextWidth = 0;
                view->nextHeight = 0;
                view->sizeRequestedBeforeBeingAttached = false;

               #if DPF_VST3_USING_HOST_RUN_LOOP
                // register a timer host run loop stuff
                view->timer = new dpf_timer_handler(view->uivst3);
                v3_cpp_obj(runloop)->register_timer(runloop,
                                                    (v3_timer_handler**)&view->timer,
                                                    DPF_VST3_TIMER_INTERVAL);
               #endif

                return V3_OK;
            }
        }

        return V3_NOT_IMPLEMENTED;
    }

    static v3_result V3_API removed(void* const self)
    {
        d_debug("dpf_plugin_view::removed => %p", self);
        dpf_plugin_view* const view = *static_cast<dpf_plugin_view**>(self);
        DISTRHO_SAFE_ASSERT_RETURN(view->uivst3 != nullptr, V3_INVALID_ARG);

       #if DPF_VST3_USING_HOST_RUN_LOOP
        // unregister our timer as needed
        if (v3_run_loop** const runloop = view->runloop)
        {
            if (view->timer != nullptr && view->timer->valid)
            {
                v3_cpp_obj(runloop)->unregister_timer(runloop, (v3_timer_handler**)&view->timer);

                if (const int refcount = --view->timer->refcounter)
                {
                    view->timer->valid = false;
                    d_stderr("VST3 warning: Host run loop did not give away timer (refcount %d)", refcount);
                }
                else
                {
                    view->timer = nullptr;
                }
            }

            v3_cpp_obj_unref(runloop);
            view->runloop = nullptr;
        }
       #endif

        view->uivst3 = nullptr;
        return V3_OK;
    }

    static v3_result V3_API on_wheel(void* const self, const float distance)
    {
#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
        d_debug("dpf_plugin_view::on_wheel => %p %f", self, distance);
        dpf_plugin_view* const view = *static_cast<dpf_plugin_view**>(self);

        UIVst3* const uivst3 = view->uivst3;
        DISTRHO_SAFE_ASSERT_RETURN(uivst3 != nullptr, V3_NOT_INITIALIZED);

        return uivst3->onWheel(distance);
#else
        return V3_NOT_IMPLEMENTED;
        // unused
        (void)self; (void)distance;
#endif
    }

    static v3_result V3_API on_key_down(void* const self, const int16_t key_char, const int16_t key_code, const int16_t modifiers)
    {
#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
        d_debug("dpf_plugin_view::on_key_down => %p %i %i %i", self, key_char, key_code, modifiers);
        dpf_plugin_view* const view = *static_cast<dpf_plugin_view**>(self);

        UIVst3* const uivst3 = view->uivst3;
        DISTRHO_SAFE_ASSERT_RETURN(uivst3 != nullptr, V3_NOT_INITIALIZED);

        return uivst3->onKeyDown(key_char, key_code, modifiers);
#else
        return V3_NOT_IMPLEMENTED;
        // unused
        (void)self; (void)key_char; (void)key_code; (void)modifiers;
#endif
    }

    static v3_result V3_API on_key_up(void* const self, const int16_t key_char, const int16_t key_code, const int16_t modifiers)
    {
#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
        d_debug("dpf_plugin_view::on_key_up => %p %i %i %i", self, key_char, key_code, modifiers);
        dpf_plugin_view* const view = *static_cast<dpf_plugin_view**>(self);

        UIVst3* const uivst3 = view->uivst3;
        DISTRHO_SAFE_ASSERT_RETURN(uivst3 != nullptr, V3_NOT_INITIALIZED);

        return uivst3->onKeyUp(key_char, key_code, modifiers);
#else
        return V3_NOT_IMPLEMENTED;
        // unused
        (void)self; (void)key_char; (void)key_code; (void)modifiers;
#endif
    }

    static v3_result V3_API get_size(void* const self, v3_view_rect* const rect)
    {
        d_debug("dpf_plugin_view::get_size => %p", self);
        dpf_plugin_view* const view = *static_cast<dpf_plugin_view**>(self);

        if (UIVst3* const uivst3 = view->uivst3)
            return uivst3->getSize(rect);

        d_debug("dpf_plugin_view::get_size => %p | NOTE: size request before attach", self);

        view->sizeRequestedBeforeBeingAttached = true;

        double scaleFactor = view->scale != nullptr ? view->scale->scaleFactor : 0.0;
       #if defined(DISTRHO_UI_DEFAULT_WIDTH) && defined(DISTRHO_UI_DEFAULT_HEIGHT)
        rect->right = DISTRHO_UI_DEFAULT_WIDTH;
        rect->bottom = DISTRHO_UI_DEFAULT_HEIGHT;
        if (d_isZero(scaleFactor))
            scaleFactor = 1.0;
       #else
        UIExporter tmpUI(nullptr, 0, view->sampleRate,
                         nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, d_nextBundlePath,
                         view->instancePointer, scaleFactor);
        rect->right = tmpUI.getWidth();
        rect->bottom = tmpUI.getHeight();
        scaleFactor = tmpUI.getScaleFactor();
        tmpUI.quit();
       #endif
        rect->left = rect->top = 0;
       #ifdef DISTRHO_OS_MAC
        rect->right /= scaleFactor;
        rect->bottom /= scaleFactor;
       #endif

        return V3_OK;
    }

    static v3_result V3_API on_size(void* const self, v3_view_rect* const rect)
    {
        d_debug("dpf_plugin_view::on_size => %p {%d,%d,%d,%d}",
                self, rect->top, rect->left, rect->right, rect->bottom);
        DISTRHO_SAFE_ASSERT_INT2_RETURN(rect->right > rect->left, rect->right, rect->left, V3_INVALID_ARG);
        DISTRHO_SAFE_ASSERT_INT2_RETURN(rect->bottom > rect->top, rect->bottom, rect->top, V3_INVALID_ARG);

        dpf_plugin_view* const view = *static_cast<dpf_plugin_view**>(self);

        if (UIVst3* const uivst3 = view->uivst3)
            return uivst3->onSize(rect);

        view->nextWidth = static_cast<uint32_t>(rect->right - rect->left);
        view->nextHeight = static_cast<uint32_t>(rect->bottom - rect->top);
        return V3_OK;
    }

    static v3_result V3_API on_focus(void* const self, const v3_bool state)
    {
#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
        d_debug("dpf_plugin_view::on_focus => %p %u", self, state);
        dpf_plugin_view* const view = *static_cast<dpf_plugin_view**>(self);

        UIVst3* const uivst3 = view->uivst3;
        DISTRHO_SAFE_ASSERT_RETURN(uivst3 != nullptr, V3_NOT_INITIALIZED);

        return uivst3->onFocus(state);
#else
        return V3_NOT_IMPLEMENTED;
        // unused
        (void)self; (void)state;
#endif
    }

    static v3_result V3_API set_frame(void* const self, v3_plugin_frame** const frame)
    {
        d_debug("dpf_plugin_view::set_frame => %p %p", self, frame);
        dpf_plugin_view* const view = *static_cast<dpf_plugin_view**>(self);

        view->frame = frame;

        if (UIVst3* const uivst3 = view->uivst3)
            return uivst3->setFrame(frame);

        return V3_OK;
    }

    static v3_result V3_API can_resize(void* const self)
    {
#if DISTRHO_UI_USER_RESIZABLE
        dpf_plugin_view* const view = *static_cast<dpf_plugin_view**>(self);

        if (UIVst3* const uivst3 = view->uivst3)
            return uivst3->canResize();

        return V3_TRUE;
#else
        return V3_FALSE;

        // unused
        (void)self;
#endif
    }

    static v3_result V3_API check_size_constraint(void* const self, v3_view_rect* const rect)
    {
        d_debug("dpf_plugin_view::check_size_constraint => %p {%d,%d,%d,%d}",
                self, rect->top, rect->left, rect->right, rect->bottom);
        dpf_plugin_view* const view = *static_cast<dpf_plugin_view**>(self);

        if (UIVst3* const uivst3 = view->uivst3)
            return uivst3->checkSizeConstraint(rect);

        return V3_NOT_INITIALIZED;
    }
};

// --------------------------------------------------------------------------------------------------------------------
// dpf_plugin_view_create (called from plugin side)

v3_plugin_view** dpf_plugin_view_create(v3_host_application** host, void* instancePointer, double sampleRate);

v3_plugin_view** dpf_plugin_view_create(v3_host_application** const host,
                                        void* const instancePointer,
                                        const double sampleRate)
{
    dpf_plugin_view** const viewptr = new dpf_plugin_view*;
    *viewptr = new dpf_plugin_view(host, instancePointer, sampleRate);
    return static_cast<v3_plugin_view**>(static_cast<void*>(viewptr));
}

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
