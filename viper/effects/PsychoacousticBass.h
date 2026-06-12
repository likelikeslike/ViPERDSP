#pragma once

#include "../utils/Harmonic.h"
#include "../utils/MultiBiquad.h"
#include <array>

class PsychoacousticBass {
public:
    PsychoacousticBass();

    void Process(float *samples, uint32_t size);
    void Reset();

    void SetEnable(bool enable);
    void SetCutoff(uint32_t value);
    void SetIntensity(uint32_t value);
    void SetHarmonicOrder(uint32_t value);
    void SetOriginalBassLevel(uint32_t value);
    void SetSamplingRate(uint32_t sampling_rate);

private:
    bool enabled_;

    uint32_t sampling_rate_;
    uint32_t cutoff_;
    uint32_t harmonic_order_;

    float intensity_;
    float original_bass_level_;

    double envelope_;

    std::array<MultiBiquad, 2> lowpass_;
    std::array<MultiBiquad, 2> highpass_;
    std::array<Harmonic, 2> harmonics_;

    void RefreshFilters();
    void ApplyHarmonicCoeffs();
};
