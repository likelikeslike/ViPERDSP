#include "DynamicBass.h"
#include "../constants.h"

DynamicBass::DynamicBass() :
    low_freq_x_(120),
    high_freq_x_(80),
    low_freq_y_(40),
    high_freq_y_(0),
    sampling_rate_(0),
    q_peak_(0.0f),
    bass_gain_(1.0f),
    side_gain_x_(1.0f),
    side_gain_y_(1.0f) {
    SetSamplingRate(VIPER_DEFAULT_SAMPLING_RATE);
    high_freq_y_ = static_cast<uint32_t>(static_cast<float>(sampling_rate_) / 4.0f);

    filter_x_.SetPassFilter(low_freq_x_, high_freq_x_);
    filter_y_.SetPassFilter(low_freq_y_, high_freq_y_);
    low_pass_.SetLowPassParameter(55.0f, sampling_rate_, q_peak_ / 666.0f + 0.5f);
}

void DynamicBass::FilterSamples(float *samples, const uint32_t size) {
    if (low_freq_x_ <= 120) {
        for (uint32_t i = 0; i < size; i++) {
            const float sample_l = samples[2 * i];
            const float sample_r = samples[2 * i + 1];
            const auto avg =
                static_cast<float>(low_pass_.ProcessSample(sample_l + sample_r));
            samples[2 * i] = sample_l + avg;
            samples[2 * i + 1] = sample_r + avg;
        }
    } else {
        for (uint32_t i = 0; i < size; i++) {
            float x1, x2, x3, x4, x5, x6, y1, y2, y3, y4, y5, y6;

            filter_x_.FilterLeft(samples[2 * i], &x1, &x2, &x3);
            filter_x_.FilterRight(samples[2 * i + 1], &x4, &x5, &x6);
            filter_y_.FilterLeft(bass_gain_ * x1, &y1, &y2, &y3);
            filter_y_.FilterRight(bass_gain_ * x4, &y4, &y5, &y6);

            samples[2 * i] = x2 + y3 + side_gain_x_ * y2 + side_gain_y_ * y1 + x3;
            samples[2 * i + 1] = x5 + y6 + side_gain_x_ * y5 + side_gain_y_ * y4 + x6;
        }
    }
}

void DynamicBass::Reset() {
    filter_x_.Reset();
    filter_y_.Reset();
    low_pass_.SetLowPassParameter(55.0f, sampling_rate_, q_peak_ / 666.0f + 0.5f);
}

void DynamicBass::SetBassGain(const float value) {
    bass_gain_ = value;

    double x = (value - 1.0) / 20.0 * 1600.0;
    if (x > 1600.0) {
        x = 1600.0;
    }
    q_peak_ = static_cast<float>(x);

    low_pass_.SetLowPassParameter(55.0f, sampling_rate_, q_peak_ / 666.0f + 0.5f);
}

void DynamicBass::SetFilterXPassFrequency(const uint32_t low, const uint32_t high) {
    low_freq_x_ = low;
    high_freq_x_ = high;

    filter_x_.SetPassFilter(low, high);
    filter_x_.SetSamplingRate(sampling_rate_);
    low_pass_.SetLowPassParameter(55.0f, sampling_rate_, q_peak_ / 666.0f + 0.5f);
}

void DynamicBass::SetFilterYPassFrequency(const uint32_t low, const uint32_t high) {
    low_freq_y_ = low;
    high_freq_y_ = high;

    filter_y_.SetPassFilter(low, high);
    filter_y_.SetSamplingRate(sampling_rate_);
    low_pass_.SetLowPassParameter(55.0f, sampling_rate_, q_peak_ / 666.0f + 0.5f);
}

void DynamicBass::SetSideGain(const float gain_x, const float gain_y) {
    side_gain_x_ = gain_x;
    side_gain_y_ = gain_y;
}

void DynamicBass::SetSamplingRate(const uint32_t sampling_rate) {
    sampling_rate_ = sampling_rate;
    filter_x_.SetSamplingRate(sampling_rate);
    filter_y_.SetSamplingRate(sampling_rate);
    low_pass_.SetLowPassParameter(55.0f, sampling_rate, q_peak_ / 666.0f + 0.5f);
}
