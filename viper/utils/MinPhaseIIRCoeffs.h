#pragma once

#include <cstdint>

class MinPhaseIIRCoeffs {
public:
    MinPhaseIIRCoeffs();
    ~MinPhaseIIRCoeffs();

    [[nodiscard]] double *GetCoefficients() const;
    [[nodiscard]] float GetIndexFrequency(uint32_t index) const;

    int UpdateCoeffs(uint32_t bands, uint32_t sampling_rate);

private:
    uint32_t bands_;

    double *coeffs_;

    static void Find_F1_F2(
        double center_freq,
        double bandwidth_octaves,
        double *lower_freq,
        double *upper_freq
    );
    static int SolveRoot(double coeff_a, double coeff_b, double coeff_c, double *root);
};
