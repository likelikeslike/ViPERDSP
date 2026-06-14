#pragma once

#include <cstdint>

class CAllPassFilter {
public:
    CAllPassFilter();

    float Process(float sample);
    void Mute() const;

    void SetBuffer(float *buffer, uint32_t size);
    void SetFeedback(float value);

    [[nodiscard]] float GetFeedback() const;

private:
    uint32_t buffer_size_;
    uint32_t buffer_index_;

    float feedback_;
    float *buffer_;
};
