#pragma once

#include <cstdint>

class Crossfeed {
public:
    struct Preset {
        uint32_t cutoff;
        uint32_t feedback;
    };

    Crossfeed();

    void Reset();

    [[nodiscard]] uint32_t GetCutoff() const;
    [[nodiscard]] float GetFeedback() const;
    [[nodiscard]] float GetLevelDelay() const;
    [[nodiscard]] Preset GetPreset() const;

    void SetCutoff(uint32_t value);
    void SetFeedback(float value);
    void SetPreset(Preset preset);
    void SetSamplingRate(uint32_t sampling_rate);

    void ProcessFrames(float *buffer, uint32_t size);
    void FilterSample(float *sample);

private:
    uint32_t sampling_rate_;

    float a0_lo_, b1_lo_;
    float a0_hi_, b1_hi_, a1_hi_;
    float gain_;

    struct {
        float asis[2], lo[2], hi[2];
    } lfs_;

    Preset preset_;

    [[nodiscard]] float ApplyLoFilter(const float in, const float out_1) const {
        return a0_lo_ * in + b1_lo_ * out_1;
    }

    [[nodiscard]] float ApplyHiFilter(
        const float in, const float in_1, const float out_1
    ) const {
        return a0_hi_ * in + a1_hi_ * in_1 + b1_hi_ * out_1;
    }
};
