#include "IIR_1st.h"
#include <cmath>

inline float Angle(const float frequency, const uint32_t sampling_rate) {
    return exp(
        -1.0f * static_cast<float>(M_PI) * frequency
        / (static_cast<float>(sampling_rate) / 2.0f)
    );
}

inline float Omega(const float frequency, const uint32_t sampling_rate) {
    return 2.0f * static_cast<float>(M_PI) * frequency
           / static_cast<float>(sampling_rate);
}

inline float Omega2(const float frequency, const uint32_t sampling_rate) {
    return static_cast<float>(M_PI) * frequency / static_cast<float>(sampling_rate);
}

IIR_1st::IIR_1st() :
    b0_(0.0f),
    b1_(0.0f),
    a1_(0.0f),
    prev_sample_(0.0f) {}

void IIR_1st::Mute() {
    prev_sample_ = 0.0f;
}

void IIR_1st::SetCoefficients(const float b0, const float b1, const float a1) {
    b0_ = b0;
    b1_ = b1;
    a1_ = a1;
}

void IIR_1st::SetHighPassFilterBW(const float frequency, const uint32_t sampling_rate) {
    const float omega_2 = Omega2(frequency, sampling_rate);
    const float tan_omega_2 = tanf(omega_2);
    b0_ = 1 / (1 + tan_omega_2);
    b1_ = -b0_;
    a1_ = (1 - tan_omega_2) / (1 + tan_omega_2);
}

void IIR_1st::SetLowPassFilterBW(const float frequency, const uint32_t sampling_rate) {
    const float omega_2 = Omega2(frequency, sampling_rate);
    const float tan_omega_2 = tanf(omega_2);
    a1_ = (1 - tan_omega_2) / (1 + tan_omega_2);
    b0_ = tan_omega_2 / (1 + tan_omega_2);
    b1_ = b0_;
}
