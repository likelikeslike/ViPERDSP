#include "ViPER.h"
#include "../include/log.h"
#include "constants.h"
#include <chrono>
#include <cstring>

ViPER::ViPER() :
    updateProcessTime(false),
    processTimeMs(0),
    samplingRate(VIPER_DEFAULT_SAMPLING_RATE),
    adaptiveBuffer(AdaptiveBuffer(2, 4096)),
    waveBuffer(WaveBuffer(2, 4096)),
    iirFilter(IIRFilter(10)) {
    VIPER_LOGI("Welcome to ViPER FX");
    VIPER_LOGI("Current version is %s (%d)", VERSION_NAME, VERSION_CODE);

    this->convolver.SetEnable(false);
    this->convolver.SetSamplingRate(this->samplingRate);
    this->convolver.Reset();

    this->vhe.SetEnable(false);
    this->vhe.SetSamplingRate(this->samplingRate);
    this->vhe.Reset();

    this->viperDdc.SetEnable(false);
    this->viperDdc.SetSamplingRate(this->samplingRate);
    this->viperDdc.Reset();

    this->spectrumExtend.SetEnable(false);
    this->spectrumExtend.SetSamplingRate(this->samplingRate);
    this->spectrumExtend.SetReferenceFrequency(7600);
    this->spectrumExtend.SetExciter(0);
    this->spectrumExtend.Reset();

    this->iirFilter.SetEnable(false);
    this->iirFilter.SetSamplingRate(this->samplingRate);
    this->iirFilter.Reset();

    this->colorfulMusic.SetEnable(false);
    this->colorfulMusic.SetSamplingRate(this->samplingRate);
    this->colorfulMusic.Reset();

    this->reverberation.SetEnable(false);
    this->reverberation.Reset();

    this->playbackGain.SetEnable(false);
    this->playbackGain.SetSamplingRate(this->samplingRate);
    this->playbackGain.Reset();

    this->fetCompressor.SetParameter(FETCompressor::ENABLE, 0.0);
    this->fetCompressor.SetSamplingRate(this->samplingRate);
    this->fetCompressor.Reset();

    this->dynamicSystem.SetEnable(false);
    this->dynamicSystem.SetSamplingRate(this->samplingRate);
    this->dynamicSystem.Reset();

    this->viperBass.SetSamplingRate(this->samplingRate);
    this->viperBass.Reset();

    this->viperClarity.SetSamplingRate(this->samplingRate);
    this->viperClarity.Reset();

    this->diffSurround.SetEnable(false);
    this->diffSurround.SetSamplingRate(this->samplingRate);
    this->diffSurround.Reset();

    this->cure.SetEnable(false);
    this->cure.SetSamplingRate(this->samplingRate);
    this->cure.Reset();

    this->tubeSimulator.SetEnable(false);
    this->tubeSimulator.Reset();

    this->analogX.SetEnable(false);
    this->analogX.SetSamplingRate(this->samplingRate);
    this->analogX.SetProcessingModel(0);
    this->analogX.Reset();

    this->speakerCorrection.SetEnable(false);
    this->speakerCorrection.SetSamplingRate(this->samplingRate);
    this->speakerCorrection.Reset();

    for (auto &softwareLimiter : this->softwareLimiters) {
        softwareLimiter.Reset();
    }

    this->frameScale = 1.0;
    this->leftPan = 1.0;
    this->rightPan = 1.0;
    this->updateProcessTime = false;
    this->processTimeMs = 0;
}

