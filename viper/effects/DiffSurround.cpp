#include "DiffSurround.h"
#include "../constants.h"

DiffSurround::DiffSurround() :
    enable_(false),
    reverse_(false),
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE),
    delay_time_(0.0f),
    wet_dry_mix_(1.0f),
    lp_cutoff_(0.0f),
    buffers_({WaveBuffer(1, 0x1000), WaveBuffer(1, 0x1000)}) {
    Reset();
}

void DiffSurround::Process(float *samples, const uint32_t size) {
    if (!enable_) return;

    float *bufs[2];
    float *out_bufs[2];

    bufs[0] = buffers_[0].PushZerosGetBuffer(size);
    bufs[1] = buffers_[1].PushZerosGetBuffer(size);

    for (uint32_t i = 0; i < size * 2; i++) {
        bufs[i % 2][i / 2] = samples[i];
    }

    out_bufs[0] = buffers_[0].GetBuffer();
    out_bufs[1] = buffers_[1].GetBuffer();

    if (wet_dry_mix_ >= 1.0f && lp_cutoff_ <= 0.0f) {
        for (uint32_t i = 0; i < size * 2; i++) {
            samples[i] = out_bufs[i % 2][i / 2];
        }
    } else {
        const int delayed_ch = reverse_ ? 0 : 1;
        const int direct_ch = 1 - delayed_ch;
        const float wet = wet_dry_mix_;
        const float dry = 1.0f - wet;

        for (uint32_t i = 0; i < size; i++) {
            const float direct_sample = out_bufs[direct_ch][i];
            float delayed_sample = out_bufs[delayed_ch][i];

            if (lp_cutoff_ > 0.0f) {
                delayed_sample =
                    static_cast<float>(lp_filter_.ProcessSample(delayed_sample));
            }

            samples[i * 2 + direct_ch] = direct_sample;
            samples[i * 2 + delayed_ch] = dry * direct_sample + wet * delayed_sample;
        }
    }

    buffers_[0].PopSamples(size, false);
    buffers_[1].PopSamples(size, false);
}

void DiffSurround::Reset() {
    buffers_[0].Reset();
    buffers_[1].Reset();

    const auto delay_samples =
        static_cast<uint32_t>(delay_time_ / 1000.0 * sampling_rate_);
    buffers_[reverse_ ? 0 : 1].PushZeros(delay_samples);

    lp_filter_.Reset();
    if (lp_cutoff_ > 0.0f) {
        lp_filter_.RefreshFilter(
            MultiBiquad::FilterType::LOW_PASS,
            0.0f,
            lp_cutoff_,
            sampling_rate_,
            0.7071f,
            false
        );
    }
}

void DiffSurround::SetEnable(const bool enable) {
    if (enable_ != enable) {
        if (!enable_) {
            Reset();
        }
        enable_ = enable;
    }
}

void DiffSurround::SetDelayTime(const float value) {
    if (delay_time_ != value) {
        delay_time_ = value;
        Reset();
    }
}

void DiffSurround::SetReverse(const bool value) {
    if (reverse_ != value) {
        reverse_ = value;
        Reset();
    }
}

void DiffSurround::SetWetDryMix(float value) {
    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;
    wet_dry_mix_ = value;
}

void DiffSurround::SetLPCutoff(float value) {
    if (value < 0.0f) value = 0.0f;
    if (value > 20000.0f) value = 20000.0f;
    if (lp_cutoff_ != value) {
        lp_cutoff_ = value;
        if (value > 0.0f) {
            lp_filter_.RefreshFilter(
                MultiBiquad::FilterType::LOW_PASS,
                0.0f,
                value,
                sampling_rate_,
                0.7071f,
                false
            );
        }
    }
}

void DiffSurround::SetSamplingRate(const uint32_t sampling_rate) {
    if (sampling_rate_ != sampling_rate) {
        sampling_rate_ = sampling_rate;
        Reset();
    }
}
