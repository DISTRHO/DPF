/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2019 Filipe Coelho <falktx@falktx.com>
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

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
# include "../extra/Sleep.hpp"
using DGL_NAMESPACE::IdleCallback;
#else
# include "../../dgl/Application.hpp"
# include "../../dgl/Window.hpp"
using DGL_NAMESPACE::Application;
using DGL_NAMESPACE::IdleCallback;
using DGL_NAMESPACE::Window;
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// Static data, see DistrhoUI.cpp

extern double      d_lastUiSampleRate;
extern void*       d_lastUiDspPtr;
#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
extern const char* g_nextBundlePath;
extern double      g_nextScaleFactor;
extern uintptr_t   g_nextWindowId;
#else
extern Window*     d_lastUiWindow;
#endif

// -----------------------------------------------------------------------
// UI callbacks

typedef void (*editParamFunc) (void* ptr, uint32_t rindex, bool started);
typedef void (*setParamFunc)  (void* ptr, uint32_t rindex, float value);
typedef void (*setStateFunc)  (void* ptr, const char* key, const char* value);
typedef void (*sendNoteFunc)  (void* ptr, uint8_t channel, uint8_t note, uint8_t velo);
typedef void (*setSizeFunc)   (void* ptr, uint width, uint height);

// -----------------------------------------------------------------------
// UI private data

struct UI::PrivateData {
    // DSP
    double   sampleRate;
    uint32_t parameterOffset;
#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
    void*    dspPtr;
#endif

    // UI
    bool automaticallyScale;
    bool resizeInProgress;
    uint minWidth;
    uint minHeight;

    // Callbacks
    void*         callbacksPtr;
    editParamFunc editParamCallbackFunc;
    setParamFunc  setParamCallbackFunc;
    setStateFunc  setStateCallbackFunc;
    sendNoteFunc  sendNoteCallbackFunc;
    setSizeFunc   setSizeCallbackFunc;

    PrivateData() noexcept
        : sampleRate(d_lastUiSampleRate),
          parameterOffset(0),
#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
          dspPtr(d_lastUiDspPtr),
#endif
          automaticallyScale(false),
          resizeInProgress(false),
          minWidth(0),
          minHeight(0),
          callbacksPtr(nullptr),
          editParamCallbackFunc(nullptr),
          setParamCallbackFunc(nullptr),
          setStateCallbackFunc(nullptr),
          sendNoteCallbackFunc(nullptr),
          setSizeCallbackFunc(nullptr)
    {
        DISTRHO_SAFE_ASSERT(d_isNotZero(sampleRate));

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
            editParamCallbackFunc(callbacksPtr, rindex, started);
    }

    void setParamCallback(const uint32_t rindex, const float value)
    {
        if (setParamCallbackFunc != nullptr)
            setParamCallbackFunc(callbacksPtr, rindex, value);
    }

    void setStateCallback(const char* const key, const char* const value)
    {
        if (setStateCallbackFunc != nullptr)
            setStateCallbackFunc(callbacksPtr, key, value);
    }

    void sendNoteCallback(const uint8_t channel, const uint8_t note, const uint8_t velocity)
    {
        if (sendNoteCallbackFunc != nullptr)
            sendNoteCallbackFunc(callbacksPtr, channel, note, velocity);
    }

    void setSizeCallback(const uint width, const uint height)
    {
        if (setSizeCallbackFunc != nullptr)
            setSizeCallbackFunc(callbacksPtr, width, height);
    }
};

// -----------------------------------------------------------------------
// Plugin Window, needed to take care of resize properly

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
static inline
UI* createUiWrapper(void* const dspPtr, const uintptr_t winId, const double scaleFactor, const char* const bundlePath)
{
    d_lastUiDspPtr    = dspPtr;
    g_nextWindowId    = winId;
    g_nextScaleFactor = scaleFactor;
    g_nextBundlePath  = bundlePath;
    UI* const ret     = createUI();
    d_lastUiDspPtr    = nullptr;
    g_nextWindowId    = 0;
    g_nextScaleFactor = 1.0;
    g_nextBundlePath  = nullptr;
    return ret;
}
#else // DISTRHO_PLUGIN_HAS_EXTERNAL_UI
static inline
UI* createUiWrapper(void* const dspPtr, Window* const window)
{
    d_lastUiDspPtr = dspPtr;
    d_lastUiWindow = window;
    UI* const ret  = createUI();
    d_lastUiDspPtr = nullptr;
    d_lastUiWindow = nullptr;
    return ret;
}

