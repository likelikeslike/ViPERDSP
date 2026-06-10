#include "ViPER.h"
#include "../include/ViPERParams.h"
#include "../include/log.h"
#include "constants.h"

using namespace viper::params;

ViPER::ViPER() :
    sampling_rate_(VIPER_DEFAULT_SAMPLING_RATE),
    process_frame_count_(0),
    frame_scale_(1.0f),
    left_pan_(1.0f),
    right_pan_(1.0f),
    adaptive_buffer_(AdaptiveBuffer(2, 4096)),
    wave_buffer_(WaveBuffer(2, 4096)),
    iir_filter_(IIRFilter(10)) {
    VIPER_LOGI("Welcome to ViPER FX");
    VIPER_LOGI("Current version is %s (%d)", VERSION_NAME, VERSION_CODE);

    convolver_.SetEnable(false);
    convolver_.SetSamplingRate(sampling_rate_);
    convolver_.Reset();

    vhe_.SetEnable(false);
    vhe_.SetSamplingRate(sampling_rate_);
    vhe_.Reset();

    viper_ddc_.SetEnable(false);
    viper_ddc_.SetSamplingRate(sampling_rate_);
    viper_ddc_.Reset();

    spectrum_extend_.SetEnable(false);
    spectrum_extend_.SetSamplingRate(sampling_rate_);
    spectrum_extend_.SetReferenceFrequency(7600);
    spectrum_extend_.SetExciter(0);
    spectrum_extend_.Reset();

    iir_filter_.SetEnable(false);
    iir_filter_.SetSamplingRate(sampling_rate_);
    iir_filter_.Reset();

    dynamic_eq_.SetEnable(false);
    dynamic_eq_.SetSamplingRate(sampling_rate_);
    dynamic_eq_.Reset();

    colorful_music_.SetEnable(false);
    colorful_music_.SetSamplingRate(sampling_rate_);
    colorful_music_.Reset();

    stereo_imager_.SetEnable(false);
    stereo_imager_.SetSamplingRate(sampling_rate_);
    stereo_imager_.Reset();

    reverberation_.SetEnable(false);
    reverberation_.Reset();

    playback_gain_.SetEnable(false);
    playback_gain_.SetSamplingRate(sampling_rate_);
    playback_gain_.Reset();

    lufs_targeting_.SetEnable(false);
    lufs_targeting_.SetSamplingRate(sampling_rate_);
    lufs_targeting_.Reset();

    fet_compressor_.SetEnable(false);
    fet_compressor_.SetSamplingRate(sampling_rate_);
    fet_compressor_.Reset();

    multiband_compressor_.SetEnable(false);
    multiband_compressor_.SetSamplingRate(sampling_rate_);
    multiband_compressor_.Reset();

    dynamic_system_.SetEnable(false);
    dynamic_system_.SetSamplingRate(sampling_rate_);
    dynamic_system_.Reset();

    viper_bass_.SetSamplingRate(sampling_rate_);
    viper_bass_.Reset();

    viper_bass_mono_.SetSamplingRate(sampling_rate_);
    viper_bass_mono_.Reset();

    psychoacoustic_bass_.SetEnable(false);
    psychoacoustic_bass_.SetSamplingRate(sampling_rate_);
    psychoacoustic_bass_.Reset();

    viper_clarity_.SetSamplingRate(sampling_rate_);
    viper_clarity_.Reset();

    diff_surround_.SetEnable(false);
    diff_surround_.SetSamplingRate(sampling_rate_);
    diff_surround_.Reset();

    cure_.SetEnable(false);
    cure_.SetSamplingRate(sampling_rate_);
    cure_.Reset();

    tube_simulator_.SetEnable(false);
    tube_simulator_.Reset();

    analog_x_.SetEnable(false);
    analog_x_.SetSamplingRate(sampling_rate_);
    analog_x_.SetProcessingModel(0);
    analog_x_.Reset();

    speaker_correction_.SetEnable(false);
    speaker_correction_.SetSamplingRate(sampling_rate_);
    speaker_correction_.Reset();

    for (auto &software_limiter : software_limiters_) {
        software_limiter.Reset();
    }
}

