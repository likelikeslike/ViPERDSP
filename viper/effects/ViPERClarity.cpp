#include "ViPERClarity.h"
#include "../constants.h"

ViPERClarity::ViPERClarity() :
    enable_(false),
    process_mode_(NATURAL),
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE),
    gain_(0.0f) {
    for (auto &high_shelf : high_shelf_) {
        high_shelf.SetFrequency(12000.0f);
        high_shelf.SetGain(1.0f);
        high_shelf.SetSamplingRate(VIPER_DEFAULT_SAMPLING_RATE);
    }
    Reset();
}

void ViPERClarity::Process(float *samples, const uint32_t size) {
    if (!enable_) return;

    switch (process_mode_) {
        case NATURAL: {
            noise_sharpening_.Process(samples, size);
            break;
        }
        case OZONE: {
            for (uint32_t i = 0; i < size * 2; i += 2) {
                samples[i] = static_cast<float>(high_shelf_[0].Process(samples[i]));
                samples[i + 1] =
                    static_cast<float>(high_shelf_[1].Process(samples[i + 1]));
            }
            break;
        }
        case XHIFI: {
            hifi_.Process(samples, size);
            break;
        }
    }
}

void ViPERClarity::Reset() {
    noise_sharpening_.SetSamplingRate(sampling_rate_);
    noise_sharpening_.Reset();
    SetClarityToFilter();
    for (auto &high_shelf : high_shelf_) {
        high_shelf.SetFrequency(8250.0f);
        high_shelf.SetSamplingRate(sampling_rate_);
    }
    hifi_.SetSamplingRate(sampling_rate_);
    hifi_.Reset();
}

void ViPERClarity::SetEnable(const bool enable) {
    if (enable_ != enable) {
        if (enable) {
            Reset();
        }
        enable_ = enable;
    }
}

void ViPERClarity::SetProcessMode(const ClarityMode mode) {
    if (process_mode_ != mode) {
        process_mode_ = mode;
        Reset();
    }
}

void ViPERClarity::SetClarityGain(const float value) {
    gain_ = value;
    if (process_mode_ == OZONE) {
        Reset();
    } else {
        SetClarityToFilter();
    }
}

void ViPERClarity::SetClarityToFilter() {
    noise_sharpening_.SetGain(gain_);
    high_shelf_[0].SetGain(gain_ + 1.0f);
    high_shelf_[1].SetGain(gain_ + 1.0f);
    hifi_.SetClarity(gain_ + 1.0f);
}

void ViPERClarity::SetSamplingRate(const uint32_t sampling_rate) {
    if (sampling_rate_ != sampling_rate) {
        sampling_rate_ = sampling_rate;
        Reset();
    }
}
