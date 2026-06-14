#include "Biquad.h"
#include <cmath>

Biquad::Biquad() {
    Reset();
    SetCoeffs(1.0, 0.0, 0.0, 1.0, 0.0, 0.0);
}

double Biquad::ProcessSample(const double sample) {
    const double out = sample * b0_ + x1_ * b1_ + x2_ * b2_ + y1_ * a1_ + y2_ * a2_;

    x2_ = x1_;
    x1_ = sample;
    y2_ = y1_;
    y1_ = out;

    return out;
}

void Biquad::Reset() {
    a1_ = 0.0;
    a2_ = 0.0;
    b0_ = 0.0;
    b1_ = 0.0;
    b2_ = 0.0;
    x1_ = 0.0;
    x2_ = 0.0;
    y1_ = 0.0;
    y2_ = 0.0;
}

void Biquad::SetCoeffs(
    const double a0,
    const double a1,
    const double a2,
    const double b0,
    const double b1,
    const double b2
) {
    a1_ = -(a1 / a0);
    a2_ = -(a2 / a0);
    b0_ = b0 / a0;
    b1_ = b1 / a0;
    b2_ = b2 / a0;
}

void Biquad::SetBandPassParameter(
    const float frequency, const uint32_t sampling_rate, const float q_factor
) {
    const double omega = 2.0 * M_PI * frequency / sampling_rate;
    const double sin_omega = sin(omega);
    const double cos_omega = cos(omega);

    const double alpha = sin_omega / (q_factor + q_factor);
    const double a0 = alpha + 1.0;
    const double a1 = cos_omega * -2.0;
    const double a2 = 1.0 - alpha;
    const double b0 = sin_omega / 2.0;
    constexpr double b1 = 0.0;
    const double b2 = -(sin_omega / 2.0);

    SetCoeffs(a0, a1, a2, b0, b1, b2);
}

void Biquad::SetHighPassParameter(
    const float frequency,
    const uint32_t sampling_rate,
    const double db_gain,
    const float q_factor
) {
    const double omega = 2.0 * M_PI * frequency / sampling_rate;
    const double sin_omega = sin(omega);
    const double cos_omega = cos(omega);

    const double A = pow(10.0, db_gain / 40.0);
    const double sqrt_a = sqrt(A);

    const double z = sin_omega / 2.0 * sqrt((1.0 / A + A) * (1.0 / q_factor - 1.0) + 2.0);

    const double a0 = A + 1.0 - (A - 1.0) * cos_omega + sqrt_a * 2.0 * z;
    const double a1 = (A - 1.0 - (A + 1.0) * cos_omega) * 2.0;
    const double a2 = A + 1.0 - (A - 1.0) * cos_omega - sqrt_a * 2.0 * z;
    const double b0 =
        (A + 1.0 + (A - 1.0) * cos_omega + sqrt_a * 2.0 * z) * A * omega;
    const double b1 = A * -2.0 * (A - 1.0 + (A + 1.0) * cos_omega) * omega;
    const double b2 =
        (A + 1.0 + (A - 1.0) * cos_omega - sqrt_a * 2.0 * z) * A * omega;

    SetCoeffs(a0, a1, a2, b0, b1, b2);
}

void Biquad::SetLowPassParameter(
    const float frequency, const uint32_t sampling_rate, const float q_factor
) {
    const double omega = 2.0 * M_PI * frequency / sampling_rate;
    const double sin_omega = sin(omega);
    const double cos_omega = cos(omega);

    const double alpha = sin_omega / (q_factor + q_factor);

    const double a0 = alpha + 1.0;
    const double a1 = cos_omega * -2.0;
    const double a2 = 1.0 - alpha;
    const double b0 = (1.0 - cos_omega) / 2.0;
    const double b1 = 1.0 - cos_omega;
    const double b2 = (1.0 - cos_omega) / 2.0;

    SetCoeffs(a0, a1, a2, b0, b1, b2);
}
