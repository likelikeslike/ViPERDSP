#include "DynamicSystem.h"
#include "../constants.h"

DynamicSystem::DynamicSystem() :
    enable_(false),
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE) {
    dynamic_bass_.SetSamplingRate(sampling_rate_);
    dynamic_bass_.Reset();
}

void DynamicSystem::Process(float *samples, const uint32_t size) {
    if (!enable_) return;

    dynamic_bass_.FilterSamples(samples, size);
}

void DynamicSystem::Reset() {
    dynamic_bass_.SetSamplingRate(sampling_rate_);
    dynamic_bass_.Reset();
}

void DynamicSystem::SetEnable(const bool enable) {
    if (enable_ != enable) {
        if (enable) {
            Reset();
        }
        enable_ = enable;
    }
}

void DynamicSystem::SetBassGain(const float gain) {
    dynamic_bass_.SetBassGain(gain);
}

void DynamicSystem::SetSamplingRate(const uint32_t sampling_rate) {
    if (sampling_rate_ != sampling_rate) {
        sampling_rate_ = sampling_rate;
        dynamic_bass_.SetSamplingRate(sampling_rate);
    }
}

void DynamicSystem::SetSideGain(const float gain_x, const float gain_y) {
    dynamic_bass_.SetSideGain(gain_x, gain_y);
}

void DynamicSystem::SetXCoeffs(const uint32_t low, const uint32_t high) {
    dynamic_bass_.SetFilterXPassFrequency(low, high);
}

void DynamicSystem::SetYCoeffs(const uint32_t low, const uint32_t high) {
    dynamic_bass_.SetFilterYPassFrequency(low, high);
}
