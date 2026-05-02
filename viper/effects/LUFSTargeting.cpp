#include "LUFSTargeting.h"
#include "../constants.h"
#include <cmath>
#include <cstring>

LUFSTargeting::LUFSTargeting() {
    this->enable = false;
    this->samplingRate = VIPER_DEFAULT_SAMPLING_RATE;
    this->targetLUFS = -14.0f;
    this->maxGainDB = 6.0f;
    this->speed = 1;
    this->smoothedGainDB = 0.0;
    this->sampleCounter = 0;
    this->windowAccumulator = 0.0;
    this->windowSampleCount = 0;
    this->windowWriteIdx = 0;
    this->windowCount = 0;
    memset(this->windowPower, 0, sizeof(this->windowPower));
    ConfigureFilters();
    UpdateSmoothingCoeffs();
    this->windowSize = (uint32_t) (this->samplingRate * 0.4);
    this->stepSize = this->windowSize / 4;
}

void LUFSTargeting::Process(float *samples, uint32_t size) {
    if (!this->enable) return;
    if (size == 0) return;

    double gateThreshold = pow(10.0, (ABSOLUTE_GATE_LUFS + 0.691) / 10.0);

    for (uint32_t i = 0; i < size; i++) {
        double left = (double) samples[i * 2];
        double right = (double) samples[i * 2 + 1];

        double kLeft = this->kWeightStage1L.ProcessSample(left);
        kLeft = this->kWeightStage2L.ProcessSample(kLeft);

        double kRight = this->kWeightStage1R.ProcessSample(right);
        kRight = this->kWeightStage2R.ProcessSample(kRight);

        this->windowAccumulator += kLeft * kLeft + kRight * kRight;
        this->windowSampleCount++;
        this->sampleCounter++;

        if (this->sampleCounter >= this->stepSize) {
            this->sampleCounter = 0;

            if (this->windowSampleCount >= this->windowSize) {
                double meanSquare =
                    this->windowAccumulator / (double) this->windowSampleCount;

                if (meanSquare > gateThreshold) {
                    this->windowPower[this->windowWriteIdx] = meanSquare;
                    this->windowWriteIdx = (this->windowWriteIdx + 1) % MAX_WINDOWS;
                    if (this->windowCount < MAX_WINDOWS) {
                        this->windowCount++;
                    }
                }

                uint32_t shiftSamples = this->windowSize - this->stepSize;
                double ratio = (double) shiftSamples / (double) this->windowSampleCount;
                this->windowAccumulator *= ratio;
                this->windowSampleCount = shiftSamples;
            }
        }

        double measuredLUFS = -70.0;
        if (this->windowCount > 0) {
            double sum = 0.0;
            for (uint32_t w = 0; w < this->windowCount; w++) {
                sum += this->windowPower[w];
            }
            double gatedMean = sum / (double) this->windowCount;
            if (gatedMean > 1e-20) {
                measuredLUFS = -0.691 + 10.0 * log10(gatedMean);
            }
        }

        double desiredGainDB = this->targetLUFS - measuredLUFS;

        if (desiredGainDB > this->maxGainDB) {
            desiredGainDB = this->maxGainDB;
        } else if (desiredGainDB < -this->maxGainDB) {
            desiredGainDB = -this->maxGainDB;
        }

        double coeff;
        if (desiredGainDB > this->smoothedGainDB) {
            coeff = this->attackCoeff;
        } else {
            coeff = this->releaseCoeff;
        }
        this->smoothedGainDB += coeff * (desiredGainDB - this->smoothedGainDB);

        double gainLinear = pow(10.0, this->smoothedGainDB / 20.0);

        samples[i * 2] *= (float) gainLinear;
        samples[i * 2 + 1] *= (float) gainLinear;
    }
}

void LUFSTargeting::Reset() {
    this->kWeightStage1L.Reset();
    this->kWeightStage1R.Reset();
    this->kWeightStage2L.Reset();
    this->kWeightStage2R.Reset();
    ConfigureFilters();
    UpdateSmoothingCoeffs();
    this->smoothedGainDB = 0.0;
    this->sampleCounter = 0;
    this->windowAccumulator = 0.0;
    this->windowSampleCount = 0;
    this->windowWriteIdx = 0;
    this->windowCount = 0;
    memset(this->windowPower, 0, sizeof(this->windowPower));
    this->windowSize = (uint32_t) (this->samplingRate * 0.4);
    this->stepSize = this->windowSize / 4;
}

