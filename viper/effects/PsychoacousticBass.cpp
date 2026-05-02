#include "PsychoacousticBass.h"
#include "../constants.h"

static const float HARMONIC_ORDER_2[10] = {
    0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
};

static const float HARMONIC_ORDER_3[10] = {
    0.0f, 0.7f, 0.3f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
};

static const float HARMONIC_ORDER_4[10] = {
    0.0f, 0.5f, 0.3f, 0.2f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
};

static const float HARMONIC_ORDER_5[10] = {
    0.0f, 0.4f, 0.25f, 0.2f, 0.15f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
};

PsychoacousticBass::PsychoacousticBass() {
    this->samplingRate = VIPER_DEFAULT_SAMPLING_RATE;
    this->cutoff = 80;
    this->harmonicOrder = 3;
    this->intensity = 0.5f;
    this->originalBassLevel = 1.0f;
    this->envelope = 1e-10;
    this->enabled = false;
    Reset();
}

void PsychoacousticBass::Process(float *samples, uint32_t size) {
    if (!this->enabled) return;

    for (uint32_t i = 0; i < size * 2; i += 2) {
        double bassL = this->lowpass[0].ProcessSample(samples[i]);
        double bassR = this->lowpass[1].ProcessSample(samples[i + 1]);

        double absL = fabs(bassL);
        double absR = fabs(bassR);
        double peak = (absL > absR) ? absL : absR;
        if (peak > this->envelope) {
            this->envelope += 0.01 * (peak - this->envelope);
        } else {
            this->envelope += 0.0001 * (peak - this->envelope);
        }
        if (this->envelope < 1e-10) this->envelope = 1e-10;

        double normL = bassL / this->envelope;
        double normR = bassR / this->envelope;
        if (normL > 1.0) normL = 1.0;
        if (normL < -1.0) normL = -1.0;
        if (normR > 1.0) normR = 1.0;
        if (normR < -1.0) normR = -1.0;

        double harmonicL = this->harmonics[0].Process(normL) * this->envelope;
        double harmonicR = this->harmonics[1].Process(normR) * this->envelope;

        harmonicL = this->highpass[0].ProcessSample(harmonicL);
        harmonicR = this->highpass[1].ProcessSample(harmonicR);

        samples[i] = samples[i] + (float) (harmonicL * this->intensity);
        samples[i + 1] = samples[i + 1] + (float) (harmonicR * this->intensity);

        if (this->originalBassLevel < 1.0f) {
            double dryL = samples[i] - (float) bassL;
            double dryR = samples[i + 1] - (float) bassR;
            samples[i] = (float) (dryL + bassL * this->originalBassLevel);
            samples[i + 1] = (float) (dryR + bassR * this->originalBassLevel);
        }
    }
}

void PsychoacousticBass::Reset() {
    RefreshFilters();
    ApplyHarmonicCoeffs();
}

void PsychoacousticBass::SetEnable(bool enable) {
    if (this->enabled != enable) {
        if (enable) {
            Reset();
        }
        this->enabled = enable;
    }
}

void PsychoacousticBass::SetSamplingRate(uint32_t samplingRate) {
    if (this->samplingRate != samplingRate) {
        this->samplingRate = samplingRate;
        Reset();
    }
}

void PsychoacousticBass::SetCutoff(uint32_t cutoff) {
    if (cutoff < 60) cutoff = 60;
    if (cutoff > 150) cutoff = 150;
    if (this->cutoff != cutoff) {
        this->cutoff = cutoff;
        RefreshFilters();
    }
}

void PsychoacousticBass::SetIntensity(uint32_t intensity) {
    if (intensity > 100) intensity = 100;
    this->intensity = (float) intensity / 100.0f;
}

void PsychoacousticBass::SetHarmonicOrder(uint32_t order) {
    if (order < 2) order = 2;
    if (order > 5) order = 5;
    if (this->harmonicOrder != order) {
        this->harmonicOrder = order;
        ApplyHarmonicCoeffs();
    }
}

void PsychoacousticBass::SetOriginalBassLevel(uint32_t level) {
    if (level > 100) level = 100;
    this->originalBassLevel = (float) level / 100.0f;
}

void PsychoacousticBass::RefreshFilters() {
    for (uint32_t ch = 0; ch < 2; ch++) {
        this->lowpass[ch].RefreshFilter(
            MultiBiquad::FilterType::LOW_PASS,
            0.0,
            (float) this->cutoff,
            this->samplingRate,
            0.717,
            false
        );
        this->highpass[ch].RefreshFilter(
            MultiBiquad::FilterType::HIGH_PASS,
            0.0,
            (float) this->cutoff,
            this->samplingRate,
            0.717,
            false
        );
    }
}

void PsychoacousticBass::ApplyHarmonicCoeffs() {
    const float *coeffs;
    switch (this->harmonicOrder) {
        case 2: coeffs = HARMONIC_ORDER_2; break;
        case 3: coeffs = HARMONIC_ORDER_3; break;
        case 4: coeffs = HARMONIC_ORDER_4; break;
        case 5: coeffs = HARMONIC_ORDER_5; break;
        default: coeffs = HARMONIC_ORDER_3; break;
    }
    this->harmonics[0].Reset();
    this->harmonics[1].Reset();
    this->harmonics[0].SetHarmonics(coeffs);
    this->harmonics[1].SetHarmonics(coeffs);
}
