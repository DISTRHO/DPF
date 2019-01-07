//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "DistrhoUI.hpp"
#include <memory>
class DemoWidgetClickable;
class DemoWidgetBanner;

class ExampleUI : public UI {
public:
    ExampleUI();
    ~ExampleUI();
    void onDisplay() override;
    void parameterChanged(uint32_t index, float value) override;

private:
    std::unique_ptr<DemoWidgetClickable> widget_clickable_;
    std::unique_ptr<DemoWidgetBanner> widget_banner_;
 };
