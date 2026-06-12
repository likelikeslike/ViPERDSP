#include "Reverberation.h"

Reverberation::Reverberation() :
    enable_(false) {
    model_.SetRoomSize(0.0f);
    model_.SetWidth(0.0f);
    model_.SetDamp(0.0f);
    model_.SetWet(0.0f);
    model_.SetDry(0.5f);
    model_.Reset();
}

void Reverberation::Process(float *buffer, const uint32_t size) {
    if (enable_) {
        model_.ProcessReplace(buffer, buffer + 1, size);
    }
}

void Reverberation::Reset() const {
    model_.Reset();
}

void Reverberation::SetEnable(const bool enable) {
    if (enable_ != enable) {
        if (!enable_) {
            Reset();
        }
        enable_ = enable;
    }
}

void Reverberation::SetDamp(const float value) {
    model_.SetDamp(value);
}

void Reverberation::SetDry(const float value) {
    model_.SetDry(value);
}

void Reverberation::SetRoomSize(const float value) {
    model_.SetRoomSize(value);
}

void Reverberation::SetWet(const float value) {
    model_.SetWet(value);
}

void Reverberation::SetWidth(const float value) {
    model_.SetWidth(value);
}
