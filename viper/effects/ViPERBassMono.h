#pragma once

#include "../utils/Biquad.h"
#include "../utils/Polyphase.h"
#include "../utils/Subwoofer.h"
#include "../utils/WaveBuffer.h"
#include <cstdint>

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
    void SetBassFactor(float bassFactor);
    void SetEnable(bool enable);
    void SetProcessMode(ProcessMode processMode);
    void SetSamplingRate(uint32_t samplingRate);
    void SetSpeaker(uint32_t speaker);
    void SetAntiPop(bool enabled);

private:
    Polyphase polyphase;
    Biquad biquad;
    Subwoofer subwoofer;
    WaveBuffer waveBuffer;
    bool enable;
    ProcessMode processMode;
    uint32_t samplingRate;
    float samplingRatePeriod;
    float antiPop;
    uint32_t speaker;
    float bassFactor;
};
