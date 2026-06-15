#pragma once

#include <cstdint>

class CCombFilter {
public:
    CCombFilter();

    float Process(float sample);
    void Mute() const;

    void SetBuffer(float *buffer, uint32_t size);
    void SetDamp(float value);
    void SetFeedback(float value);

    [[nodiscard]] float GetDamp() const;
    [[nodiscard]] float GetFeedback() const;

private:
    uint32_t buffer_size_;
    uint32_t buffer_index_;

    float feedback_;
    float filter_store_;
    float damp_;
    float damp2_;
    float *buffer_;
};
