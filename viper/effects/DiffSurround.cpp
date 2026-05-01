#include "DiffSurround.h"
#include "../constants.h"
#include <cstring>

DiffSurround::DiffSurround() :
    buffers({WaveBuffer(1, 0x1000), WaveBuffer(1, 0x1000)}) {
    this->samplingRate = VIPER_DEFAULT_SAMPLING_RATE;
    this->delayTime = 0.0f;
    this->enable = false;
    this->reverse = false;
    this->wetDryMix = 1.0f;
    this->lpCutoff = 0.0f;
    Reset();
}

void DiffSurround::Process(float *samples, uint32_t size) {
    if (!this->enable) return;

    float *bufs[2];
    float *outbufs[2];

    bufs[0] = this->buffers[0].PushZerosGetBuffer(size);
    bufs[1] = this->buffers[1].PushZerosGetBuffer(size);

    for (uint32_t i = 0; i < size * 2; i++) {
        bufs[i % 2][i / 2] = samples[i];
    }

    outbufs[0] = this->buffers[0].GetBuffer();
    outbufs[1] = this->buffers[1].GetBuffer();

    if (this->wetDryMix >= 1.0f && this->lpCutoff <= 0.0f) {
        for (uint32_t i = 0; i < size * 2; i++) {
            samples[i] = outbufs[i % 2][i / 2];
        }
    } else {
        int delayedCh = this->reverse ? 0 : 1;
        int directCh = 1 - delayedCh;
        float wet = this->wetDryMix;
        float dry = 1.0f - wet;

        for (uint32_t i = 0; i < size; i++) {
            float directSample = outbufs[directCh][i];
            float delayedSample = outbufs[delayedCh][i];

            if (this->lpCutoff > 0.0f) {
                delayedSample = (float) this->lpFilter.ProcessSample(delayedSample);
            }

            samples[i * 2 + directCh] = directSample;
            samples[i * 2 + delayedCh] = dry * directSample + wet * delayedSample;
        }
    }

    this->buffers[0].PopSamples(size, false);
    this->buffers[1].PopSamples(size, false);
}

void DiffSurround::Reset() {
    this->buffers[0].Reset();
    this->buffers[1].Reset();

    uint32_t delaySamples =
        (uint32_t) ((double) this->delayTime / 1000.0 * (double) this->samplingRate);
    this->buffers[this->reverse ? 0 : 1].PushZeros(delaySamples);

    if (this->lpCutoff > 0.0f) {
        this->lpFilter.RefreshFilter(
            MultiBiquad::FilterType::LOW_PASS,
            0.0,
            this->lpCutoff,
            this->samplingRate,
            0.7071,
            false
        );
    }
}

void DiffSurround::SetDelayTime(float delayTime) {
    if (this->delayTime != delayTime) {
        this->delayTime = delayTime;
        this->Reset();
    }
}

void DiffSurround::SetEnable(bool enable) {
    if (this->enable != enable) {
        if (!this->enable) {
            Reset();
        }
        this->enable = enable;
    }
}

void DiffSurround::SetReverse(bool reverse) {
    if (this->reverse != reverse) {
        this->reverse = reverse;
        this->Reset();
    }
}

void DiffSurround::SetSamplingRate(uint32_t samplingRate) {
    if (this->samplingRate != samplingRate) {
        this->samplingRate = samplingRate;
        this->Reset();
    }
}

void DiffSurround::SetWetDryMix(float mix) {
    if (mix < 0.0f) mix = 0.0f;
    if (mix > 1.0f) mix = 1.0f;
    this->wetDryMix = mix;
}

void DiffSurround::SetLPCutoff(float cutoff) {
    if (cutoff < 0.0f) cutoff = 0.0f;
    if (cutoff > 20000.0f) cutoff = 20000.0f;
    if (this->lpCutoff != cutoff) {
        this->lpCutoff = cutoff;
        if (cutoff > 0.0f) {
            this->lpFilter.RefreshFilter(
                MultiBiquad::FilterType::LOW_PASS,
                0.0,
                cutoff,
                this->samplingRate,
                0.7071,
                false
            );
        }
    }
}
