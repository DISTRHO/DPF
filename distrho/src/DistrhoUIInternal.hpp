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

#ifndef DISTRHO_UI_INTERNAL_HPP_INCLUDED
#define DISTRHO_UI_INTERNAL_HPP_INCLUDED

#include "DistrhoUIPrivateData.hpp"

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

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
UI* createUiWrapper(void* dspPtr, uintptr_t winId, double scaleFactor, const char* bundlePath);
#else // DISTRHO_PLUGIN_HAS_EXTERNAL_UI
UI* createUiWrapper(void* dspPtr, Window* window);
#endif

#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
// -----------------------------------------------------------------------
// Plugin Application, will set class name based on plugin details

class PluginApplication : public Application
{
public:
    PluginApplication()
      : Application(DISTRHO_UI_IS_STANDALONE)
    {
        const char* const className = (
#ifdef DISTRHO_PLUGIN_BRAND
            DISTRHO_PLUGIN_BRAND
#else
            DISTRHO_MACRO_AS_STRING(DISTRHO_NAMESPACE)
#endif
            "-" DISTRHO_PLUGIN_NAME
        );
        setClassName(className);
    }
};

// -----------------------------------------------------------------------
// Plugin Window, needed to take care of resize properly

class UIExporterWindow : public Window
{
public:
    UIExporterWindow(PluginApplication& app, const intptr_t winId, const double scaleFactor, void* const dspPtr)
        : Window(app, winId, scaleFactor, DISTRHO_UI_USER_RESIZABLE),
          fUI(createUiWrapper(dspPtr, this)),
          fIsReady(false)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fUI->uiData != nullptr,);

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

        UI::PrivateData* const uiData = fUI->uiData;
        DISTRHO_SAFE_ASSERT_RETURN(uiData != nullptr,);

        /*
        uiData->resizeInProgress = true;
        fUI->setSize(width, height);
        uiData->resizeInProgress = false;
        */

        fUI->uiReshape(width, height);
        fIsReady = true;
    }

# ifndef DGL_FILE_BROWSER_DISABLED
    // custom file-browser selected
    void onFileSelected(const char* const filename) override
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
               const fileRequestFunc fileRequestCall,
               const char* const bundlePath = nullptr,
               void* const dspPtr = nullptr,
               const double scaleFactor = 1.0,
               const uint32_t bgColor = 0,
               const uint32_t fgColor = 0xffffffff)
#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
        : fUI(createUiWrapper(dspPtr, winId, scaleFactor, bundlePath)),
#else
        : glApp(),
          glWindow(glApp, winId, scaleFactor, dspPtr),
          fChangingSize(false),
          fUI(glWindow.getUI()),
#endif
          fData((fUI != nullptr) ? fUI->uiData : nullptr)
    {
        DISTRHO_SAFE_ASSERT_RETURN(fUI != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr,);

        fData->bgColor = bgColor;
        fData->fgColor = fgColor;

        fData->callbacksPtr            = callbacksPtr;
        fData->editParamCallbackFunc   = editParamCall;
        fData->setParamCallbackFunc    = setParamCall;
        fData->setStateCallbackFunc    = setStateCall;
        fData->sendNoteCallbackFunc    = sendNoteCall;
        fData->setSizeCallbackFunc     = setSizeCall;
        fData->fileRequestCallbackFunc = fileRequestCall;

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

    uintptr_t getNativeWindowHandle() const noexcept
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

    uintptr_t getNativeWindowHandle() const noexcept
    {
        return glWindow.getNativeWindowHandle();
    }
#endif

    uint getBackgroundColor() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, 0);

        return fData->bgColor;
    }

    uint getForegroundColor() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(fData != nullptr, 0xffffffff);

        return fData->fgColor;
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

        glWindow.setVisible(true);
        glApp.addIdleCallback(cb);
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

    void setWindowTransientWinId(const uintptr_t /*winId*/)
    {
#if 0 /* TODO */
        glWindow.setTransientWinId(winId);
#endif
    }

    bool setWindowVisible(const bool yesNo)
    {
        glWindow.setVisible(yesNo);

        return ! glApp.isQuiting();
    }

    bool handlePluginKeyboard(const bool /*press*/, const uint /*key*/, const uint16_t /*mods*/)
    {
#if 0 /* TODO */
        return glWindow.handlePluginKeyboard(press, key);
#endif
        return false;
    }

    bool handlePluginSpecial(const bool /*press*/, const DGL_NAMESPACE::Key /*key*/, const uint16_t /*mods*/)
    {
#if 0 /* TODO */
        return glWindow.handlePluginSpecial(press, key);
#endif
        return false;
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

    PluginApplication glApp;
    UIExporterWindow  glWindow;

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