void ViPER::process(std::vector<float> &buffer, uint32_t size) {
    if (this->updateProcessTime) {
        auto now = std::chrono::system_clock::now();
        auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
        this->processTimeMs = now_ms.time_since_epoch().count();
    }

    uint32_t ret;
    float *tmpBuf;
    uint32_t tmpBufSize;

    if (this->convolver.GetEnabled() || this->vhe.GetEnabled()) {
        if (!this->waveBuffer.PushSamples(buffer.data(), size)) {
            this->waveBuffer.Reset();
            memset(buffer.data(), 0, size * 2 * sizeof(float));
            return;
        }

        float *ptr = this->waveBuffer.GetBuffer();
        ret = this->convolver.Process(ptr, ptr, size);
        ret = this->vhe.Process(ptr, ptr, ret);
        this->waveBuffer.SetBufferOffset(ret);

        if (!this->adaptiveBuffer.PushZero(ret)) {
            this->waveBuffer.Reset();
            this->adaptiveBuffer.FlushBuffer();
            memset(buffer.data(), 0, size * 2 * sizeof(float));
            return;
        }

        ptr = this->adaptiveBuffer.GetBuffer();
        ret = this->waveBuffer.PopSamples(ptr, ret, true);
        this->adaptiveBuffer.SetBufferOffset(ret);

        tmpBuf = ptr;
        tmpBufSize = ret;
    } else {
        if (this->adaptiveBuffer.PushFrames(buffer.data(), size)) {
            this->adaptiveBuffer.SetBufferOffset(size);

            tmpBuf = this->adaptiveBuffer.GetBuffer();
            tmpBufSize = size;
        } else {
            this->adaptiveBuffer.FlushBuffer();
            memset(buffer.data(), 0, size * 2 * sizeof(float));
            return;
        }
    }

    if (tmpBufSize != 0) {
        this->viperDdc.Process(tmpBuf, size);
        this->spectrumExtend.Process(tmpBuf, size);
        this->iirFilter.Process(tmpBuf, tmpBufSize);
        this->colorfulMusic.Process(tmpBuf, tmpBufSize);
        this->diffSurround.Process(tmpBuf, tmpBufSize);
        this->reverberation.Process(tmpBuf, tmpBufSize);
        this->speakerCorrection.Process(tmpBuf, tmpBufSize);
        this->playbackGain.Process(tmpBuf, tmpBufSize);
        this->fetCompressor.Process(tmpBuf, tmpBufSize);
        this->dynamicSystem.Process(tmpBuf, tmpBufSize);
        this->viperBass.Process(tmpBuf, tmpBufSize);
        this->viperClarity.Process(tmpBuf, tmpBufSize);
        this->cure.Process(tmpBuf, tmpBufSize);
        this->tubeSimulator.TubeProcess(tmpBuf, size);
        this->analogX.Process(tmpBuf, tmpBufSize);

        if (this->frameScale != 1.0) {
            this->adaptiveBuffer.ScaleFrames(this->frameScale);
        }

        if (this->leftPan < 1.0 || this->rightPan < 1.0) {
            this->adaptiveBuffer.PanFrames(this->leftPan, this->rightPan);
        }

        for (uint32_t i = 0; i < tmpBufSize * 2; i += 2) {
            tmpBuf[i] = this->softwareLimiters[0].Process(tmpBuf[i]);
            tmpBuf[i + 1] = this->softwareLimiters[1].Process(tmpBuf[i + 1]);
        }

        if (!this->adaptiveBuffer.PopFrames(buffer.data(), tmpBufSize)) {
            this->adaptiveBuffer.FlushBuffer();
            memset(buffer.data(), 0, size * 2 * sizeof(float));
            return;
        }

        if (size <= tmpBufSize) {
            return;
        }
    }

    memmove(
        buffer.data() + (size - tmpBufSize) * 2, buffer.data(), tmpBufSize * sizeof(float)
    );
    memset(buffer.data(), 0, (size - tmpBufSize) * sizeof(float));
}

