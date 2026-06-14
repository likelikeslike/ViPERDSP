#pragma once

#include <vector>

class TimeConstDelay {
public:
    TimeConstDelay();

    float ProcessSample(float sample);

    void SetParameters(uint32_t sampling_rate, float delay);

private:
    uint32_t offset_;
    uint32_t sample_count_;

    std::vector<float> samples_;
};
