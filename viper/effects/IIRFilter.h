#pragma once

#include "../utils/MinPhaseIIRCoeffs.h"
#include <array>
#include <cstdint>

// Iscle: Verified with the latest version at 13/12/2022

class IIRFilter {
public:
    IIRFilter(uint32_t bands);

    void Process(float *samples, uint32_t size);
    void Reset();
    void SetBandCount(uint32_t bands);
    void SetBandLevel(uint32_t band, float level);
    void SetEnable(bool enable);
    void SetSamplingRate(uint32_t samplingRate);

private:
    uint32_t bands;
    uint32_t samplingRate;
    bool enable;
    MinPhaseIIRCoeffs minPhaseIirCoeffs;
    double buf[496];
    uint32_t bufIndex0;
    uint32_t bufIndex1;
    uint32_t bufIndex2;
    std::array<float, 31> bandLevelsWithQ;
};
