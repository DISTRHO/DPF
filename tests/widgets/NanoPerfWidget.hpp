/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2012-2015 Filipe Coelho <falktx@falktx.com>
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

#ifndef NANO_PERF_WIDGET_HPP_INCLUDED
#define NANO_PERF_WIDGET_HPP_INCLUDED

// ------------------------------------------------------
// DGL Stuff

#include "NanoVG.hpp"
#include "../../dpf/distrho/extra/String.hpp"

// ------------------------------------------------------
// get time

#include <sys/time.h>
#include <time.h>

#ifdef DISTRHO_OS_WINDOWS
#else
struct TimePOSIX {
    bool monotonic;
    double resolution;
    uint64_t base;

    TimePOSIX()
        : monotonic(false),
          resolution(1e-6),
          base(0)
    {
#if defined(CLOCK_MONOTONIC)
        struct timespec ts;

        if (::clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
        {
            monotonic = true;
            resolution = 1e-9;
        }
#endif

        base = getRawTime();
    }

    uint64_t getRawTime()
    {
#if defined(CLOCK_MONOTONIC)
        if (monotonic)
        {
            struct timespec ts;

            ::clock_gettime(CLOCK_MONOTONIC, &ts);
            return (uint64_t) ts.tv_sec * (uint64_t) 1000000000 + (uint64_t) ts.tv_nsec;
        }
        else
#endif
        {
            struct timeval tv;

            ::gettimeofday(&tv, NULL);
            return (uint64_t) tv.tv_sec * (uint64_t) 1000000 + (uint64_t) tv.tv_usec;
        }
    }

    double getTime()
    {
        return (double)(getRawTime() - base) * resolution;
    }
};

static TimePOSIX gTime;
#endif

// ------------------------------------------------------
// our widget

class NanoPerfWidget : public NanoWidget,
                       public IdleCallback
{
public:
    static const int kHistoryCount = 100;

    enum RenderStyle {
        RENDER_FPS,
        RENDER_MS,
    };

    NanoPerfWidget(Window& parent, RenderStyle style, const char* name)
        : NanoWidget(parent),
          fHead(0),
          fStyle(style),
          fName(name)
    {
        parent.addIdleCallback(this);
        setSize(200, 35);

        std::memset(fValues, 0, sizeof(float)*kHistoryCount);

        createFontFromFile("sans", "./nanovg_res/Roboto-Regular.ttf");

        prevt = gTime.getTime();
    }

protected:
    void idleCallback() override
    {
        repaint();
    }

    void onNanoDisplay() override
    {
        double t, dt;

        t = gTime.getTime();
        dt = t - prevt;
        prevt = t;
        update(dt);

        const int w = 200; //getWidth();
        const int h = 35; //getHeight();

        int i;
        float avg;
        char str[64];

        avg = getAverage();

        beginPath();
        rect(0, 0, w, h);
        fillColor(0,0,0,128);
        fill();

        beginPath();
        moveTo(0, h);

        if (fStyle == RENDER_FPS)
        {
            for (i = 0; i < kHistoryCount; ++i)
            {
                float v = 1.0f / (0.00001f + fValues[(fHead+i) % kHistoryCount]);
                float vx, vy;
                if (v > 80.0f) v = 80.0f;
                vx = ((float)i/(kHistoryCount-1)) * w;
                vy = h - ((v / 80.0f) * h);
                lineTo(vx, vy);
            }
        }
        else
        {
            for (i = 0; i < kHistoryCount; ++i)
            {
                float v = fValues[(fHead+i) % kHistoryCount] * 1000.0f;
                float vx, vy;
                if (v > 20.0f) v = 20.0f;
                vx = ((float)i/(kHistoryCount-1)) * w;
                vy = h - ((v / 20.0f) * h);
                lineTo(vx, vy);
            }
        }

        lineTo(w, h);
        fillColor(255,192,0,128);
        fill();

        fontFace("sans");

        if (fName.isNotEmpty())
        {
            fontSize(14.0f);
            textAlign(ALIGN_LEFT|ALIGN_TOP);
            fillColor(240,240,240,192);
            text(3, 1, fName, nullptr);
        }

        if (fStyle == RENDER_FPS)
        {
            fontSize(18.0f);
            textAlign(ALIGN_RIGHT|ALIGN_TOP);
            fillColor(240,240,240,255);
            std::sprintf(str, "%.2f FPS", 1.0f / avg);
            text(w-3, 1, str, nullptr);

            fontSize(15.0f);
            textAlign(ALIGN_RIGHT|ALIGN_BOTTOM);
            fillColor(240,240,240,160);
            std::sprintf(str, "%.2f ms", avg * 1000.0f);
            text(w-3, h-1, str, nullptr);
        }
        else
        {
            fontSize(18.0f);
            textAlign(ALIGN_RIGHT|ALIGN_TOP);
            fillColor(240,240,240,255);
            std::sprintf(str, "%.2f ms", avg * 1000.0f);
            text(w-3, 1, str, nullptr);
        }
    }

private:
    int fHead;
    float fValues[kHistoryCount];

    const int    fStyle;
    const String fName;

    double prevt;

    void update(float frameTime) noexcept
    {
        fHead = (fHead+1) % kHistoryCount;
        fValues[fHead] = frameTime;
    }

    float getAverage() const noexcept
    {
        int i;
        float avg = 0;

        for (i = 0; i < kHistoryCount; ++i)
            avg += fValues[i];

        return avg / (float)kHistoryCount;
    }
};

// ------------------------------------------------------

#endif // NANO_PERF_WIDGET_HPP_INCLUDED
