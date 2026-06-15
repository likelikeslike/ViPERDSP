#include "SpeakerCorrection.h"
#include "../constants.h"

SpeakerCorrection::SpeakerCorrection() :
    enable_(false),
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE),
    hp_cutoff_(80u),
    lp_cutoff_(13500u),
    bp_center_(420u),
    bp_q_(3.88f) {
    Reset();
}

void SpeakerCorrection::Process(float *samples, const uint32_t size) {
    if (!enable_) return;

    for (uint32_t i = 0; i < size * 2; i += 2) {
        double out_l = samples[i];
        out_l = low_pass_[0].ProcessSample(out_l);
        out_l = high_pass_[0].ProcessSample(out_l);
        out_l /= 2.0;
        out_l += band_pass_[0].ProcessSample(out_l);
        samples[i] = static_cast<float>(out_l);

        double out_r = samples[i + 1];
        out_r = low_pass_[1].ProcessSample(out_r);
        out_r = high_pass_[1].ProcessSample(out_r);
        out_r /= 2.0;
        out_r += band_pass_[1].ProcessSample(out_r);
        samples[i + 1] = static_cast<float>(out_r);
    }
}

void SpeakerCorrection::Reset() {
    low_pass_[0].Reset();
    low_pass_[1].Reset();
    band_pass_[0].Reset();
    band_pass_[1].Reset();
    high_pass_[0].Reset();
    high_pass_[1].Reset();

    RefreshHighPass();
    RefreshLowPass();
    RefreshBandPass();
}

void SpeakerCorrection::SetEnable(const bool enable) {
    if (enable_ != enable) {
        if (enable) {
            Reset();
        }
        enable_ = enable;
    }
}

void SpeakerCorrection::SetSamplingRate(const uint32_t sampling_rate) {
    if (sampling_rate_ != sampling_rate) {
        sampling_rate_ = sampling_rate;
        Reset();
    }
}

void SpeakerCorrection::SetHighPassCutoff(const uint32_t value) {
    if (hp_cutoff_ != value) {
        hp_cutoff_ = value;
        RefreshHighPass();
    }
}

void SpeakerCorrection::SetLowPassCutoff(const uint32_t value) {
    if (lp_cutoff_ != value) {
        lp_cutoff_ = value;
        RefreshLowPass();
    }
}

void SpeakerCorrection::SetBandPassCenter(const uint32_t value) {
    if (bp_center_ != value) {
        bp_center_ = value;
        RefreshBandPass();
    }
}

void SpeakerCorrection::SetBandPassQ(const float value) {
    if (bp_q_ != value) {
        bp_q_ = value;
        RefreshBandPass();
    }
}

void SpeakerCorrection::RefreshHighPass() {
    high_pass_[0].RefreshFilter(
        MultiBiquad::FilterType::HIGH_PASS,
        0.0f,
        static_cast<float>(hp_cutoff_),
        sampling_rate_,
        1.0f,
        false
    );
    high_pass_[1].RefreshFilter(
        MultiBiquad::FilterType::HIGH_PASS,
        0.0f,
        static_cast<float>(hp_cutoff_),
        sampling_rate_,
        1.0f,
        false
    );
}

void SpeakerCorrection::RefreshLowPass() {
    low_pass_[0].SetLowPassParameter(
        static_cast<float>(lp_cutoff_), sampling_rate_, 1.0f
    );
    low_pass_[1].SetLowPassParameter(
        static_cast<float>(lp_cutoff_), sampling_rate_, 1.0f
    );
}

void SpeakerCorrection::RefreshBandPass() {
    band_pass_[0].SetBandPassParameter(
        static_cast<float>(bp_center_), sampling_rate_, bp_q_
    );
    band_pass_[1].SetBandPassParameter(
        static_cast<float>(bp_center_), sampling_rate_, bp_q_
    );
}
