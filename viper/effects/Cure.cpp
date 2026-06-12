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

void Cure::SetPreset(const uint32_t value) {
    switch (value) {
        case 0: {
            constexpr Crossfeed::Preset preset = {.cutoff = 650, .feedback = 95};
            crossfeed_.SetPreset(preset);
            break;
        }
        case 1: {
            constexpr Crossfeed::Preset preset = {.cutoff = 700, .feedback = 60};
            crossfeed_.SetPreset(preset);
            break;
        }
        case 2: {
            constexpr Crossfeed::Preset preset = {.cutoff = 700, .feedback = 45};
            crossfeed_.SetPreset(preset);
            break;
        }
        default:;
    }
}

void Cure::SetSamplingRate(const uint32_t sampling_rate) {
    crossfeed_.SetSamplingRate(sampling_rate);
    pass_filter_.SetSamplingRate(sampling_rate);
}
