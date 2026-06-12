#include "PlaybackGain.h"
#include "../constants.h"
#include <cmath>

constexpr float kWarmupSeconds = 0.4f;

PlaybackGain::PlaybackGain() :
    enable_(false),
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE),
    ramp_progress_(0),
    ramp_frames_(static_cast<uint32_t>(VIPER_DEFAULT_SAMPLING_RATE * kWarmupSeconds)),
    log_coeff_(0.4342945f),
    ratio1_(2.0f),
    ratio2_(0.5f),
    volume_(1.0f),
    max_gain_factor_(1.0f),
    current_gain_l_(1.0f),
    current_gain_r_(1.0f) {
    biquad1_.SetBandPassParameter(2200.0f, sampling_rate_, 0.33f);
    biquad2_.SetBandPassParameter(2200.0f, sampling_rate_, 0.33f);
}

void PlaybackGain::Process(float *samples, const uint32_t size) {
    if (!enable_) return;
    if (size == 0) return;

    double analyzed = AnalyseWave(samples, size);

    // Avoid log(0) or small values.
    if (analyzed < 1e-10) {
        analyzed = 1e-10;
    }
    const double a = log(analyzed);

    if (ramp_progress_ < ramp_frames_) {
        ramp_progress_ += size;
        if (ramp_progress_ > ramp_frames_) {
            ramp_progress_ = ramp_frames_;
        }
    }
    const double ramp = ramp_frames_ == 0 ? 1.0
                                          : static_cast<double>(ramp_progress_)
                                                / static_cast<double>(ramp_frames_);

    const double b = a * log_coeff_ * 10.0 + 23.0;
    const double c = ramp * (b * ratio2_ - b);
    const double d = c / 100.0;
    const double e = pow(10.0, (c - d * d * 50.0) / 20.0);
    uint32_t f;
    if (size < sampling_rate_ / 40) {
        f = sampling_rate_ / 40;
    } else {
        f = size;
    }

    const double target = e * volume_;

    for (int ch = 0; ch < 2; ch++) {
        float &gain = ch == 0 ? current_gain_l_ : current_gain_r_;
        double g = (target - gain) / f;
        if (g >= 0.0) {
            g *= 0.0625;
        }
        for (uint32_t i = 0; i < size; i++) {
            samples[i * 2 + ch] *= gain;
            gain += static_cast<float>(g);
            if (gain > max_gain_factor_) {
                gain = max_gain_factor_;
            } else if (gain < -max_gain_factor_) {
                gain = -max_gain_factor_;
            }
        }
    }
}

void PlaybackGain::Reset() {
    biquad1_.SetBandPassParameter(2200.0f, sampling_rate_, 0.33f);
    biquad2_.SetBandPassParameter(2200.0f, sampling_rate_, 0.33f);
    current_gain_l_ = 1.0f;
    ramp_progress_ = 0;
    current_gain_r_ = 1.0f;
}

void PlaybackGain::SetEnable(const bool enable) {
    if (enable_ != enable) {
        if (enable) {
            Reset();
        }
        enable_ = enable;
    }
}

void PlaybackGain::SetMaxGainFactor(const float max_gain_factor) {
    max_gain_factor_ = max_gain_factor;
}

void PlaybackGain::SetRatio(const float ratio) {
    ratio1_ = ratio + 1.0f;
    ratio2_ = 1.0f / ratio1_;
}

void PlaybackGain::SetVolume(const float volume) {
    volume_ = volume;
}

void PlaybackGain::SetSamplingRate(const uint32_t sampling_rate) {
    if (sampling_rate_ != sampling_rate) {
        sampling_rate_ = sampling_rate;
        ramp_frames_ = static_cast<uint32_t>(sampling_rate * kWarmupSeconds);
        Reset();
    }
}

double PlaybackGain::AnalyseWave(const float *samples, const uint32_t size) {
    double tmp_l = 0.0;
    double tmp_r = 0.0;

    for (uint32_t i = 0; i < size * 2; i += 2) {
        const double tmp_l2 = biquad1_.ProcessSample(samples[i]);
        tmp_l += tmp_l2 * tmp_l2;

        const double tmp_r2 = biquad2_.ProcessSample(samples[i + 1]);
        tmp_r += tmp_r2 * tmp_r2;
    }

    const double tmp = std::fmax(tmp_l, tmp_r);

    return tmp / static_cast<double>(size);
}
