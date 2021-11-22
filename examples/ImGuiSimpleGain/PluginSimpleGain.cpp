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
    : Plugin(paramCount, 0, 0), // parameters, programs, states
      fSampleRate(getSampleRate()),
      fGainDB(0.0f),
      fGainLinear(1.0f)
{
    fSmoothGain = new CParamSmooth(20.0f, fSampleRate);
}

// -----------------------------------------------------------------------
// Init

void PluginSimpleGain::initParameter(uint32_t index, Parameter& parameter)
{
    DISTRHO_SAFE_ASSERT_RETURN(index == 0,);

    parameter.ranges.min = -90.0f;
    parameter.ranges.max = 30.0f;
    parameter.ranges.def = -0.0f;
    parameter.hints = kParameterIsAutomatable;
    parameter.name = "Gain";
    parameter.shortName = "Gain";
    parameter.symbol = "gain";
    parameter.unit = "dB";
}

// -----------------------------------------------------------------------
// Internal data

/**
  Get the current value of a parameter.
*/
float PluginSimpleGain::getParameterValue(uint32_t index) const
{
    DISTRHO_SAFE_ASSERT_RETURN(index == 0, 0.0f);

    return fGainDB;
}

/**
  Change a parameter value.
*/
void PluginSimpleGain::setParameterValue(uint32_t index, float value)
{
    DISTRHO_SAFE_ASSERT_RETURN(index == 0,);

    fGainDB = value;
    fGainLinear = DB_CO(CLAMP(value, -90.0, 30.0));
}

// -----------------------------------------------------------------------
// Process

void PluginSimpleGain::activate()
{
    fSmoothGain->flush();
}

void PluginSimpleGain::run(const float** inputs, float** outputs, uint32_t frames)
{
    // get the left and right audio inputs
    const float* const inpL = inputs[0];
    const float* const inpR = inputs[1];

    // get the left and right audio outputs
    float* const outL = outputs[0];
    float* const outR = outputs[1];

    // apply gain against all samples
    for (uint32_t i=0; i < frames; ++i)
    {
        const float gain = fSmoothGain->process(fGainLinear);
        outL[i] = inpL[i] * gain;
        outR[i] = inpR[i] * gain;
    }
}

// -----------------------------------------------------------------------

/**
  Optional callback to inform the plugin about a sample rate change.
*/
void PluginSimpleGain::sampleRateChanged(double newSampleRate)
{
    fSampleRate = newSampleRate;
    fSmoothGain->setSampleRate(newSampleRate);
}

// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new PluginSimpleGain();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
