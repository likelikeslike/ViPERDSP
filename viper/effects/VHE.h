#pragma once

#include <cstdint>

#include "../utils/PConvSingle.h"

class VHE {
public:
    VHE();

    uint32_t Process(const float *source, float *dest, uint32_t frame_size);
    void Reset();

    [[nodiscard]] bool GetEnable() const;

    void SetEnable(bool enable);
    void SetEffectLevel(uint32_t value);
    void SetSamplingRate(uint32_t sampling_rate);

private:
    bool enable_;

    uint32_t sampling_rate_;
    uint32_t effect_level_;
    uint32_t conv_size_;

    PConvSingle conv_left_;
    PConvSingle conv_right_;
};
