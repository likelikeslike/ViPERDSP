#pragma once

#include "../utils/MultiBiquad.h"
#include "../utils/QuadricTube.h"
#include <array>

class TubeSimulator {
public:
    TubeSimulator();

    void Process(float *buffer, uint32_t size);
    void Reset();

    void SetEnable(bool enable);
    void SetSamplingRate(uint32_t sampling_rate);
    void SetTubeType(int tube_type);
    void SetDrive(float drive);
    void SetMix(float mix);

private:
    bool enable_;
    int tube_type_;
    float drive_;
    float mix_;

    uint32_t sampling_rate_;

    std::array<MultiBiquad, 2> high_pass_;
    std::array<QuadricTube, 2> quadric_;
    std::array<MultiBiquad, 2> low_pass_;
    std::array<MultiBiquad, 2> dry_all_pass_high_;
    std::array<MultiBiquad, 2> dry_all_pass_low_;
};
