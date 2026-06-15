#include "MultiBiquad.h"
#include <cmath>

MultiBiquad::MultiBiquad() {
    Reset();
}

double MultiBiquad::ProcessSample(const double sample) {
    const double out = sample * b0_ + x1_ * b1_ + x2_ * b2_ + y1_ * a1_ + y2_ * a2_;

    x2_ = x1_;
    x1_ = sample;
    y2_ = y1_;
    y1_ = out;

    return out;
}

void MultiBiquad::Reset() {
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

void MultiBiquad::RefreshFilter(
    const FilterType type,
    const float gain_amp,
    const float frequency,
    const uint32_t sampling_rate,
    const float q_factor,
    const bool is_bandwidth
) {
    double gain;

    if (type == PEAK || type == LOW_SHELF || type == HIGH_SHELF) {
        gain = pow(10.0, static_cast<double>(gain_amp) / 40.0);
    } else {
        gain = pow(10.0, static_cast<double>(gain_amp) / 20.0);
    }

    const double omega =
        2.0 * M_PI * static_cast<double>(frequency) / static_cast<double>(sampling_rate);
    const double sin_omega = sin(omega);
    const double cos_omega = cos(omega);

    double y;
    double z;

    if (type == LOW_SHELF || type == HIGH_SHELF) {
        y = sin_omega / 2.0
            * sqrt(
                (1.0 / gain + gain) * (1.0 / static_cast<double>(q_factor) - 1.0) + 2.0
            );
        z = sqrt(gain) * 2.0 * y;
    } else if (is_bandwidth) {
        y = sinh(static_cast<double>(q_factor) * log(2.0) * omega / 2.0 / sin_omega)
            * sin_omega;
        z = -1.0;
    } else {
        y = sin_omega / (static_cast<double>(q_factor) + static_cast<double>(q_factor));
        z = -1.0;
    }

    double a0 = 0;
    double a1 = 0;
    double a2 = 0;
    double b0 = 0;
    double b1 = 0;
    double b2 = 0;

    switch (type) {
        case LOW_PASS: {
            a0 = 1.0 + y;
            a1 = -2.0 * cos_omega;
            a2 = 1.0 - y;
            b0 = (1.0 - cos_omega) / 2.0;
            b1 = 1.0 - cos_omega;
            b2 = (1.0 - cos_omega) / 2.0;
            break;
        }
        case HIGH_PASS: {
            a0 = 1.0 + y;
            a1 = -2.0 * cos_omega;
            a2 = 1.0 - y;
            b0 = (1.0 + cos_omega) / 2.0;
            b1 = -(1.0 + cos_omega);
            b2 = (1.0 + cos_omega) / 2.0;
            break;
        }
        case BAND_PASS: {
            a0 = 1.0 + y;
            a1 = -2.0 * cos_omega;
            a2 = 1.0 - y;
            b0 = y;
            b1 = 0.0;
            b2 = -y;
            break;
        }
        case BAND_STOP: {
            a0 = 1.0 + y;
            a1 = -2.0 * cos_omega;
            a2 = 1.0 - y;
            b0 = 1.0;
            b1 = -2.0 * cos_omega;
            b2 = 1.0;
            break;
        }
        case ALL_PASS: {
            a0 = 1.0 + y;
            a1 = -2.0 * cos_omega;
            a2 = 1.0 - y;
            b0 = 1.0 - y;
            b1 = -2.0 * cos_omega;
            b2 = 1.0 + y;
            break;
        }
        case PEAK: {
            a0 = 1.0 + y / gain;
            a1 = -2.0 * cos_omega;
            a2 = 1.0 - y / gain;
            b0 = 1.0 + y * gain;
            b1 = -2.0 * cos_omega;
            b2 = 1.0 - y * gain;
            break;
        }
        case LOW_SHELF: {
            const double tmp1 = gain + 1.0 - (gain - 1.0) * cos_omega;
            const double tmp2 = gain + 1.0 + (gain - 1.0) * cos_omega;
            a1 = (gain - 1.0 + (gain + 1.0) * cos_omega) * -2.0;
            a2 = tmp2 - z;
            b1 = gain * 2.0 * (gain - 1.0 - (gain + 1.0) * cos_omega);
            a0 = tmp2 + z;
            b0 = (tmp1 + z) * gain;
            b2 = (tmp1 - z) * gain;
            break;
        }
        case HIGH_SHELF: {
            const double tmp1 = gain + 1.0 + (gain - 1.0) * cos_omega;
            const double tmp2 = gain + 1.0 - (gain - 1.0) * cos_omega;
            a2 = tmp2 - z;
            a0 = tmp2 + z;
            a1 = (gain - 1.0 - (gain + 1.0) * cos_omega) * 2.0;
            b1 = gain * -2.0 * (gain - 1.0 + (gain + 1.0) * cos_omega);
            b0 = (tmp1 + z) * gain;
            b2 = (tmp1 - z) * gain;
            break;
        }
    }

    a1_ = -(a1 / a0);
    a2_ = -(a2 / a0);
    b0_ = b0 / a0;
    b1_ = b1 / a0;
    b2_ = b2 / a0;
}
