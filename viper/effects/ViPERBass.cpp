#include "ViPERBass.h"
#include "../constants.h"

// Iscle: Verified with the latest version at 13/12/2022

ViPERBass::ViPERBass() :
    polyphase(2),
    waveBuffer(2, 4096) {
    this->speaker = 60;
    this->enable = false;
    this->processMode = ProcessMode::NATURAL_BASS;
    this->antiPop = 0.0;
    this->bassFactor = 0.0;
    this->samplingRate = VIPER_DEFAULT_SAMPLING_RATE;
    this->samplingRatePeriod = 1.0 / VIPER_DEFAULT_SAMPLING_RATE;

    for (auto &biquad : this->biquad) {
        biquad.Reset();
        biquad.SetLowPassParameter((float) this->speaker, this->samplingRate, 0.53);
    }
    this->subwoofer.SetBassGain(this->samplingRate, 0.0);
    Reset();
}

void ViPERBass::Process(float *samples, uint32_t size) {
    if (!this->enable) return;
    if (size == 0) return;

    switch (this->processMode) {
        case ProcessMode::NATURAL_BASS: {
            for (uint32_t i = 0; i < size * 2; i += 2) {
                float bassL =
                    (float) this->biquad[0].ProcessSample(samples[i]) * this->bassFactor;
                float bassR = (float) this->biquad[1].ProcessSample(samples[i + 1])
                              * this->bassFactor;
                if (this->antiPop < 1.0) {
                    bassL *= this->antiPop;
                    bassR *= this->antiPop;
                    float x = this->antiPop + this->samplingRatePeriod;
                    if (x > 1.0) x = 1.0;
                    this->antiPop = x;
                }
                samples[i] += bassL;
                samples[i + 1] += bassR;
            }
            break;
        }
        case ProcessMode::PURE_BASS_PLUS: {
            if (this->waveBuffer.PushSamples(samples, size)) {
                float *buffer = this->waveBuffer.GetBuffer();
                uint32_t bufferOffset = this->waveBuffer.GetBufferOffset();

                for (uint32_t i = 0; i < size * 2; i += 2) {
                    buffer[bufferOffset - size + i] =
                        (float) this->biquad[0].ProcessSample(samples[i]);
                    buffer[bufferOffset - size + i + 1] =
                        (float) this->biquad[1].ProcessSample(samples[i + 1]);
                }

                if (this->polyphase.Process(samples, size) == size) {
                    for (uint32_t i = 0; i < size * 2; i += 2) {
                        float bassL = buffer[i] * this->bassFactor;
                        float bassR = buffer[i + 1] * this->bassFactor;
                        if (this->antiPop < 1.0) {
                            bassL *= this->antiPop;
                            bassR *= this->antiPop;
                            float x = this->antiPop + this->samplingRatePeriod;
                            if (x > 1.0) x = 1.0;
                            this->antiPop = x;
                        }
                        samples[i] += bassL;
                        samples[i + 1] += bassR;
                    }
                    this->waveBuffer.PopSamples(size, true);
                }
            }
            break;
        }
        case ProcessMode::SUBWOOFER: {
            if (this->antiPop < 1.0) {
                for (uint32_t i = 0; i < size * 2; i += 2) {
                    float dryL = samples[i];
                    float dryR = samples[i + 1];
                    float tmpSample[2] = {dryL, dryR};
                    this->subwoofer.Process(tmpSample, 1);
                    samples[i] = dryL + this->antiPop * (tmpSample[0] - dryL);
                    samples[i + 1] = dryR + this->antiPop * (tmpSample[1] - dryR);
                    float x = this->antiPop + this->samplingRatePeriod;
                    if (x > 1.0) x = 1.0;
                    this->antiPop = x;
                }
            } else {
                this->subwoofer.Process(samples, size);
            }
            break;
        }
    }
}

void ViPERBass::Reset() {
    this->polyphase.SetSamplingRate(this->samplingRate);
    this->polyphase.Reset();
    this->waveBuffer.Reset();
    this->waveBuffer.PushZeros(this->polyphase.GetLatency());
    this->subwoofer.SetBassGain(this->samplingRate, this->bassFactor * 2.5f);
    this->biquad[0].SetLowPassParameter((float) this->speaker, this->samplingRate, 0.53);
    this->biquad[1].SetLowPassParameter((float) this->speaker, this->samplingRate, 0.53);
    this->samplingRatePeriod = 1.0f / (float) this->samplingRate;
    this->antiPop = 0.0f;
}

void ViPERBass::SetBassFactor(float bassFactor) {
    if (this->bassFactor != bassFactor) {
        this->bassFactor = bassFactor;
        this->subwoofer.SetBassGain(this->samplingRate, this->bassFactor * 2.5f);
    }
}

void ViPERBass::SetEnable(bool enable) {
    if (this->enable != enable) {
        if (enable) Reset();
        this->enable = enable;
    }
}

void ViPERBass::SetProcessMode(ProcessMode processMode) {
    if (this->processMode != processMode) {
        this->processMode = processMode;
        Reset();
    }
}

void ViPERBass::SetSamplingRate(uint32_t samplingRate) {
    if (this->samplingRate != samplingRate) {
        this->samplingRate = samplingRate;
        this->samplingRatePeriod = 1.0f / (float) samplingRate;
        this->polyphase.SetSamplingRate(this->samplingRate);
        this->biquad[0].SetLowPassParameter(
            (float) this->speaker, this->samplingRate, 0.53
        );
        this->biquad[1].SetLowPassParameter(
            (float) this->speaker, this->samplingRate, 0.53
        );
        this->subwoofer.SetBassGain(this->samplingRate, this->bassFactor * 2.5f);
    }
}

void ViPERBass::SetSpeaker(uint32_t speaker) {
    if (this->speaker != speaker) {
        this->speaker = speaker;
        this->biquad[0].SetLowPassParameter(
            (float) this->speaker, this->samplingRate, 0.53
        );
        this->biquad[1].SetLowPassParameter(
            (float) this->speaker, this->samplingRate, 0.53
        );
    }
}

void ViPERBass::SetAntiPop(bool enabled) {
    if (enabled) {
        this->antiPop = 0.0f;
    } else {
        this->antiPop = 1.0f;
    }
}
