#include "CRevModel.h"

CRevModel::CRevModel() {
    static const uint32_t sizes[24] = {1116, 1139, 1188, 1211, 1277, 1300, 1356, 1379,
                                       1422, 1445, 1491, 1514, 1557, 1580, 1617, 1640,
                                       556,  579,  441,  464,  341,  364,  225,  248};
    uint32_t total = 0;
    for (const uint32_t size : sizes)
        total += size;

    buffer_pool_ = new float[total];
    uint32_t offset = 0;
    for (int i = 0; i < 24; i++) {
        buffers_[i] = buffer_pool_ + offset;
        offset += sizes[i];
    }

    comb_l_[0].SetBuffer(buffers_[0], 1116);
    comb_r_[0].SetBuffer(buffers_[1], 1139);
    comb_l_[1].SetBuffer(buffers_[2], 1188);
    comb_r_[1].SetBuffer(buffers_[3], 1211);
    comb_l_[2].SetBuffer(buffers_[4], 1277);
    comb_r_[2].SetBuffer(buffers_[5], 1300);
    comb_l_[3].SetBuffer(buffers_[6], 1356);
    comb_r_[3].SetBuffer(buffers_[7], 1379);
    comb_l_[4].SetBuffer(buffers_[8], 1422);
    comb_r_[4].SetBuffer(buffers_[9], 1445);
    comb_l_[5].SetBuffer(buffers_[10], 1491);
    comb_r_[5].SetBuffer(buffers_[11], 1514);
    comb_l_[6].SetBuffer(buffers_[12], 1557);
    comb_r_[6].SetBuffer(buffers_[13], 1580);
    comb_l_[7].SetBuffer(buffers_[14], 1617);
    comb_r_[7].SetBuffer(buffers_[15], 1640);

    allpass_l_[0].SetBuffer(buffers_[16], 556);
    allpass_r_[0].SetBuffer(buffers_[17], 579);
    allpass_l_[1].SetBuffer(buffers_[18], 441);
    allpass_r_[1].SetBuffer(buffers_[19], 464);
    allpass_l_[2].SetBuffer(buffers_[20], 341);
    allpass_r_[2].SetBuffer(buffers_[21], 364);
    allpass_l_[3].SetBuffer(buffers_[22], 225);
    allpass_r_[3].SetBuffer(buffers_[23], 248);

    allpass_l_[0].SetFeedback(0.5f);
    allpass_r_[0].SetFeedback(0.5f);
    allpass_l_[1].SetFeedback(0.5f);
    allpass_r_[1].SetFeedback(0.5f);
    allpass_l_[2].SetFeedback(0.5f);
    allpass_r_[2].SetFeedback(0.5f);
    allpass_l_[3].SetFeedback(0.5f);
    allpass_r_[3].SetFeedback(0.5f);

    SetWet(0.167f);
    SetRoomSize(0.5f);
    SetDry(0.25f);
    SetDamp(0.5f);
    SetWidth(1.0f);

    Reset();
}

CRevModel::~CRevModel() {
    delete[] buffer_pool_;
}

void CRevModel::ProcessReplace(float *buf_l, float *buf_r, const uint32_t size) {
    for (uint32_t idx = 0; idx < size * 2; idx += 2) {
        float out_l = 0.0f;
        float out_r = 0.0f;
        const float input = (buf_l[idx] + buf_r[idx]) * gain_;

        for (uint32_t i = 0; i < 8; i++) {
            out_l += comb_l_[i].Process(input);
            out_r += comb_r_[i].Process(input);
        }

        for (uint32_t i = 0; i < 4; i++) {
            out_l = allpass_l_[i].Process(out_l);
            out_r = allpass_r_[i].Process(out_r);
        }

        buf_l[idx] = out_l * wet1_ + out_r * wet2_ + buf_l[idx] * dry_;
        buf_r[idx] = out_r * wet1_ + out_l * wet2_ + buf_r[idx] * dry_;
    }
}

void CRevModel::Mute() const {
    for (int i = 0; i < 8; i++) {
        comb_l_[i].Mute();
        comb_r_[i].Mute();
    }

    for (int i = 0; i < 4; i++) {
        allpass_l_[i].Mute();
        allpass_r_[i].Mute();
    }
}

void CRevModel::Reset() const {
    for (int i = 0; i < 8; i++) {
        comb_l_[i].Mute();
        comb_r_[i].Mute();
    }

    for (int i = 0; i < 4; i++) {
        allpass_l_[i].Mute();
        allpass_r_[i].Mute();
    }
}

void CRevModel::SetRoomSize(const float value) {
    room_size_ = value * 0.28f + 0.7f;
    UpdateCoeffs();
}

void CRevModel::SetDamp(const float value) {
    damp_ = value * 0.4f;
    UpdateCoeffs();
}

void CRevModel::SetWet(const float value) {
    wet_ = value * 3.0f;
    UpdateCoeffs();
}

void CRevModel::SetDry(const float value) {
    dry_ = value * 2.0f;
}

void CRevModel::SetWidth(const float value) {
    width_ = value;
    UpdateCoeffs();
}

float CRevModel::GetRoomSize() const {
    return (room_size_ - 0.7f) / 0.28f;
}

float CRevModel::GetDamp() const {
    return damp_ / 0.4f;
}

float CRevModel::GetDry() const {
    return dry_ / 2.0f;
}

float CRevModel::GetWet() const {
    return wet_ / 3.0f;
}

float CRevModel::GetWidth() const {
    return width_;
}

void CRevModel::UpdateCoeffs() {
    wet1_ = wet_ * (width_ / 2.0f + 0.5f);
    wet2_ = wet_ * (1.0f - width_) / 2.0f;

    internal_room_size_ = room_size_;
    internal_damp_ = damp_;
    gain_ = 0.015f;

    for (int i = 0; i < 8; i++) {
        comb_l_[i].SetFeedback(internal_room_size_);
        comb_l_[i].SetDamp(internal_damp_);
        comb_r_[i].SetFeedback(internal_room_size_);
        comb_r_[i].SetDamp(internal_damp_);
    }
}
