#include "QuadricTube.h"
#include <cmath>

namespace {

// 12AX7: published fit and operating point (Giampiccolo, DAFx 2023, Table 3).
// 12AU7/6N1P: fitted to each tube's datasheet load-line operating point.
constexpr QuadricTubeModel kModels[] = {
    {1.014e-5, 5.498e-8, 1.076e-5, 250.0, 100000.0, -1.5},
    {3.8297e-6, 2.8987e-9, 3.3816e-7, 150.0, 100000.0, -6.0},
    {4.9012e-6, 2.6011e-9, 5.8556e-7, 250.0, 100000.0, -3.0},
};

constexpr int kNumModels = 3;

} // namespace

QuadricTube::QuadricTube() {
    Configure(0, 0.5);
    Reset();
}

void QuadricTube::Configure(const int type, const double drive) {
    const int idx = type < 0 || type >= kNumModels ? 0 : type;
    const QuadricTubeModel &m = kModels[idx];
    const double clamped = drive < 0.0 ? 0.0 : drive > 1.0 ? 1.0 : drive;
    SetTubeModel(m, m.vdd, m.rp, m.bias);

    const double cutoff_disc_b = 2.0 * k_b_const_ * k_b_vgk_ - k_4a_ * k_c_vgk_;
    const double cutoff_disc_c = k_b_const_ * k_b_const_ - k_4a_ * k_c_const_;
    const double cutoff_grid_voltage = -cutoff_disc_c / cutoff_disc_b;

    const double plate_current = vdd_ / rp_;
    const double clip_disc_a = k_c_vgk2_;
    const double clip_disc_b = k_b_vgk_ * plate_current + k_c_vgk_;
    const double clip_disc_c = k_4a_ * plate_current * plate_current / 4.0
                               + k_b_const_ * plate_current + k_c_const_;
    const double clip_disc = clip_disc_b * clip_disc_b - 4.0 * clip_disc_a * clip_disc_c;
    const double clip_grid_voltage =
        (-clip_disc_b + std::sqrt(clip_disc)) / (2.0 * clip_disc_a);

    const double lower_drive = bias_ - cutoff_grid_voltage;
    const double upper_drive = clip_grid_voltage - bias_;
    max_drive_ = std::fmax(std::fmin(lower_drive, upper_drive) * 0.95, 1.0);
    drive_ = 1.0 + clamped * (max_drive_ - 1.0);
}

void QuadricTube::SetTubeModel(
    const QuadricTubeModel &model, const double vdd, const double rp, const double bias
) {
    vdd_ = vdd;
    rp_ = rp;
    bias_ = bias;

    const double ka = model.kp2 * rp_ * rp_;
    k_2a_ = 2.0 * ka;
    k_4a_ = 4.0 * ka;

    k_b_const_ = -2.0 * model.kp2 * vdd_ * rp_ - model.kp * rp_ - 1.0;
    k_b_vgk_ = -(model.kpg * rp_);

    k_c_vgk2_ = model.kpg * model.kpg / (4.0 * model.kp2);
    k_c_vgk_ = model.kpg * vdd_ + model.kp * model.kpg / (2.0 * model.kp2);
    k_c_const_ = model.kp2 * vdd_ * vdd_ + model.kp * vdd_
                 + model.kp * model.kp / (4.0 * model.kp2);

    output_scale_ = -1.0 / (vdd_ / 2.5);

    Reset();
}

double QuadricTube::SolvePlateVoltage(const double vgk) const {
    const double b = k_b_const_ + k_b_vgk_ * vgk;
    const double c = k_c_vgk2_ * vgk * vgk + k_c_vgk_ * vgk + k_c_const_;
    const double disc = b * b - k_4a_ * c;

    double ip = 0.0;
    if (disc >= -1e-12) {
        ip = (-b - std::sqrt(disc < 0.0 ? 0.0 : disc)) / k_2a_;
        if (ip < 0.0) ip = 0.0;
    }

    double vpk = vdd_ - ip * rp_;
    if (vpk < 0.0) vpk = 0.0;
    if (vpk > vdd_) vpk = vdd_;
    return vpk;
}

double QuadricTube::Process(const double sample) {
    const double prev_last = last_processed_;
    const double vgk = sample * drive_ + bias_;

    const double vpk = SolvePlateVoltage(vgk);
    const double y = vpk * output_scale_;

    last_processed_ = y;
    prev_out_ = last_processed_ + prev_out_ * 0.999 - prev_last;
    return prev_out_;
}

void QuadricTube::Reset() {
    const double vpk_q = SolvePlateVoltage(bias_);
    last_processed_ = vpk_q * output_scale_;
    prev_out_ = 0.0;
}
