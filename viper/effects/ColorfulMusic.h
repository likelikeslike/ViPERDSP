#pragma once

#include "../utils/DepthSurround.h"
#include "../utils/Stereo3DSurround.h"

class ColorfulMusic {
public:
    ColorfulMusic();

    void Process(float *samples, uint32_t size);
    void Reset();

    void SetEnable(bool enable);
    void SetDepthValue(uint32_t value);
    void SetMidImageValue(float value);
    void SetWidenValue(float value);
    void SetSamplingRate(uint32_t sampling_rate);

private:
    bool enabled_;

    uint32_t sampling_rate_;

    Stereo3DSurround stereo_3d_surround_;
    DepthSurround depth_surround_;
};
