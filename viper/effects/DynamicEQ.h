#pragma once

#include "../utils/MultiBiquad.h"
#include <array>
#include <cstdint>

class DynamicEQ {
public:
    static constexpr uint32_t kMaxBands = 8;

    DynamicEQ();

    void Process(float *samples, uint32_t size);
    void Reset();
    void SetEnable(bool enable);
    void SetSamplingRate(uint32_t sampling_rate);
    void SetBandCount(uint32_t count);
    // void SetBandParam(uint32_t band, int paramType, float value);
    void SetBandFrequency(uint32_t band, float value);
    void SetBandGain(uint32_t band, float value);
    void SetBandQ(uint32_t band, float value);
    void SetBandThreshold(uint32_t band, float value);
    void SetBandAttack(uint32_t band, float value);
    void SetBandRelease(uint32_t band, float value);
    void SetBandFilterType(uint32_t band, int value);

private:
    struct BandParam {
        float frequency;
        float q;
        float target_gain_db;
        float threshold_db;
        float attack_ms;
        float release_ms;
        MultiBiquad::FilterType filter_type;
    };

    struct BandState {
        double envelope_l;
        double envelope_r;
        double smoothed_gain_db;
        float last_applied_gain_db;
        double attack_coeff;
        double release_coeff;
    };

    bool enable_;

    uint32_t sampling_rate_;
    uint32_t band_count_;

    std::array<BandParam, kMaxBands> params_;
    std::array<BandState, kMaxBands> state_;

    std::array<MultiBiquad, kMaxBands> apply_l_;
    std::array<MultiBiquad, kMaxBands> apply_r_;

    void RecalcAttackRelease(uint32_t band);
    void ConfigureApplicationFilter(uint32_t band, float gain_db);
};
