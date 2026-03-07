#include "TimeConstDelay.h"
#include <cmath>

TimeConstDelay::TimeConstDelay() {
    this->offset = 0;
    this->sampleCount = 0;
}

float TimeConstDelay::ProcessSample(float sample) {
    float val = this->samples[this->offset];
    this->samples[this->offset] = sample;
    this->offset++;
    if (this->offset >= this->sampleCount) {
        this->offset = 0;
    }
    return val;
}

void TimeConstDelay::SetParameters(uint32_t samplingRate, float delay) {
    this->sampleCount = (uint32_t) ((float) samplingRate * delay);
    this->samples.resize(this->sampleCount);
    this->offset = 0;
}
