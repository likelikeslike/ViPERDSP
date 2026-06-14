#pragma once

#include <cstdint>

class IIR_1st {
public:
    IIR_1st();

    void Mute();

    void SetCoefficients(float b0, float b1, float a1);
    void SetHighPassFilterBW(float frequency, uint32_t sampling_rate);
    void SetLowPassFilterBW(float frequency, uint32_t sampling_rate);

    float b0_;
    float b1_;
    float a1_;
    float prev_sample_;
};

inline float Filter(IIR_1st *filter, float sample) {
    const float hist = sample * filter->b1_;
    sample = filter->prev_sample_ + sample * filter->b0_;
    filter->prev_sample_ = sample * filter->a1_ + hist;
    return sample;
}
