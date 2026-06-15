#include "IIR_NOrder_BW_LH.h"

IIR_NOrder_BW_LH::IIR_NOrder_BW_LH(const uint32_t order) {
    this->filters_ = std::vector<IIR_1st>(order);
    this->order_ = order;

    for (uint32_t x = 0; x < order; x++) {
        this->filters_[x].Mute();
    }
}

void IIR_NOrder_BW_LH::Mute() {
    for (uint32_t x = 0; x < this->order_; x++) {
        this->filters_[x].Mute();
    }
}

void IIR_NOrder_BW_LH::SetLPF(const float frequency, const uint32_t sampling_rate) {
    for (uint32_t x = 0; x < this->order_; x++) {
        this->filters_[x].SetLowPassFilterBW(frequency, sampling_rate);
    }
}

void IIR_NOrder_BW_LH::SetHPF(const float frequency, const uint32_t sampling_rate) {
    for (uint32_t x = 0; x < this->order_; x++) {
        this->filters_[x].SetHighPassFilterBW(frequency, sampling_rate);
    }
}
