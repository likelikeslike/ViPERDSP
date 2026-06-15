#pragma once

#include "../utils/HiFi.h"
#include "../utils/HighShelf.h"
#include "../utils/NoiseSharpening.h"

class ViPERClarity {
public:
    enum ClarityMode {
        NATURAL,
        OZONE,
        XHIFI
    };

    ViPERClarity();

    void Process(float *samples, uint32_t size);
    void Reset();

    void SetEnable(bool enable);
    void SetProcessMode(ClarityMode mode);
    void SetClarityGain(float value);
    void SetClarityToFilter();
    void SetSamplingRate(uint32_t sampling_rate);

private:
    bool enable_;

    ClarityMode process_mode_;

    uint32_t sampling_rate_;

    float gain_;

    NoiseSharpening noise_sharpening_;
    HighShelf high_shelf_[2];
    HiFi hifi_;
};
