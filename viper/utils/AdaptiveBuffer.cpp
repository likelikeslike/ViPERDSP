#include "AdaptiveBuffer.h"

AdaptiveBuffer::AdaptiveBuffer(const uint32_t channels, const uint32_t length) :
    length_(length),
    offset_(0),
    channels_(channels),
    buffer_(channels * length) {}
uint32_t AdaptiveBuffer::GetBufferLength() const {
    return length_;
}

uint32_t AdaptiveBuffer::GetBufferOffset() const {
    return offset_;
}

float *AdaptiveBuffer::GetBuffer() {
    return buffer_.data();
}

uint32_t AdaptiveBuffer::GetChannels() const {
    return channels_;
}

void AdaptiveBuffer::SetBufferOffset(const uint32_t value) {
    offset_ = value;
}

void AdaptiveBuffer::PanFrames(const float left, const float right) {
    if (channels_ == 2) {
        for (uint32_t i = 0; i < offset_ * channels_; i++) {
            if (i % 2 == 0) {
                buffer_[i] = buffer_[i] * left;
            } else {
                buffer_[i] = buffer_[i] * right;
            }
        }
    }
}

int AdaptiveBuffer::PopFrames(float *frames, const uint32_t length) {
    if (offset_ < length) {
        return 0;
    }

    if (length != 0) {
        memcpy(frames, buffer_.data(), length * channels_ * sizeof(float));
        offset_ = offset_ - length;
        if (offset_ != 0) {
            memmove(
                buffer_.data(),
                buffer_.data() + length * channels_,
                offset_ * channels_ * sizeof(float)
            );
        }
    }

    return 1;
}

int AdaptiveBuffer::PushFrames(const float *frames, const uint32_t length) {
    if (length != 0) {
        if (offset_ + length > length_) {
            buffer_.resize((offset_ + length) * channels_);
            length_ = offset_ + length;
        }

        memcpy(
            buffer_.data() + offset_ * channels_,
            frames,
            length * channels_ * sizeof(float)
        );
        offset_ = offset_ + length;
    }

    return 1;
}

void AdaptiveBuffer::ScaleFrames(const float scale) {
    for (uint32_t i = 0; i < offset_ * channels_; i++) {
        buffer_[i] = buffer_[i] * scale;
    }
}

int AdaptiveBuffer::PushZero(const uint32_t length) {
    if (offset_ + length > length_) {
        buffer_.resize((offset_ + length) * channels_);
        length_ = offset_ + length;
    }

    memset(buffer_.data() + offset_ * channels_, 0, length * channels_ * sizeof(float));
    offset_ = offset_ + length;

    return 1;
}

void AdaptiveBuffer::FlushBuffer() {
    offset_ = 0;
    std::fill(buffer_.begin(), buffer_.end(), 0.0f);
}
