#include "ViPERBassMono.h"
#include "../constants.h"
#include <cmath>

ViPERBassMono::ViPERBassMono() :
    enable_(false),
    process_mode_(NATURAL_BASS),
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE),
    frequency_(60),
    sampling_rate_period_(1.0f / VIPER_DEFAULT_SAMPLING_RATE),
    anti_pop_(0.0f),
    bass_factor_(0.0f),
    bass_factor_smoothed_(0.0f),
    smoothing_coeff_(0.0f),
    polyphase_(2),
    wave_buffer_(1, 4096) {
    biquad_.Reset();
    biquad_.SetLowPassParameter(static_cast<float>(frequency_), sampling_rate_, 0.53f);
    subwoofer_.SetBassGain(sampling_rate_, 0.0f);
    Reset();
}

void ViPERBassMono::Process(float *samples, const uint32_t size) {
    if (!enable_) return;
    if (size == 0) return;

    if (anti_pop_ < 1.0f) {
        for (uint32_t i = 0; i < size * 2; i += 2) {
            samples[i] *= anti_pop_;
            samples[i + 1] *= anti_pop_;

            float x = anti_pop_ + sampling_rate_period_;
            if (x > 1.0f) x = 1.0f;
            anti_pop_ = x;
        }
    }

    constexpr float KNEE = 0.5f;
    auto sat_mix = [](float &l, float &r) {
        const float a_l = std::fabs(l);
        const float a_r = std::fabs(r);
        const float drive = a_l > a_r ? a_l : a_r;
        if (drive <= KNEE) return;
        const float over = drive - KNEE;
        const float shaped = KNEE + over / std::sqrt(1.0f + over * over);
        const float scale = shaped / drive;
        l *= scale;
        r *= scale;
    };

    switch (process_mode_) {
        case NATURAL_BASS: {
            for (uint32_t i = 0; i < size * 2; i += 2) {
                bass_factor_smoothed_ +=
                    (bass_factor_ - bass_factor_smoothed_) * smoothing_coeff_;
                const double sample = (static_cast<double>(samples[i])
                                       + static_cast<double>(samples[i + 1]))
                                      / 2.0;
                const float x = static_cast<float>(biquad_.ProcessSample(sample))
                                * bass_factor_smoothed_;
                float mixed_l = samples[i] + x;
                float mixed_r = samples[i + 1] + x;
                sat_mix(mixed_l, mixed_r);
                samples[i] = mixed_l;
                samples[i + 1] = mixed_r;
            }
            break;
        }
        case PURE_BASS_PLUS: {
            if (wave_buffer_.PushSamples(samples, size)) {
                float *buffer = wave_buffer_.GetBuffer();
                const uint32_t buffer_offset = wave_buffer_.GetBufferOffset();

                for (uint32_t i = 0; i < size * 2; i += 2) {
                    const double sample = (static_cast<double>(samples[i])
                                           + static_cast<double>(samples[i + 1]))
                                          / 2.0;
                    const auto x = static_cast<float>(biquad_.ProcessSample(sample));
                    buffer[buffer_offset - size + i / 2] = x;
                }

                if (polyphase_.Process(samples, size) == size) {
                    for (uint32_t i = 0; i < size * 2; i += 2) {
                        bass_factor_smoothed_ +=
                            (bass_factor_ - bass_factor_smoothed_) * smoothing_coeff_;
                        const float x = buffer[i / 2] * bass_factor_smoothed_;
                        float mixed_l = samples[i] + x;
                        float mixed_r = samples[i + 1] + x;
                        sat_mix(mixed_l, mixed_r);
                        samples[i] = mixed_l;
                        samples[i + 1] = mixed_r;
                    }
                    wave_buffer_.PopSamples(size, true);
                }
            }
            break;
        }
        case SUBWOOFER: {
            subwoofer_.Process(samples, size);
            break;
        }
    }
}

void ViPERBassMono::Reset() {
    polyphase_.SetSamplingRate(sampling_rate_);
    polyphase_.Reset();
    wave_buffer_.Reset();
    wave_buffer_.PushZeros(polyphase_.GetLatency());
    subwoofer_.SetBassGain(sampling_rate_, bass_factor_ * 2.5f);
    biquad_.SetLowPassParameter(static_cast<float>(frequency_), sampling_rate_, 0.53f);
    sampling_rate_period_ = 1.0f / static_cast<float>(sampling_rate_);
    anti_pop_ = 0.0f;
    smoothing_coeff_ =
        1.0f - std::exp(-1.0f / (0.030f * static_cast<float>(sampling_rate_)));
    bass_factor_smoothed_ = bass_factor_;
}

void ViPERBassMono::SetEnable(const bool enable) {
    if (enable_ != enable) {
        if (enable) Reset();
        enable_ = enable;
    }
}

void ViPERBassMono::SetProcessMode(const ProcessMode mode) {
    if (process_mode_ != mode) {
        process_mode_ = mode;
        Reset();
    }
}

void ViPERBassMono::SetBassFactor(const float value) {
    if (bass_factor_ != value) {
        bass_factor_ = value;
        subwoofer_.SetBassGain(sampling_rate_, bass_factor_ * 2.5f);
    }
}

void ViPERBassMono::SetFrequency(const uint32_t value) {
    if (frequency_ != value) {
        frequency_ = value;
        biquad_.SetLowPassParameter(
            static_cast<float>(frequency_), sampling_rate_, 0.53f
        );
    }
}

void ViPERBassMono::SetAntiPop(const bool enable) {
    if (enable) {
        anti_pop_ = 0.0f;
    } else {
        anti_pop_ = 1.0f;
    }
}

void ViPERBassMono::SetSamplingRate(const uint32_t sampling_rate) {
    if (sampling_rate_ != sampling_rate) {
        sampling_rate_ = sampling_rate;
        sampling_rate_period_ = 1.0f / static_cast<float>(sampling_rate);
        polyphase_.SetSamplingRate(sampling_rate_);
        biquad_.SetLowPassParameter(
            static_cast<float>(frequency_), sampling_rate_, 0.53f
        );
        subwoofer_.SetBassGain(sampling_rate_, bass_factor_ * 2.5f);
    }
}
