#include "LUFSTargeting.h"
#include "../constants.h"
#include <cmath>
#include <cstring>

LUFSTargeting::LUFSTargeting() :
    enable_(false),
    speed_(1),
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE),
    window_size_(0),
    step_size_(0),
    sample_counter_(0),
    window_sample_count_(0),
    window_write_idx_(0),
    window_count_(0),
    target_lufs_(-14.0f),
    max_gain_db_(6.0f),
    window_accumulator_(0.0),
    smoothed_gain_db_(0.0),
    attack_coeff_(0.0),
    release_coeff_(0.0) {
    memset(window_power_, 0, sizeof(window_power_));
    ConfigureFilters();
    UpdateSmoothingCoeffs();
    window_size_ = static_cast<uint32_t>(sampling_rate_ * 0.4);
    step_size_ = window_size_ / 4;
}

void LUFSTargeting::Process(float *samples, const uint32_t size) {
    if (!enable_) return;
    if (size == 0) return;

    const double gate_threshold = pow(10.0, (kAbsoluteGateLufs + 0.691) / 10.0);

    for (uint32_t i = 0; i < size; i++) {
        const double left = samples[i * 2];
        const double right = samples[i * 2 + 1];

        double kLeft = k_weight_stage1_l_.ProcessSample(left);
        kLeft = k_weight_stage2_l_.ProcessSample(kLeft);

        double kRight = k_weight_stage1_r_.ProcessSample(right);
        kRight = k_weight_stage2_r_.ProcessSample(kRight);

        window_accumulator_ += kLeft * kLeft + kRight * kRight;
        window_sample_count_++;
        sample_counter_++;

        if (sample_counter_ >= step_size_) {
            sample_counter_ = 0;

            if (window_sample_count_ >= window_size_) {
                const double mean_square =
                    window_accumulator_ / static_cast<double>(window_sample_count_);

                if (mean_square > gate_threshold) {
                    window_power_[window_write_idx_] = mean_square;
                    window_write_idx_ = (window_write_idx_ + 1) % kMaxWindows;
                    if (window_count_ < kMaxWindows) {
                        window_count_++;
                    }
                }

                const uint32_t shift_samples = window_size_ - step_size_;
                const double ratio = static_cast<double>(shift_samples)
                                     / static_cast<double>(window_sample_count_);
                window_accumulator_ *= ratio;
                window_sample_count_ = shift_samples;
            }
        }

        double measured_lufs = -70.0;
        if (window_count_ > 0) {
            double sum = 0.0;
            for (uint32_t w = 0; w < window_count_; w++) {
                sum += window_power_[w];
            }
            const double gated_mean = sum / static_cast<double>(window_count_);
            if (gated_mean > 1e-20) {
                measured_lufs = -0.691 + 10.0 * log10(gated_mean);
            }
        }

        double desired_gain_db = target_lufs_ - measured_lufs;

        if (desired_gain_db > max_gain_db_) {
            desired_gain_db = max_gain_db_;
        } else if (desired_gain_db < -max_gain_db_) {
            desired_gain_db = -max_gain_db_;
        }

        double coeff;
        if (desired_gain_db > smoothed_gain_db_) {
            coeff = attack_coeff_;
        } else {
            coeff = release_coeff_;
        }
        smoothed_gain_db_ += coeff * (desired_gain_db - smoothed_gain_db_);

        const double gain_linear = pow(10.0, smoothed_gain_db_ / 20.0);

        samples[i * 2] *= static_cast<float>(gain_linear);
        samples[i * 2 + 1] *= static_cast<float>(gain_linear);
    }
}

void LUFSTargeting::Reset() {
    k_weight_stage1_l_.Reset();
    k_weight_stage1_r_.Reset();
    k_weight_stage2_l_.Reset();
    k_weight_stage2_r_.Reset();
    ConfigureFilters();
    UpdateSmoothingCoeffs();
    smoothed_gain_db_ = 0.0;
    sample_counter_ = 0;
    window_accumulator_ = 0.0;
    window_sample_count_ = 0;
    window_write_idx_ = 0;
    window_count_ = 0;
    memset(window_power_, 0, sizeof(window_power_));
    window_size_ = static_cast<uint32_t>(sampling_rate_ * 0.4);
    step_size_ = window_size_ / 4;
}

void LUFSTargeting::SetEnable(const bool enable) {
    if (enable_ != enable) {
        if (enable) {
            Reset();
        }
        enable_ = enable;
    }
}

void LUFSTargeting::SetTargetLUFS(const float value) {
    target_lufs_ = value;
}

void LUFSTargeting::SetMaxGain(const float value) {
    max_gain_db_ = value;
}

void LUFSTargeting::SetSpeed(const int value) {
    if (value < 0) speed_ = 0;
    if (value > 2) speed_ = 2;
    UpdateSmoothingCoeffs();
}

void LUFSTargeting::SetSamplingRate(const uint32_t sampling_rate) {
    if (sampling_rate_ != sampling_rate) {
        sampling_rate_ = sampling_rate;
        Reset();
    }
}

void LUFSTargeting::ConfigureFilters() {
    if (sampling_rate_ == 48000) {
        k_weight_stage1_l_.SetCoeffs(
            1.0,
            -1.69065929318241,
            0.73248077421585,
            1.53512485958697,
            -2.69169618940638,
            1.19839281085285
        );
        k_weight_stage1_r_.SetCoeffs(
            1.0,
            -1.69065929318241,
            0.73248077421585,
            1.53512485958697,
            -2.69169618940638,
            1.19839281085285
        );
        k_weight_stage2_l_.SetCoeffs(
            1.0, -1.99004745483398, 0.99007225036621, 1.0, -2.0, 1.0
        );
        k_weight_stage2_r_.SetCoeffs(
            1.0, -1.99004745483398, 0.99007225036621, 1.0, -2.0, 1.0
        );
    } else {
        k_weight_stage1_l_.SetCoeffs(
            1.0,
            -1.6636551132560204,
            0.7125954280732254,
            1.5308412300503478,
            -2.6509799951547297,
            1.1690790799215869
        );
        k_weight_stage1_r_.SetCoeffs(
            1.0,
            -1.6636551132560204,
            0.7125954280732254,
            1.5308412300503478,
            -2.6509799951547297,
            1.1690790799215869
        );
        k_weight_stage2_l_.SetCoeffs(
            1.0, -1.9891696736297957, 0.9891990357870394, 1.0, -2.0, 1.0
        );
        k_weight_stage2_r_.SetCoeffs(
            1.0, -1.9891696736297957, 0.9891990357870394, 1.0, -2.0, 1.0
        );
    }
}

void LUFSTargeting::UpdateSmoothingCoeffs() {
    double attack_ms, release_ms;

    switch (speed_) {
        case 0:
            attack_ms = 200.0;
            release_ms = 1000.0;
            break;
        case 2:
            attack_ms = 50.0;
            release_ms = 200.0;
            break;
        default:
            attack_ms = 100.0;
            release_ms = 500.0;
            break;
    }

    const double attack_samples =
        static_cast<double>(sampling_rate_) * attack_ms / 1000.0;
    const double release_samples =
        static_cast<double>(sampling_rate_) * release_ms / 1000.0;

    attack_coeff_ = 1.0 - exp(-1.0 / attack_samples);
    release_coeff_ = 1.0 - exp(-1.0 / release_samples);
}
