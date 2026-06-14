#include "IIR_NOrder_BW_BP.h"

IIR_NOrder_BW_BP::IIR_NOrder_BW_BP(const uint32_t order) {
    this->lowpass_ = std::vector<IIR_1st>(order);
    this->highpass_ = std::vector<IIR_1st>(order);
    this->order_ = order;

    for (uint32_t x = 0; x < order; x++) {
        this->lowpass_[x].Mute();
        this->highpass_[x].Mute();
    }
}

void IIR_NOrder_BW_BP::Mute() {
    for (uint32_t x = 0; x < this->order_; x++) {
        this->lowpass_[x].Mute();
        this->highpass_[x].Mute();
    }
}

void IIR_NOrder_BW_BP::SetBPF(
    const float high_cut, const float low_cut, const uint32_t sampling_rate
) {
    for (uint32_t x = 0; x < this->order_; x++) {
        this->lowpass_[x].SetLowPassFilterBW(low_cut, sampling_rate);
        this->highpass_[x].SetHighPassFilterBW(high_cut, sampling_rate);
    }
}
