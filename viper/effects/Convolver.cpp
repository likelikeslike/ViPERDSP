#include "Convolver.h"
#include "../constants.h"
#include "../utils/Crc32.h"
#include "../utils/WavReader.h"

constexpr int kConvSegmentSize = 0x1000;

Convolver::Convolver() :
    enable_(false),
    is_valid_cross_channel_(false),
    is_quad_channel_(0),
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE),
    kernel_id_(0),
    expected_size_(0),
    current_size_(0),
    channel_count_(0),
    current_kernel_buffer_crc_(0),
    cross_channel_(0.0f),
    kernel_file_path_{},
    kernel_buffer_(nullptr),
    wave_buffer_l_(new WaveBuffer(2, kConvSegmentSize)),
    wave_buffer_r_(new WaveBuffer(2, kConvSegmentSize)) {
    memset(kernel_file_path_, 0, sizeof(kernel_file_path_));
}

Convolver::~Convolver() {
    delete wave_buffer_l_;
    delete wave_buffer_r_;
    delete[] kernel_buffer_;
}

uint32_t Convolver::Process(float *source, float *dest, const uint32_t frame_size) {
    if (enable_ && kernel_ch1_.InstanceUsable() && kernel_ch2_.InstanceUsable()
        && wave_buffer_l_->PushSamples(source, frame_size) != 0) {
        while (wave_buffer_l_->GetBufferOffset() >= kConvSegmentSize) {
            float *buf_ptr = wave_buffer_l_->GetBuffer();
            kernel_ch1_.ConvolveInterleaved(buf_ptr, 0);
            kernel_ch2_.ConvolveInterleaved(buf_ptr, 1);

            if (is_valid_cross_channel_) {
                const float cc = cross_channel_;
                for (size_t i = 0; i < kConvSegmentSize; i++) {
                    const float L = buf_ptr[i * 2];
                    const float R = buf_ptr[i * 2 + 1];
                    buf_ptr[i * 2] = L + cc * (R - L);
                    buf_ptr[i * 2 + 1] = R + cc * (L - R);
                }
            }

            wave_buffer_r_->PushSamples(buf_ptr, kConvSegmentSize);
            wave_buffer_l_->PopSamples(kConvSegmentSize, true);
        }

        return wave_buffer_r_->PopSamples(dest, frame_size, false);
    }

    return frame_size;
}

void Convolver::Reset() {
    wave_buffer_l_->Reset();
    wave_buffer_r_->Reset();
    kernel_ch1_.Reset();
    kernel_ch2_.Reset();
}

bool Convolver::GetEnable() const {
    return enable_;
}

uint32_t Convolver::GetKernelID() const {
    return kernel_id_;
}

void Convolver::SetEnable(const bool enable) {
    if (enable_ != enable) {
        if (enable) {
            Reset();
        }
        enable_ = enable;
    }
}

