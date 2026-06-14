#pragma once

#include <cstdint>

typedef struct {
    float lower_angle;
    float upper_angle;

    float in[3];
    float x[4];
    float y[4];
} channel;

class PolesFilter {
public:
    PolesFilter();

    void FilterLeft(float sample, float *out1, float *out2, float *out3);
    void FilterRight(float sample, float *out1, float *out2, float *out3);
    void Reset();

    void SetPassFilter(uint32_t lower_freq, uint32_t upper_freq);
    void SetSamplingRate(uint32_t sampling_rate);

    void UpdateCoeff();

private:
    uint32_t lower_freq_;
    uint32_t upper_freq_;
    uint32_t sampling_rate_;

    channel channels_[2];
};
