#pragma once

#include "../utils/Biquad.h"
#include "../utils/Polyphase.h"
#include "../utils/Subwoofer.h"
#include "../utils/WaveBuffer.h"

class ViPERBassMono {
public:
    enum ProcessMode {
        NATURAL_BASS = 0,
        PURE_BASS_PLUS = 1,
        SUBWOOFER = 2,
    };

    ViPERBassMono();

    void Process(float *samples, uint32_t size);
    void Reset();
    void SetBassFactor(float bass_factor);
    void SetEnable(bool enable);
    void SetProcessMode(ProcessMode mode);
    void SetSamplingRate(uint32_t sampling_rate);
    void SetSpeaker(uint32_t speaker);
    void SetAntiPop(bool enable);

private:
    bool enable_;

    ProcessMode process_mode_;

    uint32_t sampling_rate_;
    uint32_t speaker_;

    float sampling_rate_period_;
    float anti_pop_;
    float bass_factor_;
    float bass_factor_smoothed_;
    float smoothing_coeff_;

    Polyphase polyphase_;
    Biquad biquad_;
    Subwoofer subwoofer_;
    WaveBuffer wave_buffer_;
};