void Convolver::SetKernel(const char *path) {
    if (path == nullptr) return;
    if (strlen(path) == 0) return;

    if (strcmp(path, kernel_file_path_) == 0) return;

    kernel_ch1_.Reset();
    kernel_ch2_.Reset();
    kernel_ch3_.Reset();
    kernel_ch4_.Reset();
    kernel_ch1_.UnloadKernel();
    kernel_ch2_.UnloadKernel();
    kernel_ch3_.UnloadKernel();
    kernel_ch4_.UnloadKernel();
    is_quad_channel_ = 0;
    current_kernel_buffer_crc_ = 0;

    WavData wav{};
    if (!ReadWavFile(path, &wav)) {
        memset(kernel_file_path_, 0, sizeof(kernel_file_path_));
        kernel_id_ = 0;
        Reset();
        return;
    }

    if (wav.frame_count < 16 || wav.channels == 0 || wav.channels > 2) {
        delete[] wav.samples;
        memset(kernel_file_path_, 0, sizeof(kernel_file_path_));
        kernel_id_ = 0;
        Reset();
        return;
    }

    bool success;
    if (wav.channels == 1) {
        const uint32_t ret1 =
            kernel_ch1_.LoadKernel(wav.samples, wav.frame_count, kConvSegmentSize);
        const uint32_t ret2 =
            kernel_ch2_.LoadKernel(wav.samples, wav.frame_count, kConvSegmentSize);
        success = ret1 != 0 && ret2 != 0;
    } else {
        const auto ch1 = new float[wav.frame_count];
        auto *ch2 = new float[wav.frame_count];

        for (uint32_t i = 0; i < wav.frame_count; i++) {
            ch1[i] = wav.samples[i * 2];
            ch2[i] = wav.samples[i * 2 + 1];
        }

        const uint32_t ret1 =
            kernel_ch1_.LoadKernel(ch1, wav.frame_count, kConvSegmentSize);
        const uint32_t ret2 =
            kernel_ch2_.LoadKernel(ch2, wav.frame_count, kConvSegmentSize);
        success = ret1 != 0 && ret2 != 0;

        delete[] ch1;
        delete[] ch2;
    }

    delete[] wav.samples;

    if (success) {
        is_quad_channel_ = 1;
        strncpy(kernel_file_path_, path, sizeof(kernel_file_path_) - 1);
        kernel_file_path_[sizeof(kernel_file_path_) - 1] = '\0';
        kernel_id_ = 0;
        current_kernel_buffer_crc_ = 0;
        Reset();
    } else {
        kernel_ch1_.UnloadKernel();
        kernel_ch2_.UnloadKernel();
        kernel_ch3_.UnloadKernel();
        kernel_ch4_.UnloadKernel();
        memset(kernel_file_path_, 0, sizeof(kernel_file_path_));
        kernel_id_ = 0;
        Reset();
    }
}

void Convolver::SetKernel(const float *buf, const uint32_t size) {
    if (size < 16) return;

    kernel_ch1_.Reset();
    kernel_ch2_.Reset();

    const uint32_t ret1 = kernel_ch1_.LoadKernel(buf, size, kConvSegmentSize);
    const uint32_t ret2 = kernel_ch2_.LoadKernel(buf, size, kConvSegmentSize);
    if (ret1 == 0 || ret2 == 0) {
        kernel_ch1_.UnloadKernel();
        kernel_ch2_.UnloadKernel();
    }

    kernel_id_ = 0;
    current_kernel_buffer_crc_ = 0;
    Reset();
}

void Convolver::SetKernelBuffer(const float *buf, uint32_t size) {
    if (buf == nullptr || size == 0) return;
    if (kernel_buffer_ == nullptr || expected_size_ == 0) return;

    if (current_size_ >= expected_size_) return;
    if (current_size_ + size > expected_size_) {
        size = expected_size_ - current_size_;
    }

    memcpy(kernel_buffer_ + current_size_, buf, size * sizeof(float));
    current_size_ += size;
}

void Convolver::SetKernelStereo(
    const float *ch_l, const float *ch_r, const uint32_t frame_count
) {
    if (frame_count < 16) return;

    kernel_ch1_.Reset();
    kernel_ch2_.Reset();

    const uint32_t ret1 = kernel_ch1_.LoadKernel(ch_l, frame_count, kConvSegmentSize);
    const uint32_t ret2 = kernel_ch2_.LoadKernel(ch_r, frame_count, kConvSegmentSize);
    if (ret1 == 0 || ret2 == 0) {
        kernel_ch1_.UnloadKernel();
        kernel_ch2_.UnloadKernel();
    }

    kernel_id_ = 0;
    current_kernel_buffer_crc_ = 0;
    Reset();
}

void Convolver::SetCrossChannel(float value) {
    if (value <= 0.0f) {
        is_valid_cross_channel_ = false;
        return;
    }

    if (value > 1.0f) {
        value = 1.0f;
    }

    cross_channel_ = value;
    is_valid_cross_channel_ = true;
}