void ViPER::DispatchCommand(
    int param, int val1, int val2, int val3, int val4, uint32_t arrSize, signed char *arr
) {
    switch (param) {

        // System
        case PARAM_SET_UPDATE_STATUS: {
            VIPER_LOGI("UpdateStatus: %s", val1 ? "ON" : "OFF");
            this->updateProcessTime = val1 != 0;
            break;
        }
        case PARAM_SET_RESET_STATUS: {
            VIPER_LOGI("ResetAllEffects");
            this->resetAllEffects();
            break;
        }

        // Convolver (HP and SPK share the same object)
        case PARAM_HP_CONVOLVER_ENABLE:
        case PARAM_SPK_CONVOLVER_ENABLE: {
            VIPER_LOGI(
                "Convolver[%s]: %s",
                param == PARAM_HP_CONVOLVER_ENABLE ? "HP" : "SPK",
                val1 ? "ON" : "OFF"
            );
            this->convolver.SetEnable(val1 != 0);
            break;
        }
        case PARAM_HP_CONVOLVER_SET_KERNEL:
        case PARAM_SPK_CONVOLVER_SET_KERNEL: {
            if (arrSize > 0 && arr != nullptr) {
                char path[256];
                memset(path, 0, sizeof(path));
                memcpy(path, arr, arrSize < 255 ? arrSize : 255);
                VIPER_LOGI(
                    "Convolver[%s]: SetKernel path=%s",
                    param == PARAM_HP_CONVOLVER_SET_KERNEL ? "HP" : "SPK",
                    path
                );
                this->convolver.SetKernel(path);
            }
            break;
        }
        case PARAM_HP_CONVOLVER_PREPARE_BUFFER:
        case PARAM_SPK_CONVOLVER_PREPARE_BUFFER: {
            VIPER_LOGI(
                "Convolver[%s]: PrepareBuffer channels=%d frames=%d sr=%d",
                param == PARAM_HP_CONVOLVER_PREPARE_BUFFER ? "HP" : "SPK",
                val1,
                val2,
                val3
            );
            this->convolver.PrepareKernelBuffer(val1, val2, val3);
            break;
        }
        case PARAM_HP_CONVOLVER_SET_BUFFER:
        case PARAM_SPK_CONVOLVER_SET_BUFFER: {
            VIPER_LOGI(
                "Convolver[%s]: SetBuffer offset=%d size=%u",
                param == PARAM_HP_CONVOLVER_SET_BUFFER ? "HP" : "SPK",
                val1,
                arrSize
            );
            this->convolver.SetKernelBuffer(val1, (float *) arr, arrSize);
            break;
        }
        case PARAM_HP_CONVOLVER_COMMIT_BUFFER:
        case PARAM_SPK_CONVOLVER_COMMIT_BUFFER: {
            VIPER_LOGI(
                "Convolver[%s]: CommitBuffer channels=%d frames=%d sr=%d",
                param == PARAM_HP_CONVOLVER_COMMIT_BUFFER ? "HP" : "SPK",
                val1,
                val2,
                val3
            );
            this->convolver.CommitKernelBuffer(val1, val2, val3);
            break;
        }
        case PARAM_HP_CONVOLVER_CROSS_CHANNEL:
        case PARAM_SPK_CONVOLVER_CROSS_CHANNEL: {
            VIPER_LOGI(
                "Convolver[%s]: CrossChannel=%d%%",
                param == PARAM_HP_CONVOLVER_CROSS_CHANNEL ? "HP" : "SPK",
                val1
            );
            this->convolver.SetCrossChannel((float) val1 / 100.0f);
            break;
        }

        // DDC
        case PARAM_HP_DDC_ENABLE:
        case PARAM_SPK_DDC_ENABLE: {
            VIPER_LOGI(
                "DDC[%s]: %s",
                param == PARAM_HP_DDC_ENABLE ? "HP" : "SPK",
                val1 ? "ON" : "OFF"
            );
            this->viperDdc.SetEnable(val1 != 0);
            break;
        }
        case PARAM_HP_DDC_COEFFICIENTS:
        case PARAM_SPK_DDC_COEFFICIENTS: {
            VIPER_LOGI(
                "DDC[%s]: SetCoeffs arrSize=%u",
                param == PARAM_HP_DDC_COEFFICIENTS ? "HP" : "SPK",
                arrSize
            );
            this->viperDdc.SetCoeffs(
                arrSize, (float *) arr, (float *) (arr + arrSize * sizeof(float))
            );
            break;
        }

        // EQ (IIR Filter)
        case PARAM_HP_EQ_ENABLE:
        case PARAM_SPK_EQ_ENABLE: {
            VIPER_LOGI(
                "EQ[%s]: %s",
                param == PARAM_HP_EQ_ENABLE ? "HP" : "SPK",
                val1 ? "ON" : "OFF"
            );
            this->iirFilter.SetEnable(val1 != 0);
            break;
        }
        case PARAM_HP_EQ_BAND_LEVEL:
        case PARAM_SPK_EQ_BAND_LEVEL: {
            VIPER_LOGI(
                "EQ[%s]: band=%d level=%d",
                param == PARAM_HP_EQ_BAND_LEVEL ? "HP" : "SPK",
                val1,
                val2
            );
            this->iirFilter.SetBandLevel((uint32_t) val1, (float) val2 / 100.0f);
            break;
        }
        case PARAM_HP_EQ_BAND_COUNT:
        case PARAM_SPK_EQ_BAND_COUNT: {
            VIPER_LOGI(
                "EQ[%s]: bandCount=%d",
                param == PARAM_HP_EQ_BAND_COUNT ? "HP" : "SPK",
                val1
            );
            this->iirFilter.SetBandCount((uint32_t) val1);
            break;
        }

        // Reverb
        case PARAM_HP_REVERB_ENABLE:
        case PARAM_SPK_REVERB_ENABLE: {
            VIPER_LOGI(
                "Reverb[%s]: %s",
                param == PARAM_HP_REVERB_ENABLE ? "HP" : "SPK",
                val1 ? "ON" : "OFF"
            );
            this->reverberation.SetEnable(val1 != 0);
            break;
        }
        case PARAM_HP_REVERB_ROOM_SIZE:
        case PARAM_SPK_REVERB_ROOM_SIZE: {
            VIPER_LOGI("Reverb[%s]: roomSize=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->reverberation.SetRoomSize((float) val1 / 100.0f);
            break;
        }
        case PARAM_HP_REVERB_ROOM_WIDTH:
        case PARAM_SPK_REVERB_ROOM_WIDTH: {
            VIPER_LOGI("Reverb[%s]: roomWidth=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->reverberation.SetWidth((float) val1 / 100.0f);
            break;
        }
        case PARAM_HP_REVERB_ROOM_DAMPENING:
        case PARAM_SPK_REVERB_ROOM_DAMPENING: {
            VIPER_LOGI("Reverb[%s]: dampening=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->reverberation.SetDamp((float) val1 / 100.0f);
            break;
        }
        case PARAM_HP_REVERB_ROOM_WET_SIGNAL:
        case PARAM_SPK_REVERB_ROOM_WET_SIGNAL: {
            VIPER_LOGI("Reverb[%s]: wet=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->reverberation.SetWet((float) val1 / 100.0f);
            break;
        }
        case PARAM_HP_REVERB_ROOM_DRY_SIGNAL:
        case PARAM_SPK_REVERB_ROOM_DRY_SIGNAL: {
            VIPER_LOGI("Reverb[%s]: dry=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->reverberation.SetDry((float) val1 / 100.0f);
            break;
        }

        // AGC (Automatic Gain Control / Playback Gain)
        case PARAM_HP_AGC_ENABLE:
        case PARAM_SPK_AGC_ENABLE: {
            VIPER_LOGI(
                "AGC[%s]: %s",
                param == PARAM_HP_AGC_ENABLE ? "HP" : "SPK",
                val1 ? "ON" : "OFF"
            );
            this->playbackGain.SetEnable(val1 != 0);
            break;
        }
        case PARAM_HP_AGC_RATIO:
        case PARAM_SPK_AGC_RATIO: {
            VIPER_LOGI("AGC[%s]: ratio=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->playbackGain.SetRatio((float) val1 / 100.0f);
            break;
        }
        case PARAM_HP_AGC_VOLUME:
        case PARAM_SPK_AGC_VOLUME: {
            VIPER_LOGI("AGC[%s]: volume=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->playbackGain.SetVolume((float) val1 / 100.0f);
            break;
        }
        case PARAM_HP_AGC_MAX_SCALER:
        case PARAM_SPK_AGC_MAX_SCALER: {
            VIPER_LOGI("AGC[%s]: maxScaler=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->playbackGain.SetMaxGainFactor((float) val1 / 100.0f);
            break;
        }

        // Dynamic System
        case PARAM_HP_DYNAMIC_SYSTEM_ENABLE:
        case PARAM_SPK_DYNAMIC_SYSTEM_ENABLE: {
            VIPER_LOGI(
                "DynSys[%s]: %s",
                param == PARAM_HP_DYNAMIC_SYSTEM_ENABLE ? "HP" : "SPK",
                val1 ? "ON" : "OFF"
            );
            this->dynamicSystem.SetEnable(val1 != 0);
            break;
        }
        case PARAM_HP_DYNAMIC_SYSTEM_X_COEFFICIENTS:
        case PARAM_SPK_DYNAMIC_SYSTEM_X_COEFFICIENTS: {
            VIPER_LOGI(
                "DynSys[%s]: xCoeffs=%d,%d", param < 0x10300 ? "HP" : "SPK", val1, val2
            );
            this->dynamicSystem.SetXCoeffs(val1, val2);
            break;
        }
        case PARAM_HP_DYNAMIC_SYSTEM_Y_COEFFICIENTS:
        case PARAM_SPK_DYNAMIC_SYSTEM_Y_COEFFICIENTS: {
            VIPER_LOGI(
                "DynSys[%s]: yCoeffs=%d,%d", param < 0x10300 ? "HP" : "SPK", val1, val2
            );
            this->dynamicSystem.SetYCoeffs(val1, val2);
            break;
        }
        case PARAM_HP_DYNAMIC_SYSTEM_SIDE_GAIN:
        case PARAM_SPK_DYNAMIC_SYSTEM_SIDE_GAIN: {
            VIPER_LOGI(
                "DynSys[%s]: sideGain=%d,%d", param < 0x10300 ? "HP" : "SPK", val1, val2
            );
            this->dynamicSystem.SetSideGain((float) val1 / 100.0f, (float) val2 / 100.0f);
            break;
        }
        case PARAM_HP_DYNAMIC_SYSTEM_STRENGTH:
        case PARAM_SPK_DYNAMIC_SYSTEM_STRENGTH: {
            VIPER_LOGI("DynSys[%s]: strength=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->dynamicSystem.SetBassGain((float) val1 / 100.0f);
            break;
        }

        // Bass
        case PARAM_HP_BASS_ENABLE:
        case PARAM_SPK_BASS_ENABLE: {
            VIPER_LOGI(
                "Bass[%s]: %s",
                param == PARAM_HP_BASS_ENABLE ? "HP" : "SPK",
                val1 ? "ON" : "OFF"
            );
            this->viperBass.SetEnable(val1 != 0);
            break;
        }
        case PARAM_HP_BASS_MODE:
        case PARAM_SPK_BASS_MODE: {
            VIPER_LOGI("Bass[%s]: mode=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->viperBass.SetProcessMode((ViPERBass::ProcessMode) val1);
            break;
        }
        case PARAM_HP_BASS_FREQUENCY:
        case PARAM_SPK_BASS_FREQUENCY: {
            VIPER_LOGI("Bass[%s]: freq=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->viperBass.SetSpeaker((uint32_t) val1);
            break;
        }
        case PARAM_HP_BASS_GAIN:
        case PARAM_SPK_BASS_GAIN: {
            VIPER_LOGI("Bass[%s]: gain=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->viperBass.SetBassFactor((float) val1 / 100.0f);
            break;
        }

        // Clarity
        case PARAM_HP_CLARITY_ENABLE:
        case PARAM_SPK_CLARITY_ENABLE: {
            VIPER_LOGI(
                "Clarity[%s]: %s",
                param == PARAM_HP_CLARITY_ENABLE ? "HP" : "SPK",
                val1 ? "ON" : "OFF"
            );
            this->viperClarity.SetEnable(val1 != 0);
            break;
        }
        case PARAM_HP_CLARITY_MODE:
        case PARAM_SPK_CLARITY_MODE: {
            VIPER_LOGI("Clarity[%s]: mode=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->viperClarity.SetProcessMode((ViPERClarity::ClarityMode) val1);
            break;
        }
        case PARAM_HP_CLARITY_GAIN:
        case PARAM_SPK_CLARITY_GAIN: {
            VIPER_LOGI("Clarity[%s]: gain=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->viperClarity.SetClarity((float) val1 / 100.0f);
            break;
        }

        // Headphone Surround (VHE)
        case PARAM_HP_HEADPHONE_SURROUND_ENABLE:
        case PARAM_SPK_HEADPHONE_SURROUND_ENABLE: {
            VIPER_LOGI(
                "VHE[%s]: %s",
                param == PARAM_HP_HEADPHONE_SURROUND_ENABLE ? "HP" : "SPK",
                val1 ? "ON" : "OFF"
            );
            this->vhe.SetEnable(val1 != 0);
            break;
        }
        case PARAM_HP_HEADPHONE_SURROUND_STRENGTH:
        case PARAM_SPK_HEADPHONE_SURROUND_STRENGTH: {
            VIPER_LOGI("VHE[%s]: strength=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->vhe.SetEffectLevel(val1);
            break;
        }

        // Spectrum Extension
        case PARAM_HP_SPECTRUM_EXTENSION_ENABLE:
        case PARAM_SPK_SPECTRUM_EXTENSION_ENABLE: {
            VIPER_LOGI(
                "SpecExt[%s]: %s",
                param == PARAM_HP_SPECTRUM_EXTENSION_ENABLE ? "HP" : "SPK",
                val1 ? "ON" : "OFF"
            );
            this->spectrumExtend.SetEnable(val1 != 0);
            break;
        }
        case PARAM_HP_SPECTRUM_EXTENSION_BARK:
        case PARAM_SPK_SPECTRUM_EXTENSION_BARK: {
            VIPER_LOGI("SpecExt[%s]: bark=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->spectrumExtend.SetReferenceFrequency(val1);
            break;
        }
        case PARAM_HP_SPECTRUM_EXTENSION_BARK_RECONSTRUCT:
        case PARAM_SPK_SPECTRUM_EXTENSION_BARK_RECONSTRUCT: {
            VIPER_LOGI(
                "SpecExt[%s]: reconstruct=%d", param < 0x10300 ? "HP" : "SPK", val1
            );
            this->spectrumExtend.SetExciter((float) val1 / 100.0f);
            break;
        }

        // Field Surround (Colorful Music)
        case PARAM_HP_FIELD_SURROUND_ENABLE:
        case PARAM_SPK_FIELD_SURROUND_ENABLE: {
            VIPER_LOGI(
                "FieldSurr[%s]: %s",
                param == PARAM_HP_FIELD_SURROUND_ENABLE ? "HP" : "SPK",
                val1 ? "ON" : "OFF"
            );
            this->colorfulMusic.SetEnable(val1 != 0);
            break;
        }
        case PARAM_HP_FIELD_SURROUND_WIDENING:
        case PARAM_SPK_FIELD_SURROUND_WIDENING: {
            VIPER_LOGI("FieldSurr[%s]: widen=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->colorfulMusic.SetWidenValue((float) val1 / 100.0f);
            break;
        }
        case PARAM_HP_FIELD_SURROUND_MID_IMAGE:
        case PARAM_SPK_FIELD_SURROUND_MID_IMAGE: {
            VIPER_LOGI(
                "FieldSurr[%s]: midImage=%d", param < 0x10300 ? "HP" : "SPK", val1
            );
            this->colorfulMusic.SetMidImageValue((float) val1 / 100.0f);
            break;
        }
        case PARAM_HP_FIELD_SURROUND_DEPTH:
        case PARAM_SPK_FIELD_SURROUND_DEPTH: {
            VIPER_LOGI("FieldSurr[%s]: depth=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->colorfulMusic.SetDepthValue((short) val1);
            break;
        }

        // Differential Surround
        case PARAM_HP_DIFF_SURROUND_ENABLE:
        case PARAM_SPK_DIFF_SURROUND_ENABLE: {
            VIPER_LOGI(
                "DiffSurr[%s]: %s",
                param == PARAM_HP_DIFF_SURROUND_ENABLE ? "HP" : "SPK",
                val1 ? "ON" : "OFF"
            );
            this->diffSurround.SetEnable(val1 != 0);
            break;
        }
        case PARAM_HP_DIFF_SURROUND_DELAY:
        case PARAM_SPK_DIFF_SURROUND_DELAY: {
            VIPER_LOGI("DiffSurr[%s]: delay=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->diffSurround.SetDelayTime((float) val1 / 100.0f);
            break;
        }

        // Cure (Crossfeed)
        case PARAM_HP_CURE_ENABLE:
        case PARAM_SPK_CURE_ENABLE: {
            VIPER_LOGI(
                "Cure[%s]: %s",
                param == PARAM_HP_CURE_ENABLE ? "HP" : "SPK",
                val1 ? "ON" : "OFF"
            );
            this->cure.SetEnable(val1 != 0);
            break;
        }
        case PARAM_HP_CURE_STRENGTH:
        case PARAM_SPK_CURE_STRENGTH: {
            VIPER_LOGI("Cure[%s]: strength=%d", param < 0x10300 ? "HP" : "SPK", val1);
            switch (val1) {
                case 0: {
                    struct Crossfeed::Preset preset = {.cutoff = 650, .feedback = 95};
                    this->cure.SetPreset(preset);
                    break;
                }
                case 1: {
                    struct Crossfeed::Preset preset = {.cutoff = 700, .feedback = 60};
                    this->cure.SetPreset(preset);
                    break;
                }
                case 2: {
                    struct Crossfeed::Preset preset = {.cutoff = 700, .feedback = 45};
                    this->cure.SetPreset(preset);
                    break;
                }
            }
            break;
        }

        // Tube Simulator
        case PARAM_HP_TUBE_SIMULATOR_ENABLE:
        case PARAM_SPK_TUBE_SIMULATOR_ENABLE: {
            VIPER_LOGI(
                "TubeSim[%s]: %s",
                param == PARAM_HP_TUBE_SIMULATOR_ENABLE ? "HP" : "SPK",
                val1 ? "ON" : "OFF"
            );
            this->tubeSimulator.SetEnable(val1 != 0);
            break;
        }

        // AnalogX
        case PARAM_HP_ANALOGX_ENABLE:
        case PARAM_SPK_ANALOGX_ENABLE: {
            VIPER_LOGI(
                "AnalogX[%s]: %s",
                param == PARAM_HP_ANALOGX_ENABLE ? "HP" : "SPK",
                val1 ? "ON" : "OFF"
            );
            this->analogX.SetEnable(val1 != 0);
            break;
        }
        case PARAM_HP_ANALOGX_MODE:
        case PARAM_SPK_ANALOGX_MODE: {
            VIPER_LOGI("AnalogX[%s]: mode=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->analogX.SetProcessingModel(val1);
            break;
        }

        // Output / Limiter / Pan
        case PARAM_HP_OUTPUT_VOLUME:
        case PARAM_SPK_OUTPUT_VOLUME: {
            VIPER_LOGI("OutputVol[%s]: %d", param < 0x10300 ? "HP" : "SPK", val1);
            this->frameScale = (float) val1 / 100.0f;
            break;
        }
        case PARAM_HP_CHANNEL_PAN:
        case PARAM_SPK_CHANNEL_PAN: {
            float tmp = (float) val1 / 100.0f;
            VIPER_LOGI("Pan[%s]: %d", param < 0x10300 ? "HP" : "SPK", val1);
            if (tmp < 0.0f) {
                this->leftPan = 1.0f;
                this->rightPan = 1.0f + tmp;
            } else {
                this->leftPan = 1.0f - tmp;
                this->rightPan = 1.0f;
            }
            break;
        }
        case PARAM_HP_LIMITER:
        case PARAM_SPK_LIMITER: {
            VIPER_LOGI("Limiter[%s]: %d", param < 0x10300 ? "HP" : "SPK", val1);
            this->softwareLimiters[0].SetGate((float) val1 / 100.0f);
            this->softwareLimiters[1].SetGate((float) val1 / 100.0f);
            break;
        }

        // FET Compressor
        case PARAM_HP_FET_COMPRESSOR_ENABLE:
        case PARAM_SPK_FET_COMPRESSOR_ENABLE: {
            VIPER_LOGI(
                "FET[%s]: %s",
                param == PARAM_HP_FET_COMPRESSOR_ENABLE ? "HP" : "SPK",
                val1 ? "ON" : "OFF"
            );
            this->fetCompressor.SetParameter(
                FETCompressor::ENABLE, (float) val1 / 100.0f
            );
            break;
        }
        case PARAM_HP_FET_COMPRESSOR_THRESHOLD:
        case PARAM_SPK_FET_COMPRESSOR_THRESHOLD: {
            VIPER_LOGI("FET[%s]: threshold=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->fetCompressor.SetParameter(
                FETCompressor::THRESHOLD, (float) val1 / 100.0f
            );
            break;
        }
        case PARAM_HP_FET_COMPRESSOR_RATIO:
        case PARAM_SPK_FET_COMPRESSOR_RATIO: {
            VIPER_LOGI("FET[%s]: ratio=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->fetCompressor.SetParameter(FETCompressor::RATIO, (float) val1 / 100.0f);
            break;
        }
        case PARAM_HP_FET_COMPRESSOR_KNEE:
        case PARAM_SPK_FET_COMPRESSOR_KNEE: {
            VIPER_LOGI("FET[%s]: knee=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->fetCompressor.SetParameter(FETCompressor::KNEE, (float) val1 / 100.0f);
            break;
        }
        case PARAM_HP_FET_COMPRESSOR_AUTO_KNEE:
        case PARAM_SPK_FET_COMPRESSOR_AUTO_KNEE: {
            VIPER_LOGI("FET[%s]: autoKnee=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->fetCompressor.SetParameter(
                FETCompressor::AUTO_KNEE, (float) val1 / 100.0f
            );
            break;
        }
        case PARAM_HP_FET_COMPRESSOR_GAIN:
        case PARAM_SPK_FET_COMPRESSOR_GAIN: {
            VIPER_LOGI("FET[%s]: gain=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->fetCompressor.SetParameter(FETCompressor::GAIN, (float) val1 / 100.0f);
            break;
        }
        case PARAM_HP_FET_COMPRESSOR_AUTO_GAIN:
        case PARAM_SPK_FET_COMPRESSOR_AUTO_GAIN: {
            VIPER_LOGI("FET[%s]: autoGain=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->fetCompressor.SetParameter(
                FETCompressor::AUTO_GAIN, (float) val1 / 100.0f
            );
            break;
        }
        case PARAM_HP_FET_COMPRESSOR_ATTACK:
        case PARAM_SPK_FET_COMPRESSOR_ATTACK: {
            VIPER_LOGI("FET[%s]: attack=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->fetCompressor.SetParameter(
                FETCompressor::ATTACK, (float) val1 / 100.0f
            );
            break;
        }
        case PARAM_HP_FET_COMPRESSOR_AUTO_ATTACK:
        case PARAM_SPK_FET_COMPRESSOR_AUTO_ATTACK: {
            VIPER_LOGI("FET[%s]: autoAttack=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->fetCompressor.SetParameter(
                FETCompressor::AUTO_ATTACK, (float) val1 / 100.0f
            );
            break;
        }
        case PARAM_HP_FET_COMPRESSOR_RELEASE:
        case PARAM_SPK_FET_COMPRESSOR_RELEASE: {
            VIPER_LOGI("FET[%s]: release=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->fetCompressor.SetParameter(
                FETCompressor::RELEASE, (float) val1 / 100.0f
            );
            break;
        }
        case PARAM_HP_FET_COMPRESSOR_AUTO_RELEASE:
        case PARAM_SPK_FET_COMPRESSOR_AUTO_RELEASE: {
            VIPER_LOGI("FET[%s]: autoRelease=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->fetCompressor.SetParameter(
                FETCompressor::AUTO_RELEASE, (float) val1 / 100.0f
            );
            break;
        }
        case PARAM_HP_FET_COMPRESSOR_KNEE_MULTI:
        case PARAM_SPK_FET_COMPRESSOR_KNEE_MULTI: {
            VIPER_LOGI("FET[%s]: kneeMulti=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->fetCompressor.SetParameter(
                FETCompressor::KNEE_MULTI, (float) val1 / 100.0f
            );
            break;
        }
        case PARAM_HP_FET_COMPRESSOR_MAX_ATTACK:
        case PARAM_SPK_FET_COMPRESSOR_MAX_ATTACK: {
            VIPER_LOGI("FET[%s]: maxAttack=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->fetCompressor.SetParameter(
                FETCompressor::MAX_ATTACK, (float) val1 / 100.0f
            );
            break;
        }
        case PARAM_HP_FET_COMPRESSOR_MAX_RELEASE:
        case PARAM_SPK_FET_COMPRESSOR_MAX_RELEASE: {
            VIPER_LOGI("FET[%s]: maxRelease=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->fetCompressor.SetParameter(
                FETCompressor::MAX_RELEASE, (float) val1 / 100.0f
            );
            break;
        }
        case PARAM_HP_FET_COMPRESSOR_CREST:
        case PARAM_SPK_FET_COMPRESSOR_CREST: {
            VIPER_LOGI("FET[%s]: crest=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->fetCompressor.SetParameter(FETCompressor::CREST, (float) val1 / 100.0f);
            break;
        }
        case PARAM_HP_FET_COMPRESSOR_ADAPT:
        case PARAM_SPK_FET_COMPRESSOR_ADAPT: {
            VIPER_LOGI("FET[%s]: adapt=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->fetCompressor.SetParameter(FETCompressor::ADAPT, (float) val1 / 100.0f);
            break;
        }
        case PARAM_HP_FET_COMPRESSOR_NO_CLIP:
        case PARAM_SPK_FET_COMPRESSOR_NO_CLIP: {
            VIPER_LOGI("FET[%s]: noClip=%d", param < 0x10300 ? "HP" : "SPK", val1);
            this->fetCompressor.SetParameter(
                FETCompressor::NO_CLIP, (float) val1 / 100.0f
            );
            break;
        }

        // Speaker Correction
        case PARAM_SPK_SPEAKER_CORRECTION_ENABLE: {
            VIPER_LOGI("SpkCorr: %s", val1 ? "ON" : "OFF");
            this->speakerCorrection.SetEnable(val1 != 0);
            break;
        }

        default: {
            VIPER_LOGI("Unknown param: 0x%X val1=%d val2=%d", param, val1, val2);
            break;
        }
    }
}

void ViPER::resetAllEffects() {
    this->adaptiveBuffer.FlushBuffer();

    this->waveBuffer.Reset();

    this->convolver.SetSamplingRate(this->samplingRate);
    this->convolver.Reset();

    this->vhe.SetSamplingRate(this->samplingRate);
    this->vhe.Reset();

    this->viperDdc.SetSamplingRate(this->samplingRate);
    this->viperDdc.Reset();

    this->spectrumExtend.SetSamplingRate(this->samplingRate);
    this->spectrumExtend.Reset();

    this->iirFilter.SetSamplingRate(this->samplingRate);
    this->iirFilter.Reset();

    this->colorfulMusic.SetSamplingRate(this->samplingRate);
    this->colorfulMusic.Reset();

    this->reverberation.Reset();

    this->playbackGain.SetSamplingRate(this->samplingRate);
    this->playbackGain.Reset();

    this->fetCompressor.SetSamplingRate(this->samplingRate);
    this->fetCompressor.Reset();

    this->dynamicSystem.SetSamplingRate(this->samplingRate);
    this->dynamicSystem.Reset();

    this->viperBass.SetSamplingRate(this->samplingRate);
    this->viperBass.Reset();

    this->viperClarity.SetSamplingRate(this->samplingRate);
    this->viperClarity.Reset();

    this->diffSurround.SetSamplingRate(this->samplingRate);
    this->diffSurround.Reset();

    this->cure.SetSamplingRate(this->samplingRate);
    this->cure.Reset();

    this->tubeSimulator.Reset();

    this->analogX.SetSamplingRate(this->samplingRate);
    this->analogX.Reset();

    this->speakerCorrection.SetSamplingRate(this->samplingRate);
    this->speakerCorrection.Reset();

    for (auto &softwareLimiter : softwareLimiters) {
        softwareLimiter.Reset();
    }
}
