#pragma once

#include <cstdint>

class HighShelf {
public:
    HighShelf();

    double Process(double sample);

    void SetFrequency(float value);
    void SetGain(float value);
    void SetSamplingRate(uint32_t sampling_rate);

private:
    float frequency_;

    double gain_;
    double x1_;
    double x2_;
    double y1_;
    double y2_;
    double b0_;
    double b1_;
    double b2_;
    double a0_;
    double a1_;
    double a2_;
};
