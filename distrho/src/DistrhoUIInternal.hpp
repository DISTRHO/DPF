/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2014 Filipe Coelho <falktx@falktx.com>
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

#ifndef DISTRHO_UI_INTERNAL_HPP_INCLUDED
#define DISTRHO_UI_INTERNAL_HPP_INCLUDED

#include "../DistrhoUI.hpp"

#include "../../dgl/App.hpp"
#include "../../dgl/Window.hpp"

using DGL::App;
using DGL::IdleCallback;
using DGL::Window;

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// Static data, see DistrhoUI.cpp

extern double d_lastUiSampleRate;

// -----------------------------------------------------------------------
// UI callbacks

typedef void (*editParamFunc) (void* ptr, uint32_t rindex, bool started);
typedef void (*setParamFunc)  (void* ptr, uint32_t rindex, float value);
typedef void (*setStateFunc)  (void* ptr, const char* key, const char* value);
typedef void (*sendNoteFunc)  (void* ptr, uint8_t channel, uint8_t note, uint8_t velo);
typedef void (*uiResizeFunc)  (void* ptr, uint width, uint height);

// -----------------------------------------------------------------------
// UI private data

struct UI::PrivateData {
    // DSP
    double   sampleRate;
    uint32_t parameterOffset;
#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
    void*    dspPtr;
#endif

    // Callbacks
    editParamFunc editParamCallbackFunc;
    setParamFunc  setParamCallbackFunc;
    setStateFunc  setStateCallbackFunc;
    sendNoteFunc  sendNoteCallbackFunc;
    uiResizeFunc  uiResizeCallbackFunc;
    void*         ptr;

    PrivateData() noexcept
        : sampleRate(d_lastUiSampleRate),
          parameterOffset(0),
#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
          dspPtr(nullptr),
#endif
          editParamCallbackFunc(nullptr),
          setParamCallbackFunc(nullptr),
          setStateCallbackFunc(nullptr),
          sendNoteCallbackFunc(nullptr),
          uiResizeCallbackFunc(nullptr),
          ptr(nullptr)
    {
        DISTRHO_SAFE_ASSERT(sampleRate != 0.0);

#if defined(DISTRHO_PLUGIN_TARGET_DSSI) || defined(DISTRHO_PLUGIN_TARGET_LV2)
        parameterOffset += DISTRHO_PLUGIN_NUM_INPUTS + DISTRHO_PLUGIN_NUM_OUTPUTS;
# if DISTRHO_PLUGIN_WANT_LATENCY
        parameterOffset += 1;
# endif
#endif

#ifdef DISTRHO_PLUGIN_TARGET_LV2
# if (DISTRHO_PLUGIN_IS_SYNTH || DISTRHO_PLUGIN_WANT_TIMEPOS || DISTRHO_PLUGIN_WANT_STATE)
        parameterOffset += 1;
#  if DISTRHO_PLUGIN_WANT_STATE
        parameterOffset += 1;
#  endif
# endif
#endif
    }

    void editParamCallback(const uint32_t rindex, const bool started)
    {
        if (editParamCallbackFunc != nullptr)
            editParamCallbackFunc(ptr, rindex, started);
    }

    void setParamCallback(const uint32_t rindex, const float value)
    {
        if (setParamCallbackFunc != nullptr)
            setParamCallbackFunc(ptr, rindex, value);
    }

    void setStateCallback(const char* const key, const char* const value)
    {
        if (setStateCallbackFunc != nullptr)
            setStateCallbackFunc(ptr, key, value);
    }

    void sendNoteCallback(const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        if (sendNoteCallbackFunc != nullptr)
            sendNoteCallbackFunc(ptr, channel, note, velocity);
    }

    void uiResizeCallback(const uint width, const uint height)
    {
        if (uiResizeCallbackFunc != nullptr)
            uiResizeCallbackFunc(ptr, width, height);
    }
};

// -----------------------------------------------------------------------
// Plugin Window, needed to take care of resize properly

