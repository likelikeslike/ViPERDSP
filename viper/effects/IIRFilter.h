#pragma once

#include "../utils/MinPhaseIIRCoeffs.h"
#include <array>

class IIRFilter {
public:
    explicit IIRFilter(uint32_t bands);

    void Process(float *samples, uint32_t size);
    void Reset();

    void SetEnable(bool enable);
    void SetBandCount(uint32_t bands);
    void SetBandLevel(uint32_t band, float level);
    void SetSamplingRate(uint32_t sampling_rate);

private:
    bool enable_;

    uint32_t bands_;
    uint32_t sampling_rate_;
    uint32_t buf_index0_;
    uint32_t buf_index1_;
    uint32_t buf_index2_;

    double buf_[496];
    std::array<float, 31> band_levels_with_q_;
    MinPhaseIIRCoeffs min_phase_iir_coeffs_;
};
