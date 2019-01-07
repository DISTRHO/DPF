//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

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
    return "Boost Software License";
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
