#pragma once

#include "../utils/MultiBiquad.h"
#include "FETCompressor.h"
#include <array>
#include <vector>

class MultibandCompressor {
public:
    static constexpr uint32_t kMaxBands = 5;
    static constexpr uint32_t kMaxCrossovers = 4;

    MultibandCompressor();

    void Process(float *samples, uint32_t size);
    void Reset();
    void SetEnable(bool enable);
    void SetSamplingRate(uint32_t sampling_rate);
    void SetBandCount(uint32_t band_count);
    void SetCrossoverFrequency(uint32_t index, float frequency);

    void SetBandEnable(uint32_t band, bool enable);
    void SetBandThreshold(uint32_t band, float value);
    void SetBandRatio(uint32_t band, float value);
    void SetBandKnee(uint32_t band, float value);
    void SetBandKneeAuto(uint32_t band, bool enable);
    void SetBandGain(uint32_t band, float value);
    void SetBandGainAuto(uint32_t band, bool enable);
    void SetBandAttack(uint32_t band, float value);
    void SetBandAttackAuto(uint32_t band, bool enable);
    void SetBandRelease(uint32_t band, float value);
    void SetBandReleaseAuto(uint32_t band, bool enable);
    void SetBandKneeMulti(uint32_t band, float value);
    void SetBandMaxAttack(uint32_t band, float value);
    void SetBandMaxRelease(uint32_t band, float value);
    void SetBandCrest(uint32_t band, float value);
    void SetBandAdapt(uint32_t band, float value);
    void SetBandNoClip(uint32_t band, bool enable);

private:
    bool enable_;

    uint32_t sampling_rate_;
    uint32_t band_count_;

    std::array<float, kMaxCrossovers> crossover_freqs_;

    std::array<MultiBiquad, kMaxCrossovers> lowpass_la_;
    std::array<MultiBiquad, kMaxCrossovers> lowpass_lb_;
    std::array<MultiBiquad, kMaxCrossovers> lowpass_ra_;
    std::array<MultiBiquad, kMaxCrossovers> lowpass_rb_;
    std::array<MultiBiquad, kMaxCrossovers> highpass_la_;
    std::array<MultiBiquad, kMaxCrossovers> highpass_lb_;
    std::array<MultiBiquad, kMaxCrossovers> highpass_ra_;
    std::array<MultiBiquad, kMaxCrossovers> highpass_rb_;

    std::array<FETCompressor, kMaxBands> compressors_;

    std::array<std::vector<float>, kMaxBands> band_buffers_;

    void ConfigureCrossovers();
};
