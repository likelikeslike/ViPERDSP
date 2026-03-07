#include "FETCompressor.h"
#include "../constants.h"
#include <cmath>

static const float DEFAULT_FETCOMP_PARAMETERS[] = {
    1.000000,
    0.000000,
    0.000000,
    0.000000,
    1.000000,
    0.000000,
    1.000000,
    0.514679,
    1.000000,
    0.384311,
    1.000000,
    0.500000,
    0.879450,
    0.884311,
    0.615689,
    0.660964,
    1.000000
};

static double calculate_exp_something(double param_1, double param_2) {
    return 1.0 - exp(-1.0 / (param_2 * param_1));
}

FETCompressor::FETCompressor() {
    this->samplingRate = VIPER_DEFAULT_SAMPLING_RATE;

    for (uint32_t i = 0; i < 17; i++) {
        SetParameter(
            (FETCompressor::Parameter) i,
            GetParameterDefault((FETCompressor::Parameter) i)
        );
    }

    Reset();
}

float FETCompressor::GetMeter(int param_1) {
    if (param_1 != 0) {
        return 0.0;
    }

    if (this->enable) {
        float tmp = (6.907755 - this->attackSmoothGR) / 6.907755;
        if (tmp < 1.0) {
            if (tmp < 0.0) {
                tmp = 0.0;
            }
            return tmp;
        }
    }

    return 1.0;
}

float FETCompressor::GetParameter(FETCompressor::Parameter parameter) {
    return this->parameters[parameter];
}

float FETCompressor::GetParameterDefault(FETCompressor::Parameter parameter) {
    if (parameter < 17) {
        return DEFAULT_FETCOMP_PARAMETERS[parameter];
    }
    return 0.0;
}

void FETCompressor::Process(float *samples, uint32_t size) {
    if (!this->enable || size == 0) return;

    for (uint32_t i = 0; i < size * 2; i += 2) {
        double inL = abs(samples[i]);
        double inR = abs(samples[i + 1]);

        double in;
        if (inL > inR) {
            in = inL;
        } else {
            in = inR;
        }

        double out = ProcessSidechain(in);
        if (this->enable) {
            samples[i] *= (float) out;
            samples[i + 1] *= (float) out;
        }

        this->smoothedThreshold =
            this->smoothedThreshold
            + (this->threshold - this->smoothedThreshold) * this->smoothingCoeff;
        this->smoothedGain =
            this->smoothedGain + this->smoothingCoeff * (this->gain - this->smoothedGain);
    }
}

double FETCompressor::ProcessSidechain(double in) {
    double in2 = in * in;
    if (in2 < 0.000001) {
        in2 = 0.000001;
    }

    float attackCoeff = this->attack2;
    float releaseCoeff = this->release2;
    float adaptiveAttackTime = this->attack1;

    float runningPeak =
        this->runningPeak + this->crest2 * ((float) in2 - this->runningPeak);
    float runningRMS = this->runningRMS + this->crest2 * ((float) in2 - this->runningRMS);

    if ((float) in2 < runningPeak) {
        in2 = runningPeak;
    }

    this->runningRMS = runningRMS;
    this->runningPeak = (float) in2;

    float crestRatio = (float) in2 / runningRMS;

    if (this->autoAttack) {
        adaptiveAttackTime = 2.0f * this->maxAttack / crestRatio;
        if (adaptiveAttackTime <= 0.0f) {
            attackCoeff = 1.0f;
        } else {
            attackCoeff =
                (float) calculate_exp_something(this->samplingRate, adaptiveAttackTime);
        }
    }

    if (this->autoRelease) {
        float adaptiveReleaseTime =
            2.0f * this->maxRelease / crestRatio - adaptiveAttackTime;
        if (adaptiveReleaseTime <= 0.0f) {
            releaseCoeff = 1.0f;
        } else {
            releaseCoeff =
                (float) calculate_exp_something(this->samplingRate, adaptiveReleaseTime);
        }
    }

    float logInput = logf(in >= 0.000001f ? (float) in : 0.000001f);

    float diff = logInput - this->smoothedThreshold;
    float ratioMul;
    float halfThreshGR;
    float halfKnee;
    float kneeWidth;

    if (!this->autoKnee) {
        float negRatio = -this->ratio;
        kneeWidth = this->knee;
        halfThreshGR = this->smoothedThreshold * negRatio * 0.5f;
        halfKnee = kneeWidth * 0.5f;
        ratioMul = negRatio;
    } else {
        halfThreshGR = this->smoothedThreshold * 0.5f;
        float kneeBase = this->adaptiveGainState + halfThreshGR;
        kneeWidth = -(kneeBase * this->kneeMulti);
        if (kneeWidth <= 0.0f) {
            ratioMul = 1.0f;
            halfKnee = 0.0f;
            kneeWidth = 0.0f;
        } else {
            ratioMul = 1.0f;
            halfKnee = kneeWidth * 0.5f;
        }
    }

    float gainReduction;
    if (diff >= halfKnee) {
        gainReduction = diff;
    } else if (diff <= -(kneeWidth * 0.5f)) {
        gainReduction = 0.0f;
    } else {
        float doubled = kneeWidth * 2.0f;
        float shifted = diff + halfKnee;
        gainReduction = (shifted * shifted) / doubled;
    }

    gainReduction *= ratioMul;

    float relSmoothed =
        this->releaseSmoothGR + (gainReduction - this->releaseSmoothGR) * releaseCoeff;
    if (gainReduction <= relSmoothed) {
        gainReduction = relSmoothed;
    }

    float atkDiff = gainReduction - this->attackSmoothGR;
    this->releaseSmoothGR = gainReduction;
    float smoothedGR = this->attackSmoothGR + atkDiff * attackCoeff;

    float negSmoothedGR = -smoothedGR;
    float adaptTarget = negSmoothedGR - halfThreshGR - this->adaptiveGainState;
    this->attackSmoothGR = smoothedGR;

    this->adaptiveGainState = this->adaptiveGainState + adaptTarget * this->adapt2;

    if (this->autoGain) {
        if (!this->noClip) {
            float makeupGain = this->adaptiveGainState + halfThreshGR;
            return exp(negSmoothedGR - makeupGain);
        } else {
            float outputLevel = logInput - smoothedGR;
            float makeupGain = halfThreshGR + this->adaptiveGainState;
            float check = outputLevel - makeupGain;
            if (check > 0.0011512704f) {
                outputLevel = outputLevel - halfThreshGR;
                outputLevel = outputLevel + 0.0011512704f;
                makeupGain = outputLevel + halfThreshGR;
                this->adaptiveGainState = outputLevel;
            }
            return exp(negSmoothedGR - makeupGain);
        }
    }

    return exp(this->smoothedGain - smoothedGR);
}

