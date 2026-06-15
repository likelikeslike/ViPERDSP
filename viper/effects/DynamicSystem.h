#pragma once

#include "../utils/DynamicBass.h"
#include <cstdint>

class DynamicSystem {
public:
    DynamicSystem();

    void Process(float *samples, uint32_t size);
    void Reset();

    void SetEnable(bool enable);
    void SetBassGain(float gain);
    void SetSamplingRate(uint32_t sampling_rate);
    void SetSideGain(float gain_x, float gain_y);
    void SetXCoeffs(uint32_t low, uint32_t high);
    void SetYCoeffs(uint32_t low, uint32_t high);

private:
    bool enable_;

    uint32_t sampling_rate_;

    DynamicBass dynamic_bass_;
};
