#pragma once

#include "../utils/MultiBiquad.h"
#include <array>
#include <cstdint>
#include <vector>

class StereoImager {
public:
    static constexpr uint32_t NUM_BANDS = 3;
    static constexpr uint32_t NUM_CROSSOVERS = 2;

    StereoImager();

    void Process(float *samples, uint32_t size);
    void Reset();
    void SetEnable(bool enable);
    void SetSamplingRate(uint32_t samplingRate);
    void SetLowWidth(float widthPercent);
    void SetMidWidth(float widthPercent);
    void SetHighWidth(float widthPercent);
    void SetLowCrossover(float frequency);
    void SetHighCrossover(float frequency);

private:
    void ConfigureCrossovers();

    bool enable;
    uint32_t samplingRate;

    std::array<float, NUM_BANDS> bandWidths;
    std::array<float, NUM_CROSSOVERS> crossoverFreqs;

    std::array<MultiBiquad, NUM_CROSSOVERS> lowpassLA;
    std::array<MultiBiquad, NUM_CROSSOVERS> lowpassLB;
    std::array<MultiBiquad, NUM_CROSSOVERS> lowpassRA;
    std::array<MultiBiquad, NUM_CROSSOVERS> lowpassRB;
    std::array<MultiBiquad, NUM_CROSSOVERS> highpassLA;
    std::array<MultiBiquad, NUM_CROSSOVERS> highpassLB;
    std::array<MultiBiquad, NUM_CROSSOVERS> highpassRA;
    std::array<MultiBiquad, NUM_CROSSOVERS> highpassRB;

    std::array<std::vector<float>, NUM_BANDS> bandBuffers;
};
