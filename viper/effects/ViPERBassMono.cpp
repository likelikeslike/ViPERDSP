#include "ViPERBassMono.h"
#include "../constants.h"

ViPERBassMono::ViPERBassMono() {
    this->speaker = 60;
    this->enable = false;
    this->processMode = ProcessMode::NATURAL_BASS;
    this->antiPop = 0.0;
    this->bassFactor = 0.0;
    this->samplingRate = VIPER_DEFAULT_SAMPLING_RATE;
    this->samplingRatePeriod = 1.0 / VIPER_DEFAULT_SAMPLING_RATE;
    this->polyphase = new Polyphase(2);
    this->biquad = new Biquad();
    this->subwoofer = new Subwoofer();
    this->waveBuffer = new WaveBuffer(1, 4096);

    this->biquad->Reset();
    this->biquad->SetLowPassParameter((float) this->speaker, this->samplingRate, 0.53);
    this->subwoofer->SetBassGain(this->samplingRate, 0.0);
    Reset();
}

ViPERBassMono::~ViPERBassMono() {
    delete this->polyphase;
    delete this->biquad;
    delete this->subwoofer;
    delete this->waveBuffer;
}

void ViPERBassMono::Process(float *samples, uint32_t size) {
    if (!this->enable) return;
    if (size == 0) return;

    if (this->antiPop < 1.0) {
        for (uint32_t i = 0; i < size * 2; i += 2) {
            samples[i] *= this->antiPop;
            samples[i + 1] *= this->antiPop;

            float x = this->antiPop + this->samplingRatePeriod;
            if (x > 1.0) x = 1.0;
            this->antiPop = x;
        }
    }

    switch (this->processMode) {
        case ProcessMode::NATURAL_BASS: {
            for (uint32_t i = 0; i < size * 2; i += 2) {
                double sample = ((double) samples[i] + (double) samples[i + 1]) / 2.0;
                float x = (float) this->biquad->ProcessSample(sample) * this->bassFactor;
                samples[i] += x;
                samples[i + 1] += x;
            }
            break;
        }
        case ProcessMode::PURE_BASS_PLUS: {
            if (this->waveBuffer->PushSamples(samples, size)) {
                float *buffer = this->waveBuffer->GetBuffer();
                uint32_t bufferOffset = this->waveBuffer->GetBufferOffset();

                for (uint32_t i = 0; i < size * 2; i += 2) {
                    double sample = ((double) samples[i] + (double) samples[i + 1]) / 2.0;
                    auto x = (float) this->biquad->ProcessSample(sample);
                    buffer[bufferOffset - size + i / 2] = x;
                }

                if (this->polyphase->Process(samples, size) == size) {
                    for (uint32_t i = 0; i < size * 2; i += 2) {
                        float x = buffer[i / 2] * this->bassFactor;
                        samples[i] += x;
                        samples[i + 1] += x;
                    }
                    this->waveBuffer->PopSamples(size, true);
                }
            }
            break;
        }
        case ProcessMode::SUBWOOFER: {
            this->subwoofer->Process(samples, size);
            break;
        }
    }
}

void ViPERBassMono::Reset() {
    this->polyphase->SetSamplingRate(this->samplingRate);
    this->polyphase->Reset();
    this->waveBuffer->Reset();
    this->waveBuffer->PushZeros(this->polyphase->GetLatency());
    this->subwoofer->SetBassGain(this->samplingRate, this->bassFactor * 2.5f);
    this->biquad->SetLowPassParameter((float) this->speaker, this->samplingRate, 0.53);
    this->samplingRatePeriod = 1.0f / (float) this->samplingRate;
    this->antiPop = 0.0f;
}

void ViPERBassMono::SetBassFactor(float bassFactor) {
    if (this->bassFactor != bassFactor) {
        this->bassFactor = bassFactor;
        this->subwoofer->SetBassGain(this->samplingRate, this->bassFactor * 2.5f);
    }
}

void ViPERBassMono::SetEnable(bool enable) {
    if (this->enable != enable) {
        if (enable) Reset();
        this->enable = enable;
    }
}

void ViPERBassMono::SetProcessMode(ProcessMode processMode) {
    if (this->processMode != processMode) {
        this->processMode = processMode;
        Reset();
    }
}

void ViPERBassMono::SetSamplingRate(uint32_t samplingRate) {
    if (this->samplingRate != samplingRate) {
        this->samplingRate = samplingRate;
        this->samplingRatePeriod = 1.0f / (float) samplingRate;
        this->polyphase->SetSamplingRate(this->samplingRate);
        this->biquad->SetLowPassParameter(
            (float) this->speaker, this->samplingRate, 0.53
        );
        this->subwoofer->SetBassGain(this->samplingRate, this->bassFactor * 2.5f);
    }
}

void ViPERBassMono::SetSpeaker(uint32_t speaker) {
    if (this->speaker != speaker) {
        this->speaker = speaker;
        this->biquad->SetLowPassParameter(
            (float) this->speaker, this->samplingRate, 0.53
        );
    }
}

void ViPERBassMono::SetAntiPop(bool enabled) {
    if (enabled) {
        this->antiPop = 0.0f;
    } else {
        this->antiPop = 1.0f;
    }
}
