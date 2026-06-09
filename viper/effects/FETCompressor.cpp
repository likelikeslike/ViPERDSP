#include "FETCompressor.h"
#include "../constants.h"
#include <cmath>

static float calculate_exp_something(const uint32_t sampling_rate, const float time) {
    return 1.0f - exp(-1.0f / (time * static_cast<float>(sampling_rate)));
}

static float calculate_time_coeff(
    const uint32_t sampling_rate, const float value, const float scale, const float offset
) {
    const float time = exp(value * scale + offset);
    return time <= 0.0 ? 1.0 : calculate_exp_something(sampling_rate, time);
}

FETCompressor::FETCompressor() :
    enable_(true),
    auto_knee_(true),
    auto_gain_(true),
    auto_attack_(true),
    auto_release_(true),
    no_clip_(true),
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE),
    attack_raw_(0.514679f),
    release_raw_(0.384311f),
    crest_raw_(0.615689f),
    adapt_raw_(0.660964f) {
    SetThreshold(0.0f);
    SetRatio(0.0f);
    SetKnee(0.0f);
    SetGain(0.0f);
    SetAttack(attack_raw_);
    SetRelease(release_raw_);
    SetKneeMulti(0.5f);
    SetMaxAttack(0.879450f);
    SetMaxRelease(0.884311f);
    SetCrest(crest_raw_);
    SetAdapt(adapt_raw_);
    Reset();
}

void FETCompressor::Process(float *samples, const uint32_t size) {
    if (!enable_ || size == 0) return;

    for (uint32_t i = 0; i < size * 2; i += 2) {
        const double in_l = abs(samples[i]);
        const double in_r = abs(samples[i + 1]);
        const double in = std::fmax(in_l, in_r);

        const double out = ProcessSidechain(in);
        samples[i] *= static_cast<float>(out);
        samples[i + 1] *= static_cast<float>(out);

        smoothed_threshold_ =
            smoothed_threshold_ + (threshold_ - smoothed_threshold_) * smoothing_coeff_;
        smoothed_gain_ = smoothed_gain_ + smoothing_coeff_ * (gain_ - smoothed_gain_);
    }
}

double FETCompressor::ProcessSidechain(const double in) {
    double in2 = in * in;
    if (in2 < 0.000001) {
        in2 = 0.000001;
    }

    float attack_coeff = attack2_;
    float release_coeff = release2_;
    float adaptive_attack_time = attack1_;

    const float running_peak =
        running_peak_ + crest2_ * (static_cast<float>(in2) - running_peak_);
    const float running_rms =
        running_rms_ + crest2_ * (static_cast<float>(in2) - running_rms_);

    if (static_cast<float>(in2) < running_peak) {
        in2 = running_peak;
    }

    running_rms_ = running_rms;
    running_peak_ = static_cast<float>(in2);

    const float crest_ratio = static_cast<float>(in2) / running_rms;

    if (auto_attack_) {
        adaptive_attack_time = 2.0f * max_attack_ / crest_ratio;
        if (adaptive_attack_time <= 0.0f) {
            attack_coeff = 1.0f;
        } else {
            attack_coeff = static_cast<float>(
                calculate_exp_something(sampling_rate_, adaptive_attack_time)
            );
        }
    }

    if (auto_release_) {
        const float adaptive_release_time =
            2.0f * max_release_ / crest_ratio - adaptive_attack_time;
        if (adaptive_release_time <= 0.0f) {
            release_coeff = 1.0f;
        } else {
            release_coeff = static_cast<float>(
                calculate_exp_something(sampling_rate_, adaptive_release_time)
            );
        }
    }

    const float log_input = logf(in >= 0.000001f ? static_cast<float>(in) : 0.000001f);

    const float diff = log_input - smoothed_threshold_;
    float ratio_mul;
    float half_thresh_gr;
    float half_knee;
    float knee_width;

    if (!auto_knee_) {
        const float neg_ratio = -ratio_;
        knee_width = knee_;
        half_thresh_gr = smoothed_threshold_ * neg_ratio * 0.5f;
        half_knee = knee_width * 0.5f;
        ratio_mul = neg_ratio;
    } else {
        half_thresh_gr = smoothed_threshold_ * 0.5f;
        const float knee_base = adaptive_gain_state_ + half_thresh_gr;
        knee_width = -(knee_base * knee_multi_);
        if (knee_width <= 0.0f) {
            ratio_mul = 1.0f;
            half_knee = 0.0f;
            knee_width = 0.0f;
        } else {
            ratio_mul = 1.0f;
            half_knee = knee_width * 0.5f;
        }
    }

    float gain_reduction;
    if (diff >= half_knee) {
        gain_reduction = diff;
    } else if (diff <= -(knee_width * 0.5f)) {
        gain_reduction = 0.0f;
    } else {
        const float doubled = knee_width * 2.0f;
        const float shifted = diff + half_knee;
        gain_reduction = (shifted * shifted) / doubled;
    }

    gain_reduction *= ratio_mul;

    const float rel_smoothed =
        release_smooth_gr_ + (gain_reduction - release_smooth_gr_) * release_coeff;
    if (gain_reduction <= rel_smoothed) {
        gain_reduction = rel_smoothed;
    }

    const float atk_diff = gain_reduction - attack_smooth_gr_;
    release_smooth_gr_ = gain_reduction;
    const float smoothed_gr = attack_smooth_gr_ + atk_diff * attack_coeff;

    const float neg_smoothed_gr = -smoothed_gr;
    const float adapt_target = neg_smoothed_gr - half_thresh_gr - adaptive_gain_state_;
    attack_smooth_gr_ = smoothed_gr;

    adaptive_gain_state_ = adaptive_gain_state_ + adapt_target * adapt2_;

    if (auto_gain_) {
        if (!no_clip_) {
            const float makeup_gain = adaptive_gain_state_ + half_thresh_gr;
            return exp(neg_smoothed_gr - makeup_gain);
        } else {
            float output_level = log_input - smoothed_gr;
            float makeup_gain = half_thresh_gr + adaptive_gain_state_;
            if (output_level - makeup_gain > 0.0011512704f) {
                output_level = output_level - half_thresh_gr;
                output_level = output_level + 0.0011512704f;
                makeup_gain = output_level + half_thresh_gr;
                adaptive_gain_state_ = output_level;
            }
            return exp(neg_smoothed_gr - makeup_gain);
        }
    }

    return exp(smoothed_gain_ - smoothed_gr);
}

