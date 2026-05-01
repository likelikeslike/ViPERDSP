#pragma once

#include "../utils/MultiBiquad.h"
#include "../utils/WaveBuffer.h"
#include <array>
#include <cstdint>

class DiffSurround {
public:
    DiffSurround();

    void Process(float *samples, uint32_t size);
    void Reset();
    void SetDelayTime(float delayTime);
    void SetEnable(bool enable);
    void SetReverse(bool reverse);
    void SetSamplingRate(uint32_t samplingRate);
    void SetWetDryMix(float mix);
    void SetLPCutoff(float cutoff);

    uint32_t samplingRate;
    bool enable;
    bool reverse;
    float delayTime;
    float wetDryMix;
    float lpCutoff;
    std::array<WaveBuffer, 2> buffers;
    MultiBiquad lpFilter;
};
