#include "Widget.hpp"
#include "Image.hpp"
#include "SVG.hpp"
#include "resources/MidiKeyboardResources.hpp"

START_NAMESPACE_DISTRHO

class PianoKey
{
public:
    PianoKey()
        : fPressed(false),
          fImageNormal(),
          fImageDown(),
          fIndex(-1)
    {

    }

    /**
       Set the images that will be used when drawing the key.
     */
    void setImages(const Image& imageNormal, const Image& imageDown)
    {
        fImageNormal = imageNormal;
        fImageDown = imageDown;

        fBoundingBox.setSize(imageNormal.getWidth(), imageNormal.getHeight());
    }

    /**
       Set the state of the key.
     */
    void setPressed(bool pressed) noexcept
    {
        fPressed = pressed;
    }

    /**
       Indicate if the key is currently down.
     */
    bool isPressed() noexcept
    {
        return fPressed;
    }

    /**
       Set the note index of the key.
     */
    void setIndex(const int index)
    {
        fIndex = index;
    }

    /**
       Get the note index of the key.
     */
    int getIndex()
    {
        return fIndex;
    }

    /**
       Determine if a point intersects with the key's bounding box.
     */
    bool contains(Point<int> point) noexcept
    {
        return fBoundingBox.contains(point);
    }

    /**
       Set the position of the key, relative to its parent KeyboardWidget.
     */
    void setPosition(const int x, const int y) noexcept
    {
        fBoundingBox.setPos(x, y);
    }

    /**
       Get the width of the key.
     */
    uint getWidth() noexcept
    {
        return fBoundingBox.getWidth();
    }

    /**
       Draw the key at its bounding box's position.
     */
    void draw()
    {
        Image* img;

        if (isPressed())
        {
            img = &fImageDown;
        }
        else
        {
            img = &fImageNormal;
        }

        img->drawAt(fBoundingBox.getPos());
    }

private:
    Rectangle<int> fBoundingBox;
    bool fPressed;
    Image fImageNormal;
    Image fImageDown;
    int fIndex;
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
 static const int kWhiteKeySpacing = 1;

/**
   The number of black keys in the keyboard.
 */
 static const int kBlackKeysCount = 5 * kOctaves;

/**
   The number of keys in the keyboard.
 */
 static const int kKeyCount = kWhiteKeysCount + kBlackKeysCount;


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

    KeyboardWidget(Window& parent)
        : Widget(parent),
          fCallback(nullptr),
          fMouseDown(false),
          fHeldKey(nullptr)
    {
        fSVGs[kWhiteKeyResourceIndex].loadFromMemory(MidiKeyboardResources::white_keyData,
                                                     MidiKeyboardResources::white_keyDataSize,
                                                     1.0f);

        fSVGs[kWhiteKeyPressedResourceIndex].loadFromMemory(MidiKeyboardResources::white_key_pressedData,
                                                            MidiKeyboardResources::white_key_pressedDataSize,
                                                            1.0f);

        fSVGs[kBlackKeyResourceIndex].loadFromMemory(MidiKeyboardResources::black_keyData,
                                                     MidiKeyboardResources::black_keyDataSize,
                                                     1.0f);

        fSVGs[kBlackKeyPressedResourceIndex].loadFromMemory(MidiKeyboardResources::black_key_pressedData,
                                                    MidiKeyboardResources::black_key_pressedDataSize,
                                                    1.0f);

        for (int i = 0; i < kResourcesCount; ++i)
        {
            fImages[i].loadFromSVG(fSVGs[i]);
        }

        const int whiteKeysTotalSpacing = kWhiteKeySpacing * kWhiteKeysCount;

        const uint width = fImages[kWhiteKeyResourceIndex].getWidth() * kWhiteKeysCount + whiteKeysTotalSpacing;
        const uint height = fImages[kWhiteKeyResourceIndex].getHeight();

        setSize(width, height);

        setupKeyLookupTable();
        setKeyImages();
        positionKeys();
    }

    /**
       Set the 'pressed' state of a key in the keyboard.
     */
    void setKeyPressed(const uint keyIndex, const bool pressed, const bool sendCallback = false)
    {
        DISTRHO_SAFE_ASSERT_RETURN(keyIndex < kKeyCount, )

        PianoKey& key = *fKeysLookup[keyIndex];

        if (key.isPressed() == pressed)
            return;

        key.setPressed(pressed);

        if (fCallback != nullptr && sendCallback)
        {
            if (pressed)
            {
                fCallback->keyboardKeyPressed(keyIndex);
            }
            else
            {
                fCallback->keyboardKeyReleased(keyIndex);
            }
        }

        repaint();
    }

    void setCallback(Callback* callback)
    {
        fCallback = callback;
    }

    /**
       Draw the piano keys.
     */
    void onDisplay() override
    {
        // Draw the white keys.
        for (int i = 0; i < kWhiteKeysCount; ++i)
        {
            fWhiteKeys[i].draw();
        }

        // Draw the black keys, on top of the white keys.
        for (int i = 0; i < kBlackKeysCount; ++i)
        {
            fBlackKeys[i].draw();
        }
    }

