#pragma once

#include <vector>

class AdaptiveBuffer {
public:
    AdaptiveBuffer(uint32_t channels, uint32_t length);

    [[nodiscard]] uint32_t GetBufferLength() const;
    [[nodiscard]] uint32_t GetBufferOffset() const;
    float *GetBuffer();
    [[nodiscard]] uint32_t GetChannels() const;

    void SetBufferOffset(uint32_t value);

    void PanFrames(float left, float right);
    int PopFrames(float *frames, uint32_t length);
    int PushFrames(const float *frames, uint32_t length);
    void ScaleFrames(float scale);
    int PushZero(uint32_t length);
    void FlushBuffer();

private:
    uint32_t length_;
    uint32_t offset_;
    uint32_t channels_;

    std::vector<float> buffer_;
};
