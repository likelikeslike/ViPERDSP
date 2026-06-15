#pragma once

#include "../utils/Biquad.h"
#include <cstdint>

class LUFSTargeting {
public:
    LUFSTargeting();

    void Process(float *samples, uint32_t size);
    void Reset();

    void SetEnable(bool enable);
    void SetTargetLUFS(float value);
    void SetMaxGain(float value);
    void SetSpeed(int value);
    void SetSamplingRate(uint32_t sampling_rate);

private:
    static constexpr uint32_t kMaxWindows = 40;
    static constexpr double kAbsoluteGateLufs = -70.0;

    bool enable_;

    int speed_;
    uint32_t sampling_rate_;
    uint32_t window_size_;
    uint32_t step_size_;
    uint32_t sample_counter_;
    uint32_t window_sample_count_;
    uint32_t window_write_idx_;
    uint32_t window_count_;

    float target_lufs_;
    float max_gain_db_;

    double window_accumulator_;
    double window_power_[kMaxWindows];
    double smoothed_gain_db_;
    double attack_coeff_;
    double release_coeff_;

    Biquad k_weight_stage1_l_;
    Biquad k_weight_stage1_r_;
    Biquad k_weight_stage2_l_;
    Biquad k_weight_stage2_r_;

    void ConfigureFilters();
    void UpdateSmoothingCoeffs();
};
