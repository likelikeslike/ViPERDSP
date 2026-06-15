#include "HighShelf.h"
#include <cmath>

HighShelf::HighShelf() :
    frequency_(0.0f),
    gain_(0.0),
    x1_(0.0),
    x2_(0.0),
    y1_(0.0),
    y2_(0.0),
    b0_(0.0),
    b1_(0.0),
    b2_(0.0),
    a0_(0.0),
    a1_(0.0),
    a2_(0.0) {}

double HighShelf::Process(const double sample) {
    const double out =
        (x1_ * b1_ + sample * b0_ + b2_ * x2_ - y1_ * a1_ - a2_ * y2_) * a0_;
    y2_ = y1_;
    y1_ = out;
    x2_ = x1_;
    x1_ = sample;
    return out;
}

void HighShelf::SetFrequency(const float value) {
    frequency_ = value;
}

void HighShelf::SetGain(const float value) {
    gain_ = 20.0 * log10(value);
}

void HighShelf::SetSamplingRate(const uint32_t sampling_rate) {
    const double x = 2 * M_PI * frequency_ / sampling_rate;
    const double sin_x = sin(x);
    const double cos_x = cos(x);
    const double y = exp(gain_ * log(10.0) / 40.0);

    x1_ = 0.0;
    x2_ = 0.0;
    y1_ = 0.0;
    y2_ = 0.0;

    const double z = sqrt(y * 2.0) * sin_x;
    const double a = (y - 1.0) * cos_x;
    const double b = y + 1.0 - a;
    const double c = z + b;
    const double d = (y + 1.0) * cos_x;
    const double e = y + 1.0 + a;
    const double f = y - 1.0 - d;

    a0_ = 1.0 / c;
    a1_ = f * 2.0;
    a2_ = b - z;
    b0_ = (e + z) * y;
    b1_ = -y * 2.0 * (y - 1.0 + d);
    b2_ = (e - z) * y;
}
