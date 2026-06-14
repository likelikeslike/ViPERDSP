#include "Harmonic.h"
#include <cmath>
#include <cstring>

static constexpr float kHarmonicDefault[] = {
    1.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
};

Harmonic::Harmonic() {
    UpdateCoeffs(kHarmonicDefault);
    Reset();
}

double Harmonic::Process(const double sample) {
    const double prev_last = last_processed_;

    const double x = sample;
    const float *c = coeffs_;
    double y = c[10];

    y = fma(x, y, c[9]);
    y = fma(x, y, c[8]);
    y = fma(x, y, c[7]);
    y = fma(x, y, c[6]);
    y = fma(x, y, c[5]);
    y = fma(x, y, c[4]);
    y = fma(x, y, c[3]);
    y = fma(x, y, c[2]);
    y = fma(x, y, c[1]);
    y = fma(x, y, c[0]);

    last_processed_ = y;
    prev_out_ = last_processed_ + prev_out_ * 0.999 - prev_last;

    if (sample_counter_ < biggest_coeff_) {
        sample_counter_++;
        return 0.0;
    }

    return prev_out_;
}

void Harmonic::Reset() {
    last_processed_ = 0.0;
    sample_counter_ = 0;
    prev_out_ = 0.0;
}

void Harmonic::SetHarmonics(const float *coefficients) {
    UpdateCoeffs(coefficients);
    Reset();
}

void Harmonic::UpdateCoeffs(const float *coeffs) {
    float arr1[11];
    float arr2[11];

    memset(arr1, 0, 11 * sizeof(float));

    float biggest_coeff = 0.0f;
    float abs_coeff_sum = 0.0f;
    for (uint32_t i = 0; i < 10; i++) {
        const float abs_coeff = abs(coeffs[i]);
        abs_coeff_sum += abs_coeff;
        if (abs_coeff > biggest_coeff) {
            biggest_coeff = abs_coeff;
        }
    }
    biggest_coeff_ = static_cast<uint32_t>(biggest_coeff * 10000.0f);

    memcpy(arr1 + 1, coeffs, 10 * sizeof(float));

    float norm_factor = 1.0f;
    if (abs_coeff_sum > 1.0f) {
        norm_factor = 1.0f / abs_coeff_sum;
    }
    for (uint32_t i = 1; i < 11; i++) {
        arr1[i] *= norm_factor;
    }

    memset(coeffs_, 0, 11 * sizeof(float));
    memset(arr2, 0, 11 * sizeof(float));

    coeffs_[10] = arr1[10];

    for (uint32_t i = 2; i < 11; i++) {
        for (uint32_t j = 0; j < i; j++) {
            const float tmp = arr2[i - j];
            arr2[i - j] = coeffs_[i - j];
            coeffs_[i - j] = coeffs_[i - j - 1] * 2.0f - tmp;
        }
        const float tmp = arr1[10 - i + 1] - arr2[0];
        arr2[0] = coeffs_[0];
        coeffs_[0] = tmp;
    }

    for (uint32_t i = 1; i < 11; i++) {
        coeffs_[10 - i + 1] = coeffs_[10 - i] - arr2[10 - i + 1];
    }

    coeffs_[0] = arr1[0] / 2.0f - arr2[0];
}
