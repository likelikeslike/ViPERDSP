#pragma once

#include <cstdint>

class Harmonic {
public:
    Harmonic();

    double Process(double sample);
    void Reset();

    void SetHarmonics(const float *coeffs);

    void UpdateCoeffs(const float *coeffs);

private:
    uint32_t biggest_coeff_;
    uint32_t sample_counter_;

    float coeffs_[11];
    double last_processed_;
    double prev_out_;
};
