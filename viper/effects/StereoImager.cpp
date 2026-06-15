#include "StereoImager.h"
#include "../constants.h"

static constexpr float kButterworthQ = 0.7071f;

StereoImager::StereoImager() :
    enable_(false),
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE) {
    band_widths_[0] = 1.0f;
    band_widths_[1] = 1.0f;
    band_widths_[2] = 1.0f;

    crossover_freqs_[0] = 200.0f;
    crossover_freqs_[1] = 4000.0f;

    ConfigureCrossovers();
}

void StereoImager::Process(float *samples, const uint32_t size) {
    if (!enable_) return;
    if (size == 0) return;

    const uint32_t frame_count = size * 2;

    for (uint32_t b = 0; b < kNumBands; b++) {
        if (band_buffers_[b].size() < frame_count) {
            band_buffers_[b].resize(frame_count);
        }
    }

    for (uint32_t b = 0; b < kNumBands; b++) {
        for (uint32_t i = 0; i < frame_count; i += 2) {
            double sample_l = samples[i];
            double sample_r = samples[i + 1];

            if (b == 0) {
                sample_l = lowpass_la_[0].ProcessSample(sample_l);
                sample_l = lowpass_lb_[0].ProcessSample(sample_l);
                sample_r = lowpass_ra_[0].ProcessSample(sample_r);
                sample_r = lowpass_rb_[0].ProcessSample(sample_r);
            } else if (b == 2) {
                sample_l = highpass_la_[1].ProcessSample(sample_l);
                sample_l = highpass_lb_[1].ProcessSample(sample_l);
                sample_r = highpass_ra_[1].ProcessSample(sample_r);
                sample_r = highpass_rb_[1].ProcessSample(sample_r);
            } else {
                sample_l = highpass_la_[0].ProcessSample(sample_l);
                sample_l = highpass_lb_[0].ProcessSample(sample_l);
                sample_l = lowpass_la_[1].ProcessSample(sample_l);
                sample_l = lowpass_lb_[1].ProcessSample(sample_l);
                sample_r = highpass_ra_[0].ProcessSample(sample_r);
                sample_r = highpass_rb_[0].ProcessSample(sample_r);
                sample_r = lowpass_ra_[1].ProcessSample(sample_r);
                sample_r = lowpass_rb_[1].ProcessSample(sample_r);
            }

            const auto f_l = static_cast<float>(sample_l);
            const auto f_r = static_cast<float>(sample_r);

            const float mid = (f_l + f_r) * 0.5f;
            float side = (f_l - f_r) * 0.5f;
            side *= band_widths_[b];

            band_buffers_[b][i] = mid + side;
            band_buffers_[b][i + 1] = mid - side;
        }
    }

    for (uint32_t i = 0; i < frame_count; i += 2) {
        float sum_l = 0.0f;
        float sum_r = 0.0f;
        for (uint32_t b = 0; b < kNumBands; b++) {
            sum_l += band_buffers_[b][i];
            sum_r += band_buffers_[b][i + 1];
        }
        samples[i] = sum_l;
        samples[i + 1] = sum_r;
    }
}

void StereoImager::Reset() {
    for (uint32_t i = 0; i < kNumCrossovers; i++) {
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

void StereoImager::SetEnable(const bool enable) {
    if (enable_ != enable) {
        if (enable) Reset();
        enable_ = enable;
    }
}

void StereoImager::SetLowWidth(const float value) {
    band_widths_[0] = value / 100.0f;
}

void StereoImager::SetMidWidth(const float value) {
    band_widths_[1] = value / 100.0f;
}

void StereoImager::SetHighWidth(const float value) {
    band_widths_[2] = value / 100.0f;
}

void StereoImager::SetLowCrossover(const float value) {
    if (crossover_freqs_[0] != value) {
        crossover_freqs_[0] = value;
        ConfigureCrossovers();
    }
}

void StereoImager::SetHighCrossover(const float value) {
    if (crossover_freqs_[1] != value) {
        crossover_freqs_[1] = value;
        ConfigureCrossovers();
    }
}

void StereoImager::SetSamplingRate(const uint32_t sampling_rate) {
    if (sampling_rate_ != sampling_rate) {
        sampling_rate_ = sampling_rate;
        ConfigureCrossovers();
    }
}

void StereoImager::ConfigureCrossovers() {
    for (uint32_t i = 0; i < kNumCrossovers; i++) {
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
