#include "PsychoacousticBass.h"
#include "../constants.h"

static constexpr float kHarmonicOrder2[10] = {
    0.0f,
    1.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
};

static constexpr float kHarmonicOrder3[10] = {
    0.0f,
    0.7f,
    0.3f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
};

static constexpr float kHarmonicOrder4[10] = {
    0.0f,
    0.5f,
    0.3f,
    0.2f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
};

static constexpr float kHarmonicOrder5[10] = {
    0.0f,
    0.4f,
    0.25f,
    0.2f,
    0.15f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
};

PsychoacousticBass::PsychoacousticBass() :
    enabled_(false),
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE),
    cutoff_(80),
    harmonic_order_(3),
    intensity_(0.5f),
    original_bass_level_(1.0f),
    envelope_(1e-10) {
    Reset();
}

void PsychoacousticBass::Process(float *samples, const uint32_t size) {
    if (!enabled_) return;

    for (uint32_t i = 0; i < size * 2; i += 2) {
        const double bass_l = lowpass_[0].ProcessSample(samples[i]);
        const double bass_r = lowpass_[1].ProcessSample(samples[i + 1]);

        double abs_l = fabs(bass_l);
        double abs_r = fabs(bass_r);
        const double peak = abs_l > abs_r ? abs_l : abs_r;
        if (peak > envelope_) {
            envelope_ += 0.01 * (peak - envelope_);
        } else {
            envelope_ += 0.0001 * (peak - envelope_);
        }
        if (envelope_ < 1e-10) envelope_ = 1e-10;

        double norm_l = bass_l / envelope_;
        double norm_r = bass_r / envelope_;
        if (norm_l > 1.0) norm_l = 1.0;
        if (norm_l < -1.0) norm_l = -1.0;
        if (norm_r > 1.0) norm_r = 1.0;
        if (norm_r < -1.0) norm_r = -1.0;

        double harmonic_l = harmonics_[0].Process(norm_l) * envelope_;
        double harmonic_r = harmonics_[1].Process(norm_r) * envelope_;

        harmonic_l = highpass_[0].ProcessSample(harmonic_l);
        harmonic_r = highpass_[1].ProcessSample(harmonic_r);

        samples[i] = samples[i] + static_cast<float>(harmonic_l * intensity_);
        samples[i + 1] = samples[i + 1] + static_cast<float>(harmonic_r * intensity_);

        if (original_bass_level_ < 1.0f) {
            const double dry_l = samples[i] - static_cast<float>(bass_l);
            const double dry_r = samples[i + 1] - static_cast<float>(bass_r);
            samples[i] = static_cast<float>(dry_l + bass_l * original_bass_level_);
            samples[i + 1] = static_cast<float>(dry_r + bass_r * original_bass_level_);
        }
    }
}

void PsychoacousticBass::Reset() {
    envelope_ = 1e-10;
    for (uint32_t ch = 0; ch < 2; ch++) {
        lowpass_[ch].Reset();
        highpass_[ch].Reset();
    }
    RefreshFilters();
    ApplyHarmonicCoeffs();
}

void PsychoacousticBass::SetEnable(const bool enable) {
    if (enabled_ != enable) {
        if (enable) {
            Reset();
        }
        enabled_ = enable;
    }
}

void PsychoacousticBass::SetSamplingRate(const uint32_t sampling_rate) {
    if (sampling_rate_ != sampling_rate) {
        sampling_rate_ = sampling_rate;
        Reset();
    }
}

void PsychoacousticBass::SetCutoff(uint32_t value) {
    if (value < 60) value = 60;
    if (value > 150) value = 150;
    if (cutoff_ != value) {
        cutoff_ = value;
        RefreshFilters();
    }
}

void PsychoacousticBass::SetIntensity(uint32_t value) {
    if (value > 100) value = 100;
    intensity_ = static_cast<float>(value) / 100.0f;
}

void PsychoacousticBass::SetHarmonicOrder(uint32_t value) {
    if (value < 2) value = 2;
    if (value > 5) value = 5;
    if (harmonic_order_ != value) {
        harmonic_order_ = value;
        ApplyHarmonicCoeffs();
    }
}

void PsychoacousticBass::SetOriginalBassLevel(uint32_t value) {
    if (value > 100) value = 100;
    original_bass_level_ = static_cast<float>(value) / 100.0f;
}

void PsychoacousticBass::RefreshFilters() {
    for (uint32_t ch = 0; ch < 2; ch++) {
        lowpass_[ch].RefreshFilter(
            MultiBiquad::FilterType::LOW_PASS,
            0.0f,
            static_cast<float>(cutoff_),
            sampling_rate_,
            0.717f,
            false
        );
        highpass_[ch].RefreshFilter(
            MultiBiquad::FilterType::HIGH_PASS,
            0.0f,
            static_cast<float>(cutoff_),
            sampling_rate_,
            0.717f,
            false
        );
    }
}

void PsychoacousticBass::ApplyHarmonicCoeffs() {
    const float *coeffs;
    switch (harmonic_order_) {
        case 2:
            coeffs = kHarmonicOrder2;
            break;
        case 3:
            coeffs = kHarmonicOrder3;
            break;
        case 4:
            coeffs = kHarmonicOrder4;
            break;
        case 5:
            coeffs = kHarmonicOrder5;
            break;
        default:
            coeffs = kHarmonicOrder3;
            break;
    }
    harmonics_[0].Reset();
    harmonics_[1].Reset();
    harmonics_[0].SetHarmonics(coeffs);
    harmonics_[1].SetHarmonics(coeffs);
}
