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

#include "PluginSimpleGain.hpp"

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

PluginSimpleGain::PluginSimpleGain()
    : Plugin(paramCount, presetCount, 0)  // paramCount param(s), presetCount program(s), 0 states
{
    smooth_gain = new CParamSmooth(20.0f, getSampleRate());

    for (unsigned p = 0; p < paramCount; ++p) {
        Parameter param;
        initParameter(p, param);
        setParameterValue(p, param.ranges.def);
    }
}

PluginSimpleGain::~PluginSimpleGain() {
    delete smooth_gain;
}

// -----------------------------------------------------------------------
// Init

void PluginSimpleGain::initParameter(uint32_t index, Parameter& parameter) {
    if (index >= paramCount)
        return;

    parameter.ranges.min = -90.0f;
    parameter.ranges.max = 30.0f;
    parameter.ranges.def = -0.0f;
    parameter.unit = "db";
    parameter.hints = kParameterIsAutomable;

    switch (index) {
        case paramGain:
            parameter.name = "Gain (dB)";
            parameter.shortName = "Gain";
            parameter.symbol = "gain";
            break;
    }
}

/**
  Set the name of the program @a index.
  This function will be called once, shortly after the plugin is created.
*/
void PluginSimpleGain::initProgramName(uint32_t index, String& programName) {
    if (index < presetCount) {
        programName = factoryPresets[index].name;
    }
}

// -----------------------------------------------------------------------
// Internal data

/**
  Optional callback to inform the plugin about a sample rate change.
*/
void PluginSimpleGain::sampleRateChanged(double newSampleRate) {
    fSampleRate = newSampleRate;
    smooth_gain->setSampleRate(newSampleRate);
}

/**
  Get the current value of a parameter.
*/
float PluginSimpleGain::getParameterValue(uint32_t index) const {
    return fParams[index];
}

/**
  Change a parameter value.
*/
void PluginSimpleGain::setParameterValue(uint32_t index, float value) {
    fParams[index] = value;

    switch (index) {
        case paramGain:
            gain = DB_CO(CLAMP(fParams[paramGain], -90.0, 30.0));
            break;
    }
}

/**
  Load a program.
  The host may call this function from any context,
  including realtime processing.
*/
void PluginSimpleGain::loadProgram(uint32_t index) {
    if (index < presetCount) {
        for (int i=0; i < paramCount; i++) {
            setParameterValue(i, factoryPresets[index].params[i]);
        }
    }
}

// -----------------------------------------------------------------------
// Process

void PluginSimpleGain::activate() {
    // plugin is activated
}



void PluginSimpleGain::run(const float** inputs, float** outputs,
                           uint32_t frames) {

    // get the left and right audio inputs
    const float* const inpL = inputs[0];
    const float* const inpR = inputs[1];

    // get the left and right audio outputs
    float* const outL = outputs[0];
    float* const outR = outputs[1];

    // apply gain against all samples
    for (uint32_t i=0; i < frames; ++i) {
        float gainval = smooth_gain->process(gain);
        outL[i] = inpL[i] * gainval;
        outR[i] = inpR[i] * gainval;
    }
}

// -----------------------------------------------------------------------

Plugin* createPlugin() {
    return new PluginSimpleGain();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
