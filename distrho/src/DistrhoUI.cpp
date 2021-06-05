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

#include "DistrhoUIPrivateData.hpp"
#include "src/WindowPrivateData.hpp"
#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
# include "src/TopLevelWidgetPrivateData.hpp"
#endif

START_NAMESPACE_DISTRHO

/* ------------------------------------------------------------------------------------------------------------
 * Static data, see DistrhoUIInternal.hpp */

#if DISTRHO_PLUGIN_HAS_EXTERNAL_UI
uintptr_t   g_nextWindowId    = 0;
double      g_nextScaleFactor = 1.0;
const char* g_nextBundlePath  = nullptr;
#endif

/* ------------------------------------------------------------------------------------------------------------
 * UI::PrivateData special handling */

UI::PrivateData* UI::PrivateData::s_nextPrivateData = nullptr;

PluginWindow& UI::PrivateData::createNextWindow(UI* const ui, const uint width, const uint height)
{
    UI::PrivateData* const pData = s_nextPrivateData;
    pData->window = new PluginWindow(ui, pData, width, height);
    return pData->window.getObject();
}

/* ------------------------------------------------------------------------------------------------------------
 * UI */

UI::UI(const uint width, const uint height)
    : UIWidget(UI::PrivateData::createNextWindow(this, width, height)),
      uiData(UI::PrivateData::s_nextPrivateData)
{
#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
    if (width > 0 && height > 0)
        Widget::setSize(width, height);
#endif
}

UI::~UI()
{
}

/* ------------------------------------------------------------------------------------------------------------
 * Host state */

bool UI::isResizable() const noexcept
{
#if DISTRHO_UI_USER_RESIZABLE
    return uiData->window->isResizable();
#else
    return false;
#endif
}

uint UI::getBackgroundColor() const noexcept
{
    return uiData->bgColor;
}

uint UI::getForegroundColor() const noexcept
{
    return uiData->fgColor;
}

double UI::getSampleRate() const noexcept
{
    return uiData->sampleRate;
}

void UI::editParameter(uint32_t index, bool started)
{
    uiData->editParamCallback(index + uiData->parameterOffset, started);
}

void UI::setParameterValue(uint32_t index, float value)
{
    uiData->setParamCallback(index + uiData->parameterOffset, value);
}

#if DISTRHO_PLUGIN_WANT_STATE
void UI::setState(const char* key, const char* value)
{
    uiData->setStateCallback(key, value);
}
#endif

#if DISTRHO_PLUGIN_WANT_STATEFILES
bool UI::requestStateFile(const char* key)
{
    return uiData->fileRequestCallback(key);
}
#endif

#if DISTRHO_PLUGIN_WANT_MIDI_INPUT
void UI::sendNote(uint8_t channel, uint8_t note, uint8_t velocity)
{
    uiData->sendNoteCallback(channel, note, velocity);
}
#endif

#if DISTRHO_PLUGIN_WANT_DIRECT_ACCESS
/* ------------------------------------------------------------------------------------------------------------
 * Direct DSP access */

void* UI::getPluginInstancePointer() const noexcept
{
    return uiData->dspPtr;
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
#endif

/* ------------------------------------------------------------------------------------------------------------
 * DSP/Plugin Callbacks (optional) */

void UI::sampleRateChanged(double)
{
}

#if !DISTRHO_PLUGIN_HAS_EXTERNAL_UI
/* ------------------------------------------------------------------------------------------------------------
 * UI Callbacks (optional) */

void UI::uiFocus(bool, CrossingMode)
{
}

void UI::uiReshape(uint, uint)
{
    // NOTE this must be the same as Window::onReshape
    pData->fallbackOnResize();
}

void UI::uiScaleFactorChanged(double)
{
}

# ifndef DGL_FILE_BROWSER_DISABLED
void UI::uiFileBrowserSelected(const char*)
{
}
# endif

/* ------------------------------------------------------------------------------------------------------------
 * UI Resize Handling, internal */

void UI::onResize(const ResizeEvent& ev)
{
    UIWidget::onResize(ev);

    const uint width = ev.size.getWidth();
    const uint height = ev.size.getHeight();
    uiData->setSizeCallback(width, height);
}
#endif // !DISTRHO_PLUGIN_HAS_EXTERNAL_UI

// -----------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO
