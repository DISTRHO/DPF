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

#ifndef DGL_COMMON_HPP_INCLUDED
#define DGL_COMMON_HPP_INCLUDED

#include "../ImageBaseWidgets.hpp"

START_NAMESPACE_DGL

// -----------------------------------------------------------------------

template <class ImageType>
struct ImageBaseKnob<ImageType>::PrivateData {
    ImageType image;
    float minimum;
    float maximum;
    float step;
    float value;
    float valueDef;
    float valueTmp;
    bool usingDefault;
    bool usingLog;
    Orientation orientation;

    int rotationAngle;
    bool dragging;
    double lastX;
    double lastY;

    Callback* callback;

    bool alwaysRepaint;
    bool isImgVertical;
    uint imgLayerWidth;
    uint imgLayerHeight;
    uint imgLayerCount;
    bool isReady;

    union {
        uint glTextureId;
        void* cairoSurface;
    };

    explicit PrivateData(const ImageType& img, const Orientation o);
    explicit PrivateData(PrivateData* const other);
    void assignFrom(PrivateData* const other);

    ~PrivateData()
    {
        cleanup();
    }

    void init();
    void cleanup();

    inline float logscale(const float v) const
    {
        const float b = std::log(maximum/minimum)/(maximum-minimum);
        const float a = maximum/std::exp(maximum*b);
        return a * std::exp(b*v);
    }

    inline float invlogscale(const float v) const
    {
        const float b = std::log(maximum/minimum)/(maximum-minimum);
        const float a = maximum/std::exp(maximum*b);
        return std::log(v/a)/b;
    }

    DISTRHO_DECLARE_NON_COPYABLE(PrivateData)
};

// -----------------------------------------------------------------------

END_NAMESPACE_DGL

#endif // DGL_APP_PRIVATE_DATA_HPP_INCLUDED
