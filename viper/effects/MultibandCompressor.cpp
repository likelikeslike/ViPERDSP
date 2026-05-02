#include "MultibandCompressor.h"
#include "../constants.h"
#include <cstring>

static constexpr float DEFAULT_3BAND_FREQS[] = {200.0f, 4000.0f};
static constexpr float DEFAULT_5BAND_FREQS[] = {120.0f, 500.0f, 4000.0f, 8000.0f};
static constexpr float BUTTERWORTH_Q = 0.7071f;

MultibandCompressor::MultibandCompressor() {
    this->enable = false;
    this->samplingRate = VIPER_DEFAULT_SAMPLING_RATE;
    this->bandCount = 3;

    this->crossoverFreqs[0] = DEFAULT_3BAND_FREQS[0];
    this->crossoverFreqs[1] = DEFAULT_3BAND_FREQS[1];
    this->crossoverFreqs[2] = 0.0f;
    this->crossoverFreqs[3] = 0.0f;

    for (uint32_t i = 0; i < MAX_BANDS; i++) {
        this->compressors[i].SetSamplingRate(this->samplingRate);
        this->compressors[i].Reset();
    }

    ConfigureCrossovers();
}

void MultibandCompressor::Process(float *samples, uint32_t size) {
    if (!this->enable) return;
    if (size == 0) return;

    uint32_t numCrossovers = this->bandCount - 1;
    uint32_t frameCount = size * 2;

    for (uint32_t b = 0; b < this->bandCount; b++) {
        if (this->bandBuffers[b].size() < frameCount) {
            this->bandBuffers[b].resize(frameCount);
        }
    }

    for (uint32_t b = 0; b < this->bandCount; b++) {
        for (uint32_t i = 0; i < frameCount; i += 2) {
            double sL = static_cast<double>(samples[i]);
            double sR = static_cast<double>(samples[i + 1]);

            if (b == 0) {
                sL = this->lowpassLA[0].ProcessSample(sL);
                sL = this->lowpassLB[0].ProcessSample(sL);
                sR = this->lowpassRA[0].ProcessSample(sR);
                sR = this->lowpassRB[0].ProcessSample(sR);
            } else if (b == numCrossovers) {
                sL = this->highpassLA[numCrossovers - 1].ProcessSample(sL);
                sL = this->highpassLB[numCrossovers - 1].ProcessSample(sL);
                sR = this->highpassRA[numCrossovers - 1].ProcessSample(sR);
                sR = this->highpassRB[numCrossovers - 1].ProcessSample(sR);
            } else {
                sL = this->highpassLA[b - 1].ProcessSample(sL);
                sL = this->highpassLB[b - 1].ProcessSample(sL);
                sL = this->lowpassLA[b].ProcessSample(sL);
                sL = this->lowpassLB[b].ProcessSample(sL);
                sR = this->highpassRA[b - 1].ProcessSample(sR);
                sR = this->highpassRB[b - 1].ProcessSample(sR);
                sR = this->lowpassRA[b].ProcessSample(sR);
                sR = this->lowpassRB[b].ProcessSample(sR);
            }

            this->bandBuffers[b][i] = static_cast<float>(sL);
            this->bandBuffers[b][i + 1] = static_cast<float>(sR);
        }

        this->compressors[b].Process(this->bandBuffers[b].data(), size);
    }

    for (uint32_t i = 0; i < frameCount; i += 2) {
        float sumL = 0.0f;
        float sumR = 0.0f;
        for (uint32_t b = 0; b < this->bandCount; b++) {
            sumL += this->bandBuffers[b][i];
            sumR += this->bandBuffers[b][i + 1];
        }
        samples[i] = sumL;
        samples[i + 1] = sumR;
    }
}

void MultibandCompressor::Reset() {
    for (uint32_t i = 0; i < MAX_BANDS; i++) {
        this->compressors[i].Reset();
    }
    ConfigureCrossovers();
}

