#pragma once

#include <cstdint>

class Stereo3DSurround {
public:
    Stereo3DSurround();

    void Process(float *sample, uint32_t size) const;

    void SetMiddleImage(float value);
    void SetStereoWiden(float value);

private:
    float stereo_widen_;
    float middle_image_;
    float coeff_left_;
    float coeff_right_;

    void ConfigureVariables();
};
