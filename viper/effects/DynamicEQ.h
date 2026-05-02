#pragma once

#include "../utils/MultiBiquad.h"
#include <array>
#include <cstdint>

class DynamicEQ {
public:
    static constexpr uint32_t MAX_BANDS = 8;

    enum ParamType {
        FREQ,
        Q,
        GAIN,
        THRESHOLD,
        ATTACK,
        RELEASE,
        FILTER_TYPE
    };

    DynamicEQ();

    void Process(float *samples, uint32_t size);
    void Reset();
    void SetEnable(bool enable);
    void SetSamplingRate(uint32_t samplingRate);
    void SetBandCount(uint32_t count);
    void SetBandParam(uint32_t band, int paramType, float value);

private:
    struct BandParam {
        float frequency;
        float q;
        float targetGainDb;
        float thresholdDb;
        float attackMs;
        float releaseMs;
        MultiBiquad::FilterType filterType;
    };

    struct BandState {
        double envelopeL;
        double envelopeR;
        double smoothedGainDb;
        float lastAppliedGainDb;
        double attackCoeff;
        double releaseCoeff;
    };

    void RecalcAttackRelease(uint32_t band);
    void ConfigureApplicationFilter(uint32_t band, float gainDb);

    bool enable;
    uint32_t samplingRate;
    uint32_t bandCount;

    std::array<BandParam, MAX_BANDS> params;
    std::array<BandState, MAX_BANDS> state;

    std::array<MultiBiquad, MAX_BANDS> applyL;
    std::array<MultiBiquad, MAX_BANDS> applyR;
};
