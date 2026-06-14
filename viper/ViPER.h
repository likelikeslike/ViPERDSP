#pragma once

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

    void Process(std::vector<float> &buffer, uint32_t size);
    void DispatchRawParam(
        int param,
        int val1,
        int val2,
        int val3,
        int val4,
        uint32_t arr_size,
        signed char *arr
    );

    void RequestEffectsReset();
    void ResetAllEffects();

    void RequestBuffersReset();
    void ResetBuffers();

    [[nodiscard]] uint32_t GetSamplingRate() const { return sampling_rate_; }
    [[nodiscard]] uint64_t GetProcessedFrames() const { return process_frame_count_; }
    [[nodiscard]] uint32_t GetConvolverKernelID() const {
        return convolver_.GetKernelID();
    }

    void SetSamplingRate(const uint32_t rate) { sampling_rate_ = rate; }

private:
    std::atomic<bool> pending_effects_reset_{false};
    std::atomic<bool> pending_buffers_reset_{false};

    uint32_t sampling_rate_;
    uint64_t process_frame_count_;

    float frame_scale_;
    float left_pan_;
    float right_pan_;

    // Effects
    AdaptiveBuffer adaptive_buffer_;
    WaveBuffer wave_buffer_;
    Convolver convolver_;
    VHE vhe_;
    ViPERDDC viper_ddc_;
    SpectrumExtend spectrum_extend_;
    StereoImager stereo_imager_;
    IIRFilter iir_filter_;
    DynamicEQ dynamic_eq_;
    ColorfulMusic colorful_music_;
    Reverberation reverberation_;
    PlaybackGain playback_gain_;
    LUFSTargeting lufs_targeting_;
    FETCompressor fet_compressor_;
    MultibandCompressor multiband_compressor_;
    DynamicSystem dynamic_system_;
    ViPERBass viper_bass_;
    ViPERBassMono viper_bass_mono_;
    PsychoacousticBass psychoacoustic_bass_;
    ViPERClarity viper_clarity_;
    DiffSurround diff_surround_;
    Cure cure_;
    TubeSimulator tube_simulator_;
    AnalogX analog_x_;
    SpeakerCorrection speaker_correction_;
    std::array<SoftwareLimiter, 2> software_limiters_;
};
