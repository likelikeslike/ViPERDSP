#pragma once

#include <vector>

class WaveBuffer {
public:
    WaveBuffer(uint32_t channels, uint32_t length);

    void Reset();

    [[nodiscard]] uint32_t GetBufferOffset() const;
    [[nodiscard]] uint32_t GetBufferSize() const;
    float *GetBuffer();

    void SetBufferOffset(uint32_t offset);

    uint32_t PopSamples(uint32_t size, bool reset_idx);
    uint32_t PopSamples(float *dest, uint32_t size, bool reset_idx);
    int PushSamples(const float *source, uint32_t size);
    int PushZeros(uint32_t size);
    float *PushZerosGetBuffer(uint32_t size);

private:
    uint32_t index_;
    uint32_t channels_;

    std::vector<float> buffer_;
};
