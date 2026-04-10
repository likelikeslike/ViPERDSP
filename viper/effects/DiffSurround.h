#pragma once

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

    uint32_t samplingRate;
    bool enable;
    bool reverse;
    float delayTime;
    std::array<WaveBuffer, 2> buffers;
};
