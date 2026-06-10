#include "PConvSingle.h"
#include "pffft.h"
#include <cstdlib>
#include <cstring>

PConvSingle::PConvSingle() :
    instance_usable_(false),
    segment_count_(0),
    segment_size_(0),
    fft_size_(0),
    delay_line_index_(0),
    fft_setup_(nullptr),
    fft_work_(nullptr),
    filter_segments_(nullptr),
    input_history_(nullptr),
    overlap_buffer_(nullptr),
    fft_buffer_(nullptr),
    accum_buffer_(nullptr),
    mono_buffer_(nullptr) {}

PConvSingle::~PConvSingle() {
    ReleaseResources();
}

void PConvSingle::Convolve(float *buffer) {
    ConvSegment(buffer, false, 0);
}

void PConvSingle::ConvolveInterleaved(float *buffer, const int channel) {
    ConvSegment(buffer, true, channel);
}

void PConvSingle::ConvSegment(float *buffer, const bool interleaved, const int channel) {
    if (!instance_usable_) return;

    float *input;
    if (interleaved) {
        for (int i = 0; i < segment_size_; i++) {
            mono_buffer_[i] = buffer[i * 2 + channel];
        }
        input = mono_buffer_;
    } else {
        input = buffer;
    }

    // Overlap-save: form [previous_overlap | current_input] in fft_buffer_
    memcpy(fft_buffer_, overlap_buffer_, segment_size_ * sizeof(float));
    memcpy(fft_buffer_ + segment_size_, input, segment_size_ * sizeof(float));

    // Save current input as overlap for next call
    memcpy(overlap_buffer_, input, segment_size_ * sizeof(float));

    // Forward FFT the combined buffer into the current delay line slot
    pffft_transform(
        fft_setup_,
        fft_buffer_,
        input_history_[delay_line_index_],
        fft_work_,
        PFFFT_FORWARD
    );

    // Frequency-domain multiply-accumulate across all kernel segments
    memset(accum_buffer_, 0, fft_size_ * sizeof(float));
    for (int k = 0; k < segment_count_; k++) {
        const uint32_t idx = (delay_line_index_ - k + segment_count_) % segment_count_;
        pffft_zconvolve_accumulate(
            fft_setup_, input_history_[idx], filter_segments_[k], accum_buffer_, 1.0f
        );
    }

    // Inverse FFT
    pffft_transform(fft_setup_, accum_buffer_, fft_buffer_, fft_work_, PFFFT_BACKWARD);

    // Scale by 1/N (pffft does not normalize)
    const float scale = 1.0f / static_cast<float>(fft_size_);
    for (int i = 0; i < fft_size_; i++) {
        fft_buffer_[i] *= scale;
    }

    // Output the second half (valid overlap-save region)
    const float *output = fft_buffer_ + segment_size_;

    if (interleaved) {
        for (int i = 0; i < segment_size_; i++) {
            buffer[i * 2 + channel] = output[i];
        }
    } else {
        memcpy(buffer, output, segment_size_ * sizeof(float));
    }

    // Advance delay line ring buffer
    delay_line_index_ = (delay_line_index_ + 1) % segment_count_;
}

uint32_t PConvSingle::GetFFTSize() const {
    return segment_size_ * 2;
}

uint32_t PConvSingle::GetSegmentCount() const {
    return segment_count_;
}

uint32_t PConvSingle::GetSegmentSize() const {
    return segment_size_;
}

bool PConvSingle::InstanceUsable() const {
    return instance_usable_;
}

uint32_t PConvSingle::LoadKernel(
    const float *kernel, const uint32_t kernel_size, const uint32_t segment_size
) {
    if (kernel != nullptr && kernel_size >= 2 && segment_size >= 2
        && (segment_size & (segment_size - 1)) == 0) {
        instance_usable_ = false;
        ReleaseResources();
        segment_size_ = segment_size;
        const uint32_t n = ProcessKernel(kernel, kernel_size);
        if (n != 0) {
            instance_usable_ = true;
            return n;
        }
        ReleaseResources();
    }
    return 0;
}

uint32_t PConvSingle::LoadKernel(
    const float *kernel,
    const float gain,
    const uint32_t kernel_size,
    const uint32_t segment_size
) {
    if (kernel != nullptr && kernel_size >= 2 && segment_size >= 2
        && (segment_size & (segment_size - 1)) == 0) {
        instance_usable_ = false;
        ReleaseResources();
        segment_size_ = segment_size;
        const uint32_t n = ProcessKernel(kernel, gain, kernel_size);
        if (n != 0) {
            instance_usable_ = true;
            return n;
        }
        ReleaseResources();
    }
    return 0;
}

