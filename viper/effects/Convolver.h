#pragma once

#include "../utils/PConvSingle.h"
#include "../utils/WaveBuffer.h"

class Convolver {
public:
    Convolver();
    ~Convolver();

    void CommitKernelBuffer(
        uint32_t expected_size, uint32_t expected_crc, uint32_t kernel_id
    );
    [[nodiscard]] bool GetEnabled() const;
    [[nodiscard]] uint32_t GetKernelID() const;
    void PrepareKernelBuffer(
        uint32_t buffer_size, uint32_t channel_count, int32_t reset_flag
    );
    uint32_t Process(float *source, float *dest, uint32_t frame_size);
    void Reset();
    void SetCrossChannel(float cross_channel);
    void SetEnable(bool enable);
    void SetKernel(const char *path);
    void SetKernel(const float *buf, uint32_t len);
    void SetKernelBuffer(const float *buf, uint32_t len);
    void SetKernelStereo(
        const float *left_channel, const float *right_channel, uint32_t frame_count
    );
    void SetSamplingRate(uint32_t sampling_rate);

private:
    bool enable_;
    bool is_valid_cross_channel_;

    int is_quad_channel_;
    uint32_t sampling_rate_;
    uint32_t kernel_id_;
    uint32_t expected_size_;
    uint32_t current_size_;
    uint32_t channel_count_;
    uint32_t current_kernel_buffer_crc_;

    float cross_channel_;

    char kernel_file_path_[256];
    float *kernel_buffer_;
    WaveBuffer *wave_buffer_l_;
    WaveBuffer *wave_buffer_r_;
    PConvSingle kernel_ch1_;
    PConvSingle kernel_ch2_;
    PConvSingle kernel_ch3_;
    PConvSingle kernel_ch4_;

    void ClearKernelBuffer();
};
