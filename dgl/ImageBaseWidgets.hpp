/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2021 Filipe Coelho <falktx@falktx.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any purpose with
 * or without fee is hereby granted, provided that the above copyright notice and this
 * permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
 * TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef DGL_IMAGE_BASE_WIDGETS_HPP_INCLUDED
#define DGL_IMAGE_BASE_WIDGETS_HPP_INCLUDED

#include "EventHandlers.hpp"
#include "StandaloneWindow.hpp"
#include "SubWidget.hpp"

START_NAMESPACE_DGL

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
class ImageBaseAboutWindow : public StandaloneWindow
{
public:
    explicit ImageBaseAboutWindow(Window& parentWindow, const ImageType& image = ImageType());
    explicit ImageBaseAboutWindow(TopLevelWidget* parentTopLevelWidget, const ImageType& image = ImageType());

    void setImage(const ImageType& image);

protected:
    void onDisplay() override;
    bool onKeyboard(const KeyboardEvent&) override;
    bool onMouse(const MouseEvent&) override;

private:
    ImageType img;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImageBaseAboutWindow)
};

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
class ImageBaseButton : public SubWidget,
                        public ButtonEventHandler
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void imageButtonClicked(ImageBaseButton* imageButton, int button) = 0;
    };

    explicit ImageBaseButton(Widget* parentWidget, const ImageType& image);
    explicit ImageBaseButton(Widget* parentWidget, const ImageType& imageNormal, const ImageType& imageDown);
    explicit ImageBaseButton(Widget* parentWidget, const ImageType& imageNormal, const ImageType& imageHover, const ImageType& imageDown);

    ~ImageBaseButton() override;

    void setCallback(Callback* callback) noexcept;

protected:
     void onDisplay() override;
     bool onMouse(const MouseEvent&) override;
     bool onMotion(const MotionEvent&) override;

private:
    struct PrivateData;
    PrivateData* const pData;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImageBaseButton)
};

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
class ImageBaseKnob : public SubWidget
{
public:
    enum Orientation {
        Horizontal,
        Vertical
    };

    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void imageKnobDragStarted(ImageBaseKnob* imageKnob) = 0;
        virtual void imageKnobDragFinished(ImageBaseKnob* imageKnob) = 0;
        virtual void imageKnobValueChanged(ImageBaseKnob* imageKnob, float value) = 0;
    };

    explicit ImageBaseKnob(Widget* parentWidget, const ImageType& image, Orientation orientation = Vertical) noexcept;
    explicit ImageBaseKnob(const ImageBaseKnob& imageKnob);
    ImageBaseKnob& operator=(const ImageBaseKnob& imageKnob);
    ~ImageBaseKnob() override;

    float getValue() const noexcept;

    void setDefault(float def) noexcept;
    void setRange(float min, float max) noexcept;
    void setStep(float step) noexcept;
    void setValue(float value, bool sendCallback = false) noexcept;
    void setUsingLogScale(bool yesNo) noexcept;

    void setCallback(Callback* callback) noexcept;
    void setOrientation(Orientation orientation) noexcept;
    void setRotationAngle(int angle);

    void setImageLayerCount(uint count) noexcept;

protected:
     void onDisplay() override;
     bool onMouse(const MouseEvent&) override;
     bool onMotion(const MotionEvent&) override;
     bool onScroll(const ScrollEvent&) override;

private:
    struct PrivateData;
    PrivateData* const pData;

    DISTRHO_LEAK_DETECTOR(ImageBaseKnob)
};

// --------------------------------------------------------------------------------------------------------------------

// note set range and step before setting the value

template <class ImageType>
class ImageBaseSlider : public SubWidget
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void imageSliderDragStarted(ImageBaseSlider* imageSlider) = 0;
        virtual void imageSliderDragFinished(ImageBaseSlider* imageSlider) = 0;
        virtual void imageSliderValueChanged(ImageBaseSlider* imageSlider, float value) = 0;
    };

    explicit ImageBaseSlider(Widget* parentWidget, const ImageType& image) noexcept;
    ~ImageBaseSlider() override;

    float getValue() const noexcept;
    void setValue(float value, bool sendCallback = false) noexcept;
    void setDefault(float def) noexcept;

    void setStartPos(const Point<int>& startPos) noexcept;
    void setStartPos(int x, int y) noexcept;
    void setEndPos(const Point<int>& endPos) noexcept;
    void setEndPos(int x, int y) noexcept;

    void setInverted(bool inverted) noexcept;
    void setRange(float min, float max) noexcept;
    void setStep(float step) noexcept;

    void setCallback(Callback* callback) noexcept;

protected:
     void onDisplay() override;
     bool onMouse(const MouseEvent&) override;
     bool onMotion(const MotionEvent&) override;

private:
    struct PrivateData;
    PrivateData* const pData;

    // these should not be used
    void setAbsoluteX(int) const noexcept {}
    void setAbsoluteY(int) const noexcept {}
    void setAbsolutePos(int, int) const noexcept {}
    void setAbsolutePos(const Point<int>&) const noexcept {}

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ImageBaseSlider)
};

// --------------------------------------------------------------------------------------------------------------------

template <class ImageType>
class ImageBaseSwitch : public SubWidget
{
public:
    class Callback
    {
    public:
        virtual ~Callback() {}
        virtual void imageSwitchClicked(ImageBaseSwitch* imageSwitch, bool down) = 0;
    };

    explicit ImageBaseSwitch(Widget* parentWidget, const ImageType& imageNormal, const ImageType& imageDown) noexcept;
    explicit ImageBaseSwitch(const ImageBaseSwitch& imageSwitch) noexcept;
    ImageBaseSwitch& operator=(const ImageBaseSwitch& imageSwitch) noexcept;
    ~ImageBaseSwitch() override;

    bool isDown() const noexcept;
    void setDown(bool down) noexcept;

    void setCallback(Callback* callback) noexcept;

protected:
     void onDisplay() override;
     bool onMouse(const MouseEvent&) override;

private:
    struct PrivateData;
    PrivateData* const pData;

    DISTRHO_LEAK_DETECTOR(ImageBaseSwitch)
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_IMAGE_BASE_WIDGETS_HPP_INCLUDED
