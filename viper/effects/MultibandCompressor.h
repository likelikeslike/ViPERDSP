#pragma once

#include "../utils/MultiBiquad.h"
#include "FETCompressor.h"
#include <array>
#include <cstdint>
#include <vector>

class MultibandCompressor {
public:
    static constexpr uint32_t MAX_BANDS = 5;
    static constexpr uint32_t MAX_CROSSOVERS = 4;

    MultibandCompressor();

    void Process(float *samples, uint32_t size);
    void Reset();
    void SetEnable(bool enable);
    void SetSamplingRate(uint32_t samplingRate);
    void SetBandCount(uint32_t bandCount);
    void SetCrossoverFrequency(uint32_t index, float frequency);
    void SetBandParameter(uint32_t band, FETCompressor::Parameter param, float value);

private:
    void ConfigureCrossovers();

    bool enable;
    uint32_t samplingRate;
    uint32_t bandCount;

    std::array<float, MAX_CROSSOVERS> crossoverFreqs;

    std::array<MultiBiquad, MAX_CROSSOVERS> lowpassLA;
    std::array<MultiBiquad, MAX_CROSSOVERS> lowpassLB;
    std::array<MultiBiquad, MAX_CROSSOVERS> lowpassRA;
    std::array<MultiBiquad, MAX_CROSSOVERS> lowpassRB;
    std::array<MultiBiquad, MAX_CROSSOVERS> highpassLA;
    std::array<MultiBiquad, MAX_CROSSOVERS> highpassLB;
    std::array<MultiBiquad, MAX_CROSSOVERS> highpassRA;
    std::array<MultiBiquad, MAX_CROSSOVERS> highpassRB;

    std::array<FETCompressor, MAX_BANDS> compressors;

    std::array<std::vector<float>, MAX_BANDS> bandBuffers;
};
