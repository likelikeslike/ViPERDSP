#include "CAllPassFilter.h"
#include <cstring>

CAllPassFilter::CAllPassFilter() :
    buffer_size_(0),
    buffer_index_(0),
    feedback_(0.0f),
    buffer_(nullptr) {}

float CAllPassFilter::Process(const float sample) {
    const float out = buffer_[buffer_index_];
    buffer_[buffer_index_] = sample + out * feedback_;
    buffer_index_++;
    if (buffer_index_ >= buffer_size_) {
        buffer_index_ = 0;
    }
    return out - sample;
}

void CAllPassFilter::Mute() const {
    memset(buffer_, 0, buffer_size_ * sizeof(float));
}

void CAllPassFilter::SetBuffer(float *buffer, const uint32_t size) {
    buffer_ = buffer;
    buffer_size_ = size;
    buffer_index_ = 0;
}

void CAllPassFilter::SetFeedback(const float value) {
    feedback_ = value;
}

float CAllPassFilter::GetFeedback() const {
    return feedback_;
}
