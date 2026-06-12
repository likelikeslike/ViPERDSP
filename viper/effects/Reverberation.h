#pragma once

#include "../utils/CRevModel.h"
#include <cstdint>

class Reverberation {
public:
    Reverberation();

    void Process(float *buffer, uint32_t size);
    void Reset() const;

    void SetEnable(bool enable);
    void SetDamp(float value);
    void SetDry(float value);
    void SetRoomSize(float value);
    void SetWet(float value);
    void SetWidth(float value);

private:
    bool enable_;

    CRevModel model_;
};