void FETCompressor::Reset() {
    this->smoothingCoeff = calculate_exp_something(this->samplingRate, 0.05);
    this->smoothedThreshold = this->threshold;
    this->smoothedGain = this->gain;
    this->runningPeak = 0.000001;
    this->runningRMS = 0.000001;
    this->releaseSmoothGR = 0.0;
    this->attackSmoothGR = 0.0;
    this->adaptiveGainState = 0.0;
}

void FETCompressor::SetParameter(FETCompressor::Parameter parameter, float value) {
    this->parameters[parameter] = value;

    switch (parameter) {
        case ENABLE: {
            this->enable = value >= 0.5;
            break;
        }
        case THRESHOLD: {
            this->threshold = log(pow(10.0, (value * -60.0) / 20.0));
            break;
        }
        case RATIO: {
            this->ratio = -value;
            break;
        }
        case KNEE: {
            this->knee = log(pow(10.0, (value * 60.0) / 20));
            break;
        }
        case AUTO_KNEE: {
            this->autoKnee = value >= 0.5;
            break;
        }
        case GAIN: {
            this->gain = log(pow(10.0, (value * 60.0) / 20.0));
            break;
        }
        case AUTO_GAIN: {
            this->autoGain = value >= 0.5;
            break;
        }
        case ATTACK: {
            double tmp = exp(value * 7.600903 - 9.21034);
            this->attack1 = tmp;
            if (tmp <= 0.0) {
                tmp = 1.0;
            } else {
                tmp = calculate_exp_something(this->samplingRate, tmp);
            }
            this->attack2 = tmp;
            break;
        }
        case AUTO_ATTACK: {
            this->autoAttack = value >= 0.5;
            break;
        }
        case RELEASE: {
            double tmp = exp(value * 5.991465 - 5.298317);
            this->release1 = tmp;
            if (tmp <= 0.0) {
                tmp = 1.0;
            } else {
                tmp = calculate_exp_something(this->samplingRate, tmp);
            }
            this->release2 = tmp;
            break;
        }
        case AUTO_RELEASE: {
            this->autoRelease = value >= 0.5;
            break;
        }
        case KNEE_MULTI: {
            this->kneeMulti = value * 4.0;
            break;
        }
        case MAX_ATTACK: {
            this->maxAttack = exp(value * 7.600903 - 9.21034);
            break;
        }
        case MAX_RELEASE: {
            this->maxRelease = exp(value * 5.991465 - 5.298317);
            break;
        }
        case CREST: {
            double tmp = exp(value * 5.991465 - 5.298317);
            this->crest1 = tmp;
            if (tmp <= 0.0) {
                tmp = 1.0;
            } else {
                tmp = calculate_exp_something(this->samplingRate, tmp);
            }
            this->crest2 = tmp;
            break;
        }
        case ADAPT: {
            double tmp = exp(value * 1.386294);
            this->adapt1 = tmp;
            if (tmp <= 0.0) {
                tmp = 1.0;
            } else {
                tmp = calculate_exp_something(this->samplingRate, tmp);
            }
            this->adapt2 = tmp;
            break;
        }
        case NO_CLIP: {
            this->noClip = value >= 0.5;
            break;
        }
    }
}

void FETCompressor::SetSamplingRate(uint32_t samplingRate) {
    this->samplingRate = samplingRate;

    for (uint32_t i = 0; i < 17; i++) {
        SetParameter(
            (FETCompressor::Parameter) i, GetParameter((FETCompressor::Parameter) i)
        );
    }

    Reset();
}
