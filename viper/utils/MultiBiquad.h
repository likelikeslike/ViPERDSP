#pragma once

#include <cstdint>

class MultiBiquad {
public:
    enum FilterType {
        LOW_PASS,
        HIGH_PASS,
        BAND_PASS,
        BAND_STOP,
        ALL_PASS,
        PEAK,
        LOW_SHELF,
        HIGH_SHELF
    };

    MultiBiquad();

    double ProcessSample(double sample);
    void Reset();

    void RefreshFilter(
        FilterType type,
        float gain_amp,
        float frequency,
        uint32_t sampling_rate,
        float q_factor,
        bool is_bandwidth
    );

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
