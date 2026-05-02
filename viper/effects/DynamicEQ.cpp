#include "DynamicEQ.h"
#include "../constants.h"
#include <algorithm>
#include <cmath>

static constexpr float GAIN_CHANGE_THRESHOLD = 0.1f;
static constexpr float OVERSHOOT_RANGE_DB = 12.0f;
static constexpr float MIN_ENVELOPE = 1e-20f;

DynamicEQ::DynamicEQ() {
    this->enable = false;
    this->samplingRate = VIPER_DEFAULT_SAMPLING_RATE;
    this->bandCount = 0;

    for (uint32_t i = 0; i < MAX_BANDS; i++) {
        this->params[i].frequency = 1000.0f;
        this->params[i].q = 1.0f;
        this->params[i].targetGainDb = 0.0f;
        this->params[i].thresholdDb = -24.0f;
        this->params[i].attackMs = 10.0f;
        this->params[i].releaseMs = 100.0f;
        this->params[i].filterType = MultiBiquad::PEAK;

        this->state[i].envelopeL = 0.0;
        this->state[i].envelopeR = 0.0;
        this->state[i].smoothedGainDb = 0.0;
        this->state[i].lastAppliedGainDb = 0.0f;
        this->state[i].attackCoeff = 0.0;
        this->state[i].releaseCoeff = 0.0;
    }

    Reset();
}

void DynamicEQ::Process(float *samples, uint32_t size) {
    if (!this->enable) return;
    if (this->bandCount == 0) return;
    if (size == 0) return;

    uint32_t frameCount = size * 2;

    for (uint32_t b = 0; b < this->bandCount; b++) {
        double attackCoeff = this->state[b].attackCoeff;
        double releaseCoeff = this->state[b].releaseCoeff;
        double envL = this->state[b].envelopeL;
        double envR = this->state[b].envelopeR;
        double smoothedGain = this->state[b].smoothedGainDb;
        float lastApplied = this->state[b].lastAppliedGainDb;
        float targetGain = this->params[b].targetGainDb;
        float threshold = this->params[b].thresholdDb;

        for (uint32_t i = 0; i < frameCount; i += 2) {
            double sL = static_cast<double>(samples[i]);
            double sR = static_cast<double>(samples[i + 1]);

            double powerL = sL * sL;
            double powerR = sR * sR;

            double smoothCoeffL = (powerL > envL) ? attackCoeff : releaseCoeff;
            envL += smoothCoeffL * (powerL - envL);

            double smoothCoeffR = (powerR > envR) ? attackCoeff : releaseCoeff;
            envR += smoothCoeffR * (powerR - envR);

            double rmsLinear = sqrt(std::max(envL, envR));
            if (rmsLinear < MIN_ENVELOPE) rmsLinear = MIN_ENVELOPE;

            double envelopeDb = 20.0 * log10(rmsLinear);

            double overshoot = envelopeDb - static_cast<double>(threshold);
            double desiredGainDb = 0.0;
            if (overshoot > 0.0) {
                double ratio = overshoot / static_cast<double>(OVERSHOOT_RANGE_DB);
                if (ratio > 1.0) ratio = 1.0;
                desiredGainDb = static_cast<double>(targetGain) * ratio;
            }

            double gainCoeff =
                (fabs(desiredGainDb) > fabs(smoothedGain)) ? attackCoeff : releaseCoeff;
            smoothedGain += gainCoeff * (desiredGainDb - smoothedGain);

            float currentGainDb = static_cast<float>(smoothedGain);
            if (fabs(currentGainDb - lastApplied) > GAIN_CHANGE_THRESHOLD) {
                ConfigureApplicationFilter(b, currentGainDb);
                lastApplied = currentGainDb;
            }

            samples[i] = static_cast<float>(this->applyL[b].ProcessSample(sL));
            samples[i + 1] = static_cast<float>(this->applyR[b].ProcessSample(sR));
        }

        this->state[b].envelopeL = envL;
        this->state[b].envelopeR = envR;
        this->state[b].smoothedGainDb = smoothedGain;
        this->state[b].lastAppliedGainDb = lastApplied;
    }
}

void DynamicEQ::Reset() {
    for (uint32_t i = 0; i < MAX_BANDS; i++) {
        this->state[i].envelopeL = 0.0;
        this->state[i].envelopeR = 0.0;
        this->state[i].smoothedGainDb = 0.0;
        this->state[i].lastAppliedGainDb = 0.0f;

        RecalcAttackRelease(i);
        ConfigureApplicationFilter(i, 0.0f);
    }
}

void DynamicEQ::SetEnable(bool enable) {
    if (this->enable != enable) {
        if (enable) Reset();
        this->enable = enable;
    }
}

void DynamicEQ::SetSamplingRate(uint32_t samplingRate) {
    if (this->samplingRate != samplingRate) {
        this->samplingRate = samplingRate;
        Reset();
    }
}

void DynamicEQ::SetBandCount(uint32_t count) {
    if (count > MAX_BANDS) count = MAX_BANDS;
    if (this->bandCount != count) {
        this->bandCount = count;
        Reset();
    }
}

void DynamicEQ::SetBandParam(uint32_t band, int paramType, float value) {
    if (band >= MAX_BANDS) return;

    switch (static_cast<ParamType>(paramType)) {
        case FREQ:
            this->params[band].frequency = value;
            ConfigureApplicationFilter(band, 0.0f);
            this->state[band].lastAppliedGainDb = 0.0f;
            break;
        case Q:
            this->params[band].q = value;
            ConfigureApplicationFilter(band, 0.0f);
            this->state[band].lastAppliedGainDb = 0.0f;
            break;
        case GAIN:
            this->params[band].targetGainDb = value;
            break;
        case THRESHOLD:
            this->params[band].thresholdDb = value;
            break;
        case ATTACK:
            this->params[band].attackMs = value;
            RecalcAttackRelease(band);
            break;
        case RELEASE:
            this->params[band].releaseMs = value;
            RecalcAttackRelease(band);
            break;
        case FILTER_TYPE: {
            int type = static_cast<int>(value);
            if (type == MultiBiquad::PEAK || type == MultiBiquad::LOW_SHELF
                || type == MultiBiquad::HIGH_SHELF) {
                this->params[band].filterType =
                    static_cast<MultiBiquad::FilterType>(type);
                ConfigureApplicationFilter(band, 0.0f);
                this->state[band].lastAppliedGainDb = 0.0f;
            }
            break;
        }
    }
}

void DynamicEQ::RecalcAttackRelease(uint32_t band) {
    double attackSec = static_cast<double>(this->params[band].attackMs) / 1000.0;
    double releaseSec = static_cast<double>(this->params[band].releaseMs) / 1000.0;
    double sr = static_cast<double>(this->samplingRate);

    if (attackSec > 0.0) {
        this->state[band].attackCoeff = 1.0 - exp(-1.0 / (attackSec * sr));
    } else {
        this->state[band].attackCoeff = 1.0;
    }

    if (releaseSec > 0.0) {
        this->state[band].releaseCoeff = 1.0 - exp(-1.0 / (releaseSec * sr));
    } else {
        this->state[band].releaseCoeff = 1.0;
    }
}

void DynamicEQ::ConfigureApplicationFilter(uint32_t band, float gainDb) {
    this->applyL[band].RefreshFilter(
        this->params[band].filterType,
        gainDb,
        this->params[band].frequency,
        this->samplingRate,
        this->params[band].q,
        false
    );
    this->applyR[band].RefreshFilter(
        this->params[band].filterType,
        gainDb,
        this->params[band].frequency,
        this->samplingRate,
        this->params[band].q,
        false
    );
}
