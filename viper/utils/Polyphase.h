#pragma once

#include "FIR.h"
#include "WaveBuffer.h"

class Polyphase {
public:
    explicit Polyphase(int coeff_type);

    uint32_t Process(float *samples, uint32_t size);
    void Reset();

    [[nodiscard]] uint32_t GetLatency() const;

    void SetSamplingRate(uint32_t sampling_rate);

private:
    uint32_t sampling_rate_;
    uint32_t latency_;

    float buffer_[0x7e0];

    FIR fir1_;
    FIR fir2_;
    WaveBuffer wave_buffer1_;
    WaveBuffer wave_buffer2_;
};
