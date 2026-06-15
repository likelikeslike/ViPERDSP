#pragma once

#include <array>
#include <vector>

class ViPERDDC {
public:
    ViPERDDC();

    void Process(float *samples, uint32_t size);
    void Reset();

    void SetEnable(bool enable);
    void SetCoeffs(
        uint32_t coeffs_size, const float *coeffs_44100, const float *coeffs_48000
    );
    void SetSamplingRate(uint32_t sampling_rate);

private:
    bool enable_;
    bool set_coeffs_ok_;

    uint32_t sampling_rate_;
    uint32_t arr_size_;

    std::vector<std::array<float, 5>> coeffs_arr44100_;
    std::vector<std::array<float, 5>> coeffs_arr48000_;
    std::vector<float> x1_l_;
    std::vector<float> x1_r_;
    std::vector<float> x2_l_;
    std::vector<float> x2_r_;
    std::vector<float> y1_l_;
    std::vector<float> y1_r_;
    std::vector<float> y2_l_;
    std::vector<float> y2_r_;

    void ReleaseResources();
};
