#include "ViPERBass.h"
#include "../constants.h"
#include <cmath>

ViPERBass::ViPERBass() :
    enable_(false),
    process_mode_(NATURAL_BASS),
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE),
    speaker_(60),
    sampling_rate_period_(1.0f / VIPER_DEFAULT_SAMPLING_RATE),
    anti_pop_(0.0f),
    bass_factor_(0.0f),
    bass_factor_smoothed_(0.0f),
    smoothing_coeff_(0.0f),
    polyphase_(2),
    wave_buffer_(2, 4096) {
    for (auto &biquad : biquad_) {
        biquad.Reset();
        biquad.SetLowPassParameter(static_cast<float>(speaker_), sampling_rate_, 0.53f);
    }
    subwoofer_.SetBassGain(sampling_rate_, 0.0f);
    Reset();
}

void ViPERBass::Process(float *samples, const uint32_t size) {
    if (!enable_) return;
    if (size == 0) return;

    constexpr float KNEE = 0.5f;
    auto sat_mix = [](float &l, float &r) {
        const float a_l = std::fabs(l);
        const float a_r = std::fabs(r);
        const float drive = (a_l > a_r) ? a_l : a_r;
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
                float bass_l = static_cast<float>(biquad_[0].ProcessSample(samples[i]))
                               * bass_factor_smoothed_;
                float bass_r =
                    static_cast<float>(biquad_[1].ProcessSample(samples[i + 1]))
                    * bass_factor_smoothed_;
                if (anti_pop_ < 1.0f) {
                    bass_l *= anti_pop_;
                    bass_r *= anti_pop_;
                    float x = anti_pop_ + sampling_rate_period_;
                    if (x > 1.0f) x = 1.0f;
                    anti_pop_ = x;
                }
                float mixed_l = samples[i] + bass_l;
                float mixed_r = samples[i + 1] + bass_r;
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
                    buffer[buffer_offset - size + i] =
                        static_cast<float>(biquad_[0].ProcessSample(samples[i]));
                    buffer[buffer_offset - size + i + 1] =
                        static_cast<float>(biquad_[1].ProcessSample(samples[i + 1]));
                }

                if (polyphase_.Process(samples, size) == size) {
                    for (uint32_t i = 0; i < size * 2; i += 2) {
                        bass_factor_smoothed_ +=
                            (bass_factor_ - bass_factor_smoothed_) * smoothing_coeff_;
                        float bass_l = buffer[i] * bass_factor_smoothed_;
                        float bass_r = buffer[i + 1] * bass_factor_smoothed_;
                        if (anti_pop_ < 1.0f) {
                            bass_l *= anti_pop_;
                            bass_r *= anti_pop_;
                            float x = anti_pop_ + sampling_rate_period_;
                            if (x > 1.0f) x = 1.0f;
                            anti_pop_ = x;
                        }
                        float mixed_l = samples[i] + bass_l;
                        float mixed_r = samples[i + 1] + bass_r;
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
            if (anti_pop_ < 1.0f) {
                for (uint32_t i = 0; i < size * 2; i += 2) {
                    const float dry_l = samples[i];
                    const float dry_r = samples[i + 1];
                    float tmp_sample[2] = {dry_l, dry_r};
                    subwoofer_.Process(tmp_sample, 1);
                    samples[i] = dry_l + anti_pop_ * (tmp_sample[0] - dry_l);
                    samples[i + 1] = dry_r + anti_pop_ * (tmp_sample[1] - dry_r);
                    float x = anti_pop_ + sampling_rate_period_;
                    if (x > 1.0f) x = 1.0f;
                    anti_pop_ = x;
                }
            } else {
                subwoofer_.Process(samples, size);
            }
            break;
        }
    }
}

void ViPERBass::Reset() {
    polyphase_.SetSamplingRate(sampling_rate_);
    polyphase_.Reset();
    wave_buffer_.Reset();
    wave_buffer_.PushZeros(polyphase_.GetLatency());
    subwoofer_.SetBassGain(sampling_rate_, bass_factor_ * 2.5f);
    biquad_[0].SetLowPassParameter(static_cast<float>(speaker_), sampling_rate_, 0.53f);
    biquad_[1].SetLowPassParameter(static_cast<float>(speaker_), sampling_rate_, 0.53f);
    sampling_rate_period_ = 1.0f / static_cast<float>(sampling_rate_);
    anti_pop_ = 0.0f;
    smoothing_coeff_ =
        1.0f - std::exp(-1.0f / (0.030f * static_cast<float>(sampling_rate_)));
    bass_factor_smoothed_ = bass_factor_;
}

void ViPERBass::SetBassFactor(const float bass_factor) {
    if (bass_factor_ != bass_factor) {
        bass_factor_ = bass_factor;
        subwoofer_.SetBassGain(sampling_rate_, bass_factor_ * 2.5f);
    }
}

void ViPERBass::SetEnable(const bool enable) {
    if (enable_ != enable) {
        if (enable) Reset();
        enable_ = enable;
    }
}

void ViPERBass::SetProcessMode(const ProcessMode mode) {
    if (process_mode_ != mode) {
        process_mode_ = mode;
        Reset();
    }
}

void ViPERBass::SetSamplingRate(const uint32_t sampling_rate) {
    if (sampling_rate_ != sampling_rate) {
        sampling_rate_ = sampling_rate;
        sampling_rate_period_ = 1.0f / static_cast<float>(sampling_rate);
        polyphase_.SetSamplingRate(sampling_rate_);
        biquad_[0].SetLowPassParameter(
            static_cast<float>(speaker_), sampling_rate_, 0.53f
        );
        biquad_[1].SetLowPassParameter(
            static_cast<float>(speaker_), sampling_rate_, 0.53f
        );
        subwoofer_.SetBassGain(sampling_rate_, bass_factor_ * 2.5f);
    }
}

void ViPERBass::SetSpeaker(const uint32_t speaker) {
    if (speaker_ != speaker) {
        speaker_ = speaker;
        biquad_[0].SetLowPassParameter(
            static_cast<float>(speaker_), sampling_rate_, 0.53f
        );
        biquad_[1].SetLowPassParameter(
            static_cast<float>(speaker_), sampling_rate_, 0.53f
        );
    }
}

void ViPERBass::SetAntiPop(const bool enable) {
    if (enable) {
        anti_pop_ = 0.0f;
    } else {
        anti_pop_ = 1.0f;
    }
}
