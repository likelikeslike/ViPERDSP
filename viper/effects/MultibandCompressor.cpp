#include "MultibandCompressor.h"
#include "../constants.h"

static constexpr float kDefault3BandFreqs[] = {200.0f, 4000.0f};
static constexpr float kDefault5BandFreqs[] = {120.0f, 500.0f, 4000.0f, 8000.0f};
static constexpr float kButterworthQ = 0.7071f;

MultibandCompressor::MultibandCompressor() :
    enable_(false),
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE),
    band_count_(3) {
    crossover_freqs_[0] = kDefault3BandFreqs[0];
    crossover_freqs_[1] = kDefault3BandFreqs[1];
    crossover_freqs_[2] = 0.0f;
    crossover_freqs_[3] = 0.0f;

    for (uint32_t i = 0; i < kMaxBands; i++) {
        compressors_[i].SetSamplingRate(sampling_rate_);
        compressors_[i].Reset();
    }

    ConfigureCrossovers();
}

void MultibandCompressor::Process(float *samples, const uint32_t size) {
    if (!enable_) return;
    if (size == 0) return;

    const uint32_t num_crossovers = band_count_ - 1;
    const uint32_t frame_count = size * 2;

    for (uint32_t b = 0; b < band_count_; b++) {
        if (band_buffers_[b].size() < frame_count) {
            band_buffers_[b].resize(frame_count);
        }
    }

    for (uint32_t b = 0; b < band_count_; b++) {
        for (uint32_t i = 0; i < frame_count; i += 2) {
            auto sample_l = static_cast<double>(samples[i]);
            auto sample_r = static_cast<double>(samples[i + 1]);

            if (b == 0) {
                sample_l = lowpass_la_[0].ProcessSample(sample_l);
                sample_l = lowpass_lb_[0].ProcessSample(sample_l);
                sample_r = lowpass_ra_[0].ProcessSample(sample_r);
                sample_r = lowpass_rb_[0].ProcessSample(sample_r);
            } else if (b == num_crossovers) {
                sample_l = highpass_la_[num_crossovers - 1].ProcessSample(sample_l);
                sample_l = highpass_lb_[num_crossovers - 1].ProcessSample(sample_l);
                sample_r = highpass_ra_[num_crossovers - 1].ProcessSample(sample_r);
                sample_r = highpass_rb_[num_crossovers - 1].ProcessSample(sample_r);
            } else {
                sample_l = highpass_la_[b - 1].ProcessSample(sample_l);
                sample_l = highpass_lb_[b - 1].ProcessSample(sample_l);
                sample_l = lowpass_la_[b].ProcessSample(sample_l);
                sample_l = lowpass_lb_[b].ProcessSample(sample_l);
                sample_r = highpass_ra_[b - 1].ProcessSample(sample_r);
                sample_r = highpass_rb_[b - 1].ProcessSample(sample_r);
                sample_r = lowpass_ra_[b].ProcessSample(sample_r);
                sample_r = lowpass_rb_[b].ProcessSample(sample_r);
            }

            band_buffers_[b][i] = static_cast<float>(sample_l);
            band_buffers_[b][i + 1] = static_cast<float>(sample_r);
        }

        compressors_[b].Process(band_buffers_[b].data(), size);
    }

    for (uint32_t i = 0; i < frame_count; i += 2) {
        float sum_l = 0.0f;
        float sum_r = 0.0f;
        for (uint32_t b = 0; b < band_count_; b++) {
            sum_l += band_buffers_[b][i];
            sum_r += band_buffers_[b][i + 1];
        }
        samples[i] = sum_l;
        samples[i + 1] = sum_r;
    }
}

void MultibandCompressor::Reset() {
    for (uint32_t i = 0; i < kMaxBands; i++) {
        compressors_[i].Reset();
    }

    for (uint32_t i = 0; i < kMaxCrossovers; i++) {
        lowpass_la_[i].Reset();
        lowpass_lb_[i].Reset();
        lowpass_ra_[i].Reset();
        lowpass_rb_[i].Reset();
        highpass_la_[i].Reset();
        highpass_lb_[i].Reset();
        highpass_ra_[i].Reset();
        highpass_rb_[i].Reset();
    }
    ConfigureCrossovers();
}

void MultibandCompressor::SetEnable(const bool enable) {
    if (enable_ != enable) {
        if (enable) Reset();
        enable_ = enable;
    }
}

void MultibandCompressor::SetBandCount(const uint32_t count) {
    if (count != 3 && count != 5) return;
    if (band_count_ != count) {
        band_count_ = count;
        if (count == 3) {
            crossover_freqs_[0] = kDefault3BandFreqs[0];
            crossover_freqs_[1] = kDefault3BandFreqs[1];
            crossover_freqs_[2] = 0.0f;
            crossover_freqs_[3] = 0.0f;
        } else {
            crossover_freqs_[0] = kDefault5BandFreqs[0];
            crossover_freqs_[1] = kDefault5BandFreqs[1];
            crossover_freqs_[2] = kDefault5BandFreqs[2];
            crossover_freqs_[3] = kDefault5BandFreqs[3];
        }
        Reset();
    }
}

void MultibandCompressor::SetCrossoverFrequency(
    const uint32_t index, const float frequency
) {
    const uint32_t num_crossovers = band_count_ - 1;
    if (index >= num_crossovers) return;
    if (crossover_freqs_[index] != frequency) {
        crossover_freqs_[index] = frequency;
        ConfigureCrossovers();
    }
}

