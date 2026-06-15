#pragma once

#include "IIR_1st.h"

class NoiseSharpening {
public:
    NoiseSharpening();

    void Process(float *buffer, uint32_t size);
    void Reset();
    void SetGain(float gain);
    void SetSamplingRate(uint32_t sampling_rate);

private:
    uint32_t sampling_rate_;

    float gain_;
    float in_[2];

    IIR_1st filters_[2];
};
