#pragma once

#include "MultiBiquad.h"
#include <cstdint>

class Subwoofer {
public:
    Subwoofer();

    void Process(float *samples, uint32_t size);

    void SetBassGain(uint32_t sampling_rate, float gain_db);

private:
    uint32_t sampling_rate_;

    float gain_;
    float gain_lower_;

    MultiBiquad peak_[2];
    MultiBiquad peak_low_[2];
    MultiBiquad lowpass_[2];
};