void MultibandCompressor::SetSamplingRate(const uint32_t sampling_rate) {
    if (sampling_rate_ != sampling_rate) {
        sampling_rate_ = sampling_rate;
        for (uint32_t i = 0; i < kMaxBands; i++) {
            compressors_[i].SetSamplingRate(sampling_rate_);
        }
        ConfigureCrossovers();
    }
}

auto MultibandCompressor::SetBandEnable(const uint32_t band, const bool enable) -> void {
    if (band >= band_count_) return;
    compressors_[band].SetEnable(enable);
}

void MultibandCompressor::SetBandThreshold(const uint32_t band, const float value) {
    if (band >= band_count_) return;
    compressors_[band].SetThreshold(value);
}

void MultibandCompressor::SetBandRatio(const uint32_t band, const float value) {
    if (band >= band_count_) return;
    compressors_[band].SetRatio(value);
}

void MultibandCompressor::SetBandKnee(const uint32_t band, const float value) {
    if (band >= band_count_) return;
    compressors_[band].SetKnee(value);
}

void MultibandCompressor::SetBandKneeAuto(const uint32_t band, const bool enable) {
    if (band >= band_count_) return;
    compressors_[band].SetKneeAuto(enable);
}

void MultibandCompressor::SetBandGain(const uint32_t band, const float value) {
    if (band >= band_count_) return;
    compressors_[band].SetGain(value);
}

void MultibandCompressor::SetBandGainAuto(const uint32_t band, const bool enable) {
    if (band >= band_count_) return;
    compressors_[band].SetGainAuto(enable);
}

void MultibandCompressor::SetBandAttack(const uint32_t band, const float value) {
    if (band >= band_count_) return;
    compressors_[band].SetAttack(value);
}

void MultibandCompressor::SetBandAttackAuto(const uint32_t band, const bool enable) {
    if (band >= band_count_) return;
    compressors_[band].SetAttackAuto(enable);
}

void MultibandCompressor::SetBandRelease(const uint32_t band, const float value) {
    if (band >= band_count_) return;
    compressors_[band].SetRelease(value);
}

void MultibandCompressor::SetBandReleaseAuto(const uint32_t band, const bool enable) {
    if (band >= band_count_) return;
    compressors_[band].SetReleaseAuto(enable);
}

void MultibandCompressor::SetBandKneeMulti(const uint32_t band, const float value) {
    if (band >= band_count_) return;
    compressors_[band].SetKneeMulti(value);
}

void MultibandCompressor::SetBandMaxAttack(const uint32_t band, const float value) {
    if (band >= band_count_) return;
    compressors_[band].SetMaxAttack(value);
}

void MultibandCompressor::SetBandMaxRelease(const uint32_t band, const float value) {
    if (band >= band_count_) return;
    compressors_[band].SetMaxRelease(value);
}

void MultibandCompressor::SetBandCrest(const uint32_t band, const float value) {
    if (band >= band_count_) return;
    compressors_[band].SetCrest(value);
}

void MultibandCompressor::SetBandAdapt(const uint32_t band, const float value) {
    if (band >= band_count_) return;
    compressors_[band].SetAdapt(value);
}

void MultibandCompressor::SetBandNoClip(const uint32_t band, const bool enable) {
    if (band >= band_count_) return;
    compressors_[band].SetNoClip(enable);
}

void MultibandCompressor::ConfigureCrossovers() {
    const uint32_t num_crossovers = band_count_ - 1;
    for (uint32_t i = 0; i < num_crossovers; i++) {
        lowpass_la_[i].RefreshFilter(
            MultiBiquad::LOW_PASS,
            1.0f,
            crossover_freqs_[i],
            sampling_rate_,
            kButterworthQ,
            false
        );
        lowpass_lb_[i].RefreshFilter(
            MultiBiquad::LOW_PASS,
            1.0f,
            crossover_freqs_[i],
            sampling_rate_,
            kButterworthQ,
            false
        );
        lowpass_ra_[i].RefreshFilter(
            MultiBiquad::LOW_PASS,
            1.0f,
            crossover_freqs_[i],
            sampling_rate_,
            kButterworthQ,
            false
        );
        lowpass_rb_[i].RefreshFilter(
            MultiBiquad::LOW_PASS,
            1.0f,
            crossover_freqs_[i],
            sampling_rate_,
            kButterworthQ,
            false
        );
        highpass_la_[i].RefreshFilter(
            MultiBiquad::HIGH_PASS,
            1.0f,
            crossover_freqs_[i],
            sampling_rate_,
            kButterworthQ,
            false
        );
        highpass_lb_[i].RefreshFilter(
            MultiBiquad::HIGH_PASS,
            1.0f,
            crossover_freqs_[i],
            sampling_rate_,
            kButterworthQ,
            false
        );
        highpass_ra_[i].RefreshFilter(
            MultiBiquad::HIGH_PASS,
            1.0f,
            crossover_freqs_[i],
            sampling_rate_,
            kButterworthQ,
            false
        );
        highpass_rb_[i].RefreshFilter(
            MultiBiquad::HIGH_PASS,
            1.0f,
            crossover_freqs_[i],
            sampling_rate_,
            kButterworthQ,
            false
        );
    }
}
