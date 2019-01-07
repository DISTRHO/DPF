//          Copyright Jean Pierre Cimalando 2018.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include "Widget.hpp"

class DemoWidgetClickable : public Widget {
public:
    explicit DemoWidgetClickable(Widget *group);
    void onDisplay() override;
    bool onMouse(const MouseEvent &event) override;

private:
    unsigned colorid_ = 0;
};
