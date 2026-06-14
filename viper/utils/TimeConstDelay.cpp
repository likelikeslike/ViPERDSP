#include "TimeConstDelay.h"

TimeConstDelay::TimeConstDelay() :
    offset_(0),
    sample_count_(0) {}

float TimeConstDelay::ProcessSample(const float sample) {
    const float val = samples_[offset_];
    samples_[offset_] = sample;
    offset_++;
    if (offset_ >= sample_count_) {
        offset_ = 0;
    }
    return val;
}

void TimeConstDelay::SetParameters(const uint32_t sampling_rate, const float delay) {
    sample_count_ = static_cast<uint32_t>(static_cast<float>(sampling_rate) * delay);
    samples_.assign(sample_count_, 0.0f);
    offset_ = 0;
}
