#pragma once

#include "../utils/Crossfeed.h"
#include "../utils/PassFilter.h"

class Cure {
public:
    Cure();

    void Process(float *buffer, uint32_t size);
    void Reset();

    [[nodiscard]] uint32_t GetCutoff() const;
    [[nodiscard]] float GetFeedback() const;
    [[nodiscard]] float GetLevelDelay() const;
    [[nodiscard]] Crossfeed::Preset GetPreset() const;

    void SetEnable(bool enable);
    void SetCutoff(uint32_t value);
    void SetFeedback(float value);
    void SetPreset(Crossfeed::Preset preset);
    void SetSamplingRate(uint32_t sampling_rate);

private:
    bool enabled_;

    Crossfeed crossfeed_;
    PassFilter pass_filter_;
};