void LUFSTargeting::SetEnable(bool enable) {
    if (this->enable != enable) {
        if (enable) {
            Reset();
        }
        this->enable = enable;
    }
}

void LUFSTargeting::SetMaxGain(float maxGainDB) {
    this->maxGainDB = maxGainDB;
}

void LUFSTargeting::SetSamplingRate(uint32_t samplingRate) {
    if (this->samplingRate != samplingRate) {
        this->samplingRate = samplingRate;
        Reset();
    }
}

void LUFSTargeting::SetSpeed(int speed) {
    if (speed < 0) speed = 0;
    if (speed > 2) speed = 2;
    this->speed = speed;
    UpdateSmoothingCoeffs();
}

void LUFSTargeting::SetTargetLUFS(float targetLUFS) {
    this->targetLUFS = targetLUFS;
}

void LUFSTargeting::ConfigureFilters() {
    if (this->samplingRate == 48000) {
        this->kWeightStage1L.SetCoeffs(
            1.0,
            -1.69065929318241,
            0.73248077421585,
            1.53512485958697,
            -2.69169618940638,
            1.19839281085285
        );
        this->kWeightStage1R.SetCoeffs(
            1.0,
            -1.69065929318241,
            0.73248077421585,
            1.53512485958697,
            -2.69169618940638,
            1.19839281085285
        );
        this->kWeightStage2L.SetCoeffs(
            1.0, -1.99004745483398, 0.99007225036621, 1.0, -2.0, 1.0
        );
        this->kWeightStage2R.SetCoeffs(
            1.0, -1.99004745483398, 0.99007225036621, 1.0, -2.0, 1.0
        );
    } else if (this->samplingRate == 44100) {
        this->kWeightStage1L.SetCoeffs(
            1.0,
            -1.6636551132560204,
            0.7125954280732254,
            1.5308412300503478,
            -2.6509799951547297,
            1.1690790799215869
        );
        this->kWeightStage1R.SetCoeffs(
            1.0,
            -1.6636551132560204,
            0.7125954280732254,
            1.5308412300503478,
            -2.6509799951547297,
            1.1690790799215869
        );
        this->kWeightStage2L.SetCoeffs(
            1.0, -1.9891696736297957, 0.9891990357870394, 1.0, -2.0, 1.0
        );
        this->kWeightStage2R.SetCoeffs(
            1.0, -1.9891696736297957, 0.9891990357870394, 1.0, -2.0, 1.0
        );
    } else {
        this->kWeightStage1L.SetCoeffs(
            1.0,
            -1.6636551132560204,
            0.7125954280732254,
            1.5308412300503478,
            -2.6509799951547297,
            1.1690790799215869
        );
        this->kWeightStage1R.SetCoeffs(
            1.0,
            -1.6636551132560204,
            0.7125954280732254,
            1.5308412300503478,
            -2.6509799951547297,
            1.1690790799215869
        );
        this->kWeightStage2L.SetCoeffs(
            1.0, -1.9891696736297957, 0.9891990357870394, 1.0, -2.0, 1.0
        );
        this->kWeightStage2R.SetCoeffs(
            1.0, -1.9891696736297957, 0.9891990357870394, 1.0, -2.0, 1.0
        );
    }
}

void LUFSTargeting::UpdateSmoothingCoeffs() {
    double attackMs, releaseMs;

    switch (this->speed) {
        case 0:
            attackMs = 200.0;
            releaseMs = 1000.0;
            break;
        case 2:
            attackMs = 50.0;
            releaseMs = 200.0;
            break;
        default:
            attackMs = 100.0;
            releaseMs = 500.0;
            break;
    }

    double attackSamples = (double) this->samplingRate * attackMs / 1000.0;
    double releaseSamples = (double) this->samplingRate * releaseMs / 1000.0;

    this->attackCoeff = 1.0 - exp(-1.0 / attackSamples);
    this->releaseCoeff = 1.0 - exp(-1.0 / releaseSamples);
}
