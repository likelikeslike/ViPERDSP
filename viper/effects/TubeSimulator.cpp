#include "TubeSimulator.h"
#include "../constants.h"

TubeSimulator::TubeSimulator() :
    enable_(false),
    tube_type_(0),
    drive_(0.5f),
    mix_(0.3f),
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE) {
    Reset();
}

void TubeSimulator::Process(float *buffer, const uint32_t size) {
    if (!enable_) return;

    const double wet_gain = mix_;
    const double dry_gain = 1.0 - wet_gain;

    for (uint32_t i = 0; i < size; i++) {
        const double in_l = buffer[i * 2];
        const double dry_l =
            dry_all_pass_low_[0].ProcessSample(dry_all_pass_high_[0].ProcessSample(in_l));
        double harm_l = high_pass_[0].ProcessSample(in_l);
        harm_l = quadric_[0].Process(harm_l);
        harm_l = low_pass_[0].ProcessSample(harm_l);
        buffer[i * 2] = static_cast<float>(dry_l * dry_gain + harm_l * wet_gain);

        const double in_r = buffer[i * 2 + 1];
        const double dry_r =
            dry_all_pass_low_[1].ProcessSample(dry_all_pass_high_[1].ProcessSample(in_r));
        double harm_r = high_pass_[1].ProcessSample(in_r);
        harm_r = quadric_[1].Process(harm_r);
        harm_r = low_pass_[1].ProcessSample(harm_r);
        buffer[i * 2 + 1] = static_cast<float>(dry_r * dry_gain + harm_r * wet_gain);
    }
}

void TubeSimulator::Reset() {
    const float lp_cutoff = static_cast<float>(sampling_rate_) / 2.0f - 2000.0f;

    for (uint32_t ch = 0; ch < 2; ch++) {
        high_pass_[ch].RefreshFilter(
            MultiBiquad::FilterType::HIGH_PASS,
            0.0f,
            120.0f,
            sampling_rate_,
            0.717f,
            false
        );
        low_pass_[ch].RefreshFilter(
            MultiBiquad::FilterType::LOW_PASS,
            0.0f,
            lp_cutoff,
            sampling_rate_,
            0.717f,
            false
        );
        dry_all_pass_high_[ch].RefreshFilter(
            MultiBiquad::FilterType::ALL_PASS, 0.0f, 120.0f, sampling_rate_, 0.717f, false
        );
        dry_all_pass_low_[ch].RefreshFilter(
            MultiBiquad::FilterType::ALL_PASS,
            0.0f,
            lp_cutoff,
            sampling_rate_,
            0.717f,
            false
        );
        quadric_[ch].Configure(tube_type_, drive_);
        quadric_[ch].Reset();
    }
}

void TubeSimulator::SetEnable(const bool enable) {
    if (enable_ != enable) {
        if (!enable_) {
            Reset();
        }
        enable_ = enable;
    }
}

void TubeSimulator::SetSamplingRate(const uint32_t sampling_rate) {
    if (sampling_rate_ != sampling_rate) {
        sampling_rate_ = sampling_rate;
        Reset();
    }
}

void TubeSimulator::SetTubeType(const int tube_type) {
    if (tube_type_ != tube_type) {
        tube_type_ = tube_type;
        Reset();
    }
}

void TubeSimulator::SetDrive(const float drive) {
    const float clamped = drive < 0.0f ? 0.0f : drive > 1.0f ? 1.0f : drive;
    if (drive_ != clamped) {
        drive_ = clamped;
        Reset();
    }
}

void TubeSimulator::SetMix(const float mix) {
    const float clamped = mix < 0.0f ? 0.0f : mix > 1.0f ? 1.0f : mix;
    if (mix_ != clamped) {
        mix_ = clamped;
    }
}
