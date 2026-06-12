#pragma once

#include "../utils/Biquad.h"
#include "../utils/MultiBiquad.h"
#include <array>

class SpeakerCorrection {
public:
    SpeakerCorrection();

    void Process(float *samples, uint32_t size);
    void Reset();

    void SetEnable(bool enable);
    void SetHighPassCutoff(uint32_t value);
    void SetLowPassCutoff(uint32_t value);
    void SetBandPassCenter(uint32_t value);
    void SetBandPassQ(float value);
    void SetSamplingRate(uint32_t sampling_rate);

private:
    bool enable_;

    uint32_t sampling_rate_;
    uint32_t hp_cutoff_;
    uint32_t lp_cutoff_;
    uint32_t bp_center_;
    float bp_q_;

    std::array<MultiBiquad, 2> high_pass_;
    std::array<Biquad, 2> low_pass_;
    std::array<Biquad, 2> band_pass_;

    void RefreshHighPass();
    void RefreshLowPass();
    void RefreshBandPass();
};
