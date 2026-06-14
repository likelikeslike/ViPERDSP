#pragma once

#include "IIR_1st.h"
#include <vector>

class IIR_NOrder_BW_LH {
public:
    explicit IIR_NOrder_BW_LH(uint32_t order);

    void Mute();

    void SetLPF(float frequency, uint32_t sampling_rate);
    void SetHPF(float frequency, uint32_t sampling_rate);

    uint32_t order_;

    std::vector<IIR_1st> filters_;
};

inline float FilterLH(IIR_NOrder_BW_LH *filter, float sample) {
    for (uint32_t idx = 0; idx < filter->order_; idx++) {
        sample = Filter(&filter->filters_[idx], sample);
    }
    return sample;
}

inline float FilterLH(IIR_NOrder_BW_LH &filter, float sample) {
    for (uint32_t idx = 0; idx < filter.order_; idx++) {
        sample = Filter(&filter.filters_[idx], sample);
    }
    return sample;
}
