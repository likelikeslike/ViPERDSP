#include "Crossfeed.h"
#include "../constants.h"
#include <cmath>
#include <cstring>

Crossfeed::Crossfeed() :
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE),
    a0_lo_(0.0f),
    b1_lo_(0.0f),
    a0_hi_(0.0f),
    b1_hi_(0.0f),
    a1_hi_(0.0f),
    gain_(0.0f) {
    memset(&lfs_, 0, 6 * sizeof(float));
    preset_.cutoff = 700;
    preset_.feedback = 45;
}

void Crossfeed::ProcessFrames(float *buffer, const uint32_t size) {
    for (uint32_t i = 0; i < size * 2; i += 2) {
        FilterSample(buffer + i);
    }
}

void Crossfeed::Reset() {
    const uint32_t cutoff = preset_.cutoff;
    uint32_t level = preset_.feedback;

    level /= 10;

    const double gb_lo = level * -5.0 / 6.0 - 3.0;
    const double gb_hi = level / 6.0 - 3.0;

    const double g_lo = pow(10, gb_lo / 20.0);
    const double g_hi = 1.0 - pow(10, gb_hi / 20.0);
    const double fc_hi = cutoff * pow(2.0, (gb_lo - 20.0 * log10(g_hi)) / 12.0);

    double x = exp(-2.0 * M_PI * cutoff / sampling_rate_);
    b1_lo_ = static_cast<float>(x);
    a0_lo_ = static_cast<float>(g_lo * (1.0 - x));

    x = exp(-2.0 * M_PI * fc_hi / sampling_rate_);
    b1_hi_ = static_cast<float>(x);
    a0_hi_ = static_cast<float>(1.0 - g_hi * (1.0 - x));
    a1_hi_ = static_cast<float>(-x);

    gain_ = static_cast<float>(1.0 / (1.0 - g_hi + g_lo));
    memset(&lfs_, 0, 6 * sizeof(float));
}

uint32_t Crossfeed::GetCutoff() const {
    return preset_.cutoff;
}

float Crossfeed::GetFeedback() const {
    return static_cast<float>(preset_.feedback) / 10.0f;
}

float Crossfeed::GetLevelDelay() const {
    if (preset_.cutoff <= 1800) {
        return 18700.0f / static_cast<float>(preset_.cutoff) * 10.0f;
    }
    return 0.0f;
}

Crossfeed::Preset Crossfeed::GetPreset() const {
    return preset_;
}

void Crossfeed::SetCutoff(const uint32_t value) {
    preset_.cutoff = value;
    Reset();
}

void Crossfeed::SetFeedback(const float value) {
    preset_.feedback = static_cast<uint32_t>(value * 10.0f);
    Reset();
}

void Crossfeed::SetPreset(const Preset preset) {
    preset_ = preset;
    Reset();
}

void Crossfeed::SetSamplingRate(const uint32_t sampling_rate) {
    if (sampling_rate_ != sampling_rate) {
        sampling_rate_ = sampling_rate;
        Reset();
    }
}

void Crossfeed::FilterSample(float *sample) {
    lfs_.lo[0] = ApplyLoFilter(sample[0], lfs_.lo[0]);
    lfs_.lo[1] = ApplyLoFilter(sample[1], lfs_.lo[1]);

    lfs_.hi[0] = ApplyHiFilter(sample[0], lfs_.asis[0], lfs_.hi[0]);
    lfs_.hi[1] = ApplyHiFilter(sample[1], lfs_.asis[1], lfs_.hi[1]);
    lfs_.asis[0] = sample[0];
    lfs_.asis[1] = sample[1];

    sample[0] = (lfs_.hi[0] + lfs_.lo[1]) * gain_;
    sample[1] = (lfs_.hi[1] + lfs_.lo[0]) * gain_;
}
