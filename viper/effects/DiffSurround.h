#pragma once

#include "../utils/MultiBiquad.h"
#include "../utils/WaveBuffer.h"
#include <array>

class DiffSurround {
public:
    DiffSurround();

    void Process(float *samples, uint32_t size);
    void Reset();

    void SetEnable(bool enable);
    void SetDelayTime(float value);
    void SetReverse(bool value);
    void SetWetDryMix(float value);
    void SetLPCutoff(float value);
    void SetSamplingRate(uint32_t sampling_rate);

private:
    bool enable_;
    bool reverse_;

    uint32_t sampling_rate_;

    float delay_time_;
    float wet_dry_mix_;
    float lp_cutoff_;

    std::array<WaveBuffer, 2> buffers_;
    MultiBiquad lp_filter_;
};
