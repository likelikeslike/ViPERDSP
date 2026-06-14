#include "NoiseSharpening.h"
#include "../constants.h"

NoiseSharpening::NoiseSharpening() :
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE),
    gain_(0.0f) {
    Reset();
}

void NoiseSharpening::Process(float *buffer, const uint32_t size) {
    for (uint32_t i = 0; i < size; i++) {
        const float sample_l = buffer[i * 2];
        const float sample_r = buffer[i * 2 + 1];
        const float prev_l = in_[0];
        const float prev_r = in_[1];
        in_[0] = sample_l;
        in_[1] = sample_r;
        const float diff_l = (sample_l - prev_l) * gain_;
        const float diff_r = (sample_r - prev_r) * gain_;

        const float sample_l_in = sample_l + diff_l;
        const float sample_r_in = sample_r + diff_r;

        float hist = sample_l_in * filters_[0].b1_;
        const float left = filters_[0].prev_sample_ + sample_l_in * filters_[0].b0_;
        filters_[0].prev_sample_ = sample_l_in * filters_[0].a1_ + hist;

        hist = sample_r_in * filters_[1].b1_;
        const float right = filters_[1].prev_sample_ + sample_r_in * filters_[1].b0_;
        filters_[1].prev_sample_ = sample_r_in * filters_[1].a1_ + hist;

        buffer[i * 2] = left;
        buffer[i * 2 + 1] = right;
    }
}

void NoiseSharpening::Reset() {
    for (int i = 0; i < 2; i++) {
        filters_[i].SetLowPassFilterBW(
            static_cast<float>(sampling_rate_ / 2.0 - 1000.0), sampling_rate_
        );
        filters_[i].Mute();
        in_[i] = 0.0f;
    }
}

void NoiseSharpening::SetGain(const float gain) {
    gain_ = gain;
}

void NoiseSharpening::SetSamplingRate(const uint32_t sampling_rate) {
    sampling_rate_ = sampling_rate;
    Reset();
}
