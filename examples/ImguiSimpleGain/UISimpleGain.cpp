/*
 * Simple Gain audio effect based on DISTRHO Plugin Framework (DPF)
 *
 * SPDX-License-Identifier: MIT
 *
 * Copyright (C) 2021 Jean Pierre Cimalando <jp-dev@inbox.ru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "UISimpleGain.hpp"
#include <imgui.h>
// #include "Window.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------
// Init / Deinit

UISimpleGain::UISimpleGain()
: UI(600, 400)
{
    setGeometryConstraints(600, 400, true);
}

UISimpleGain::~UISimpleGain() {

}

// -----------------------------------------------------------------------
// DSP/Plugin callbacks

/**
  A parameter has changed on the plugin side.
  This is called by the host to inform the UI about parameter changes.
*/
void UISimpleGain::parameterChanged(uint32_t index, float value) {
    params[index] = value;

    switch (index) {
        case PluginSimpleGain::paramGain:
            // do something when Gain param is set, such as update a widget
            break;
    }

    (void)value;
}

/**
  A program has been loaded on the plugin side.
  This is called by the host to inform the UI about program changes.
*/
void UISimpleGain::programLoaded(uint32_t index) {
    if (index < presetCount) {
        for (int i=0; i < PluginSimpleGain::paramCount; i++) {
            // set values for each parameter and update their widgets
            parameterChanged(i, factoryPresets[index].params[i]);
        }
    }
}

/**
  Optional callback to inform the UI about a sample rate change on the plugin side.
*/
void UISimpleGain::sampleRateChanged(double newSampleRate) {
    (void)newSampleRate;
}

// -----------------------------------------------------------------------
// Widget callbacks


/**
  A function called to draw the view contents.
*/
void UISimpleGain::onImGuiDisplay() {
    float width = getWidth();
    float height = getHeight();
    float margin = 20.0f;

    ImGui::SetNextWindowPos(ImVec2(margin, margin));
    ImGui::SetNextWindowSize(ImVec2(width - 2 * margin, height - 2 * margin));

    if (ImGui::Begin("Simple gain")) {
        static char aboutText[256] =
            "This is a demo plugin made with ImGui.\n";
        ImGui::InputTextMultiline("About", aboutText, sizeof(aboutText));

        float& gain = params[PluginSimpleGain::paramGain];
        if (ImGui::SliderFloat("Gain (dB)", &gain, -90.0f, 30.0f))
        {
            if (ImGui::IsItemActivated())
            {
                editParameter(PluginSimpleGain::paramGain, true);
            }
            setParameterValue(PluginSimpleGain::paramGain, gain);
        }
        if (ImGui::IsItemDeactivated())
        {
            editParameter(PluginSimpleGain::paramGain, false);
        }
    }
    ImGui::End();
}

// -----------------------------------------------------------------------

UI* createUI() {
    return new UISimpleGain();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
