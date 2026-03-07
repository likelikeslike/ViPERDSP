#include "SoftwareLimiter.h"
#include <algorithm>
#include <cmath>
#include <cstring>

SoftwareLimiter::SoftwareLimiter() {
    this->ready = false;
    this->writeIndex = 0;
    this->gainEnvelope = 1.0;
    this->gate = 0.999999;
    this->smoothedGain = 1.0;
    this->targetGain = 1.0;

    Reset();
}

float SoftwareLimiter::Process(float sample) {
    if (!std::isfinite(sample)) sample = 0.0f;

    float peakVal = std::abs(sample);
    bool aboveGate = peakVal >= this->gate;

    if (aboveGate && !this->ready) {
        memset(this->arr512, 0, sizeof(this->arr512));
        this->ready = true;
    }

    if (this->ready) {
        uint32_t idx = this->writeIndex;
        for (uint32_t level = 8; level > 0; --level) {
            int offset = 2 << (level & 0xff);
            uint32_t sibling = idx ^ 1;
            this->arr512[512 + idx - offset] = peakVal;
            if (peakVal < this->arr512[512 + sibling - offset]) {
                peakVal = this->arr512[512 + sibling - offset];
            }
            idx /= 2;
        }
    }

    float newTargetGain = this->targetGain;
    if (this->ready && peakVal > this->gate) {
        newTargetGain = this->gate / peakVal;
    } else if (this->ready && peakVal <= this->gate) {
        this->ready = false;
    }

    uint32_t wi = this->writeIndex;
    this->arr256[wi] = sample;
    wi = (wi + 1) % 256;
    this->writeIndex = wi;
    float delayed = this->arr256[wi];

    float envelope = this->gainEnvelope * 0.9999 + 0.0001;
    float smoothed = newTargetGain * 0.0999 + this->smoothedGain * 0.8999;
    this->smoothedGain = smoothed;
    float gain = std::min(envelope, smoothed);
    this->gainEnvelope = gain;

    float out = delayed * gain;
    if (std::abs(out) >= this->gate) {
        gain = this->gate / std::abs(delayed);
        this->gainEnvelope = gain;
        out = delayed * gain;
    }

    return out;
}

void SoftwareLimiter::Reset() {
    memset(this->arr256, 0, sizeof(this->arr256));
    memset(this->arr512, 0, sizeof(this->arr512));
    this->ready = false;
    this->writeIndex = 0;
    this->gainEnvelope = 1.0;
    this->smoothedGain = 1.0;
    this->targetGain = 1.0;
}

void SoftwareLimiter::SetGate(float gate) {
    this->gate = gate;
}