    /**
       Get the key that is under the specified point.
       Return nullptr if the point is not hovering any key.
     */
    PianoKey* tryGetHoveredKey(const Point<int>& point)
    {
        // Since the black keys are on top of the white keys, we check for a mouse event on the black ones first
        for (int i = 0; i < kBlackKeysCount; ++i)
        {
            if (fBlackKeys[i].contains(point))
            {
                return &fBlackKeys[i];
            }
        }

        // Check for mouse event on white keys
        for (int i = 0; i < kWhiteKeysCount; ++i)
        {
            if (fWhiteKeys[i].contains(point))
            {
                return &fWhiteKeys[i];
            }
        }

        return nullptr;
    }

    /**
       Handle mouse events.
     */
    bool onMouse(const MouseEvent& ev) override
    {
        // We only care about left mouse button events.
        if (ev.button != 1)
        {
            return false;
        }

        fMouseDown = ev.press;

        if (!fMouseDown)
        {
            if (fHeldKey != nullptr)
            {
                setKeyPressed(fHeldKey->getIndex(), false, true);
                fHeldKey = nullptr;

                return true;
            }
        }

        if (!contains(ev.pos))
        {
            return false;
        }

        PianoKey* key = tryGetHoveredKey(ev.pos);

        if (key != nullptr)
        {
            setKeyPressed(key->getIndex(), ev.press, true);
            fHeldKey = key;

            return true;
        }

        return false;
    }

    /**
       Handle mouse motion.
     */
    bool onMotion(const MotionEvent& ev) override
    {
        if (!fMouseDown)
        {
            return false;
        }

        PianoKey* key = tryGetHoveredKey(ev.pos);

        if (key != fHeldKey)
        {
            if (fHeldKey != nullptr)
            {
                setKeyPressed(fHeldKey->getIndex(), false, true);
            }

            if (key != nullptr)
            {
                setKeyPressed(key->getIndex(), true, true);
            }

            fHeldKey = key;

            repaint();
        }

        return true;
    }

private:
    void setupKeyLookupTable()
    {
        int whiteKeysCounter = 0;
        int blackKeysCounter = 0;

        for (int i = 0; i < kKeyCount; ++i)
        {
            if (isBlackKey(i))
            {
                fKeysLookup[i] = &fBlackKeys[blackKeysCounter++];
            }
            else
            {
                fKeysLookup[i] = &fWhiteKeys[whiteKeysCounter++];
            }
        }
    }

    void setKeyImages()
    {
        for (int i = 0; i < kWhiteKeysCount; ++i)
        {
            fWhiteKeys[i].setImages(fImages[kWhiteKeyResourceIndex], fImages[kWhiteKeyPressedResourceIndex]);
        }

        for (int i = 0; i < kBlackKeysCount; ++i)
        {
            fBlackKeys[i].setImages(fImages[kBlackKeyResourceIndex], fImages[kBlackKeyPressedResourceIndex]);
        }
    }

    /**
       Put the keys at their proper position in the keyboard.
     */
    void positionKeys()
    {
        const int whiteKeyWidth = fImages[kWhiteKeyResourceIndex].getWidth();
        const int blackKeyWidth = fImages[kBlackKeyResourceIndex].getWidth();

        int whiteKeysCounter = 0;
        int blackKeysCounter = 0;
        
        for (int i = 0; i < kKeyCount; ++i)
        {
            const int totalSpacing = kWhiteKeySpacing * whiteKeysCounter;

            PianoKey* key;
            int xPos;

            if (isBlackKey(i))
            {
                key = &fBlackKeys[blackKeysCounter];
                xPos = whiteKeysCounter * whiteKeyWidth + totalSpacing - blackKeyWidth / 2;

                blackKeysCounter++;
            }
            else
            {
                key = &fWhiteKeys[whiteKeysCounter];
                xPos = whiteKeysCounter * whiteKeyWidth + totalSpacing;

                whiteKeysCounter++;
            }

            key->setPosition(xPos, 0);
            key->setIndex(i);
        }
    }

    /**
     * Determine if a note at a certain index is associated with a white key.
     */
    bool isWhiteKey(const uint noteIndex)
    {
        return !isBlackKey(noteIndex);
    }

    /**
     * Determine if a note at a certain index is associated with a black key.
     */
    bool isBlackKey(const uint noteIndex)
    {
        // Bring the index down to the first octave
        const uint adjustedIndex = noteIndex % 12;

        return adjustedIndex == 1
            || adjustedIndex == 3
            || adjustedIndex == 6
            || adjustedIndex == 8
            || adjustedIndex == 10;
    }

    /**
     * Identifiers used for accessing the graphical resources of the widget.
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
       The piano key that is currently pressed with the mouse.
       It is a nullptr if no key is currently being held.
    */
    PianoKey* fHeldKey;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KeyboardWidget)
};

END_NAMESPACE_DISTRHO
