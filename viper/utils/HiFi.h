#pragma once

#include "IIR_NOrder_BW_BP.h"
#include "IIR_NOrder_BW_LH.h"
#include "WaveBuffer.h"

class HiFi {
public:
    HiFi();
    ~HiFi();

    void Process(float *samples, uint32_t size);
    void Reset();

    void SetClarity(float value);
    void SetSamplingRate(uint32_t sampling_rate);

private:
    uint32_t sampling_rate_;

    float gain_;

    WaveBuffer *buffers_[2];
    struct {
        IIR_NOrder_BW_LH *lowpass;
        IIR_NOrder_BW_LH *highpass;
        IIR_NOrder_BW_BP *bandpass;
    } filters_[2];
};
