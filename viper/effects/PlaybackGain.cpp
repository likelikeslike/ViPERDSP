#include "PlaybackGain.h"
#include "../constants.h"
#include <cmath>

PlaybackGain::PlaybackGain() {
    this->enable = false;
    this->samplingRate = VIPER_DEFAULT_SAMPLING_RATE;
    this->logCoeff = 0.4342945;
    this->counterTo100 = 0;
    this->ratio1 = 2.0;
    this->ratio2 = 0.5;
    this->volume = 1.0;
    this->maxGainFactor = 1.0;
    this->currentGainL = 1.0;
    this->currentGainR = 1.0;
    this->biquad1.SetBandPassParameter(2200.0, this->samplingRate, 0.33);
    this->biquad2.SetBandPassParameter(2200.0, this->samplingRate, 0.33);
}

double PlaybackGain::AnalyseWave(float *samples, uint32_t size) {
    double tmpL = 0.0;
    double tmpR = 0.0;

    for (uint32_t i = 0; i < size * 2; i += 2) {
        double tmpL2 = this->biquad1.ProcessSample(samples[i]);
        tmpL += tmpL2 * tmpL2;

        double tmpR2 = this->biquad2.ProcessSample(samples[i + 1]);
        tmpR += tmpR2 * tmpR2;
    }

    double tmp;
    if (tmpL > tmpR) {
        tmp = tmpL;
    } else {
        tmp = tmpR;
    }

    return tmp / (double) size;
}

void PlaybackGain::Process(float *samples, uint32_t size) {
    if (!this->enable) return;
    if (size == 0) return;

    double analyzed = AnalyseWave(samples, size);

    // Avoid log(0) or small values.
    if (analyzed < 1e-10) {
        analyzed = 1e-10;
    }
    double a = log(analyzed);

    if (this->counterTo100 < 100) {
        this->counterTo100++;
    }

    double b = a * this->logCoeff * 10.0 + 23.0;
    double c = ((double) this->counterTo100 / 100.0) * (b * this->ratio2 - b);
    double d = c / 100.0;
    double e = pow(10.0, (c - d * d * 50.0) / 20.0);
    uint32_t f;
    if (size < this->samplingRate / 40) {
        f = this->samplingRate / 40;
    } else {
        f = size;
    }

    double target = e * this->volume;

    for (int ch = 0; ch < 2; ch++) {
        float &gain = (ch == 0) ? this->currentGainL : this->currentGainR;
        double g = (target - gain) / f;
        if (g >= 0.0) {
            g *= 0.0625;
        }
        for (uint32_t i = 0; i < size; i++) {
            samples[i * 2 + ch] *= gain;
            gain += (float) g;
            if (gain > this->maxGainFactor) {
                gain = this->maxGainFactor;
            } else if (gain < -this->maxGainFactor) {
                gain = -this->maxGainFactor;
            }
        }
    }
}

void PlaybackGain::Reset() {
    this->biquad1.SetBandPassParameter(2200.0, this->samplingRate, 0.33);
    this->biquad2.SetBandPassParameter(2200.0, this->samplingRate, 0.33);
    this->currentGainL = 1.0;
    this->counterTo100 = 0;
    this->currentGainR = 1.0;
}

void PlaybackGain::SetEnable(bool enable) {
    if (this->enable != enable) {
        if (enable) {
            Reset();
        }
        this->enable = enable;
    }
}

void PlaybackGain::SetMaxGainFactor(float maxGainFactor) {
    this->maxGainFactor = maxGainFactor;
}

void PlaybackGain::SetRatio(float ratio) {
    this->ratio1 = ratio + 1.0f;
    this->ratio2 = 1.0f / this->ratio1;
}

void PlaybackGain::SetSamplingRate(uint32_t samplingRate) {
    if (this->samplingRate != samplingRate) {
        this->samplingRate = samplingRate;
        Reset();
    }
}

void PlaybackGain::SetVolume(float volume) {
    this->volume = volume;
}