void ViPER::Process(std::vector<float> &buffer, const uint32_t size) {
    process_frame_count_ += size;

    float *tmp_buf;
    uint32_t tmp_buf_size;

    if (convolver_.GetEnabled() || vhe_.GetEnabled()) {
        if (!wave_buffer_.PushSamples(buffer.data(), size)) {
            wave_buffer_.Reset();
            memset(buffer.data(), 0, size * 2 * sizeof(float));
            return;
        }

        float *ptr = wave_buffer_.GetBuffer();
        uint32_t ret = convolver_.Process(ptr, ptr, size);
        ret = vhe_.Process(ptr, ptr, ret);
        wave_buffer_.SetBufferOffset(ret);

        if (!adaptive_buffer_.PushZero(ret)) {
            wave_buffer_.Reset();
            adaptive_buffer_.FlushBuffer();
            memset(buffer.data(), 0, size * 2 * sizeof(float));
            return;
        }

        ptr = adaptive_buffer_.GetBuffer();
        ret = wave_buffer_.PopSamples(ptr, ret, true);
        adaptive_buffer_.SetBufferOffset(ret);

        tmp_buf = ptr;
        tmp_buf_size = ret;
    } else {
        if (adaptive_buffer_.PushFrames(buffer.data(), size)) {
            adaptive_buffer_.SetBufferOffset(size);

            tmp_buf = adaptive_buffer_.GetBuffer();
            tmp_buf_size = size;
        } else {
            adaptive_buffer_.FlushBuffer();
            memset(buffer.data(), 0, size * 2 * sizeof(float));
            return;
        }
    }

    if (tmp_buf_size != 0) {
        viper_ddc_.Process(tmp_buf, tmp_buf_size);
        spectrum_extend_.Process(tmp_buf, tmp_buf_size);
        iir_filter_.Process(tmp_buf, tmp_buf_size);
        dynamic_eq_.Process(tmp_buf, tmp_buf_size);
        colorful_music_.Process(tmp_buf, tmp_buf_size);
        stereo_imager_.Process(tmp_buf, tmp_buf_size);
        diff_surround_.Process(tmp_buf, tmp_buf_size);
        playback_gain_.Process(tmp_buf, tmp_buf_size);
        multiband_compressor_.Process(tmp_buf, tmp_buf_size);
        fet_compressor_.Process(tmp_buf, tmp_buf_size);
        dynamic_system_.Process(tmp_buf, tmp_buf_size);
        tube_simulator_.TubeProcess(tmp_buf, tmp_buf_size);
        psychoacoustic_bass_.Process(tmp_buf, tmp_buf_size);
        viper_bass_.Process(tmp_buf, tmp_buf_size);
        viper_bass_mono_.Process(tmp_buf, tmp_buf_size);
        viper_clarity_.Process(tmp_buf, tmp_buf_size);
        cure_.Process(tmp_buf, tmp_buf_size);
        analog_x_.Process(tmp_buf, tmp_buf_size);
        reverberation_.Process(tmp_buf, tmp_buf_size);
        speaker_correction_.Process(tmp_buf, tmp_buf_size);
        lufs_targeting_.Process(tmp_buf, tmp_buf_size);

        if (frame_scale_ != 1.0) {
            adaptive_buffer_.ScaleFrames(frame_scale_);
        }

        if (left_pan_ < 1.0 || right_pan_ < 1.0) {
            adaptive_buffer_.PanFrames(left_pan_, right_pan_);
        }

        for (uint32_t i = 0; i < tmp_buf_size * 2; i += 2) {
            tmp_buf[i] = software_limiters_[0].Process(tmp_buf[i]);
            tmp_buf[i + 1] = software_limiters_[1].Process(tmp_buf[i + 1]);
        }

        if (!adaptive_buffer_.PopFrames(buffer.data(), tmp_buf_size)) {
            adaptive_buffer_.FlushBuffer();
            memset(buffer.data(), 0, size * 2 * sizeof(float));
            return;
        }

        if (size <= tmp_buf_size) {
            return;
        }
    }

    memmove(
        buffer.data() + (size - tmp_buf_size) * 2,
        buffer.data(),
        tmp_buf_size * 2 * sizeof(float)
    );
    memset(buffer.data(), 0, (size - tmp_buf_size) * 2 * sizeof(float));
}

