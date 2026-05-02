#pragma once

#include "../utils/Harmonic.h"
#include "../utils/MultiBiquad.h"
#include <array>
#include <cstdint>

class PsychoacousticBass {
public:
    PsychoacousticBass();

    void Process(float *samples, uint32_t size);
    void Reset();
    void SetEnable(bool enable);
    void SetSamplingRate(uint32_t samplingRate);
    void SetCutoff(uint32_t cutoff);
    void SetIntensity(uint32_t intensity);
    void SetHarmonicOrder(uint32_t order);
    void SetOriginalBassLevel(uint32_t level);

private:
    void RefreshFilters();
    void ApplyHarmonicCoeffs();

    std::array<MultiBiquad, 2> lowpass;
    std::array<MultiBiquad, 2> highpass;
    std::array<Harmonic, 2> harmonics;
    bool enabled;
    uint32_t samplingRate;
    uint32_t cutoff;
    uint32_t harmonicOrder;
    float intensity;
    float originalBassLevel;
    double envelope;
};
