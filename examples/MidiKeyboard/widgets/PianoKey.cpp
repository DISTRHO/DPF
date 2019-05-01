#include "PianoKey.hpp"

START_NAMESPACE_DISTRHO

PianoKey::PianoKey()
    : fPressed(false),
      fImageNormal(),
      fImageDown(),
      fIndex(-1)
{

}

void PianoKey::setImages(const Image& imageNormal, const Image& imageDown) noexcept
{
    fImageNormal = imageNormal;
    fImageDown = imageDown;

    fBoundingBox.setSize(imageNormal.getWidth(), imageNormal.getHeight());
}

void PianoKey::setPressed(bool pressed) noexcept
{
    fPressed = pressed;
}

bool PianoKey::isPressed() const noexcept
{
    return fPressed;
}

void PianoKey::setIndex(const int index) noexcept
{
    fIndex = index;
}

int PianoKey::getIndex() const noexcept
{
    return fIndex;
}

bool PianoKey::contains(Point<int> point) const noexcept
{
    return fBoundingBox.contains(point);
}

void PianoKey::setPosition(const int x, const int y) noexcept
{
    fBoundingBox.setPos(x, y);
}

uint PianoKey::getWidth() const noexcept
{
    return fBoundingBox.getWidth();
}

void PianoKey::draw() noexcept
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

END_NAMESPACE_DISTRHO
