#pragma once

#include <cstdint>

class SoftwareLimiter {
public:
    SoftwareLimiter();

    float Process(float sample);
    void Reset();
    void SetGate(float gate);

private:
    float gate;
    float targetGain;
    float gainEnvelope;
    float smoothedGain;
    float arr256[256];
    float arr512[512];
    uint32_t writeIndex;
    bool ready;
};
