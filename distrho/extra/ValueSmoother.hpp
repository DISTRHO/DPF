/*
 * DISTRHO Plugin Framework (DPF)
 * Copyright (C) 2021 Jean Pierre Cimalando <jp-dev@inbox.ru>
 * Copyright (C) 2021-2023 Filipe Coelho <falktx@falktx.com>
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

#ifndef DISTRHO_VALUE_SMOOTHER_HPP_INCLUDED
#define DISTRHO_VALUE_SMOOTHER_HPP_INCLUDED

#include "../DistrhoUtils.hpp"

START_NAMESPACE_DISTRHO

// --------------------------------------------------------------------------------------------------------------------

/**
 * @brief An exponential smoother for control values
 *
 * This continually smooths a value towards a defined target,
 * using a low-pass filter of the 1st order, which creates an exponential curve.
 *
 * The length of the curve is defined by a T60 constant,
 * which is the time it takes for a 1-to-0 smoothing to fall to -60dB.
 *
 * Note that this smoother has asymptotical behavior,
 * and it must not be assumed that the final target is ever reached.
 */
class ExponentialValueSmoother {
    float coef;
    float target;
    float mem;
    float tau;
    float sampleRate;

public:
    ExponentialValueSmoother()
        : coef(0.f),
          target(0.f),
          mem(0.f),
          tau(0.f),
          sampleRate(0.f) {}

    void setSampleRate(const float newSampleRate) noexcept
    {
        if (d_isNotEqual(sampleRate, newSampleRate))
        {
            sampleRate = newSampleRate;
            updateCoef();
        }
    }

    void setTimeConstant(const float newT60) noexcept
    {
        const float newTau = newT60 * (float)(1.0 / 6.91);

        if (d_isNotEqual(tau, newTau))
        {
            tau = newTau;
            updateCoef();
        }
    }

    float getCurrentValue() const noexcept
    {
        return mem;
    }

    float getTargetValue() const noexcept
    {
        return target;
    }

    void setTargetValue(const float newTarget) noexcept
    {
        target = newTarget;
    }

    void clearToTargetValue() noexcept
    {
        mem = target;
    }

    inline float peek() const noexcept
    {
        return mem * coef + target * (1.f - coef);
    }

    inline float next() noexcept
    {
        return (mem = mem * coef + target * (1.f - coef));
    }

private:
    void updateCoef() noexcept
    {
        coef = std::exp(-1.f / (tau * sampleRate));
    }
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * @brief A linear smoother for control values
 *
 * This continually smooths a value towards a defined target, using linear segments.
 *
 * The duration of the smoothing segment is defined by the given time constant.
 * Every time the target changes, a new segment restarts for the whole duration of the time constant.
 *
 * Note that this smoother, unlike an exponential smoother, eventually should converge to its target value.
 */
class LinearValueSmoother {
    float step;
    float target;
    float mem;
    float tau;
    float sampleRate;

public:
    LinearValueSmoother()
        : step(0.f),
          target(0.f),
          mem(0.f),
          tau(0.f),
          sampleRate(0.f) {}

    void setSampleRate(const float newSampleRate) noexcept
    {
        if (d_isNotEqual(sampleRate, newSampleRate))
        {
            sampleRate = newSampleRate;
            updateStep();
        }
    }

    void setTimeConstant(const float newTau) noexcept
    {
        if (d_isNotEqual(tau, newTau))
        {
            tau = newTau;
            updateStep();
        }
    }

    float getCurrentValue() const noexcept
    {
        return mem;
    }

    float getTargetValue() const noexcept
    {
        return target;
    }

    void setTargetValue(const float newTarget) noexcept
    {
        if (d_isNotEqual(target, newTarget))
        {
            target = newTarget;
            updateStep();
        }
    }

    void clearToTargetValue() noexcept
    {
        mem = target;
    }

    inline float peek() const noexcept
    {
        const float dy = target - mem;
        return mem + std::copysign(std::fmin(std::abs(dy), std::abs(step)), dy);
    }

    inline float next() noexcept
    {
        const float y0 = mem;
        const float dy = target - y0;
        return (mem = y0 + std::copysign(std::fmin(std::abs(dy), std::abs(step)), dy));
    }

private:
    void updateStep() noexcept
    {
        step = (target - mem) / (tau * sampleRate);
    }
};

// --------------------------------------------------------------------------------------------------------------------

END_NAMESPACE_DISTRHO

#endif // DISTRHO_VALUE_SMOOTHER_HPP_INCLUDED