void FETCompressor::Reset() {
    smoothing_coeff_ = calculate_exp_something(sampling_rate_, 0.05);
    smoothed_threshold_ = threshold_;
    smoothed_gain_ = gain_;
    running_peak_ = 0.000001f;
    running_rms_ = 0.000001f;
    release_smooth_gr_ = 0.0f;
    attack_smooth_gr_ = 0.0f;
    adaptive_gain_state_ = 0.0f;
}

void FETCompressor::SetEnable(const bool enable) {
    enable_ = enable;
}

void FETCompressor::SetThreshold(const float value) {
    threshold_ = log(pow(10.0f, (value * -60.0f) / 20.0f));
}

void FETCompressor::SetRatio(const float value) {
    ratio_ = -value;
}

void FETCompressor::SetKnee(const float value) {
    knee_ = log(pow(10.0f, (value * 60.0f) / 20.0f));
}

void FETCompressor::SetKneeAuto(const bool enable) {
    auto_knee_ = enable;
}

void FETCompressor::SetGain(const float value) {
    gain_ = log(pow(10.0f, (value * 60.0f) / 20.0f));
}

void FETCompressor::SetGainAuto(const bool enable) {
    auto_gain_ = enable;
}

void FETCompressor::SetAttack(const float value) {
    attack_raw_ = value;
    attack1_ = exp(value * 7.600903f - 9.21034f);
    attack2_ = calculate_time_coeff(sampling_rate_, value, 7.600903f, -9.21034f);
}

void FETCompressor::SetAttackAuto(const bool enable) {
    auto_attack_ = enable;
}

void FETCompressor::SetRelease(const float value) {
    release_raw_ = value;
    release1_ = exp(value * 5.991465f - 5.298317f);
    release2_ = calculate_time_coeff(sampling_rate_, value, 5.991465f, -5.298317f);
}

void FETCompressor::SetReleaseAuto(const bool enable) {
    auto_release_ = enable;
}

void FETCompressor::SetKneeMulti(const float value) {
    knee_multi_ = value * 4.0f;
}

void FETCompressor::SetMaxAttack(const float value) {
    max_attack_ = exp(value * 7.600903f - 9.21034f);
}

void FETCompressor::SetMaxRelease(const float value) {
    max_release_ = exp(value * 5.991465f - 5.298317f);
}

void FETCompressor::SetCrest(const float value) {
    crest_raw_ = value;
    crest1_ = exp(value * 5.991465f - 5.298317f);
    crest2_ = calculate_time_coeff(sampling_rate_, value, 5.991465f, -5.298317f);
}

void FETCompressor::SetAdapt(const float value) {
    adapt_raw_ = value;
    adapt1_ = exp(value * 1.386294f);
    adapt2_ = calculate_time_coeff(sampling_rate_, value, 1.386294f, 0.0f);
}

void FETCompressor::SetNoClip(const bool enable) {
    no_clip_ = enable;
}

void FETCompressor::SetSamplingRate(const uint32_t sampling_rate) {
    sampling_rate_ = sampling_rate;
    SetAttack(attack_raw_);
    SetRelease(release_raw_);
    SetCrest(crest_raw_);
    SetAdapt(adapt_raw_);
    Reset();
}
