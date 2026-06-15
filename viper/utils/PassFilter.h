#pragma once

#include "IIR_NOrder_BW_LH.h"
#include <array>

class PassFilter {
public:
    PassFilter();

    void ProcessFrames(float *buffer, uint32_t size);
    void Reset();

    void SetSamplingRate(uint32_t sampling_rate);

private:
    uint32_t sampling_rate_;

    std::array<IIR_NOrder_BW_LH, 4> filters_;
};
