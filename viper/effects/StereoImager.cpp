#include "StereoImager.h"
#include "../constants.h"

static constexpr float BUTTERWORTH_Q = 0.7071f;

StereoImager::StereoImager() {
    this->enable = false;
    this->samplingRate = VIPER_DEFAULT_SAMPLING_RATE;

    this->bandWidths[0] = 1.0f;
    this->bandWidths[1] = 1.0f;
    this->bandWidths[2] = 1.0f;

    this->crossoverFreqs[0] = 200.0f;
    this->crossoverFreqs[1] = 4000.0f;

    ConfigureCrossovers();
}

void StereoImager::Process(float *samples, uint32_t size) {
    if (!this->enable) return;
    if (size == 0) return;

    uint32_t frameCount = size * 2;

    for (uint32_t b = 0; b < NUM_BANDS; b++) {
        if (this->bandBuffers[b].size() < frameCount) {
            this->bandBuffers[b].resize(frameCount);
        }
    }

    for (uint32_t b = 0; b < NUM_BANDS; b++) {
        for (uint32_t i = 0; i < frameCount; i += 2) {
            double sL = static_cast<double>(samples[i]);
            double sR = static_cast<double>(samples[i + 1]);

            if (b == 0) {
                sL = this->lowpassLA[0].ProcessSample(sL);
                sL = this->lowpassLB[0].ProcessSample(sL);
                sR = this->lowpassRA[0].ProcessSample(sR);
                sR = this->lowpassRB[0].ProcessSample(sR);
            } else if (b == 2) {
                sL = this->highpassLA[1].ProcessSample(sL);
                sL = this->highpassLB[1].ProcessSample(sL);
                sR = this->highpassRA[1].ProcessSample(sR);
                sR = this->highpassRB[1].ProcessSample(sR);
            } else {
                sL = this->highpassLA[0].ProcessSample(sL);
                sL = this->highpassLB[0].ProcessSample(sL);
                sL = this->lowpassLA[1].ProcessSample(sL);
                sL = this->lowpassLB[1].ProcessSample(sL);
                sR = this->highpassRA[0].ProcessSample(sR);
                sR = this->highpassRB[0].ProcessSample(sR);
                sR = this->lowpassRA[1].ProcessSample(sR);
                sR = this->lowpassRB[1].ProcessSample(sR);
            }

            float fL = static_cast<float>(sL);
            float fR = static_cast<float>(sR);

            float mid = (fL + fR) * 0.5f;
            float side = (fL - fR) * 0.5f;
            side *= this->bandWidths[b];

            this->bandBuffers[b][i] = mid + side;
            this->bandBuffers[b][i + 1] = mid - side;
        }
    }

    for (uint32_t i = 0; i < frameCount; i += 2) {
        float sumL = 0.0f;
        float sumR = 0.0f;
        for (uint32_t b = 0; b < NUM_BANDS; b++) {
            sumL += this->bandBuffers[b][i];
            sumR += this->bandBuffers[b][i + 1];
        }
        samples[i] = sumL;
        samples[i + 1] = sumR;
    }
}

void StereoImager::Reset() {
    ConfigureCrossovers();
}

void StereoImager::SetEnable(bool enable) {
    if (this->enable != enable) {
        if (enable) Reset();
        this->enable = enable;
    }
}

void StereoImager::SetSamplingRate(uint32_t samplingRate) {
    if (this->samplingRate != samplingRate) {
        this->samplingRate = samplingRate;
        ConfigureCrossovers();
    }
}

void StereoImager::SetLowWidth(float widthPercent) {
    this->bandWidths[0] = widthPercent / 100.0f;
}

void StereoImager::SetMidWidth(float widthPercent) {
    this->bandWidths[1] = widthPercent / 100.0f;
}

void StereoImager::SetHighWidth(float widthPercent) {
    this->bandWidths[2] = widthPercent / 100.0f;
}

void StereoImager::SetLowCrossover(float frequency) {
    if (this->crossoverFreqs[0] != frequency) {
        this->crossoverFreqs[0] = frequency;
        ConfigureCrossovers();
    }
}

void StereoImager::SetHighCrossover(float frequency) {
    if (this->crossoverFreqs[1] != frequency) {
        this->crossoverFreqs[1] = frequency;
        ConfigureCrossovers();
    }
}

void StereoImager::ConfigureCrossovers() {
    for (uint32_t i = 0; i < NUM_CROSSOVERS; i++) {
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
