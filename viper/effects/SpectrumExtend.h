#pragma once

#include "../utils/Harmonic.h"
#include "../utils/MultiBiquad.h"
#include <array>

class SpectrumExtend {
public:
    SpectrumExtend();

    void Process(float *samples, uint32_t size);
    void Reset();

    void SetEnable(bool enable);
    void SetExciter(float value);
    void SetReferenceFrequency(uint32_t value);
    void SetSamplingRate(uint32_t sampling_rate);

private:
    bool enabled_;

    uint32_t sampling_rate_;
    uint32_t reference_freq_;

    float exciter_;

    std::array<MultiBiquad, 2> highpass_;
    std::array<MultiBiquad, 2> lowpass_;
    std::array<Harmonic, 2> harmonics_;
};
