#pragma once
#include <cstdint>

typedef struct PFFFT_Setup PFFFT_Setup;

class PConvSingle {
public:
    PConvSingle();
    ~PConvSingle();

    void Reset();

    [[nodiscard]] uint32_t GetFFTSize() const;
    [[nodiscard]] uint32_t GetSegmentCount() const;
    [[nodiscard]] uint32_t GetSegmentSize() const;
    [[nodiscard]] bool InstanceUsable() const;

    void Convolve(float *buffer);
    void ConvolveInterleaved(float *buffer, int channel);
    void ConvSegment(float *buffer, bool interleaved, int channel);

    uint32_t LoadKernel(const float *kernel, uint32_t kernel_size, uint32_t segment_size);
    uint32_t LoadKernel(
        const float *kernel, float gain, uint32_t kernel_size, uint32_t segment_size
    );

    uint32_t ProcessKernel(const float *kernel, uint32_t kernel_size);
    uint32_t ProcessKernel(const float *kernel, float gain, uint32_t kernel_size);

    void ReleaseResources();
    void UnloadKernel();

private:
    bool instance_usable_;

    uint32_t segment_count_;
    uint32_t segment_size_;
    uint32_t fft_size_;
    uint32_t delay_line_index_;

    PFFFT_Setup *fft_setup_;
    float *fft_work_;
    float **filter_segments_;
    float **input_history_;
    float *overlap_buffer_;
    float *fft_buffer_;
    float *accum_buffer_;
    float *mono_buffer_;
};
