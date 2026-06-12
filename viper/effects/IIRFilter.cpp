#include "IIRFilter.h"
#include "../constants.h"
#include <cmath>
#include <cstring>

IIRFilter::IIRFilter(const uint32_t bands) :
    enable_(false),
    bands_(0),
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE) {
    if (bands == 10 || bands == 15 || bands == 25 || bands == 31) {
        bands_ = bands;
        min_phase_iir_coeffs_.UpdateCoeffs(bands_, sampling_rate_);
    }

    for (auto &level_with_q : band_levels_with_q_) {
        level_with_q = 0.636f;
    }

    Reset();
}

void IIRFilter::Process(float *samples, const uint32_t size) {
    if (!enable_) return;

    const double *coeffs = min_phase_iir_coeffs_.GetCoefficients();
    if (coeffs == nullptr || size == 0) return;

    for (uint32_t i = 0; i < size; i++) {
        for (uint32_t j = 0; j < 2; j++) {
            const double sample = samples[i * 2 + j];
            double accumulated = 0.0;

            for (uint32_t k = 0; k < bands_; k++) {
                const uint32_t buf_idx = buf_index0_ + j * 8 + k * 16;
                buf_[buf_idx] = sample;

                const double coeff1 = coeffs[k * 4];
                const double coeff2 = coeffs[k * 4 + 1];
                const double coeff3 = coeffs[k * 4 + 2];

                const double a = coeff3 * buf_[buf_idx + (buf_index1_ + 3 - buf_index0_)];
                const double b =
                    coeff2 * (sample - buf_[buf_idx + (buf_index2_ - buf_index0_)]);
                const double c = coeff1 * buf_[buf_idx + (buf_index2_ - buf_index0_ + 3)];

                const double tmp = a + b - c;

                buf_[buf_idx + 3] = tmp;
                accumulated += tmp * band_levels_with_q_[k];
            }

            samples[i * 2 + j] = static_cast<float>(accumulated);
        }

        buf_index0_ = (buf_index0_ + 1) % 3;
        buf_index1_ = (buf_index1_ + 1) % 3;
        buf_index2_ = (buf_index2_ + 1) % 3;
    }
}

void IIRFilter::Reset() {
    memset(buf_, 0, sizeof(buf_));
    buf_index0_ = 2;
    buf_index1_ = 1;
    buf_index2_ = 0;
}

void IIRFilter::SetEnable(const bool enable) {
    if (enable_ != enable) {
        if (enable) {
            Reset();
        }
        enable_ = enable;
    }
}

void IIRFilter::SetBandCount(const uint32_t bands) {
    if (bands != 10 && bands != 15 && bands != 25 && bands != 31) return;
    if (bands_ == bands) return;
    bands_ = bands;
    min_phase_iir_coeffs_.UpdateCoeffs(bands_, sampling_rate_);
    for (auto &level_with_q : band_levels_with_q_) {
        level_with_q = 0.636f;
    }
    Reset();
}

void IIRFilter::SetBandLevel(const uint32_t band, const float level) {
    if (band > 30) return;
    const double band_level = pow(10.0, static_cast<double>(level) / 20.0);
    band_levels_with_q_[band] = static_cast<float>(band_level * 0.636);
}

void IIRFilter::SetSamplingRate(const uint32_t sampling_rate) {
    if (sampling_rate_ != sampling_rate) {
        sampling_rate_ = sampling_rate;
        if (bands_ != 0) {
            min_phase_iir_coeffs_.UpdateCoeffs(bands_, sampling_rate);
        }
        Reset();
    }
}
