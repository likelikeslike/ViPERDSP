#pragma once

#include "../utils/Biquad.h"
#include <cstdint>

class PlaybackGain {
public:
    PlaybackGain();

    void Process(float *samples, uint32_t size);
    void Reset();

    void SetEnable(bool enable);
    void SetMaxGainFactor(float max_gain_factor);
    void SetRatio(float ratio);
    void SetVolume(float volume);
    void SetSamplingRate(uint32_t sampling_rate);

private:
    bool enable_;

    uint32_t sampling_rate_;
    uint32_t ramp_progress_;
    uint32_t ramp_frames_;

    float log_coeff_;
    float ratio1_;
    float ratio2_;
    float volume_;
    float max_gain_factor_;
    float current_gain_l_;
    float current_gain_r_;

    Biquad biquad1_;
    Biquad biquad2_;

    double AnalyseWave(const float *samples, uint32_t size);
};
