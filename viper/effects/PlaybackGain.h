#pragma once

#include "../utils/Biquad.h"
#include <cstdint>

class PlaybackGain {
public:
    PlaybackGain();

    double AnalyseWave(float *samples, uint32_t size);
    void Process(float *samples, uint32_t size);
    void Reset();
    void SetEnable(bool enable);
    void SetMaxGainFactor(float maxGainFactor);
    void SetRatio(float ratio);
    void SetSamplingRate(uint32_t samplingRate);
    void SetVolume(float volume);

private:
    float ratio2;
    float logCoeff;
    uint32_t counterTo100;
    float ratio1;
    float volume;
    float maxGainFactor;
    float currentGainL;
    float currentGainR;
    Biquad biquad1;
    Biquad biquad2;
    uint32_t samplingRate;
    bool enable;
};