class PluginWindow : public Window
{
public:
    PluginWindow(App& app, const intptr_t winId)
        : Window(app, winId),
          fUi(createUI())
    {
    }

    ~PluginWindow()
    {
        delete fUi;
    }

    UI* getUI() const noexcept
    {
        return fUi;
    }

protected:
    void onReshape(const int width, const int height) override
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUi != nullptr,);

        fUi->setSize(width, height);
        fUi->d_uiResize(width, height);
    }

private:
    UI* const fUi;
};

// -----------------------------------------------------------------------
// UI exporter class

class UIExporter : public IdleCallback
{
public:
    UIExporter(void* const ptr, const intptr_t winId,
               const editParamFunc editParamCall, const setParamFunc setParamCall, const setStateFunc setStateCall, const sendNoteFunc sendNoteCall, const uiResizeFunc uiResizeCall,
               void* const dspPtr = nullptr)
        : glApp(),
          glWindow(glApp, winId),
          fUi(glWindow.getUI()),
          fData((fUi != nullptr) ? fUi->pData : nullptr)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUi != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr,);

        fData->ptr = ptr;
        fData->editParamCallbackFunc = editParamCall;
        fData->setParamCallbackFunc  = setParamCall;
        fData->setStateCallbackFunc  = setStateCall;
        fData->sendNoteCallbackFunc  = sendNoteCall;
        fData->uiResizeCallbackFunc  = uiResizeCall;

        glWindow.setResizable(false);
        glWindow.setSize(fUi->d_getWidth(), fUi->d_getHeight());

#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
        fData->dspPtr = dspPtr;
#else
        return; // unused
        (void)dspPtr;
#endif
    }

    // -------------------------------------------------------------------

    const char* getName() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUi != nullptr, "");

        return fUi->d_getName();
    }

    uint getWidth() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUi != nullptr, 0);

        return fUi->d_getWidth();
    }

    uint getHeight() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUi != nullptr, 0);

        return fUi->d_getHeight();
    }

    // -------------------------------------------------------------------

    uint32_t getParameterOffset() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, 0);

        return fData->parameterOffset;
    }

    // -------------------------------------------------------------------

    void parameterChanged(const uint32_t index, const float value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUi != nullptr,);

        fUi->d_parameterChanged(index, value);
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void programChanged(const uint32_t index)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUi != nullptr,);

        fUi->d_programChanged(index);
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    void stateChanged(const char* const key, const char* const value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUi != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        DISTRHO_SAFE_ASSERT_RETURN(value != nullptr,);

        fUi->d_stateChanged(key, value);
    }
#endif

    // -------------------------------------------------------------------

    void exec()
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUi != nullptr,);

        glWindow.addIdleCallback(this);
        glWindow.setVisible(true);
        glApp.exec();
    }

    bool idle()
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUi != nullptr, false);

        fUi->d_uiIdle();
        glApp.idle();

        return ! glApp.isQuiting();
    }

    bool isVisible() const noexcept
    {
        return glWindow.isVisible();
    }

    void quit()
    {
        glWindow.close();
        glApp.quit();
    }

    void setSize(const uint width, const uint height)
    {
        glWindow.setSize(width, height);
    }

    void setTitle(const char* const uiTitle)
    {
        glWindow.setTitle(uiTitle);
    }

    void setTransientWinId(const intptr_t winId)
    {
        glWindow.setTransientWinId(winId);
    }

    bool setVisible(const bool yesNo)
    {
        glWindow.setVisible(yesNo);

        return ! glApp.isQuiting();
    }

protected:
    void idleCallback() override
    {
        fUi->d_uiIdle();
    }

private:
    // -------------------------------------------------------------------
    // DGL Application and Window for this widget

    App          glApp;
    PluginWindow glWindow;

    // -------------------------------------------------------------------
    // Widget and DistrhoUI data

    UI* const fUi;
    UI::PrivateData* const fData;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UIExporter)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_INTERNAL_HPP_INCLUDED
