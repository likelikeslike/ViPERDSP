#pragma once

#include "MultiBiquad.h"
#include <cstdint>

class Subwoofer {
public:
    Subwoofer();

    void Process(float *samples, uint32_t size);
    void SetBassGain(uint32_t samplingRate, float gainDb);

private:
    MultiBiquad peak[2];
    MultiBiquad peakLow[2];
    MultiBiquad lowpass[2];
};