void Convolver::SetSamplingRate(const uint32_t sampling_rate) {
    if (sampling_rate_ != sampling_rate) {
        sampling_rate_ = sampling_rate;
    }
}

void Convolver::PrepareKernelBuffer(
    const uint32_t buf_size, const uint32_t ch_count, const bool reset
) {
    if (!reset) {
        if (ch_count - 1 < 2 && buf_size > 0) {
            // Pre-allocate full kernel buffer up front. expected_size_ is
            // known here, SetKernelBuffer becomes an O(1) bounds-checked
            // memcpy instead of an O(N) reallocate-and-copy-prefix on every
            // chunk. Total load cost drops from O(N^2) to O(N) memcpy bytes.
            delete[] kernel_buffer_;
            kernel_buffer_ = new float[buf_size];
            expected_size_ = buf_size;
            current_size_ = 0;
            channel_count_ = ch_count;
        }
    } else {
        delete[] kernel_buffer_;
        kernel_buffer_ = nullptr;
        expected_size_ = 0;
        current_size_ = 0;
        channel_count_ = 0;
        current_kernel_buffer_crc_ = 0;
        kernel_ch1_.Reset();
        kernel_ch2_.Reset();
        kernel_ch1_.UnloadKernel();
        kernel_ch2_.UnloadKernel();
        memset(kernel_file_path_, 0, sizeof(kernel_file_path_));
        kernel_id_ = 0;
    }
}

void Convolver::CommitKernelBuffer(
    const uint32_t expected_size, const uint32_t expected_crc, const uint32_t kernel_id
) {
    if (kernel_buffer_ == nullptr || expected_size_ != expected_size
        || current_size_ == 0) {
        ClearKernelBuffer();
        return;
    }

    const uint32_t calculated_crc =
        Crc32(reinterpret_cast<uint8_t *>(kernel_buffer_), current_size_ * 4);
    if (channel_count_ - 1 > 1 || calculated_crc != expected_crc
        || calculated_crc == current_kernel_buffer_crc_) {
        ClearKernelBuffer();
        return;
    }

    current_kernel_buffer_crc_ = calculated_crc;

    const int frames_per_channel = static_cast<int>(current_size_ / channel_count_);
    bool loaded;

    if (channel_count_ == 1) {
        const uint32_t ret1 =
            kernel_ch1_.LoadKernel(kernel_buffer_, frames_per_channel, kConvSegmentSize);
        const uint32_t ret2 =
            kernel_ch2_.LoadKernel(kernel_buffer_, frames_per_channel, kConvSegmentSize);
        loaded = ret1 != 0 && ret2 != 0;
    } else {
        const auto ch1 = new float[frames_per_channel];
        auto *ch2 = new float[frames_per_channel];

        for (size_t i = 0; i < frames_per_channel; i++) {
            ch1[i] = kernel_buffer_[i * 2];
            ch2[i] = kernel_buffer_[i * 2 + 1];
        }

        const uint32_t ret1 =
            kernel_ch1_.LoadKernel(ch1, frames_per_channel, kConvSegmentSize);
        const uint32_t ret2 =
            kernel_ch2_.LoadKernel(ch2, frames_per_channel, kConvSegmentSize);
        loaded = ret1 != 0 && ret2 != 0;

        delete[] ch1;
        delete[] ch2;
    }

    if (!loaded) {
        kernel_ch1_.UnloadKernel();
        kernel_ch2_.UnloadKernel();
        current_kernel_buffer_crc_ = 0;
        kernel_id_ = 0;
        ClearKernelBuffer();
        Reset();
        return;
    }

    memset(kernel_file_path_, 0, sizeof(kernel_file_path_));
    kernel_id_ = kernel_id;
    ClearKernelBuffer();
    Reset();
}

void Convolver::ClearKernelBuffer() {
    delete[] kernel_buffer_;
    kernel_buffer_ = nullptr;
    expected_size_ = 0;
    current_size_ = 0;
    channel_count_ = 0;
}
