#include "KeyboardWidget.hpp"
#include "resources/MidiKeyboardResources.hpp"

START_NAMESPACE_DISTRHO

KeyboardWidget::KeyboardWidget(Window& parent)
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

void KeyboardWidget::setKeyPressed(const int keyIndex, const bool pressed, const bool sendCallback)
{
    DISTRHO_SAFE_ASSERT_RETURN(keyIndex >= 0 && keyIndex < kKeyCount, )

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

void KeyboardWidget::setCallback(Callback* callback)
{
    fCallback = callback;
}

void KeyboardWidget::onDisplay()
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

PianoKey* KeyboardWidget::tryGetHoveredKey(const Point<int>& point)
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

bool KeyboardWidget::onMouse(const MouseEvent& ev)
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

bool KeyboardWidget::onMotion(const MotionEvent& ev)
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

void KeyboardWidget::setupKeyLookupTable()
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

void KeyboardWidget::setKeyImages()
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

void KeyboardWidget::positionKeys()
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

bool KeyboardWidget::isWhiteKey(const uint noteIndex)
{
    return !isBlackKey(noteIndex);
}

bool KeyboardWidget::isBlackKey(const uint noteIndex)
{
    // Bring the index down to the first octave
    const uint adjustedIndex = noteIndex % 12;

    return adjustedIndex == 1
        || adjustedIndex == 3
        || adjustedIndex == 6
        || adjustedIndex == 8
        || adjustedIndex == 10;
}

END_NAMESPACE_DISTRHO
