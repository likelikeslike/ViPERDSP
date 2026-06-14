#include "CCombFilter.h"
#include <cstring>

CCombFilter::CCombFilter() :
    buffer_size_(0),
    buffer_index_(0),
    feedback_(0.0f),
    filter_store_(0.0f),
    damp_(0.0f),
    damp2_(0.0f),
    buffer_(nullptr) {}

float CCombFilter::Process(const float sample) {
    const float out = buffer_[buffer_index_];
    filter_store_ = out * damp2_ + filter_store_ * damp_;
    buffer_[buffer_index_] = sample + filter_store_ * feedback_;
    buffer_index_ = (buffer_index_ + 1) % buffer_size_;
    return out;
}

void CCombFilter::Mute() const {
    memset(buffer_, 0, buffer_size_ * sizeof(float));
}

void CCombFilter::SetBuffer(float *buffer, const uint32_t size) {
    buffer_ = buffer;
    buffer_size_ = size;
    buffer_index_ = 0;
}

void CCombFilter::SetDamp(const float value) {
    damp_ = value;
    damp2_ = 1.0f - value;
}

void CCombFilter::SetFeedback(const float value) {
    feedback_ = value;
}

float CCombFilter::GetDamp() const {
    return damp_;
}

float CCombFilter::GetFeedback() const {
    return feedback_;
}
