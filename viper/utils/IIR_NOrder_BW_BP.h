#pragma once

#include "IIR_1st.h"
#include <vector>

class IIR_NOrder_BW_BP {
public:
    explicit IIR_NOrder_BW_BP(uint32_t order);

    void Mute();

    void SetBPF(float high_cut, float low_cut, uint32_t sampling_rate);

    uint32_t order_;

    std::vector<IIR_1st> lowpass_;
    std::vector<IIR_1st> highpass_;
};

inline float FilterBPLowPass(IIR_NOrder_BW_BP *filter, float sample) {
    for (uint32_t idx = 0; idx < filter->order_; idx++) {
        sample = Filter(&filter->lowpass_[idx], sample);
    }
    return sample;
}

inline float FilterBPHighPass(IIR_NOrder_BW_BP *filter, float sample) {
    for (uint32_t idx = 0; idx < filter->order_; idx++) {
        sample = Filter(&filter->highpass_[idx], sample);
    }
    return sample;
}

inline float FilterBP(IIR_NOrder_BW_BP *filter, const float sample) {
    return FilterBPHighPass(filter, FilterBPLowPass(filter, sample));
}
