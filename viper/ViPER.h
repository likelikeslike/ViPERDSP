#pragma once

#include "../include/ViPERParams.h"
#include "effects/AnalogX.h"
#include "effects/ColorfulMusic.h"
#include "effects/Convolver.h"
#include "effects/Cure.h"
#include "effects/DiffSurround.h"
#include "effects/DynamicEQ.h"
#include "effects/DynamicSystem.h"
#include "effects/FETCompressor.h"
#include "effects/IIRFilter.h"
#include "effects/LUFSTargeting.h"
#include "effects/MultibandCompressor.h"
#include "effects/PlaybackGain.h"
#include "effects/PsychoacousticBass.h"
#include "effects/Reverberation.h"
#include "effects/SoftwareLimiter.h"
#include "effects/SpeakerCorrection.h"
#include "effects/SpectrumExtend.h"
#include "effects/StereoImager.h"
#include "effects/TubeSimulator.h"
#include "effects/VHE.h"
#include "effects/ViPERBass.h"
#include "effects/ViPERBassMono.h"
#include "effects/ViPERClarity.h"
#include "effects/ViPERDDC.h"
#include "utils/AdaptiveBuffer.h"
#include "utils/WaveBuffer.h"
#include <array>

class ViPER {
public:
    ViPER();

    void process(std::vector<float> &buffer, uint32_t size);
    void DispatchCommand(
        int param,
        int val1,
        int val2,
        int val3,
        int val4,
        uint32_t arrSize,
        signed char *arr
    );
    void resetAllEffects();

    void SetSamplingRate(uint32_t rate) { samplingRate = rate; }
    uint32_t GetSamplingRate() const { return samplingRate; }
    uint64_t GetProcessTimeMs() const { return processTimeMs; }
    uint32_t GetConvolverKernelID() { return convolver.GetKernelID(); }

private:
    uint64_t processTimeMs;
    uint32_t samplingRate;

    // Effects
    AdaptiveBuffer adaptiveBuffer;
    WaveBuffer waveBuffer;
    Convolver convolver;
    VHE vhe;
    ViPERDDC viperDdc;
    SpectrumExtend spectrumExtend;
    StereoImager stereoImager;
    IIRFilter iirFilter;
    DynamicEQ dynamicEQ;
    ColorfulMusic colorfulMusic;
    Reverberation reverberation;
    PlaybackGain playbackGain;
    LUFSTargeting lufsTargeting;
    FETCompressor fetCompressor;
    MultibandCompressor multibandCompressor;
    DynamicSystem dynamicSystem;
    ViPERBass viperBass;
    ViPERBassMono viperBassMono;
    PsychoacousticBass psychoacousticBass;
    ViPERClarity viperClarity;
    DiffSurround diffSurround;
    Cure cure;
    TubeSimulator tubeSimulator;
    AnalogX analogX;
    SpeakerCorrection speakerCorrection;
    std::array<SoftwareLimiter, 2> softwareLimiters;

    float frameScale;
    float leftPan;
    float rightPan;
};
