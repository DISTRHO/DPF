#ifndef PIANO_KEY_H_INCLUDED
#define PIANO_KEY_H_INCLUDED

#include "Image.hpp"

START_NAMESPACE_DISTRHO

class PianoKey
{
public:
    PianoKey();

    /**
       Set the images that will be used when drawing the key.
     */
    void setImages(const Image& imageNormal, const Image& imageDown) noexcept;

    /**
       Set the state of the key.
     */
    void setPressed(bool pressed) noexcept;

    /**
       Indicate if the key is currently down.
     */
    bool isPressed() const noexcept;

    /**
       Set the note index of the key.
     */
    void setIndex(const int index) noexcept;

    /**
       Get the note index of the key.
     */
    int getIndex() const noexcept;

    /**
       Determine if a point intersects with the key's bounding box.
     */
    bool contains(Point<int> point) const noexcept;

    /**
       Set the position of the key, relative to its parent KeyboardWidget.
     */
    void setPosition(const int x, const int y) noexcept;

    /**
       Get the width of the key.
     */
    uint getWidth() const noexcept;

    /**
       Draw the key at its bounding box's position.
     */
    void draw() noexcept;

private:
    Rectangle<int> fBoundingBox;
    bool fPressed;
    Image fImageNormal;
    Image fImageDown;
    int fIndex;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PianoKey)
};

END_NAMESPACE_DISTRHO

#endif
