#include "AnalogX.h"
#include "../constants.h"
#include <cstring>

static constexpr float kAnalogXHarmonics[] = {
    0.01f, 0.02f, 0.0001f, 0.001f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f
};

AnalogX::AnalogX() :
    enable_(false),
    processing_model_(0),
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE),
    freq_range_(0),
    gain_(0.0f) {
    Reset();
}

void AnalogX::Process(float *samples, const uint32_t size) {
    if (enable_) {
        for (uint32_t i = 0; i < size * 2; i += 2) {
            const double in_l = samples[i];
            double out_l = high_pass_[0].ProcessSample(in_l);
            out_l = harmonic_[0].Process(out_l);
            out_l = low_pass_[0].ProcessSample(in_l + out_l * gain_);
            out_l = peak_[0].ProcessSample(out_l * 0.8);
            samples[i] = static_cast<float>(out_l);

            const double in_r = samples[i + 1];
            double out_r = high_pass_[1].ProcessSample(in_r);
            out_r = harmonic_[1].Process(out_r);
            out_r = low_pass_[1].ProcessSample(in_r + out_r * gain_);
            out_r = peak_[1].ProcessSample(out_r * 0.8);
            samples[i + 1] = static_cast<float>(out_r);
        }

        if (freq_range_ < sampling_rate_ / 4) {
            freq_range_ += size;
            memset(samples, 0, size * 2 * sizeof(float));
        }
    }
}

void AnalogX::Reset() {
    static constexpr struct {
        float gain;
        float cutoff;
    } models[] = {{0.6f, 19650.0f}, {1.2f, 18233.0f}, {2.4f, 16307.0f}};

    high_pass_[0].RefreshFilter(
        MultiBiquad::FilterType::HIGH_PASS, 0.0f, 240.0f, sampling_rate_, 0.717f, false
    );
    high_pass_[1].RefreshFilter(
        MultiBiquad::FilterType::HIGH_PASS, 0.0f, 240.0f, sampling_rate_, 0.717f, false
    );

    peak_[0].RefreshFilter(
        MultiBiquad::FilterType::PEAK, 0.58f, 633.0f, sampling_rate_, 6.28f, true
    );
    peak_[1].RefreshFilter(
        MultiBiquad::FilterType::PEAK, 0.58f, 633.0f, sampling_rate_, 6.28f, true
    );

    harmonic_[0].Reset();
    harmonic_[1].Reset();

    if (processing_model_ >= 0 && processing_model_ <= 2) {
        harmonic_[0].SetHarmonics(kAnalogXHarmonics);
        harmonic_[1].SetHarmonics(kAnalogXHarmonics);
        gain_ = models[processing_model_].gain;
        const float cutoff = models[processing_model_].cutoff;
        low_pass_[0].RefreshFilter(
            MultiBiquad::FilterType::LOW_PASS, 0.0f, cutoff, sampling_rate_, 0.717f, false
        );
        low_pass_[1].RefreshFilter(
            MultiBiquad::FilterType::LOW_PASS, 0.0f, cutoff, sampling_rate_, 0.717f, false
        );
    }

    freq_range_ = 0;
}

void AnalogX::SetEnable(const bool enable) {
    if (enable_ != enable) {
        if (!enable_) {
            Reset();
        }
        enable_ = enable;
    }
}

void AnalogX::SetProcessingModel(const int model) {
    if (processing_model_ != model) {
        processing_model_ = model;
        Reset();
    }
}

void AnalogX::SetSamplingRate(const uint32_t sampling_rate) {
    if (sampling_rate_ != sampling_rate) {
        sampling_rate_ = sampling_rate;
        Reset();
    }
}
