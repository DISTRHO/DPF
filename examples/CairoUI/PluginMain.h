//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "DistrhoPlugin.hpp"

class ExamplePlugin : public Plugin {
public:
    ExamplePlugin();
    const char *getLabel() const override;
    const char *getMaker() const override;
    const char *getLicense() const override;
    uint32_t getVersion() const override;
    int64_t getUniqueId() const override;
    void initParameter(uint32_t index, Parameter &parameter) override;
    float getParameterValue(uint32_t index) const override;
    void setParameterValue(uint32_t index, float value) override;
    void run(const float **inputs, float **outputs, uint32_t frames) override;
};