class UIExporterWindow : public Window
{
public:
    UIExporterWindow(Application& app, const intptr_t winId, const double scaleFactor, void* const dspPtr)
        : Window(app, winId, scaleFactor, DISTRHO_UI_USER_RESIZABLE),
          fUI(createUiWrapper(dspPtr, this)),
          fIsReady(false)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fUI->pData != nullptr,);

        setSize(fUI->getWidth(), fUI->getHeight());
    }

    ~UIExporterWindow()
    {
        delete fUI;
    }

    UI* getUI() const noexcept
    {
        return fUI;
    }

    bool isReady() const noexcept
    {
        return fIsReady;
    }

protected:
    // custom window reshape
    void onReshape(uint width, uint height) override
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);

        UI::PrivateData* const pData = fUI->pData;
        DISTRHO_SAFE_ASSERT_RETURN(pData != nullptr,);

        if (pData->automaticallyScale)
        {
            const double scaleHorizontal = static_cast<double>(width) / static_cast<double>(pData->minWidth);
            const double scaleVertical   = static_cast<double>(height) / static_cast<double>(pData->minHeight);
            _setAutoScaling(scaleHorizontal < scaleVertical ? scaleHorizontal : scaleVertical);
        }

        pData->resizeInProgress = true;
        fUI->setSize(width, height);
        pData->resizeInProgress = false;

        fUI->uiReshape(width, height);
        fIsReady = true;
    }

# ifndef DGL_FILE_BROWSER_DISABLED
    // custom file-browser selected
    void fileBrowserSelected(const char* filename) override
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);

        fUI->uiFileBrowserSelected(filename);
    }
# endif

private:
    UI* const fUI;
    bool fIsReady;
};
#endif // DISTRHO_PLUGIN_HAS_EXTERNAL_UI

// -----------------------------------------------------------------------
// UI exporter class

class UIExporter
{
public:
    UIExporter(void* const callbacksPtr,
               const intptr_t winId,
               const editParamFunc editParamCall,
               const setParamFunc setParamCall,
               const setStateFunc setStateCall,
               const sendNoteFunc sendNoteCall,
               const setSizeFunc setSizeCall,
               const float scaleFactor = 1.0f,
               void* const dspPtr = nullptr,
               const char* const bundlePath = nullptr)
#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
        : fUI(createUiWrapper(dspPtr, winId, scaleFactor, bundlePath)),
#else
        : glApp(),
          glWindow(glApp, winId, scaleFactor, dspPtr),
          fChangingSize(false),
          fUI(glWindow.getUI()),
#endif
          fData((fUI != nullptr) ? fUI->pData : nullptr)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr,);

        fData->callbacksPtr          = callbacksPtr;
        fData->editParamCallbackFunc = editParamCall;
        fData->setParamCallbackFunc  = setParamCall;
        fData->setStateCallbackFunc  = setStateCall;
        fData->sendNoteCallbackFunc  = sendNoteCall;
        fData->setSizeCallbackFunc   = setSizeCall;

#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
        // unused
        return; (void)bundlePath;
#endif
    }

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
    ~UIExporter()
    {
        delete fUI;
    }
#endif

    // -------------------------------------------------------------------

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
    uint getWidth() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr, 1);
        return fUI->getWidth();
    }

    uint getHeight() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr, 1);
        return fUI->getHeight();
    }

    bool isVisible() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr, false);
        return fUI->isRunning();
    }

    intptr_t getWindowId() const noexcept
    {
        return 0;
    }
#else
    uint getWidth() const noexcept
    {
        return glWindow.getWidth();
    }

    uint getHeight() const noexcept
    {
        return glWindow.getHeight();
    }

    bool isVisible() const noexcept
    {
        return glWindow.isVisible();
    }

    intptr_t getWindowId() const noexcept
    {
        return glWindow.getWindowId();
    }
