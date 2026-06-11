#include "Cure.h"

Cure::Cure() :
    enabled_(false) {
    Reset();
}

void Cure::Process(float *buffer, const uint32_t size) {
    if (!enabled_) return;

    crossfeed_.ProcessFrames(buffer, size);
    pass_filter_.ProcessFrames(buffer, size);
}

void Cure::Reset() {
    crossfeed_.Reset();
    pass_filter_.Reset();
}

uint32_t Cure::GetCutoff() const {
    return crossfeed_.GetCutoff();
}

float Cure::GetFeedback() const {
    return crossfeed_.GetFeedback();
}

float Cure::GetLevelDelay() const {
    return crossfeed_.GetLevelDelay();
}

Crossfeed::Preset Cure::GetPreset() const {
    return crossfeed_.GetPreset();
}

void Cure::SetEnable(const bool enable) {
    if (enabled_ != enable) {
        if (enable) {
            Reset();
        }
        enabled_ = enable;
    }
}

void Cure::SetCutoff(const uint32_t value) {
    crossfeed_.SetCutoff(value);
}

void Cure::SetFeedback(const float value) {
    crossfeed_.SetFeedback(value);
}

void Cure::SetPreset(const Crossfeed::Preset preset) {
    crossfeed_.SetPreset(preset);
}

void Cure::SetSamplingRate(const uint32_t sampling_rate) {
    crossfeed_.SetSamplingRate(sampling_rate);
    pass_filter_.SetSamplingRate(sampling_rate);
}
