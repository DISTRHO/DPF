/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2020 Filipe Coelho <falktx@falktx.com>
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

#include "DistrhoUIPrivateData.hpp"
#include "src/WindowPrivateData.hpp"
#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
# include "src/WidgetPrivateData.hpp"
#endif

START_NAMESPACE_DISTRHO

/* ------------------------------------------------------------------------------------------------------------
 * Static data, see DistrhoUIInternal.hpp and DistrhoUIPrivateData.hpp  */

double      d_lastUiSampleRate = 0.0;
void*       d_lastUiDspPtr     = nullptr;
#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
const char* g_nextBundlePath   = nullptr;
double      g_nextScaleFactor  = 1.0;
uintptr_t   g_nextWindowId     = 0;
#else
Window*     d_lastUiWindow     = nullptr;
#endif

// -----------------------------------------------------------------------------------------------------------

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
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
#else
UI* createUiWrapper(void* const dspPtr, Window* const window)
{
    d_lastUiDspPtr = dspPtr;
    d_lastUiWindow = window;
    UI* const ret  = createUI();
    d_lastUiDspPtr = nullptr;
    d_lastUiWindow = nullptr;
    return ret;
}
#endif // DISTRHO_PLUGIN_HAS_EXTERNAL_UI

/* ------------------------------------------------------------------------------------------------------------
 * UI */

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
UI::UI(uint width, uint height)
    : UIWidget(width, height),
      pData(new PrivateData()) {}
#else
UI::UI(uint width, uint height)
    : UIWidget(*d_lastUiWindow),
      pData(new PrivateData())
{
    ((UIWidget*)this)->pData->needsFullViewport = false;

    if (width > 0 && height > 0)
        setSize(width, height);
}
#endif

UI::~UI()
{
    delete pData;
}

#if DISTRHO_UI_USER_RESIZABLE && !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
void UI::setGeometryConstraints(uint minWidth, uint minHeight, bool keepAspectRatio, bool automaticallyScale)
{
    DISTRHO_SAFE_ASSERT_RETURN(minWidth > 0,);
    DISTRHO_SAFE_ASSERT_RETURN(minHeight > 0,);

    pData->automaticallyScale = automaticallyScale;
    pData->minWidth = minWidth;
    pData->minHeight = minHeight;

    Window& window(getParentWindow());

    const double uiScaleFactor = window.getScaling();
    window.setGeometryConstraints(minWidth * uiScaleFactor, minHeight * uiScaleFactor, keepAspectRatio);

    if (d_isNotZero(uiScaleFactor - 1.0))
        setSize(getWidth() * uiScaleFactor, getHeight() * uiScaleFactor);
}
#endif

/* ------------------------------------------------------------------------------------------------------------
 * Host state */

uint UI::getBackgroundColor() const noexcept
{
    return pData->bgColor;
}

uint UI::getForegroundColor() const noexcept
{
    return pData->fgColor;
}

double UI::getSampleRate() const noexcept
{
    return pData->sampleRate;
}

void UI::editParameter(uint32_t index, bool started)
{
    pData->editParamCallback(index + pData->parameterOffset, started);
}

void UI::setParameterValue(uint32_t index, float value)
{
    pData->setParamCallback(index + pData->parameterOffset, value);
}

#if DISTRHO_PLUGIN_WANT_STATE
void UI::setState(const char* key, const char* value)
{
    pData->setStateCallback(key, value);
}
#endif

#if DISTRHO_PLUGIN_WANT_STATEFILES
bool UI::requestStateFile(const char* key)
{
    return pData->fileRequestCallback(key);
}
#endif

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
void UI::sendNote(uint8_t channel, uint8_t note, uint8_t velocity)
{
    pData->sendNoteCallback(channel, note, velocity);
}
#endif

#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
/* ------------------------------------------------------------------------------------------------------------
 * Direct DSP access */

void* UI::getPluginInstancePointer() const noexcept
{
    return pData->dspPtr;
}
#endif

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
/* ------------------------------------------------------------------------------------------------------------
 * External UI helpers */

const char* UI::getNextBundlePath() noexcept
{
    return g_nextBundlePath;
}

double UI::getNextScaleFactor() noexcept
{
    return g_nextScaleFactor;
}

# if DISTRHO_PLUGIN_HAS_EMBED_UI
uintptr_t UI::getNextWindowId() noexcept
{
    return g_nextWindowId;
}
# endif
#endif // DISTRHO_PLUGIN_HAS_EXTERNAL_UI

/* ------------------------------------------------------------------------------------------------------------
 * DSP/Plugin Callbacks (optional) */

void UI::sampleRateChanged(double) {}

#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
/* ------------------------------------------------------------------------------------------------------------
 * UI Callbacks (optional) */

# ifndef DGL_FILE_BROWSER_DISABLED
void UI::uiFileBrowserSelected(const char*)
{
}
# endif

void UI::uiReshape(uint width, uint height)
{
    Window::PrivateData::Fallback::onReshape(width, height);
}

/* ------------------------------------------------------------------------------------------------------------
 * UI Resize Handling, internal */

void UI::onResize(const ResizeEvent& ev)
{
    if (pData->resizeInProgress)
        return;

    pData->setSizeCallback(ev.size.getWidth(), ev.size.getHeight());
}
#endif // !DISTRHO_PLUGIN_HAS_EXTERNAL_UI

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
