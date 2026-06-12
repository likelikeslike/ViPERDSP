#pragma once

#include "../utils/Harmonic.h"
#include "../utils/MultiBiquad.h"
#include <array>

class AnalogX {
public:
    AnalogX();

    void Process(float *samples, uint32_t size);
    void Reset();

    void SetEnable(bool enable);
    void SetProcessingModel(int model);
    void SetSamplingRate(uint32_t sampling_rate);

private:
    bool enable_;

    int processing_model_;
    uint32_t sampling_rate_;
    uint32_t freq_range_;

    float gain_;

    std::array<MultiBiquad, 2> high_pass_;
    std::array<Harmonic, 2> harmonic_;
    std::array<MultiBiquad, 2> low_pass_;
    std::array<MultiBiquad, 2> peak_;
};
