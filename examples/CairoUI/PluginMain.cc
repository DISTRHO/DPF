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

#include "PluginMain.h"
#include <string.h>

ExamplePlugin::ExamplePlugin()
    : Plugin(0, 0, 0)
{
}

const char *ExamplePlugin::getLabel() const
{
    return "Cairo DPF Example";
}

const char *ExamplePlugin::getMaker() const
{
    return "Jean Pierre Cimalando";
}

const char *ExamplePlugin::getLicense() const
{
    return "ISC";
}

uint32_t ExamplePlugin::getVersion() const
{
    return 0;
}

int64_t ExamplePlugin::getUniqueId() const
{
    return 0;
}

void ExamplePlugin::initParameter(uint32_t index, Parameter &parameter)
{
}

float ExamplePlugin::getParameterValue(uint32_t index) const
{
    return 0;
}

void ExamplePlugin::setParameterValue(uint32_t index, float value)
{
}

void ExamplePlugin::run(const float **inputs, float **outputs, uint32_t frames)
{
    memcpy(outputs[0], inputs[0], frames * sizeof(float));
}

Plugin *DISTRHO::createPlugin()
{
    return new ExamplePlugin;
}
