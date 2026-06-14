#include "Subwoofer.h"
#include "../constants.h"
#include <cmath>

Subwoofer::Subwoofer() :
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE),
    gain_(0.0f),
    gain_lower_(0.0f) {
    peak_[0].RefreshFilter(
        MultiBiquad::FilterType::PEAK, gain_, 37.0f, sampling_rate_, 1.0f, false
    );
    peak_[1].RefreshFilter(
        MultiBiquad::FilterType::PEAK, gain_, 37.0f, sampling_rate_, 1.0f, false
    );
    peak_low_[0].RefreshFilter(
        MultiBiquad::FilterType::PEAK, gain_lower_, 75.0f, sampling_rate_, 1.0f, false
    );
    peak_low_[1].RefreshFilter(
        MultiBiquad::FilterType::PEAK, gain_lower_, 75.0f, sampling_rate_, 1.0f, false
    );
    lowpass_[0].RefreshFilter(
        MultiBiquad::FilterType::LOW_PASS, 0.0f, 200.0f, sampling_rate_, 1.0f, false
    );
    lowpass_[1].RefreshFilter(
        MultiBiquad::FilterType::LOW_PASS, 0.0f, 200.0f, sampling_rate_, 1.0f, false
    );
}

void Subwoofer::Process(float *samples, const uint32_t size) {
    for (uint32_t i = 0; i < size * 2; i += 2) {

        double tmp = peak_[0].ProcessSample(samples[i]);
        tmp = peak_low_[0].ProcessSample(tmp);
        tmp = lowpass_[0].ProcessSample(tmp - samples[i]);
        samples[i] = samples[i] * 0.5f + static_cast<float>(tmp) * 0.6f;

        tmp = peak_[1].ProcessSample(samples[i + 1]);
        tmp = peak_low_[1].ProcessSample(tmp);
        tmp = lowpass_[1].ProcessSample(tmp - samples[i + 1]);
        samples[i + 1] = samples[i + 1] * 0.5f + static_cast<float>(tmp) * 0.6f;
    }
}

void Subwoofer::SetBassGain(const uint32_t sampling_rate, const float gain_db) {
    gain_ = 20.0f * log10(gain_db);
    gain_lower_ = 20.0f * log10(gain_db / 8.0f);

    peak_[0].RefreshFilter(
        MultiBiquad::FilterType::PEAK, gain_, 44.0f, sampling_rate, 0.75f, true
    );
    peak_[1].RefreshFilter(
        MultiBiquad::FilterType::PEAK, gain_, 44.0f, sampling_rate, 0.75f, true
    );
    peak_low_[0].RefreshFilter(
        MultiBiquad::FilterType::PEAK, gain_lower_, 80.0f, sampling_rate, 0.2f, true
    );
    peak_low_[1].RefreshFilter(
        MultiBiquad::FilterType::PEAK, gain_lower_, 80.0f, sampling_rate, 0.2f, true
    );
    lowpass_[0].RefreshFilter(
        MultiBiquad::FilterType::LOW_PASS, 0.0f, 380.0f, sampling_rate, 0.6f, false
    );
    lowpass_[1].RefreshFilter(
        MultiBiquad::FilterType::LOW_PASS, 0.0f, 380.0f, sampling_rate, 0.6f, false
    );
}
