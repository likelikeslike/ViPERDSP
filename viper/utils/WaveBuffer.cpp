#include "WaveBuffer.h"

WaveBuffer::WaveBuffer(const uint32_t channels, const uint32_t length) :
    index_(0),
    channels_(channels),
    buffer_(length * channels) {}

void WaveBuffer::Reset() {
    index_ = 0;
    std::fill(buffer_.begin(), buffer_.end(), 0.0f);
}

uint32_t WaveBuffer::GetBufferOffset() const {
    return index_ / channels_;
}

uint32_t WaveBuffer::GetBufferSize() const {
    return buffer_.size() / channels_;
}

float *WaveBuffer::GetBuffer() {
    return buffer_.data();
}

void WaveBuffer::SetBufferOffset(const uint32_t offset) {
    const uint32_t max_offset = buffer_.size() / channels_;
    if (offset <= max_offset) {
        index_ = offset * channels_;
    }
}

uint32_t WaveBuffer::PopSamples(const uint32_t size, const bool reset_idx) {
    if (buffer_.empty()) {
        return 0;
    }

    if (channels_ * size <= index_) {
        index_ -= channels_ * size;
        memmove(
            buffer_.data(), buffer_.data() + channels_ * size, index_ * sizeof(float)
        );
        return size;
    }

    if (reset_idx) {
        const uint32_t ret = index_ / channels_;
        index_ = 0;
        return ret;
    }

    return 0;
}

uint32_t WaveBuffer::PopSamples(float *dest, const uint32_t size, const bool reset_idx) {
    if (buffer_.empty() || dest == nullptr) {
        return 0;
    }

    if (channels_ * size <= index_) {
        memcpy(dest, buffer_.data(), channels_ * size * sizeof(float));
        index_ -= channels_ * size;
        memmove(
            buffer_.data(), buffer_.data() + channels_ * size, index_ * sizeof(float)
        );
        return size;
    }

    if (reset_idx) {
        const uint32_t ret = index_ / channels_;
        memcpy(dest, buffer_.data(), index_ * sizeof(float));
        index_ = 0;
        return ret;
    }

    return 0;
}

int WaveBuffer::PushSamples(const float *source, const uint32_t size) {
    if (size > 0) {
        const uint32_t required_size = channels_ * size + index_;
        if (required_size > buffer_.size()) {
            buffer_.resize(required_size);
        }
        memcpy(buffer_.data() + index_, source, channels_ * size * sizeof(float));
        index_ += channels_ * size;
    }

    return 1;
}

int WaveBuffer::PushZeros(const uint32_t size) {
    if (size > 0) {
        const uint32_t required_size = channels_ * size + index_;
        if (required_size > buffer_.size()) {
            buffer_.resize(required_size);
        }
        memset(buffer_.data() + index_, 0, channels_ * size * sizeof(float));
        index_ += channels_ * size;
    }

    return 1;
}

float *WaveBuffer::PushZerosGetBuffer(const uint32_t size) {
    const uint32_t old_idx = index_;

    if (size > 0) {
        const uint32_t required_size = channels_ * size + index_;
        if (required_size > buffer_.size()) {
            buffer_.resize(required_size);
        }
        memset(buffer_.data() + index_, 0, channels_ * size * sizeof(float));
        index_ += channels_ * size;
    }

    return buffer_.data() + old_idx;
}
