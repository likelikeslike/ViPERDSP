#include "SoftwareLimiter.h"
#include "../constants.h"
#include <cmath>
#include <cstring>

constexpr uint32_t kLookahead = 256;
constexpr float kReleaseTauSec = 0.080f;
constexpr float kDenormFix = 1e-25f;
const float kReleaseCoeff =
    1.0f
    - std::exp(
        -1.0f / (kReleaseTauSec * static_cast<float>(VIPER_DEFAULT_SAMPLING_RATE))
    );

SoftwareLimiter::SoftwareLimiter() :
    ready_(false),
    write_index_(0),
    gate_(0.999999f),
    target_gain_(1.0f),
    gain_envelope_(1.0f),
    smoothed_gain_(1.0f),
    arr256_{},
    arr512_{} {
    Reset();
}

float SoftwareLimiter::Process(float sample) {
    if (!std::isfinite(sample)) sample = 0.0f;

    const uint32_t wi = write_index_;

    const float delayed = arr256_[wi];
    const float window_peak = arr512_[1];

    const float target_gain = (window_peak > gate_) ? (gate_ / window_peak) : 1.0f;
    const float released =
        gain_envelope_ + kReleaseCoeff * (1.0f - gain_envelope_) + kDenormFix;
    float gain = (target_gain < released) ? target_gain : released;
    if (gain > 1.0f) gain = 1.0f;
    gain_envelope_ = gain;

    uint32_t node = kLookahead + wi;
    arr512_[node] = std::fabs(sample);
    while (node > 1) {
        const uint32_t parent = node >> 1;
        const uint32_t sibling = node ^ 1;
        float a = arr512_[node];
        float b = arr512_[sibling];
        arr512_[parent] = (a > b) ? a : b;
        node = parent;
    }
    arr256_[wi] = sample;
    write_index_ = (wi + 1) & (kLookahead - 1);

    return delayed * gain;
}

void SoftwareLimiter::Reset() {
    memset(arr256_, 0, sizeof(arr256_));
    memset(arr512_, 0, sizeof(arr512_));
    ready_ = false;
    write_index_ = 0;
    gain_envelope_ = 1.0f;
    smoothed_gain_ = 1.0f;
    target_gain_ = 1.0f;
}

void SoftwareLimiter::SetGate(const float gate) {
    gate_ = gate;
}
