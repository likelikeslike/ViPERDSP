#include "SpectrumExtend.h"
#include "../constants.h"

static constexpr float kSpectrumHarmonics[10] = {
    0.02f,
    0.0f,
    0.02f,
    0.0f,
    0.02f,
    0.0f,
    0.02f,
    0.0f,
    0.02f,
    0.0f,
};

SpectrumExtend::SpectrumExtend() :
    enabled_(false),
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE),
    reference_freq_(7600),
    exciter_(0.0f) {
    Reset();
}

void SpectrumExtend::Process(float *samples, const uint32_t size) {
    if (!enabled_) return;

    for (uint32_t i = 0; i < size * 2; i += 2) {

        double tmp = highpass_[0].ProcessSample(samples[i]);
        tmp = harmonics_[0].Process(tmp);
        tmp = lowpass_[0].ProcessSample(tmp * exciter_);
        samples[i] = samples[i] + static_cast<float>(tmp);

        tmp = highpass_[1].ProcessSample(samples[i + 1]);
        tmp = harmonics_[1].Process(tmp);
        tmp = lowpass_[1].ProcessSample(tmp * exciter_);
        samples[i + 1] = samples[i + 1] + static_cast<float>(tmp);
    }
}

void SpectrumExtend::Reset() {
    highpass_[0].Reset();
    highpass_[1].Reset();
    lowpass_[0].Reset();
    lowpass_[1].Reset();

    highpass_[0].RefreshFilter(
        MultiBiquad::FilterType::HIGH_PASS,
        0.0f,
        static_cast<float>(reference_freq_),
        sampling_rate_,
        0.717f,
        false
    );
    highpass_[1].RefreshFilter(
        MultiBiquad::FilterType::HIGH_PASS,
        0.0f,
        static_cast<float>(reference_freq_),
        sampling_rate_,
        0.717f,
        false
    );

    lowpass_[0].RefreshFilter(
        MultiBiquad::FilterType::LOW_PASS,
        0.0f,
        static_cast<float>(sampling_rate_) / 2.0f - 2000.0f,
        sampling_rate_,
        0.717f,
        false
    );
    lowpass_[1].RefreshFilter(
        MultiBiquad::FilterType::LOW_PASS,
        0.0f,
        static_cast<float>(sampling_rate_) / 2.0f - 2000.0f,
        sampling_rate_,
        0.717f,
        false
    );

    harmonics_[0].Reset();
    harmonics_[1].Reset();

    harmonics_[0].SetHarmonics(kSpectrumHarmonics);
    harmonics_[1].SetHarmonics(kSpectrumHarmonics);
}

void SpectrumExtend::SetEnable(const bool enable) {
    if (enabled_ != enable) {
        if (enable) {
            Reset();
        }
        enabled_ = enable;
    }
}

void SpectrumExtend::SetExciter(const float value) {
    exciter_ = value;
}

void SpectrumExtend::SetReferenceFrequency(uint32_t value) {
    if (sampling_rate_ / 2 - 100 < value) {
        value = sampling_rate_ / 2 - 100;
    }
    reference_freq_ = value;
    Reset();
}

void SpectrumExtend::SetSamplingRate(const uint32_t sampling_rate) {
    if (sampling_rate_ != sampling_rate) {
        sampling_rate_ = sampling_rate;
        if (sampling_rate / 2 - 100 < reference_freq_) {
            reference_freq_ = sampling_rate / 2 - 100;
        }
        Reset();
    }
}
