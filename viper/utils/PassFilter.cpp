#include "PassFilter.h"
#include "../constants.h"

PassFilter::PassFilter() :
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE),
    filters_(
        {IIR_NOrder_BW_LH(3),
         IIR_NOrder_BW_LH(3),
         IIR_NOrder_BW_LH(1),
         IIR_NOrder_BW_LH(1)}
    ) {
    Reset();
}

void PassFilter::ProcessFrames(float *buffer, const uint32_t size) {
    for (uint32_t x = 0; x < size; x++) {
        float left = buffer[2 * x];
        float right = buffer[2 * x + 1];

        left = FilterLH(filters_[2], left);
        left = FilterLH(filters_[0], left);
        right = FilterLH(filters_[3], right);
        right = FilterLH(filters_[1], right);

        buffer[2 * x] = left;
        buffer[2 * x + 1] = right;
    }
}

void PassFilter::Reset() {
    float cutoff;
    if (sampling_rate_ < 44100) {
        cutoff = static_cast<float>(sampling_rate_) - 100.0f;
    } else {
        cutoff = 18000.0f;
    }

    filters_[0].SetLPF(cutoff, sampling_rate_);
    filters_[1].SetLPF(cutoff, sampling_rate_);
    filters_[2].SetHPF(10.0f, sampling_rate_);
    filters_[3].SetHPF(10.0f, sampling_rate_);

    filters_[0].Mute();
    filters_[1].Mute();
    filters_[2].Mute();
    filters_[3].Mute();
}

void PassFilter::SetSamplingRate(const uint32_t sampling_rate) {
    if (sampling_rate_ != sampling_rate) {
        sampling_rate_ = sampling_rate;
        Reset();
    }
}
