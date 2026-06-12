#pragma once

#include "../utils/Biquad.h"
#include "../utils/Polyphase.h"
#include "../utils/Subwoofer.h"
#include "../utils/WaveBuffer.h"

class ViPERBass {
public:
    enum ProcessMode {
        NATURAL_BASS = 0,
        PURE_BASS_PLUS = 1,
        SUBWOOFER = 2,
    };

    ViPERBass();

    void Process(float *samples, uint32_t size);
    void Reset();

    void SetEnable(bool enable);
    void SetProcessMode(ProcessMode mode);
    void SetBassFactor(float value);
    void SetFrequency(uint32_t value);
    void SetAntiPop(bool enable);
    void SetSamplingRate(uint32_t sampling_rate);

private:
    bool enable_;

    ProcessMode process_mode_;

    uint32_t sampling_rate_;
    uint32_t frequency_;

    float sampling_rate_period_;
    float anti_pop_;
    float bass_factor_;
    float bass_factor_smoothed_;
    float smoothing_coeff_;

    Polyphase polyphase_;
    Biquad biquad_[2];
    Subwoofer subwoofer_;
    WaveBuffer wave_buffer_;
};
