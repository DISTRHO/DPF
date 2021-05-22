#ifndef KEYBOARD_WIDGET_H_INCLUDED
#define KEYBOARD_WIDGET_H_INCLUDED

#include "Widget.hpp"
#include "Image.hpp"
#include "SVG.hpp"
#include "PianoKey.hpp"

START_NAMESPACE_DISTRHO

class KeyboardWidget : public Widget
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void keyboardKeyPressed(const uint keyIndex) = 0;
        virtual void keyboardKeyReleased(const uint keyIndex) = 0;
    };

    KeyboardWidget(Window& parent);

    /**
       Set the 'pressed' state of a key in the keyboard.
     */
    void setKeyPressed(const int keyIndex, const bool pressed, const bool sendCallback = false);

    void setCallback(Callback* callback);

protected:
    /**
       Draw the piano keys.
     */
    void onDisplay() override;

    /**
       Handle mouse events.
     */
    bool onMouse(const MouseEvent& ev) override;

    /**
       Handle mouse motion.
     */
    bool onMotion(const MotionEvent& ev) override;

private:
    /**
       Get the key that is under the specified point.
       Return nullptr if the point is not hovering any key.
     */
    PianoKey* tryGetHoveredKey(const Point<int>& point);

    void setupKeyLookupTable();

    /**
       Associate every key with its proper images.
     */
    void setKeyImages();

    /**
       Put the keys at their proper position in the keyboard.
     */
    void positionKeys();

    /**
       Determine if a note at a certain index is associated with a white key.
     */
    bool isWhiteKey(const uint noteIndex);

    /**
       Determine if a note at a certain index is associated with a black key.
     */
    bool isBlackKey(const uint noteIndex);

    /**
       Identifiers used for accessing the graphical resources of the widget.
     */
    enum Resources
    {
        kWhiteKeyResourceIndex = 0,
        kWhiteKeyPressedResourceIndex,
        kBlackKeyResourceIndex,
        kBlackKeyPressedResourceIndex,
        kResourcesCount
    };

   /**
      The number of octaves displayed in the keyboard.
    */
    static const int kOctaves = 2;

   /**
      The number of white keys displayed in the keyboard.
    */
    static const int kWhiteKeysCount = 7 * kOctaves + 1;

   /**
      The spacing in pixels between the white keys.
     */
    static const int kWhiteKeySpacing = 3;

   /**
      The number of black keys in the keyboard.
    */
    static const int kBlackKeysCount = 5 * kOctaves;

   /**
      The number of keys in the keyboard.
    */
    static const int kKeyCount = kWhiteKeysCount + kBlackKeysCount;

   /**
      The keyboard's white keys.
    */
    PianoKey fWhiteKeys[kWhiteKeysCount];

   /**
      The keyboard's black keys.
    */
    PianoKey fBlackKeys[kBlackKeysCount];

   /**
      Zero-indexed lookup table that maps notes to piano keys.
      In this example, 0 is equal to C4.
    */
    PianoKey* fKeysLookup[kKeyCount];

   /**
      Graphical resources.
    */
    SVG fSVGs[kResourcesCount];
    Image fImages[kResourcesCount];

   /**
      The callback pointer, used for notifying the UI of piano key presses and releases.
    */
    Callback* fCallback;

   /**
      Whether or not the left mouse button is currently pressed.
    */
    bool fMouseDown;

   /**
      The piano key that is currently being pressed with the mouse.
      It is a nullptr if no key is currently being held down.
    */
    PianoKey* fHeldKey;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KeyboardWidget)
};

END_NAMESPACE_DISTRHO

#endif
