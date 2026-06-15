#pragma once

#include <cstdint>

class FETCompressor {
public:
    FETCompressor();

    void Process(float *samples, uint32_t size);
    void Reset();

    void SetEnable(bool enable);
    void SetThreshold(float value);
    void SetRatio(float value);
    void SetKnee(float value);
    void SetKneeAuto(bool enable);
    void SetGain(float value);
    void SetGainAuto(bool enable);
    void SetAttack(float value);
    void SetAttackAuto(bool enable);
    void SetRelease(float value);
    void SetReleaseAuto(bool enable);
    void SetKneeMulti(float value);
    void SetMaxAttack(float value);
    void SetMaxRelease(float value);
    void SetCrest(float value);
    void SetAdapt(float value);
    void SetNoClip(bool enable);
    void SetSamplingRate(uint32_t sampling_rate);

private:
    bool enable_;
    bool auto_knee_;
    bool auto_gain_;
    bool auto_attack_;
    bool auto_release_;
    bool no_clip_;

    uint32_t sampling_rate_;

    float attack_raw_;
    float release_raw_;
    float crest_raw_;
    float adapt_raw_;

    float smoothing_coeff_;
    float release_smooth_gr_;
    float attack_smooth_gr_;
    float adaptive_gain_state_;
    float smoothed_threshold_;
    float threshold_;
    float knee_;
    float smoothed_gain_;
    float gain_;
    float ratio_;
    float running_peak_;
    float running_rms_;
    float attack1_;
    float attack2_;
    float release1_;
    float release2_;
    float knee_multi_;
    float max_attack_;
    float max_release_;
    float crest1_;
    float crest2_;
    float adapt1_;
    float adapt2_;

    double ProcessSidechain(double in);
};
