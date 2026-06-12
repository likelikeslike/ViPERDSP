#pragma once

#include <array>

class TubeSimulator {
public:
    TubeSimulator();

    void Process(float *buffer, uint32_t size);
    void Reset();

    void SetEnable(bool enable);

private:
    bool enable_;

    std::array<double, 2> acc_;
};
