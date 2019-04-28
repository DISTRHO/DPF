#include "Widget.hpp"


START_NAMESPACE_DISTRHO

class KeyboardWidget : public Widget
{
public:
    KeyboardWidget(Window& parent) : Widget(parent)
    {

    }

    void onDisplay() override
    {
        const SVG &svg = SVG(MidiKeyboardResources::black_keyData, 
                             MidiKeyboardResources::black_keyWidth, 
                             MidiKeyboardResources::black_keyHeight,
                             1.0f);
        
        Image img = Image();
        img.loadFromSVG(svg);

        img.drawAt(0,0);
    }
};

END_NAMESPACE_DISTRHO