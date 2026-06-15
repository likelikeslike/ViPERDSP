#include "ViPERDDC.h"
#include "../../include/log.h"
#include "../constants.h"

ViPERDDC::ViPERDDC() :
    enable_(false),
    set_coeffs_ok_(false),
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE),
    arr_size_(0) {}

void ViPERDDC::Process(float *samples, const uint32_t size) {
    if (!set_coeffs_ok_ || arr_size_ == 0) return;
    if (!enable_) return;

    std::vector<std::array<float, 5>> *coeffs_arr;

    switch (sampling_rate_) {
        case 44100: {
            coeffs_arr = &coeffs_arr44100_;
            break;
        }
        case 48000: {
            coeffs_arr = &coeffs_arr48000_;
            break;
        }
        default: {
            VIPER_LOGD("ViPERDDC: Unsupported sampling rate: %d", sampling_rate_);
            return;
        }
    }

    for (uint32_t i = 0; i < size * 2; i += 2) {
        float sample_l = samples[i];
        float sample_r = samples[i + 1];

        for (uint32_t j = 0; j < arr_size_; j++) {
            const std::array<float, 5> *coeffs = &(*coeffs_arr)[j];

            const float b0 = (*coeffs)[0];
            const float b1 = (*coeffs)[1];
            const float b2 = (*coeffs)[2];
            const float a1 = (*coeffs)[3];
            const float a2 = (*coeffs)[4];

            const float out_l = sample_l * b0 + x1_l_[j] * b1 + x2_l_[j] * b2
                                + y1_l_[j] * a1 + y2_l_[j] * a2;

            x2_l_[j] = x1_l_[j];
            x1_l_[j] = sample_l;
            y2_l_[j] = y1_l_[j];
            y1_l_[j] = out_l;

            sample_l = out_l;

            const float out_r = sample_r * b0 + x1_r_[j] * b1 + x2_r_[j] * b2
                                + y1_r_[j] * a1 + y2_r_[j] * a2;

            x2_r_[j] = x1_r_[j];
            x1_r_[j] = sample_r;
            y2_r_[j] = y1_r_[j];
            y1_r_[j] = out_r;

            sample_r = out_r;
        }

        samples[i] = sample_l;
        samples[i + 1] = sample_r;
    }
}

void ViPERDDC::Reset() {
    if (!set_coeffs_ok_) return;
    if (arr_size_ == 0) return;

    memset(x1_l_.data(), 0, arr_size_ * sizeof(float));
    memset(x1_r_.data(), 0, arr_size_ * sizeof(float));
    memset(x2_l_.data(), 0, arr_size_ * sizeof(float));
    memset(x2_r_.data(), 0, arr_size_ * sizeof(float));
    memset(y1_l_.data(), 0, arr_size_ * sizeof(float));
    memset(y1_r_.data(), 0, arr_size_ * sizeof(float));
    memset(y2_l_.data(), 0, arr_size_ * sizeof(float));
    memset(y2_r_.data(), 0, arr_size_ * sizeof(float));
}

void ViPERDDC::SetEnable(const bool enable) {
    if (enable_ != enable) {
        enable_ = enable;
        if (enable) {
            Reset();
        }
    }
}

void ViPERDDC::SetCoeffs(
    const uint32_t coeffs_size, const float *coeffs_44100, const float *coeffs_48000
) {
    ReleaseResources();

    if (coeffs_size == 0) return;

    arr_size_ = coeffs_size / 5;
    coeffs_arr44100_.resize(arr_size_);
    coeffs_arr48000_.resize(arr_size_);

    for (uint32_t i = 0; i < arr_size_; i++) {
        coeffs_arr44100_[i][0] = coeffs_44100[i * 5];
        coeffs_arr44100_[i][1] = coeffs_44100[i * 5 + 1];
        coeffs_arr44100_[i][2] = coeffs_44100[i * 5 + 2];
        coeffs_arr44100_[i][3] = coeffs_44100[i * 5 + 3];
        coeffs_arr44100_[i][4] = coeffs_44100[i * 5 + 4];

        coeffs_arr48000_[i][0] = coeffs_48000[i * 5];
        coeffs_arr48000_[i][1] = coeffs_48000[i * 5 + 1];
        coeffs_arr48000_[i][2] = coeffs_48000[i * 5 + 2];
        coeffs_arr48000_[i][3] = coeffs_48000[i * 5 + 3];
        coeffs_arr48000_[i][4] = coeffs_48000[i * 5 + 4];
    }

    x1_l_.resize(arr_size_);
    x1_r_.resize(arr_size_);
    x2_l_.resize(arr_size_);
    x2_r_.resize(arr_size_);
    y1_l_.resize(arr_size_);
    y1_r_.resize(arr_size_);
    y2_l_.resize(arr_size_);
    y2_r_.resize(arr_size_);

    set_coeffs_ok_ = true;
}

void ViPERDDC::SetSamplingRate(const uint32_t sampling_rate) {
    if (sampling_rate_ != sampling_rate) {
        sampling_rate_ = sampling_rate;
        Reset();
    }
}

void ViPERDDC::ReleaseResources() {
    set_coeffs_ok_ = false;

    coeffs_arr44100_.resize(0);
    coeffs_arr48000_.resize(0);

    x1_l_.resize(0);
    x1_r_.resize(0);
    x2_l_.resize(0);
    x2_r_.resize(0);
    y1_l_.resize(0);
    y1_r_.resize(0);
    y2_l_.resize(0);
    y2_r_.resize(0);
}