void MultibandCompressor::SetEnable(bool enable) {
    if (this->enable != enable) {
        if (enable) Reset();
        this->enable = enable;
    }
}

void MultibandCompressor::SetSamplingRate(uint32_t samplingRate) {
    if (this->samplingRate != samplingRate) {
        this->samplingRate = samplingRate;
        for (uint32_t i = 0; i < MAX_BANDS; i++) {
            this->compressors[i].SetSamplingRate(this->samplingRate);
        }
        ConfigureCrossovers();
    }
}

void MultibandCompressor::SetBandCount(uint32_t bandCount) {
    if (bandCount != 3 && bandCount != 5) return;
    if (this->bandCount != bandCount) {
        this->bandCount = bandCount;
        if (bandCount == 3) {
            this->crossoverFreqs[0] = DEFAULT_3BAND_FREQS[0];
            this->crossoverFreqs[1] = DEFAULT_3BAND_FREQS[1];
            this->crossoverFreqs[2] = 0.0f;
            this->crossoverFreqs[3] = 0.0f;
        } else {
            this->crossoverFreqs[0] = DEFAULT_5BAND_FREQS[0];
            this->crossoverFreqs[1] = DEFAULT_5BAND_FREQS[1];
            this->crossoverFreqs[2] = DEFAULT_5BAND_FREQS[2];
            this->crossoverFreqs[3] = DEFAULT_5BAND_FREQS[3];
        }
        Reset();
    }
}

void MultibandCompressor::SetCrossoverFrequency(uint32_t index, float frequency) {
    uint32_t numCrossovers = this->bandCount - 1;
    if (index >= numCrossovers) return;
    if (this->crossoverFreqs[index] != frequency) {
        this->crossoverFreqs[index] = frequency;
        ConfigureCrossovers();
    }
}

void MultibandCompressor::SetBandParameter(
    uint32_t band, FETCompressor::Parameter param, float value
) {
    if (band >= this->bandCount) return;
    this->compressors[band].SetParameter(param, value);
}

void MultibandCompressor::ConfigureCrossovers() {
    uint32_t numCrossovers = this->bandCount - 1;
    for (uint32_t i = 0; i < numCrossovers; i++) {
        this->lowpassLA[i].RefreshFilter(
            MultiBiquad::LOW_PASS,
            1.0f,
            this->crossoverFreqs[i],
            this->samplingRate,
            BUTTERWORTH_Q,
            false
        );
        this->lowpassLB[i].RefreshFilter(
            MultiBiquad::LOW_PASS,
            1.0f,
            this->crossoverFreqs[i],
            this->samplingRate,
            BUTTERWORTH_Q,
            false
        );
        this->lowpassRA[i].RefreshFilter(
            MultiBiquad::LOW_PASS,
            1.0f,
            this->crossoverFreqs[i],
            this->samplingRate,
            BUTTERWORTH_Q,
            false
        );
        this->lowpassRB[i].RefreshFilter(
            MultiBiquad::LOW_PASS,
            1.0f,
            this->crossoverFreqs[i],
            this->samplingRate,
            BUTTERWORTH_Q,
            false
        );
        this->highpassLA[i].RefreshFilter(
            MultiBiquad::HIGH_PASS,
            1.0f,
            this->crossoverFreqs[i],
            this->samplingRate,
            BUTTERWORTH_Q,
            false
        );
        this->highpassLB[i].RefreshFilter(
            MultiBiquad::HIGH_PASS,
            1.0f,
            this->crossoverFreqs[i],
            this->samplingRate,
            BUTTERWORTH_Q,
            false
        );
        this->highpassRA[i].RefreshFilter(
            MultiBiquad::HIGH_PASS,
            1.0f,
            this->crossoverFreqs[i],
            this->samplingRate,
            BUTTERWORTH_Q,
            false
        );
        this->highpassRB[i].RefreshFilter(
            MultiBiquad::HIGH_PASS,
            1.0f,
            this->crossoverFreqs[i],
            this->samplingRate,
            BUTTERWORTH_Q,
            false
        );
    }
}
