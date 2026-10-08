#pragma once

struct QuadricTubeModel {
    double kp;
    double kp2;
    double kpg;
    double vdd;
    double rp;
    double bias;
};

class QuadricTube {
public:
    QuadricTube();

    // type: 0=12AX7, 1=12AU7, 2=6N1P.
    void Configure(int type, double drive);

    double Process(double sample);
    void Reset();

private:
    void SetTubeModel(const QuadricTubeModel &model, double vdd, double rp, double bias);
    [[nodiscard]] double SolvePlateVoltage(double vgk) const;

    double vdd_;
    double rp_;
    double bias_;
    double drive_;
    double max_drive_;
    double output_scale_;

    double k_2a_;
    double k_4a_;
    double k_b_const_;
    double k_b_vgk_;
    double k_c_vgk2_;
    double k_c_vgk_;
    double k_c_const_;

    double last_processed_;
    double prev_out_;
};
