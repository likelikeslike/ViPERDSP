#pragma once

#include <vector>

class FIR {
public:
    FIR();

    void FilterSamplesInterleaved(float *samples, uint32_t size, uint32_t channels);
    void Reset();

    [[nodiscard]] uint32_t GetBlockLength() const;

    int LoadCoefficients(
        const float *coeffs, uint32_t coeffs_size, uint32_t block_length
    );

private:
    bool has_coefficients_;

    uint32_t coeffs_size_;
    uint32_t block_length_;

    std::vector<float> offset_block_;
    std::vector<float> coeffs_;
    std::vector<float> block_;
};