void ViPER::DispatchRawParam(
    const int param,
    int val1,
    const int val2,
    const int val3,
    int val4, // TODO: Remove this.
    const uint32_t arr_size,
    signed char *arr
) {
    switch (param) {

        // System
        case kParamResetAllEffects: {
            VIPER_LOGI("ResetAllEffects");
            ResetAllEffects();
            break;
        }

        // Master Limiter / Output
        case kParamMasterLimiterThreshold: {
            VIPER_LOGI("Limiter: %d", val1);
            software_limiters_[0].SetGate(static_cast<float>(val1) / 100.0f);
            software_limiters_[1].SetGate(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamMasterLimiterOutputVolume: {
            VIPER_LOGI("OutputVol: %d", val1);
            frame_scale_ = static_cast<float>(val1) / 100.0f;
            break;
        }
        case kParamMasterLimiterChannelPan: {
            const float tmp = static_cast<float>(val1) / 100.0f;
            VIPER_LOGI("Pan: %d", val1);
            if (tmp < 0.0f) {
                left_pan_ = 1.0f;
                right_pan_ = 1.0f + tmp;
            } else {
                left_pan_ = 1.0f - tmp;
                right_pan_ = 1.0f;
            }
            break;
        }

        // Playback Gain Control
        case kParamPlaybackGainControlEnable: {
            VIPER_LOGI("PlaybackGain: %s", val1 ? "ON" : "OFF");
            playback_gain_.SetEnable(val1 != 0);
            break;
        }
        case kParamPlaybackGainControlStrength: {
            VIPER_LOGI("PlaybackGain: strength=%d", val1);
            playback_gain_.SetRatio(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamPlaybackGainControlMaxGain: {
            VIPER_LOGI("PlaybackGain: max_gain=%d", val1);
            playback_gain_.SetMaxGainFactor(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamPlaybackGainControlOutputThreshold: {
            VIPER_LOGI("PlaybackGain: output_threshold=%d", val1);
            playback_gain_.SetVolume(static_cast<float>(val1) / 100.0f);
            break;
        }

        // LUFS Targeting
        case kParamLufsEnable: {
            VIPER_LOGI("LUFS: %s", val1 ? "ON" : "OFF");
            lufs_targeting_.SetEnable(val1 != 0);
            break;
        }
        case kParamLufsTarget: {
            VIPER_LOGI("LUFS: target=%d", val1);
            lufs_targeting_.SetTargetLUFS(static_cast<float>(val1) / -10.0f);
            break;
        }
        case kParamLufsMaxGain: {
            VIPER_LOGI("LUFS: max_gain=%d", val1);
            lufs_targeting_.SetMaxGain(static_cast<float>(val1) / 10.0f);
            break;
        }
        case kParamLufsSpeed: {
            VIPER_LOGI("LUFS: speed=%d", val1);
            lufs_targeting_.SetSpeed(val1);
            break;
        }

        // FET Compressor
        case kParamFetCompressorEnable: {
            VIPER_LOGI("FET: %s", val1 ? "ON" : "OFF");
            fet_compressor_.SetEnable(val1 != 0);
            break;
        }
        case kParamFetCompressorThreshold: {
            VIPER_LOGI("FET: threshold=%d", val1);
            fet_compressor_.SetThreshold(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFetCompressorRatio: {
            VIPER_LOGI("FET: ratio=%d", val1);
            fet_compressor_.SetRatio(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFetCompressorKnee: {
            VIPER_LOGI("FET: knee=%d", val1);
            fet_compressor_.SetKnee(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFetCompressorKneeAuto: {
            VIPER_LOGI("FET: knee_auto=%d", val1);
            fet_compressor_.SetKneeAuto(val1 != 0);
            break;
        }
        case kParamFetCompressorGain: {
            VIPER_LOGI("FET: gain=%d", val1);
            fet_compressor_.SetGain(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFetCompressorGainAuto: {
            VIPER_LOGI("FET: gain_auto=%d", val1);
            fet_compressor_.SetGainAuto(val1 != 0);
            break;
        }
        case kParamFetCompressorAttack: {
            VIPER_LOGI("FET: attack=%d", val1);
            fet_compressor_.SetAttack(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFetCompressorAttackAuto: {
            VIPER_LOGI("FET: attack_auto=%d", val1);
            fet_compressor_.SetAttackAuto(val1 != 0);
            break;
        }
        case kParamFetCompressorRelease: {
            VIPER_LOGI("FET: release=%d", val1);
            fet_compressor_.SetRelease(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFetCompressorReleaseAuto: {
            VIPER_LOGI("FET: release_auto=%d", val1);
            fet_compressor_.SetReleaseAuto(val1 != 0);
            break;
        }
        case kParamFetCompressorKneeMulti: {
            VIPER_LOGI("FET: knee_multi=%d", val1);
            fet_compressor_.SetKneeMulti(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFetCompressorMaxAttack: {
            VIPER_LOGI("FET: max_attack=%d", val1);
            fet_compressor_.SetMaxAttack(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFetCompressorMaxRelease: {
            VIPER_LOGI("FET: max_release=%d", val1);
            fet_compressor_.SetMaxRelease(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFetCompressorCrest: {
            VIPER_LOGI("FET: crest=%d", val1);
            fet_compressor_.SetCrest(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFetCompressorAdapt: {
            VIPER_LOGI("FET: adapt=%d", val1);
            fet_compressor_.SetAdapt(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFetCompressorNoClip: {
            VIPER_LOGI("FET: no_clip=%d", val1);
            fet_compressor_.SetNoClip(val1 != 0);
            break;
        }

        // Bass
        case kParamBassEnable: {
            VIPER_LOGI("Bass: %s", val1 ? "ON" : "OFF");
            viper_bass_.SetEnable(val1 != 0);
            break;
        }
        case kParamBassMode: {
            VIPER_LOGI("Bass: mode=%d", val1);
            viper_bass_.SetProcessMode(static_cast<ViPERBass::ProcessMode>(val1));
            break;
        }
        case kParamBassFrequency: {
            VIPER_LOGI("Bass: freq=%d", val1);
            viper_bass_.SetSpeaker(static_cast<uint32_t>(val1));
            break;
        }
        case kParamBassGain: {
            VIPER_LOGI("Bass: gain=%d", val1);
            viper_bass_.SetBassFactor(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamBassAntiPop: {
            VIPER_LOGI("Bass: anti_pop=%s", val1 ? "ON" : "OFF");
            viper_bass_.SetAntiPop(val1 != 0);
            break;
        }

        // Bass Mono
        case kParamBassMonoEnable: {
            VIPER_LOGI("BassMono: %s", val1 ? "ON" : "OFF");
            viper_bass_mono_.SetEnable(val1 != 0);
            break;
        }
        case kParamBassMonoMode: {
            VIPER_LOGI("BassMono: mode=%d", val1);
            viper_bass_mono_.SetProcessMode(
                static_cast<ViPERBassMono::ProcessMode>(val1)
            );
            break;
        }
        case kParamBassMonoFrequency: {
            VIPER_LOGI("BassMono: freq=%d", val1);
            viper_bass_mono_.SetSpeaker(static_cast<uint32_t>(val1));
            break;
        }
        case kParamBassMonoGain: {
            VIPER_LOGI("BassMono: gain=%d", val1);
            viper_bass_mono_.SetBassFactor(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamBassMonoAntiPop: {
            VIPER_LOGI("BassMono: anti_pop=%s", val1 ? "ON" : "OFF");
            viper_bass_mono_.SetAntiPop(val1 != 0);
            break;
        }

        // Psychoacoustic Bass
        case kParamPsychoacousticBassEnable: {
            VIPER_LOGI("PsychoBass: %s", val1 ? "ON" : "OFF");
            psychoacoustic_bass_.SetEnable(val1 != 0);
            break;
        }
        case kParamPsychoacousticBassCutoff: {
            VIPER_LOGI("PsychoBass: cutoff=%d", val1);
            psychoacoustic_bass_.SetCutoff(static_cast<uint32_t>(val1));
            break;
        }
        case kParamPsychoacousticBassIntensity: {
            VIPER_LOGI("PsychoBass: intensity=%d", val1);
            psychoacoustic_bass_.SetIntensity(static_cast<uint32_t>(val1));
            break;
        }
        case kParamPsychoacousticBassHarmonicOrder: {
            VIPER_LOGI("PsychoBass: harmonic_order=%d", val1);
            psychoacoustic_bass_.SetHarmonicOrder(static_cast<uint32_t>(val1));
            break;
        }
        case kParamPsychoacousticBassOriginalLevel: {
            VIPER_LOGI("PsychoBass: original_level=%d", val1);
            psychoacoustic_bass_.SetOriginalBassLevel(static_cast<uint32_t>(val1));
            break;
        }

        // Spectrum Extension
        case kParamSpectrumExtensionEnable: {
            VIPER_LOGI("SpecExt: %s", val1 ? "ON" : "OFF");
            spectrum_extend_.SetEnable(val1 != 0);
            break;
        }
        case kParamSpectrumExtensionStrength: {
            VIPER_LOGI("SpecExt: strength=%d", val1);
            spectrum_extend_.SetReferenceFrequency(val1);
            break;
        }
        case kParamSpectrumExtensionExciter: {
            VIPER_LOGI("SpecExt: exciter=%d", val1);
            spectrum_extend_.SetExciter(static_cast<float>(val1) / 100.0f);
            break;
        }

        // Equalizer (IIR Filter)
        case kParamEqualizerEnable: {
            VIPER_LOGI("EQ: %s", val1 ? "ON" : "OFF");
            iir_filter_.SetEnable(val1 != 0);
            break;
        }
        case kParamEqualizerBandLevel: {
            VIPER_LOGI("EQ: band=%d level=%d", val1, val2);
            iir_filter_.SetBandLevel(
                static_cast<uint32_t>(val1), static_cast<float>(val2) / 100.0f
            );
            break;
        }
        case kParamEqualizerBandCount: {
            VIPER_LOGI("EQ: band_count=%d", val1);
            iir_filter_.SetBandCount(static_cast<uint32_t>(val1));
            break;
        }

        // Convolver
        case kParamConvolverEnable: {
            VIPER_LOGI("Convolver: %s", val1 ? "ON" : "OFF");
            convolver_.SetEnable(val1 != 0);
            break;
        }
        case kParamConvolverSetKernel: {
            if (arr_size > 0 && arr != nullptr) {
                char path[256] = {};
                memcpy(path, arr, arr_size < 255 ? arr_size : 255);
                VIPER_LOGI("Convolver: SetKernel path=%s", path);
                convolver_.SetKernel(path);
            }
            break;
        }
        case kParamConvolverPrepareBuffer: {
            VIPER_LOGI(
                "Convolver: PrepareBuffer channels=%d frames=%d sr=%d", val1, val2, val3
            );
            convolver_.PrepareKernelBuffer(val1, val2, val3);
            break;
        }
        case kParamConvolverSetBuffer: {
            VIPER_LOGI("Convolver: SetBuffer offset=%d size=%u", val1, arr_size);
            convolver_.SetKernelBuffer(val1, reinterpret_cast<float *>(arr), arr_size);
            break;
        }
        case kParamConvolverCommitBuffer: {
            VIPER_LOGI(
                "Convolver: CommitBuffer channels=%d frames=%d sr=%d", val1, val2, val3
            );
            convolver_.CommitKernelBuffer(val1, val2, val3);
            break;
        }
        case kParamConvolverCrossChannel: {
            VIPER_LOGI("Convolver: CrossChannel=%d%%", val1);
            convolver_.SetCrossChannel(static_cast<float>(val1) / 100.0f);
            break;
        }

        // DDC
        case kParamDdcEnable: {
            VIPER_LOGI("DDC: %s", val1 ? "ON" : "OFF");
            viper_ddc_.SetEnable(val1 != 0);
            break;
        }
        case kParamDdcCoefficients: {
            VIPER_LOGI("DDC: SetCoeffs arr_size=%u", arr_size);
            viper_ddc_.SetCoeffs(
                arr_size,
                reinterpret_cast<float *>(arr),
                reinterpret_cast<float *>(arr + arr_size * sizeof(float))
            );
            break;
        }

        // Field Surround (Colorful Music)
        case kParamFieldSurroundEnable: {
            VIPER_LOGI("FieldSurr: %s", val1 ? "ON" : "OFF");
            colorful_music_.SetEnable(val1 != 0);
            break;
        }
        case kParamFieldSurroundWidening: {
            VIPER_LOGI("FieldSurr: widen=%d", val1);
            colorful_music_.SetWidenValue(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFieldSurroundMidImage: {
            VIPER_LOGI("FieldSurr: mid_image=%d", val1);
            colorful_music_.SetMidImageValue(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFieldSurroundDepth: {
            VIPER_LOGI("FieldSurr: depth=%d", val1);
            colorful_music_.SetDepthValue(static_cast<short>(val1));
            break;
        }

        // Differential Surround
        case kParamDiffSurroundEnable: {
            VIPER_LOGI("DiffSurr: %s", val1 ? "ON" : "OFF");
            diff_surround_.SetEnable(val1 != 0);
            break;
        }
        case kParamDiffSurroundDelay: {
            VIPER_LOGI("DiffSurr: delay=%d", val1);
            diff_surround_.SetDelayTime(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamDiffSurroundReverse: {
            VIPER_LOGI("DiffSurr: reverse=%s", val1 ? "ON" : "OFF");
            diff_surround_.SetReverse(val1 != 0);
            break;
        }
        case kParamDiffSurroundWetDryMix: {
            VIPER_LOGI("DiffSurr: wet_dry_mix=%d", val1);
            diff_surround_.SetWetDryMix(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamDiffSurroundLpCutoff: {
            VIPER_LOGI("DiffSurr: lp_cutoff=%d", val1);
            diff_surround_.SetLPCutoff(static_cast<float>(val1));
            break;
        }

        // Stereo Imager
        case kParamStereoImagerEnable: {
            VIPER_LOGI("StereoImg: %s", val1 ? "ON" : "OFF");
            stereo_imager_.SetEnable(val1 != 0);
            break;
        }
        case kParamStereoImagerLowWidth: {
            VIPER_LOGI("StereoImg: low_width=%d", val1);
            stereo_imager_.SetLowWidth(static_cast<float>(val1));
            break;
        }
        case kParamStereoImagerMidWidth: {
            VIPER_LOGI("StereoImg: mid_width=%d", val1);
            stereo_imager_.SetMidWidth(static_cast<float>(val1));
            break;
        }
        case kParamStereoImagerHighWidth: {
            VIPER_LOGI("StereoImg: high_width=%d", val1);
            stereo_imager_.SetHighWidth(static_cast<float>(val1));
            break;
        }
        case kParamStereoImagerLowCrossover: {
            VIPER_LOGI("StereoImg: low_crossover=%d", val1);
            stereo_imager_.SetLowCrossover(static_cast<float>(val1));
            break;
        }
        case kParamStereoImagerHighCrossover: {
            VIPER_LOGI("StereoImg: high_crossover=%d", val1);
            stereo_imager_.SetHighCrossover(static_cast<float>(val1));
            break;
        }

        // Headphone Surround (VHE)
        case kParamHeadphoneSurroundEnable: {
            VIPER_LOGI("VHE: %s", val1 ? "ON" : "OFF");
            vhe_.SetEnable(val1 != 0);
            break;
        }
        case kParamHeadphoneSurroundQuality: {
            VIPER_LOGI("VHE: quality=%d", val1);
            vhe_.SetEffectLevel(val1);
            break;
        }

        // Reverb
        case kParamReverbEnable: {
            VIPER_LOGI("Reverb: %s", val1 ? "ON" : "OFF");
            reverberation_.SetEnable(val1 != 0);
            break;
        }
        case kParamReverbRoomSize: {
            VIPER_LOGI("Reverb: room_size=%d", val1);
            reverberation_.SetRoomSize(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamReverbWidth: {
            VIPER_LOGI("Reverb: width=%d", val1);
            reverberation_.SetWidth(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamReverbDamp: {
            VIPER_LOGI("Reverb: damp=%d", val1);
            reverberation_.SetDamp(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamReverbWet: {
            VIPER_LOGI("Reverb: wet=%d", val1);
            reverberation_.SetWet(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamReverbDry: {
            VIPER_LOGI("Reverb: dry=%d", val1);
            reverberation_.SetDry(static_cast<float>(val1) / 100.0f);
            break;
        }

        // Dynamic System
        case kParamDynamicSystemEnable: {
            VIPER_LOGI("DynSys: %s", val1 ? "ON" : "OFF");
            dynamic_system_.SetEnable(val1 != 0);
            break;
        }
        case kParamDynamicSystemXCoefficients: {
            VIPER_LOGI("DynSys: x_coeffs=%d,%d", val1, val2);
            dynamic_system_.SetXCoeffs(val1, val2);
            break;
        }
        case kParamDynamicSystemYCoefficients: {
            VIPER_LOGI("DynSys: y_coeffs=%d,%d", val1, val2);
            dynamic_system_.SetYCoeffs(val1, val2);
            break;
        }
        case kParamDynamicSystemSideGain: {
            VIPER_LOGI("DynSys: side_gain=%d,%d", val1, val2);
            dynamic_system_.SetSideGain(
                static_cast<float>(val1) / 100.0f, static_cast<float>(val2) / 100.0f
            );
            break;
        }
        case kParamDynamicSystemStrength: {
            VIPER_LOGI("DynSys: strength=%d", val1);
            dynamic_system_.SetBassGain(static_cast<float>(val1) / 100.0f);
            break;
        }

        // Clarity
        case kParamClarityEnable: {
            VIPER_LOGI("Clarity: %s", val1 ? "ON" : "OFF");
            viper_clarity_.SetEnable(val1 != 0);
            break;
        }
        case kParamClarityMode: {
            VIPER_LOGI("Clarity: mode=%d", val1);
            viper_clarity_.SetProcessMode(static_cast<ViPERClarity::ClarityMode>(val1));
            break;
        }
        case kParamClarityGain: {
            VIPER_LOGI("Clarity: gain=%d", val1);
            viper_clarity_.SetClarity(static_cast<float>(val1) / 100.0f);
            break;
        }

        // Cure (Crossfeed)
        case kParamCureEnable: {
            VIPER_LOGI("Cure: %s", val1 ? "ON" : "OFF");
            cure_.SetEnable(val1 != 0);
            break;
        }
        case kParamCureCrossfeedPreset: {
            VIPER_LOGI("Cure: crossfeed_preset=%d", val1);
            switch (val1) {
                case 0: {
                    constexpr Crossfeed::Preset preset = {.cutoff = 650, .feedback = 95};
                    cure_.SetPreset(preset);
                    break;
                }
                case 1: {
                    constexpr Crossfeed::Preset preset = {.cutoff = 700, .feedback = 60};
                    cure_.SetPreset(preset);
                    break;
                }
                case 2: {
                    constexpr Crossfeed::Preset preset = {.cutoff = 700, .feedback = 45};
                    cure_.SetPreset(preset);
                    break;
                }
                default:;
            }
            break;
        }

        // Tube Simulator
        case kParamTubeSimulatorEnable: {
            VIPER_LOGI("TubeSim: %s", val1 ? "ON" : "OFF");
            tube_simulator_.SetEnable(val1 != 0);
            break;
        }

        // AnalogX
        case kParamAnalogXEnable: {
            VIPER_LOGI("AnalogX: %s", val1 ? "ON" : "OFF");
            analog_x_.SetEnable(val1 != 0);
            break;
        }
        case kParamAnalogXMode: {
            VIPER_LOGI("AnalogX: mode=%d", val1);
            analog_x_.SetProcessingModel(val1);
            break;
        }

        // Speaker Correction (toggle only — cutoffs/Q live as constants
        // in SpeakerCorrection::SpeakerCorrection() until UI surfaces
        // sliders; the SetHighPassCutoff / SetLowPassCutoff /
        // SetBandPassCenter / SetBandPassQ setters stay defined on the
        // effect class so all layers grow back together when needed.)
        case kParamSpeakerCorrectionEnable: {
            VIPER_LOGI("SpkCorr: %s", val1 ? "ON" : "OFF");
            speaker_correction_.SetEnable(val1 != 0);
            break;
        }

        // Multiband Compressor
        case kParamMultibandCompressorEnable: {
            VIPER_LOGI("MBComp: %s", val1 ? "ON" : "OFF");
            multiband_compressor_.SetEnable(val1 != 0);
            break;
        }
        case kParamMultibandCompressorBandCount: {
            VIPER_LOGI("MBComp: band_count=%d", val1);
            multiband_compressor_.SetBandCount(val1);
            break;
        }
        case kParamMultibandCompressorCrossoverFrequency: {
            VIPER_LOGI("MBComp: xover[%d]=%d", val1, val2);
            multiband_compressor_.SetCrossoverFrequency(val1, static_cast<float>(val2));
            break;
        }
        case kParamMultibandCompressorBandThreshold: {
            VIPER_LOGI("MBComp: band[%d] threshold=%d", val1, val2);
            multiband_compressor_.SetBandThreshold(
                val1, static_cast<float>(val2) / 100.0f
            );
            break;
        }
        case kParamMultibandCompressorBandRatio: {
            VIPER_LOGI("MBComp: band[%d] ratio=%d", val1, val2);
            multiband_compressor_.SetBandRatio(val1, static_cast<float>(val2) / 100.0f);
            break;
        }
        case kParamMultibandCompressorBandKnee: {
            VIPER_LOGI("MBComp: band[%d] knee=%d", val1, val2);
            multiband_compressor_.SetBandKnee(val1, static_cast<float>(val2) / 100.0f);
            break;
        }
        case kParamMultibandCompressorBandKneeAuto: {
            VIPER_LOGI("MBComp: band[%d] knee_auto=%d", val1, val2);
            multiband_compressor_.SetBandKneeAuto(val1, val2 != 0);
            break;
        }
        case kParamMultibandCompressorBandGain: {
            VIPER_LOGI("MBComp: band[%d] gain=%d", val1, val2);
            multiband_compressor_.SetBandGain(val1, static_cast<float>(val2) / 100.0f);
            break;
        }
        case kParamMultibandCompressorBandGainAuto: {
            VIPER_LOGI("MBComp: band[%d] gain_auto=%d", val1, val2);
            multiband_compressor_.SetBandGainAuto(val1, val2 != 0);
            break;
        }
        case kParamMultibandCompressorBandAttack: {
            VIPER_LOGI("MBComp: band[%d] attack=%d", val1, val2);
            multiband_compressor_.SetBandAttack(val1, static_cast<float>(val2) / 100.0f);
            break;
        }
        case kParamMultibandCompressorBandAttackAuto: {
            VIPER_LOGI("MBComp: band[%d] attack_auto=%d", val1, val2);
            multiband_compressor_.SetBandAttackAuto(val1, val2 != 0);
            break;
        }
        case kParamMultibandCompressorBandRelease: {
            VIPER_LOGI("MBComp: band[%d] release=%d", val1, val2);
            multiband_compressor_.SetBandRelease(val1, static_cast<float>(val2) / 100.0f);
            break;
        }
        case kParamMultibandCompressorBandReleaseAuto: {
            VIPER_LOGI("MBComp: band[%d] release_auto=%d", val1, val2);
            multiband_compressor_.SetBandReleaseAuto(val1, val2 != 0);
            break;
        }
        case kParamMultibandCompressorBandKneeMulti: {
            VIPER_LOGI("MBComp: band[%d] knee_multi=%d", val1, val2);
            multiband_compressor_.SetBandKneeMulti(
                val1, static_cast<float>(val2) / 100.0f
            );
            break;
        }
        case kParamMultibandCompressorBandMaxAttack: {
            VIPER_LOGI("MBComp: band[%d] max_attack=%d", val1, val2);
            multiband_compressor_.SetBandMaxAttack(
                val1, static_cast<float>(val2) / 100.0f
            );
            break;
        }
        case kParamMultibandCompressorBandMaxRelease: {
            VIPER_LOGI("MBComp: band[%d] max_release=%d", val1, val2);
            multiband_compressor_.SetBandMaxRelease(
                val1, static_cast<float>(val2) / 100.0f
            );
            break;
        }
        case kParamMultibandCompressorBandCrest: {
            VIPER_LOGI("MBComp: band[%d] crest=%d", val1, val2);
            multiband_compressor_.SetBandCrest(val1, static_cast<float>(val2) / 100.0f);
            break;
        }
        case kParamMultibandCompressorBandAdapt: {
            VIPER_LOGI("MBComp: band[%d] adapt=%d", val1, val2);
            multiband_compressor_.SetBandAdapt(val1, static_cast<float>(val2) / 100.0f);
            break;
        }
        case kParamMultibandCompressorBandNoClip: {
            VIPER_LOGI("MBComp: band[%d] no_clip=%d", val1, val2);
            multiband_compressor_.SetBandNoClip(val1, val2 != 0);
            break;
        }
        case kParamMultibandCompressorBandEnable: {
            VIPER_LOGI("MBComp: band[%d] enable=%d", val1, val2);
            multiband_compressor_.SetBandEnable(val1, val2 != 0);
            break;
        }

        // Dynamic EQ
        case kParamDynamicEqEnable: {
            VIPER_LOGI("DynEQ: %s", val1 ? "ON" : "OFF");
            dynamic_eq_.SetEnable(val1 != 0);
            break;
        }
        case kParamDynamicEqBandCount: {
            VIPER_LOGI("DynEQ: band_count=%d", val1);
            dynamic_eq_.SetBandCount(val1);
            break;
        }
        case kParamDynamicEqBandFrequency: {
            VIPER_LOGI("DynEQ: band[%d] freq=%d", val1, val2);
            dynamic_eq_.SetBandFrequency(val1, static_cast<float>(val2));
            break;
        }
        case kParamDynamicEqBandQ: {
            VIPER_LOGI("DynEQ: band[%d] Q=%d", val1, val2);
            dynamic_eq_.SetBandQ(val1, static_cast<float>(val2) / 100.0f);
            break;
        }
        case kParamDynamicEqBandGain: {
            VIPER_LOGI("DynEQ: band[%d] gain=%d", val1, val2);
            dynamic_eq_.SetBandGain(val1, static_cast<float>(val2) / 10.0f);
            break;
        }
        case kParamDynamicEqBandThreshold: {
            VIPER_LOGI("DynEQ: band[%d] threshold=%d", val1, val2);
            dynamic_eq_.SetBandThreshold(val1, static_cast<float>(val2) / 10.0f);
            break;
        }
        case kParamDynamicEqBandAttack: {
            VIPER_LOGI("DynEQ: band[%d] attack=%d", val1, val2);
            dynamic_eq_.SetBandAttack(val1, static_cast<float>(val2));
            break;
        }
        case kParamDynamicEqBandRelease: {
            VIPER_LOGI("DynEQ: band[%d] release=%d", val1, val2);
            dynamic_eq_.SetBandRelease(val1, static_cast<float>(val2));
            break;
        }
        case kParamDynamicEqBandFilterType: {
            VIPER_LOGI("DynEQ: band[%d] filter_type=%d", val1, val2);
            dynamic_eq_.SetBandFilterType(val1, val2);
            break;
        }

        default: {
            VIPER_LOGI("Unknown param: 0x%X val1=%d val2=%d", param, val1, val2);
            break;
        }
    }
}

void ViPER::ResetAllEffects() {
    adaptive_buffer_.FlushBuffer();

    wave_buffer_.Reset();

    convolver_.SetSamplingRate(sampling_rate_);
    convolver_.Reset();

    vhe_.SetSamplingRate(sampling_rate_);
    vhe_.Reset();

    viper_ddc_.SetSamplingRate(sampling_rate_);
    viper_ddc_.Reset();

    spectrum_extend_.SetSamplingRate(sampling_rate_);
    spectrum_extend_.Reset();

    iir_filter_.SetSamplingRate(sampling_rate_);
    iir_filter_.Reset();

    dynamic_eq_.SetSamplingRate(sampling_rate_);
    dynamic_eq_.Reset();

    colorful_music_.SetSamplingRate(sampling_rate_);
    colorful_music_.Reset();

    stereo_imager_.SetSamplingRate(sampling_rate_);
    stereo_imager_.Reset();

    reverberation_.Reset();

    playback_gain_.SetSamplingRate(sampling_rate_);
    playback_gain_.Reset();

    lufs_targeting_.SetSamplingRate(sampling_rate_);
    lufs_targeting_.Reset();

    fet_compressor_.SetSamplingRate(sampling_rate_);
    fet_compressor_.Reset();

    multiband_compressor_.SetSamplingRate(sampling_rate_);
    multiband_compressor_.Reset();

    dynamic_system_.SetSamplingRate(sampling_rate_);
    dynamic_system_.Reset();

    viper_bass_.SetSamplingRate(sampling_rate_);
    viper_bass_.Reset();

    viper_bass_mono_.SetSamplingRate(sampling_rate_);
    viper_bass_mono_.Reset();

    psychoacoustic_bass_.SetSamplingRate(sampling_rate_);
    psychoacoustic_bass_.Reset();

    viper_clarity_.SetSamplingRate(sampling_rate_);
    viper_clarity_.Reset();

    diff_surround_.SetSamplingRate(sampling_rate_);
    diff_surround_.Reset();

    cure_.SetSamplingRate(sampling_rate_);
    cure_.Reset();

    tube_simulator_.Reset();

    analog_x_.SetSamplingRate(sampling_rate_);
    analog_x_.Reset();

    speaker_correction_.SetSamplingRate(sampling_rate_);
    speaker_correction_.Reset();

    for (auto &software_limiter : software_limiters_) {
        software_limiter.Reset();
    }
}
