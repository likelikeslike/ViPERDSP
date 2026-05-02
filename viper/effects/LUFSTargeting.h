#pragma once

#include "../utils/Biquad.h"
#include <cstdint>

class LUFSTargeting {
public:
    LUFSTargeting();

    void Process(float *samples, uint32_t size);
    void Reset();
    void SetEnable(bool enable);
    void SetMaxGain(float maxGainDB);
    void SetSamplingRate(uint32_t samplingRate);
    void SetSpeed(int speed);
    void SetTargetLUFS(float targetLUFS);

private:
    static constexpr uint32_t MAX_WINDOWS = 40;
    static constexpr double ABSOLUTE_GATE_LUFS = -70.0;

    void ConfigureFilters();
    void UpdateSmoothingCoeffs();

    bool enable;
    uint32_t samplingRate;
    float targetLUFS;
    float maxGainDB;
    int speed;

    Biquad kWeightStage1L;
    Biquad kWeightStage1R;
    Biquad kWeightStage2L;
    Biquad kWeightStage2R;

    uint32_t windowSize;
    uint32_t stepSize;
    uint32_t sampleCounter;

    double windowAccumulator;
    uint32_t windowSampleCount;

    double windowPower[MAX_WINDOWS];
    uint32_t windowWriteIdx;
    uint32_t windowCount;

    double smoothedGainDB;
    double attackCoeff;
    double releaseCoeff;
};
