#pragma once

#include "Biquad.h"
#include "PolesFilter.h"
#include <cstdint>

class DynamicBass {
public:
    DynamicBass();

    void FilterSamples(float *samples, uint32_t size);
    void Reset();

    void SetBassGain(float value);
    void SetFilterXPassFrequency(uint32_t low, uint32_t high);
    void SetFilterYPassFrequency(uint32_t low, uint32_t high);
    void SetSideGain(float gain_x, float gain_y);
    void SetSamplingRate(uint32_t sampling_rate);

private:
    uint32_t low_freq_x_;
    uint32_t high_freq_x_;
    uint32_t low_freq_y_;
    uint32_t high_freq_y_;
    uint32_t sampling_rate_;

    float q_peak_;
    float bass_gain_;
    float side_gain_x_;
    float side_gain_y_;

    PolesFilter filter_x_;
    PolesFilter filter_y_;
    Biquad low_pass_;
};