uint32_t PConvSingle::ProcessKernel(const float *kernel, const uint32_t kernel_size) {
    fft_size_ = segment_size_ * 2;
    segment_count_ = (kernel_size + segment_size_ - 1) / segment_size_;

    fft_setup_ = pffft_new_setup(static_cast<int>(fft_size_), PFFFT_REAL);
    if (fft_setup_ == nullptr) return 0;

    fft_work_ = static_cast<float *>(pffft_aligned_malloc(fft_size_ * sizeof(float)));
    fft_buffer_ = static_cast<float *>(pffft_aligned_malloc(fft_size_ * sizeof(float)));
    accum_buffer_ = static_cast<float *>(pffft_aligned_malloc(fft_size_ * sizeof(float)));
    overlap_buffer_ =
        static_cast<float *>(pffft_aligned_malloc(segment_size_ * sizeof(float)));
    mono_buffer_ =
        static_cast<float *>(pffft_aligned_malloc(segment_size_ * sizeof(float)));

    if (!fft_work_ || !fft_buffer_ || !accum_buffer_ || !overlap_buffer_
        || !mono_buffer_) {
        return 0;
    }

    memset(overlap_buffer_, 0, segment_size_ * sizeof(float));
    memset(mono_buffer_, 0, segment_size_ * sizeof(float));

    filter_segments_ = new float *[segment_count_];
    input_history_ = new float *[segment_count_];
    for (int i = 0; i < segment_count_; i++) {
        filter_segments_[i] =
            static_cast<float *>(pffft_aligned_malloc(fft_size_ * sizeof(float)));
        input_history_[i] =
            static_cast<float *>(pffft_aligned_malloc(fft_size_ * sizeof(float)));
        memset(input_history_[i], 0, fft_size_ * sizeof(float));
    }

    // Split kernel into segments, zero-pad each to fft_size_, and forward FFT
    for (int i = 0; i < segment_count_; i++) {
        memset(fft_buffer_, 0, fft_size_ * sizeof(float));
        const uint32_t offset = i * segment_size_;
        uint32_t remaining = kernel_size - offset;
        const uint32_t count = remaining < segment_size_ ? remaining : segment_size_;
        memcpy(fft_buffer_, kernel + offset, count * sizeof(float));
        pffft_transform(
            fft_setup_, fft_buffer_, filter_segments_[i], fft_work_, PFFFT_FORWARD
        );
    }

    delay_line_index_ = 0;
    return segment_count_;
}

uint32_t PConvSingle::ProcessKernel(
    const float *kernel, const float gain, const uint32_t kernel_size
) {
    // Scale kernel by gain factor before processing
    auto *scaled =
        static_cast<float *>(pffft_aligned_malloc(kernel_size * sizeof(float)));
    if (!scaled) return 0;
    for (int i = 0; i < kernel_size; i++) {
        scaled[i] = kernel[i] * gain;
    }
    const uint32_t result = ProcessKernel(scaled, kernel_size);
    pffft_aligned_free(scaled);
    return result;
}

void PConvSingle::ReleaseResources() {
    if (filter_segments_ != nullptr) {
        for (int i = 0; i < segment_count_; i++) {
            pffft_aligned_free(filter_segments_[i]);
        }
        delete[] filter_segments_;
        filter_segments_ = nullptr;
    }

    if (input_history_ != nullptr) {
        for (int i = 0; i < segment_count_; i++) {
            pffft_aligned_free(input_history_[i]);
        }
        delete[] input_history_;
        input_history_ = nullptr;
    }

    if (overlap_buffer_ != nullptr) {
        pffft_aligned_free(overlap_buffer_);
        overlap_buffer_ = nullptr;
    }

    if (fft_buffer_ != nullptr) {
        pffft_aligned_free(fft_buffer_);
        fft_buffer_ = nullptr;
    }

    if (accum_buffer_ != nullptr) {
        pffft_aligned_free(accum_buffer_);
        accum_buffer_ = nullptr;
    }

    if (mono_buffer_ != nullptr) {
        pffft_aligned_free(mono_buffer_);
        mono_buffer_ = nullptr;
    }

    if (fft_work_ != nullptr) {
        pffft_aligned_free(fft_work_);
        fft_work_ = nullptr;
    }

    if (fft_setup_ != nullptr) {
        pffft_destroy_setup(fft_setup_);
        fft_setup_ = nullptr;
    }

    instance_usable_ = false;
    segment_count_ = 0;
    segment_size_ = 0;
    fft_size_ = 0;
    delay_line_index_ = 0;
}

void PConvSingle::Reset() {
    if (!instance_usable_) return;

    for (int i = 0; i < segment_count_; i++) {
        memset(input_history_[i], 0, fft_size_ * sizeof(float));
    }
    memset(overlap_buffer_, 0, segment_size_ * sizeof(float));
    memset(fft_buffer_, 0, fft_size_ * sizeof(float));
    memset(accum_buffer_, 0, fft_size_ * sizeof(float));
    memset(mono_buffer_, 0, segment_size_ * sizeof(float));
    memset(fft_work_, 0, fft_size_ * sizeof(float));
    delay_line_index_ = 0;
}

void PConvSingle::UnloadKernel() {
    instance_usable_ = false;
    ReleaseResources();
}
