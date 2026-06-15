#pragma once

#include "CAllPassFilter.h"
#include "CCombFilter.h"

class CRevModel {
public:
    CRevModel();
    ~CRevModel();

    void ProcessReplace(float *buf_l, float *buf_r, uint32_t size);
    void Mute() const;
    void Reset() const;

    void SetRoomSize(float value);
    void SetDamp(float value);
    void SetWet(float value);
    void SetDry(float value);
    void SetWidth(float value);

    [[nodiscard]] float GetRoomSize() const;
    [[nodiscard]] float GetDamp() const;
    [[nodiscard]] float GetWet() const;
    [[nodiscard]] float GetDry() const;
    [[nodiscard]] float GetWidth() const;

    void UpdateCoeffs();

private:
    float gain_;
    float room_size_;
    float internal_room_size_;
    float damp_;
    float internal_damp_;
    float wet_;
    float wet1_;
    float wet2_;
    float dry_;
    float width_;

    CCombFilter comb_l_[8];
    CCombFilter comb_r_[8];

    CAllPassFilter allpass_l_[4];
    CAllPassFilter allpass_r_[4];

    float *buffer_pool_;
    float *buffers_[24];
};
