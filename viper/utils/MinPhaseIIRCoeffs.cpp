#include "MinPhaseIIRCoeffs.h"
#include <cmath>

static constexpr float kMinPhaseIirCoeffsFreq10Bands[] = {
    31.0f, 62.0f, 125.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f, 16000.0f
};

static constexpr float kMinPhaseIirCoeffsFreq15Bands[] = {
    25.0f,
    40.0f,
    63.0f,
    100.0f,
    160.0f,
    250.0f,
    400.0f,
    630.0f,
    1000.0f,
    1600.0f,
    2500.0f,
    4000.0f,
    6300.0f,
    10000.0f,
    16000.0f
};

static constexpr float kMinPhaseIirCoeffsFreq25Bands[] = {
    20.0f,   31.5f,   40.0f,   50.0f,    80.0f,    100.0f,   125.0f,  160.0f,  250.0f,
    315.0f,  400.0f,  500.0f,  800.0f,   1000.0f,  1250.0f,  1600.0f, 2500.0f, 3150.0f,
    4000.0f, 5000.0f, 8000.0f, 10000.0f, 12500.0f, 16000.0f, 20000.0f
};

static constexpr float kMinPhaseIirCoeffsFreq31Bands[] = {
    20.0f,   25.0f,   31.5f,   40.0f,    50.0f,    63.0f,    80.0f,   100.0f,
    125.0f,  160.0f,  200.0f,  250.0f,   315.0f,   400.0f,   500.0f,  630.0f,
    800.0f,  1000.0f, 1250.0f, 1600.0f,  2000.0f,  2500.0f,  3150.0f, 4000.0f,
    5000.0f, 6300.0f, 8000.0f, 10000.0f, 12500.0f, 16000.0f, 20000.0f
};

MinPhaseIIRCoeffs::MinPhaseIIRCoeffs() :
    bands_(0),
    coeffs_(nullptr) {}

MinPhaseIIRCoeffs::~MinPhaseIIRCoeffs() {
    delete[] coeffs_;
}

double *MinPhaseIIRCoeffs::GetCoefficients() const {
    return coeffs_;
}

float MinPhaseIIRCoeffs::GetIndexFrequency(const uint32_t index) const {
    switch (bands_) {
        case 10:
            return kMinPhaseIirCoeffsFreq10Bands[index];
        case 15:
            return kMinPhaseIirCoeffsFreq15Bands[index];
        case 25:
            return kMinPhaseIirCoeffsFreq25Bands[index];
        case 31:
            return kMinPhaseIirCoeffsFreq31Bands[index];
        default:
            return 0.0f;
    }
}

int MinPhaseIIRCoeffs::UpdateCoeffs(const uint32_t bands, const uint32_t sampling_rate) {
    if (bands != 10 && bands != 15 && bands != 25 && bands != 31) {
        return 0;
    }

    bands_ = bands;

    delete[] coeffs_;
    coeffs_ = new double[bands * 4]();

    const float *band_freqs = nullptr;
    double tmp = 0;

    switch (bands) {
        case 10:
            band_freqs = kMinPhaseIirCoeffsFreq10Bands;
            tmp = 1.0;
            break;
        case 15:
            band_freqs = kMinPhaseIirCoeffsFreq15Bands;
            tmp = 2.0 / 3.0;
            break;
        case 25:
            band_freqs = kMinPhaseIirCoeffsFreq25Bands;
            tmp = 1.0 / 3.0;
            break;
        case 31:
            band_freqs = kMinPhaseIirCoeffsFreq31Bands;
            tmp = 1.0 / 3.0;
            break;
        default:;
    }

    for (uint32_t i = 0; i < bands; i++) {
        double ret1;
        double ret2;

        Find_F1_F2(band_freqs[i], tmp, &ret2, &ret1);

        const double x = 2.0 * M_PI * static_cast<double>(band_freqs[i])
                         / static_cast<double>(sampling_rate);
        const double y = 2.0 * M_PI * ret2 / static_cast<double>(sampling_rate);

        const double cos_x = cos(x);
        const double cos_y = cos(y);
        const double sin_y = sin(y);

        const double a = cos_x * cos_y;
        const double b = cos_x * cos_x / 2.0;
        const double c = sin_y * sin_y;

        const double d = b - a + 0.5 - c;
        const double e = c + (b + cos_y * cos_y - a - 0.5);
        const double f = cos_x * cos_x * 0.125 - cos_x * cos_y * 0.25 + 0.125 - c * 0.25;

        if (SolveRoot(d, e, f, &ret1) == 0) {
            coeffs_[i * 4] = ret1 + ret1;
            coeffs_[i * 4 + 1] = 0.5 - ret1;
            coeffs_[i * 4 + 2] = (ret1 + 0.5) * cos_x * 2.0;
        }
    }

    return 1;
}

void MinPhaseIIRCoeffs::Find_F1_F2(
    const double center_freq,
    const double bandwidth_octaves,
    double *lower_freq,
    double *upper_freq
) {
    const double x = pow(2.0, bandwidth_octaves / 2.0);
    *lower_freq = center_freq / x;
    *upper_freq = center_freq * x;
}

int MinPhaseIIRCoeffs::SolveRoot(
    const double coeff_a, const double coeff_b, const double coeff_c, double *root
) {
    const double x = (coeff_c - coeff_b * coeff_b / (coeff_a * 4.0)) / coeff_a;
    const double y = coeff_b / (coeff_a + coeff_a);

    if (x >= 0.0) {
        return -1;
    }

    const double z = sqrt(-x);
    const double a = -y - z;
    const double b = z - y;
    if (a > b) {
        *root = b;
    } else {
        *root = a;
    }

    return 0;
}
