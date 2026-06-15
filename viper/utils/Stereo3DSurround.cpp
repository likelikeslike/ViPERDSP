#include "Stereo3DSurround.h"

Stereo3DSurround::Stereo3DSurround() :
    stereo_widen_(0.0f),
    middle_image_(1.0f),
    coeff_left_(0.5f),
    coeff_right_(0.5f) {}

void Stereo3DSurround::Process(float *sample, const uint32_t size) const {
    if (size == 0) return;

    const uint32_t pairs = size / 2;
    const uint32_t remainder = size % 2;

    if (pairs > 0) {
        for (uint32_t i = 0; i < pairs; i++) {
            const float a = coeff_left_ * (sample[4 * i] + sample[4 * i + 1]);
            const float b = coeff_right_ * (sample[4 * i + 1] - sample[4 * i]);
            const float c = coeff_left_ * (sample[4 * i + 2] + sample[4 * i + 3]);
            const float d = coeff_right_ * (sample[4 * i + 3] - sample[4 * i + 2]);

            sample[4 * i] = a - b;
            sample[4 * i + 1] = a + b;
            sample[4 * i + 2] = c - d;
            sample[4 * i + 3] = c + d;
        }
    }

    if (remainder > 0) {
        for (uint32_t i = 4 * pairs; i < 2 * size; i += 2) {
            const float a = sample[i];
            const float b = sample[i + 1];
            const float c = coeff_left_ * (a + b);
            const float d = coeff_right_ * (b - a);

            sample[i] = c - d;
            sample[i + 1] = c + d;
        }
    }
}

void Stereo3DSurround::SetMiddleImage(const float value) {
    middle_image_ = value;
    ConfigureVariables();
}

void Stereo3DSurround::SetStereoWiden(const float value) {
    stereo_widen_ = value;
    ConfigureVariables();
}

void Stereo3DSurround::ConfigureVariables() {
    const float tmp = stereo_widen_ + 1.0f;

    const float x = tmp + 1.0f;
    float y;
    if (x < 2.0f) {
        y = 0.5f;
    } else {
        y = 1.0f / x;
    }

    coeff_left_ = middle_image_ * y;
    coeff_right_ = tmp * y;
}
