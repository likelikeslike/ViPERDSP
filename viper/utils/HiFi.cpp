#include "HiFi.h"
#include "../constants.h"

HiFi::HiFi() :
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE),
    gain_(1.0f) {
    for (int i = 0; i < 2; i++) {
        buffers_[i] = new WaveBuffer(2, 0x800);
        filters_[i].lowpass = new IIR_NOrder_BW_LH(1);
        filters_[i].highpass = new IIR_NOrder_BW_LH(3);
        filters_[i].bandpass = new IIR_NOrder_BW_BP(3);
    }
    Reset();
}

HiFi::~HiFi() {
    for (int i = 0; i < 2; i++) {
        delete buffers_[i];
        delete filters_[i].lowpass;
        delete filters_[i].highpass;
        delete filters_[i].bandpass;
    }
}

void HiFi::Process(float *samples, const uint32_t size) {
    if (size > 0) {
        float *bp_buf = buffers_[0]->PushZerosGetBuffer(size);
        float *lp_buf = buffers_[1]->PushZerosGetBuffer(size);
        if (bp_buf == nullptr || lp_buf == nullptr) {
            Reset();
            return;
        }

        for (uint32_t i = 0; i < size * 2; i++) {
            const uint32_t index = i % 2;
            const float out1 = FilterLH(filters_[index].lowpass, samples[i]);
            const float out2 = FilterLH(filters_[index].highpass, samples[i]);
            const float out3 = FilterBP(filters_[index].bandpass, samples[i]);
            samples[i] = out2;
            lp_buf[i] = out1;
            bp_buf[i] = out3;
        }
        const float *bp_out = buffers_[0]->GetBuffer();
        const float *lp_out = buffers_[1]->GetBuffer();
        for (uint32_t i = 0; i < size * 2; i++) {
            const float hp = samples[i] * gain_ * 1.2f;
            const float bp = bp_out[i] * gain_;
            samples[i] = hp + bp + lp_out[i];
        }
        buffers_[0]->PopSamples(size, false);
        buffers_[1]->PopSamples(size, false);
    }
}

void HiFi::Reset() {
    for (const auto &filter : filters_) {
        filter.lowpass->SetLPF(120.0f, sampling_rate_);
        filter.lowpass->Mute();
        filter.highpass->SetHPF(1200.0f, sampling_rate_);
        filter.highpass->Mute();
        filter.bandpass->SetBPF(120.0f, 1200.0f, sampling_rate_);
        filter.bandpass->Mute();
    }
    buffers_[0]->Reset();
    buffers_[0]->PushZeros(sampling_rate_ / 400);
    buffers_[1]->Reset();
    buffers_[1]->PushZeros(sampling_rate_ / 200);
}

void HiFi::SetClarity(const float value) {
    gain_ = value;
}

void HiFi::SetSamplingRate(const uint32_t sampling_rate) {
    sampling_rate_ = sampling_rate;
    Reset();
}
