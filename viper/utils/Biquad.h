#pragma once

#include <cstdint>

class Biquad {
public:
    Biquad();

    double ProcessSample(double sample);
    void Reset();

    void SetCoeffs(double a0, double a1, double a2, double b0, double b1, double b2);
    void SetBandPassParameter(float frequency, uint32_t sampling_rate, float q_factor);
    void SetHighPassParameter(
        float frequency, uint32_t sampling_rate, double db_gain, float q_factor
    );
    void SetLowPassParameter(float frequency, uint32_t sampling_rate, float q_factor);

private:
    double x1_;
    double x2_;
    double y1_;
    double y2_;
    double a1_;
    double a2_;
    double b0_;
    double b1_;
    double b2_;
};
