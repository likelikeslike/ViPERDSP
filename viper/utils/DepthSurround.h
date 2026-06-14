#pragma once

#include "Biquad.h"
#include "TimeConstDelay.h"

class DepthSurround {
public:
    DepthSurround();

    void Process(float *samples, uint32_t size);

    void SetStrength(uint32_t value);
    void SetSamplingRate(uint32_t sampling_rate);

    void RefreshStrength(uint32_t strength);

private:
    bool enable_;
    bool strength_at_least500_;

    uint32_t strength_;

    float gain_;
    float prev_[2] = {};

    TimeConstDelay time_const_delay_[2];
    Biquad highpass_;
};
