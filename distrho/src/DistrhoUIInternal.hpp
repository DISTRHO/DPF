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
#endif

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// Static data, see DistrhoUI.cpp

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
extern uintptr_t   g_nextWindowId;
extern double      g_nextScaleFactor;
extern const char* g_nextBundlePath;
#endif

// -----------------------------------------------------------------------
// UI exporter class

class UIExporter
{
    // -------------------------------------------------------------------
    // UI Widget and its private data

    UI* ui;
    UI::PrivateData* uiData;

    // -------------------------------------------------------------------

public:
    UIExporter(void* const callbacksPtr,
               const uintptr_t winId,
               const double sampleRate,
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
        : ui(nullptr),
          uiData(new UI::PrivateData())
    {
        uiData->sampleRate = sampleRate;
        uiData->dspPtr = dspPtr;

        uiData->bgColor = bgColor;
        uiData->fgColor = fgColor;
#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
        uiData->scaleFactor = scaleFactor;
        uiData->winId = winId;
#endif

        uiData->callbacksPtr            = callbacksPtr;
        uiData->editParamCallbackFunc   = editParamCall;
        uiData->setParamCallbackFunc    = setParamCall;
        uiData->setStateCallbackFunc    = setStateCall;
        uiData->sendNoteCallbackFunc    = sendNoteCall;
        uiData->setSizeCallbackFunc     = setSizeCall;
        uiData->fileRequestCallbackFunc = fileRequestCall;

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
        g_nextWindowId    = winId;
        g_nextScaleFactor = scaleFactor;
        g_nextBundlePath  = bundlePath;
#endif
        UI::PrivateData::s_nextPrivateData = uiData;

        UI* const uiPtr = createUI();

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
        g_nextWindowId    = 0;
        g_nextScaleFactor = 0.0;
        g_nextBundlePath  = nullptr;
#else
        uiData->window->leaveContext();
#endif
        UI::PrivateData::s_nextPrivateData = nullptr;

        DISTRHO_SAFE_ASSERT_RETURN(uiPtr != nullptr,);
        ui = uiPtr;

#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
        // unused
        (void)bundlePath;
#endif
    }

    ~UIExporter()
    {
        delete ui;
        delete uiData;
    }

    // -------------------------------------------------------------------

    uint getWidth() const noexcept
    {
        return uiData->window->getWidth();
    }

    uint getHeight() const noexcept
    {
        return uiData->window->getHeight();
    }

    bool isVisible() const noexcept
    {
        return uiData->window->isVisible();
    }

    uintptr_t getNativeWindowHandle() const noexcept
    {
        return uiData->window->getNativeWindowHandle();
    }

    uint getBackgroundColor() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(uiData != nullptr, 0);

        return uiData->bgColor;
    }

    uint getForegroundColor() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(uiData != nullptr, 0xffffffff);

        return uiData->fgColor;
    }

    // -------------------------------------------------------------------

    uint32_t getParameterOffset() const noexcept
    {
        DISTRHO_SAFE_ASSERT_RETURN(uiData != nullptr, 0);

        return uiData->parameterOffset;
    }

    // -------------------------------------------------------------------

    void parameterChanged(const uint32_t index, const float value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(ui != nullptr,);

        ui->parameterChanged(index, value);
    }

#if DISTRHO_PLUGIN_WANT_PROGRAMS
    void programLoaded(const uint32_t index)
    {
        DISTRHO_SAFE_ASSERT_RETURN(ui != nullptr,);

        ui->programLoaded(index);
    }
#endif

#if DISTRHO_PLUGIN_WANT_STATE
    void stateChanged(const char* const key, const char* const value)
    {
        DISTRHO_SAFE_ASSERT_RETURN(ui != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(key != nullptr && key[0] != '\0',);
        DISTRHO_SAFE_ASSERT_RETURN(value != nullptr,);

        ui->stateChanged(key, value);
    }
#endif

    // -------------------------------------------------------------------

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
    void exec(DGL_NAMESPACE::IdleCallback* const cb)
    {
        DISTRHO_SAFE_ASSERT_RETURN(cb != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(ui != nullptr,);

        ui->setVisible(true);
        cb->idleCallback();

        while (ui->isRunning())
        {
            d_msleep(10);
            cb->idleCallback();
        }
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
        DISTRHO_SAFE_ASSERT_RETURN(ui != nullptr,);

        ui->setVisible(false);
        ui->terminateAndWaitForProcess();
    }
#else
# if DISTRHO_UI_IS_STANDALONE
    void exec(DGL_NAMESPACE::IdleCallback* const cb)
    {
        DISTRHO_SAFE_ASSERT_RETURN(cb != nullptr,);

        uiData->window->show();
        uiData->app.addIdleCallback(cb);
        uiData->app.exec();
    }

    void exec_idle()
    {
        DISTRHO_SAFE_ASSERT_RETURN(ui != nullptr, );

        ui->uiIdle();
    }
# else
    bool plugin_idle()
    {
        DISTRHO_SAFE_ASSERT_RETURN(ui != nullptr, false);

        uiData->app.idle();
        ui->uiIdle();
        return ! uiData->app.isQuiting();
    }
# endif

    void focus()
    {
        uiData->window->focus();
    }

    void quit()
    {
        uiData->window->close();
        uiData->app.quit();
    }
#endif

    // -------------------------------------------------------------------

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
    void setWindowTitle(const char* const uiTitle)
    {
        DISTRHO_SAFE_ASSERT_RETURN(ui != nullptr,);

        ui->setTitle(uiTitle);
    }

    void setWindowSize(const uint width, const uint height)
    {
        DISTRHO_SAFE_ASSERT_RETURN(ui != nullptr,);

        ui->setSize(width, height);
    }

    void setWindowTransientWinId(const uintptr_t winId)
    {
        DISTRHO_SAFE_ASSERT_RETURN(ui != nullptr,);

        ui->setTransientWinId(winId);
    }

    bool setWindowVisible(const bool yesNo)
    {
        DISTRHO_SAFE_ASSERT_RETURN(ui != nullptr, false);

        ui->setVisible(yesNo);

        return ui->isRunning();
    }
#else
    void setWindowTitle(const char* const uiTitle)
    {
        uiData->window->setTitle(uiTitle);
    }

    void setWindowTransientWinId(const uintptr_t /*winId*/)
    {
#if 0 /* TODO */
        glWindow.setTransientWinId(winId);
#endif
    }

    bool setWindowVisible(const bool yesNo)
    {
        uiData->window->setVisible(yesNo);

        return ! uiData->app.isQuiting();
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
        DISTRHO_SAFE_ASSERT_RETURN(ui != nullptr,);
        DISTRHO_SAFE_ASSERT_RETURN(uiData != nullptr,);
        DISTRHO_SAFE_ASSERT(sampleRate > 0.0);

        if (d_isEqual(uiData->sampleRate, sampleRate))
            return;

        uiData->sampleRate = sampleRate;

        if (doCallback)
            ui->sampleRateChanged(sampleRate);
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(UIExporter)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_UI_INTERNAL_HPP_INCLUDED
