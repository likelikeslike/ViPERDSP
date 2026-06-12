#include "TubeSimulator.h"

TubeSimulator::TubeSimulator() :
    enable_(false),
    acc_({0.0, 0.0}) {}

void TubeSimulator::Process(float *buffer, const uint32_t size) {
    if (!enable_) return;

    for (uint32_t i = 0; i < size; i++) {
        acc_[0] = (acc_[0] + buffer[i * 2]) / 2.0;
        acc_[1] = (acc_[1] + buffer[i * 2 + 1]) / 2.0;
        buffer[i * 2] = static_cast<float>(acc_[0]);
        buffer[i * 2 + 1] = static_cast<float>(acc_[1]);
    }
}

void TubeSimulator::Reset() {
    acc_[0] = 0.0;
    acc_[1] = 0.0;
    enable_ = false;
}

void TubeSimulator::SetEnable(const bool enable) {
    if (enable_ != enable) {
        if (!enable_) {
            Reset();
        }
        enable_ = enable;
    }
}
