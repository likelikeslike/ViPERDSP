#pragma once

#include "../utils/MultiBiquad.h"
#include <array>
#include <vector>

class StereoImager {
public:
    static constexpr uint32_t kNumBands = 3;
    static constexpr uint32_t kNumCrossovers = 2;

    StereoImager();

    void Process(float *samples, uint32_t size);
    void Reset();

    void SetEnable(bool enable);
    void SetLowWidth(float value);
    void SetMidWidth(float value);
    void SetHighWidth(float value);
    void SetLowCrossover(float value);
    void SetHighCrossover(float value);
    void SetSamplingRate(uint32_t sampling_rate);

private:
    bool enable_;

    uint32_t sampling_rate_;

    std::array<float, kNumBands> band_widths_;
    std::array<float, kNumCrossovers> crossover_freqs_;

    std::array<MultiBiquad, kNumCrossovers> lowpass_la_;
    std::array<MultiBiquad, kNumCrossovers> lowpass_lb_;
    std::array<MultiBiquad, kNumCrossovers> lowpass_ra_;
    std::array<MultiBiquad, kNumCrossovers> lowpass_rb_;
    std::array<MultiBiquad, kNumCrossovers> highpass_la_;
    std::array<MultiBiquad, kNumCrossovers> highpass_lb_;
    std::array<MultiBiquad, kNumCrossovers> highpass_ra_;
    std::array<MultiBiquad, kNumCrossovers> highpass_rb_;

    std::array<std::vector<float>, kNumBands> band_buffers_;

    void ConfigureCrossovers();
};