#endif

    // -------------------------------------------------------------------

    uint32_t getParameterOffset() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, 0);

        return fData->parameterOffset;
    }

    // -------------------------------------------------------------------

    void parameterChanged(const uint32_t index, const float value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);

        fUI->parameterChanged(index, value);
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void programLoaded(const uint32_t index)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);

        fUI->programLoaded(index);
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    void stateChanged(const char* const key, const char* const value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        DISTRHO_SAFE_ASSERT_RETURN(value != nullptr,);

        fUI->stateChanged(key, value);
    }
#endif

    // -------------------------------------------------------------------

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
    void exec(IdleCallback* const cb)
    {
        DISTRHO_SAFE_ASSERT_RETURN(cb != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);

        fUI->setVisible(true);
        cb->idleCallback();

        while (fUI->isRunning())
        {
            d_msleep(10);
            cb->idleCallback();
        }
    }

    void exec_idle()
    {
    }

    bool idle()
    {
        return true;
    }

    void focus()
    {
    }

    void quit()
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);

        fUI->setVisible(false);
        fUI->terminateAndWaitForProcess();
    }
#else
    void exec(IdleCallback* const cb)
    {
        DISTRHO_SAFE_ASSERT_RETURN(cb != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);

        glWindow.addIdleCallback(cb);
        glWindow.setVisible(true);
        glApp.exec();
    }

    void exec_idle()
    {
        if (glWindow.isReady())
            fUI->uiIdle();
    }

    void focus()
    {
        glWindow.focus();
    }

    bool idle()
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr, false);

        glApp.idle();

        if (glWindow.isReady())
            fUI->uiIdle();

        return ! glApp.isQuiting();
    }

    void quit()
    {
        glWindow.close();
        glApp.quit();
    }

#endif
    // -------------------------------------------------------------------

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
    void setWindowTitle(const char* const uiTitle)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);

        fUI->setTitle(uiTitle);
    }

    void setWindowSize(const uint width, const uint height, const bool = false)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);

        fUI->setSize(width, height);
    }

    void setWindowTransientWinId(const uintptr_t winId)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);

        fUI->setTransientWinId(winId);
    }

    bool setWindowVisible(const bool yesNo)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr, false);

        fUI->setVisible(yesNo);

        return fUI->isRunning();
    }
#else
    void setWindowTitle(const char* const uiTitle)
    {
        glWindow.setTitle(uiTitle);
    }

    void setWindowSize(const uint width, const uint height, const bool updateUI = false)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(! fChangingSize,);

        fChangingSize = true;

        if (updateUI)
            fUI->setSize(width, height);

        glWindow.setSize(width, height);

        fChangingSize = false;
    }

    void setWindowTransientWinId(const uintptr_t winId)
    {
        glWindow.setTransientWinId(winId);
    }

    bool setWindowVisible(const bool yesNo)
    {
        glWindow.setVisible(yesNo);

        return ! glApp.isQuiting();
    }

    bool handlePluginKeyboard(const bool press, const uint key)
    {
        return glWindow.handlePluginKeyboard(press, key);
    }

    bool handlePluginSpecial(const bool press, const DGL_NAMESPACE::Key key)
    {
        return glWindow.handlePluginSpecial(press, key);
    }
#endif

    // -------------------------------------------------------------------

    void setSampleRate(const double sampleRate, const bool doCallback = false)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);
        DISTRHO_SAFE_ASSERT(sampleRate > 0.0);

        if (d_isEqual(fData->sampleRate, sampleRate))
            return;

        fData->sampleRate = sampleRate;

        if (doCallback)
            fUI->sampleRateChanged(sampleRate);
    }

private:
#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
    // -------------------------------------------------------------------
    // DGL Application and Window for this widget

    Application      glApp;
    UIExporterWindow glWindow;

    // prevent recursion
    bool fChangingSize;
#endif

    // -------------------------------------------------------------------
    // Widget and DistrhoUI data

    UI* const fUI;
    UI::PrivateData* const fData;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UIExporter)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_INTERNAL_HPP_INCLUDED
