#include "DynamicEQ.h"
#include "../constants.h"
#include <algorithm>
#include <cmath>

static constexpr float kGainChangeThreshold = 0.1f;
static constexpr float kOvershootRangeDb = 12.0f;
static constexpr float kMinEnvelope = 1e-20f;

DynamicEQ::DynamicEQ() :
    enable_(false),
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE),
    band_count_(0) {
    for (uint32_t i = 0; i < kMaxBands; i++) {
        params_[i].frequency = 1000.0f;
        params_[i].q = 1.0f;
        params_[i].target_gain_db = 0.0f;
        params_[i].threshold_db = -24.0f;
        params_[i].attack_ms = 10.0f;
        params_[i].release_ms = 100.0f;
        params_[i].filter_type = MultiBiquad::PEAK;

        state_[i].envelope_l = 0.0;
        state_[i].envelope_r = 0.0;
        state_[i].smoothed_gain_db = 0.0;
        state_[i].last_applied_gain_db = 0.0f;
        state_[i].attack_coeff = 0.0;
        state_[i].release_coeff = 0.0;
    }

    Reset();
}

void DynamicEQ::Process(float *samples, const uint32_t size) {
    if (!enable_) return;
    if (band_count_ == 0) return;
    if (size == 0) return;

    const uint32_t frame_count = size * 2;

    for (uint32_t b = 0; b < band_count_; b++) {
        double attack_coeff = state_[b].attack_coeff;
        double release_coeff = state_[b].release_coeff;
        double env_l = state_[b].envelope_l;
        double env_r = state_[b].envelope_r;
        double smoothed_gain = state_[b].smoothed_gain_db;
        float last_applied = state_[b].last_applied_gain_db;
        const float target_gain = params_[b].target_gain_db;
        const float threshold = params_[b].threshold_db;

        for (uint32_t i = 0; i < frame_count; i += 2) {
            const auto sample_l = static_cast<double>(samples[i]);
            const auto sample_r = static_cast<double>(samples[i + 1]);

            const double power_l = sample_l * sample_l;
            const double power_r = sample_r * sample_r;

            const double smooth_coeff_l = power_l > env_l ? attack_coeff : release_coeff;
            env_l += smooth_coeff_l * (power_l - env_l);

            const double smooth_coeff_r = power_r > env_r ? attack_coeff : release_coeff;
            env_r += smooth_coeff_r * (power_r - env_r);

            double rms_linear = sqrt(std::max(env_l, env_r));
            if (rms_linear < kMinEnvelope) rms_linear = kMinEnvelope;

            const double envelope_db = 20.0 * log10(rms_linear);

            const double overshoot = envelope_db - static_cast<double>(threshold);
            double desired_gain_db = 0.0;
            if (overshoot > 0.0) {
                double ratio = overshoot / static_cast<double>(kOvershootRangeDb);
                if (ratio > 1.0) ratio = 1.0;
                desired_gain_db = static_cast<double>(target_gain) * ratio;
            }

            const double gain_coeff = fabs(desired_gain_db) > fabs(smoothed_gain)
                                          ? attack_coeff
                                          : release_coeff;
            smoothed_gain += gain_coeff * (desired_gain_db - smoothed_gain);

            const auto current_gain_db = static_cast<float>(smoothed_gain);
            if (fabs(current_gain_db - last_applied) > kGainChangeThreshold) {
                ConfigureApplicationFilter(b, current_gain_db);
                last_applied = current_gain_db;
            }

            samples[i] = static_cast<float>(apply_l_[b].ProcessSample(sample_l));
            samples[i + 1] = static_cast<float>(apply_r_[b].ProcessSample(sample_r));
        }

        state_[b].envelope_l = env_l;
        state_[b].envelope_r = env_r;
        state_[b].smoothed_gain_db = smoothed_gain;
        state_[b].last_applied_gain_db = last_applied;
    }
}

