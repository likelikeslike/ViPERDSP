#include "PolesFilter.h"
#include "../constants.h"
#include <cmath>
#include <cstring>

PolesFilter::PolesFilter() :
    lower_freq_(160),
    upper_freq_(8000),
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE) {
    Reset();
}

static constexpr float kAntiDenormal = 1e-25f;

static void FilterSide(
    channel *side, const float sample, float *out1, float *out2, float *out3
) {
    const float oldest_sample_in = side->in[2];
    side->in[2] = side->in[1];
    side->in[1] = side->in[0];
    side->in[0] = sample;

    side->x[0] += side->lower_angle * (sample - side->x[0]) + kAntiDenormal;
    side->x[1] += side->lower_angle * (side->x[0] - side->x[1]) + kAntiDenormal;
    side->x[2] += side->lower_angle * (side->x[1] - side->x[2]) + kAntiDenormal;
    side->x[3] += side->lower_angle * (side->x[2] - side->x[3]) + kAntiDenormal;

    side->y[0] += side->upper_angle * (sample - side->y[0]) + kAntiDenormal;
    side->y[1] += side->upper_angle * (side->y[0] - side->y[1]) + kAntiDenormal;
    side->y[2] += side->upper_angle * (side->y[1] - side->y[2]) + kAntiDenormal;
    side->y[3] += side->upper_angle * (side->y[2] - side->y[3]) + kAntiDenormal;

    *out1 = side->x[3];
    *out2 = oldest_sample_in - side->y[3];
    *out3 = side->y[3] - side->x[3];
}

void PolesFilter::FilterLeft(const float sample, float *out1, float *out2, float *out3) {
    FilterSide(&channels_[0], sample, out1, out2, out3);
}

void PolesFilter::FilterRight(const float sample, float *out1, float *out2, float *out3) {
    FilterSide(&channels_[1], sample, out1, out2, out3);
}

void PolesFilter::Reset() {
    memset(&channels_[0], 0, sizeof(channel));
    memset(&channels_[1], 0, sizeof(channel));
    UpdateCoeff();
}

void PolesFilter::SetPassFilter(const uint32_t lower_freq, const uint32_t upper_freq) {
    lower_freq_ = lower_freq;
    upper_freq_ = upper_freq;
    UpdateCoeff();
}

void PolesFilter::SetSamplingRate(const uint32_t sampling_rate) {
    sampling_rate_ = sampling_rate;
    UpdateCoeff();
}

void PolesFilter::UpdateCoeff() {
    channels_[0].lower_angle = static_cast<float>(lower_freq_) * static_cast<float>(M_PI)
                               / static_cast<float>(sampling_rate_);
    channels_[1].lower_angle = static_cast<float>(lower_freq_) * static_cast<float>(M_PI)
                               / static_cast<float>(sampling_rate_);
    channels_[0].upper_angle = static_cast<float>(upper_freq_) * static_cast<float>(M_PI)
                               / static_cast<float>(sampling_rate_);
    channels_[1].upper_angle = static_cast<float>(upper_freq_) * static_cast<float>(M_PI)
                               / static_cast<float>(sampling_rate_);
}
