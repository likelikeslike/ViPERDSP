#include "FIR.h"

FIR::FIR() :
    has_coefficients_(false),
    coeffs_size_(0),
    block_length_(0) {}

void FIR::FilterSamplesInterleaved(
    float *samples, const uint32_t size, const uint32_t channels
) {
    if (!has_coefficients_ || size == 0) return;

    for (uint32_t i = 0; i < size; i++) {
        block_[i] = samples[i * channels];
    }

    if (block_length_ > size) {
        memset(block_.data() + size, 0, (block_length_ - size) * sizeof(float));
    }

    memcpy(
        offset_block_.data() + coeffs_size_ - 1,
        block_.data(),
        block_length_ * sizeof(float)
    );

    for (uint32_t i = 0; i < block_length_; i++) {
        float sample = 0.0f;

        for (uint32_t j = 0; j < coeffs_size_; j++) {
            sample += coeffs_[j] * offset_block_[coeffs_size_ + i - j - 1];
        }

        if (i < size) {
            samples[i * channels] = sample;
        }
    }

    if (coeffs_size_ > 1) {
        const uint32_t carry_count = coeffs_size_ - 1;
        for (uint32_t i = 0; i < carry_count; i++) {
            offset_block_[i] = block_[block_length_ - carry_count + i];
        }
    }
}

void FIR::Reset() {
    if (coeffs_size_ + block_length_ > 0) {
        memset(
            offset_block_.data(), 0, (coeffs_size_ + block_length_ + 1) * sizeof(float)
        );
    }
}

int FIR::LoadCoefficients(
    const float *coeffs, const uint32_t coeffs_size, const uint32_t block_length
) {
    if (coeffs == nullptr || coeffs_size == 0 || block_length == 0) return 0;

    offset_block_ = std::vector<float>(coeffs_size + block_length + 1);
    coeffs_ = std::vector<float>(coeffs_size);
    block_ = std::vector<float>(block_length);

    coeffs_size_ = coeffs_size;
    block_length_ = block_length;

    memcpy(coeffs_.data(), coeffs, coeffs_size * sizeof(float));

    Reset();
    has_coefficients_ = true;

    return 1;
}

uint32_t FIR::GetBlockLength() const {
    return block_length_;
}