void DynamicEQ::Reset() {
    for (uint32_t i = 0; i < kMaxBands; i++) {
        state_[i].envelope_l = 0.0;
        state_[i].envelope_r = 0.0;
        state_[i].smoothed_gain_db = 0.0;
        state_[i].last_applied_gain_db = 0.0f;

        apply_l_[i].Reset();
        apply_r_[i].Reset();

        RecalcAttackRelease(i);
        ConfigureApplicationFilter(i, 0.0f);
    }
}

void DynamicEQ::SetEnable(const bool enable) {
    if (enable_ != enable) {
        if (enable) Reset();
        enable_ = enable;
    }
}

void DynamicEQ::SetBandCount(uint32_t count) {
    if (count > kMaxBands) count = kMaxBands;
    if (band_count_ != count) {
        band_count_ = count;
        Reset();
    }
}

void DynamicEQ::SetSamplingRate(const uint32_t sampling_rate) {
    if (sampling_rate_ != sampling_rate) {
        sampling_rate_ = sampling_rate;
        Reset();
    }
}

void DynamicEQ::SetBandFrequency(const uint32_t band, const float value) {
    params_[band].frequency = value;
    ConfigureApplicationFilter(band, 0.0f);
    state_[band].last_applied_gain_db = 0.0f;
}

void DynamicEQ::SetBandQ(const uint32_t band, const float value) {
    params_[band].q = value;
    ConfigureApplicationFilter(band, 0.0f);
    state_[band].last_applied_gain_db = 0.0f;
}

void DynamicEQ::SetBandGain(const uint32_t band, const float value) {
    params_[band].target_gain_db = value;
}

void DynamicEQ::SetBandThreshold(const uint32_t band, const float value) {
    params_[band].threshold_db = value;
}

void DynamicEQ::SetBandAttack(const uint32_t band, const float value) {
    params_[band].attack_ms = value;
    RecalcAttackRelease(band);
}

void DynamicEQ::SetBandRelease(const uint32_t band, const float value) {
    params_[band].release_ms = value;
    RecalcAttackRelease(band);
}

void DynamicEQ::SetBandFilterType(const uint32_t band, const int value) {
    MultiBiquad::FilterType resolved;
    switch (value) {
        case 0:
        case MultiBiquad::PEAK:
            resolved = MultiBiquad::PEAK;
            break;
        case 1:
        case MultiBiquad::LOW_SHELF:
            resolved = MultiBiquad::LOW_SHELF;
            break;
        case 2:
        case MultiBiquad::HIGH_SHELF:
            resolved = MultiBiquad::HIGH_SHELF;
            break;
        default:
            resolved = MultiBiquad::PEAK;
            break;
    }
    params_[band].filter_type = resolved;
    ConfigureApplicationFilter(band, 0.0f);
    state_[band].last_applied_gain_db = 0.0f;
}

void DynamicEQ::RecalcAttackRelease(const uint32_t band) {
    const auto attack_sec = static_cast<double>(params_[band].attack_ms) / 1000.0;
    const auto release_sec = static_cast<double>(params_[band].release_ms) / 1000.0;
    const auto sr = static_cast<double>(sampling_rate_);

    if (attack_sec > 0.0) {
        state_[band].attack_coeff = 1.0 - exp(-1.0 / (attack_sec * sr));
    } else {
        state_[band].attack_coeff = 1.0;
    }

    if (release_sec > 0.0) {
        state_[band].release_coeff = 1.0 - exp(-1.0 / (release_sec * sr));
    } else {
        state_[band].release_coeff = 1.0;
    }
}

void DynamicEQ::ConfigureApplicationFilter(const uint32_t band, const float gain_db) {
    apply_l_[band].RefreshFilter(
        params_[band].filter_type,
        gain_db,
        params_[band].frequency,
        sampling_rate_,
        params_[band].q,
        false
    );
    apply_r_[band].RefreshFilter(
        params_[band].filter_type,
        gain_db,
        params_[band].frequency,
        sampling_rate_,
        params_[band].q,
        false
    );
}
