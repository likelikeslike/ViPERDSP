#pragma once

#include <cstdint>

class SoftwareLimiter {
public:
    SoftwareLimiter();

    float Process(float sample);
    void Reset();
    void SetGate(float gate);

private:
    bool ready_;

    uint32_t write_index_;

    float gate_;
    float target_gain_;
    float gain_envelope_;
    float smoothed_gain_;
    float arr256_[256];
    float arr512_[512];
};
