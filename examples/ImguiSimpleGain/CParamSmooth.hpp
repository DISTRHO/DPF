/**
 * One-pole LPF for smooth parameter changes
 *
 * https://www.musicdsp.org/en/latest/Filters/257-1-pole-lpf-for-smooth-parameter-changes.html
 */

#ifndef C_PARAM_SMOOTH_H
#define C_PARAM_SMOOTH_H

#include <math.h>

#define TWO_PI 6.283185307179586476925286766559f

class CParamSmooth {
public:
    CParamSmooth(float smoothingTimeMs, float samplingRate)
        : t(smoothingTimeMs)
    {
        setSampleRate(samplingRate);
    }

    ~CParamSmooth() { }

    void setSampleRate(float samplingRate) {
        if (samplingRate != fs) {
            fs = samplingRate;
            a = exp(-TWO_PI / (t * 0.001f * samplingRate));
            b = 1.0f - a;
            z = 0.0f;
        }
    }

    inline float process(float in) {
        return z = (in * b) + (z * a);
    }
private:
    float a, b, t, z;
    double fs = 0.0;
};

#endif  // #ifndef C_PARAM_SMOOTH_H
