#include "ViPER.h"
#include "../include/ViPERParams.h"
#include "../include/log.h"
#include "constants.h"
#include "utils/Crc32.h"

using namespace viper::params;

template <typename T>
bool BitEqual(const T &a, const T &b) {
    static_assert(
        std::is_trivially_copyable_v<T>,
        "ViPERParams sub-structs must be trivially copyable"
    );
    return std::memcmp(&a, &b, sizeof(T)) == 0;
}

constexpr uint32_t kKernelChunkFloats = 2046;

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
    tube_simulator_.SetSamplingRate(sampling_rate_);
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
    if (pending_effects_reset_.exchange(false, std::memory_order_acquire)) {
        pending_buffers_reset_.store(false, std::memory_order_relaxed);
        ResetAllEffects();
    } else if (pending_buffers_reset_.exchange(false, std::memory_order_acquire)) {
        ResetBuffers();
    }
    process_frame_count_ += size;

    float *tmp_buf;
    uint32_t tmp_buf_size;

    if (convolver_.GetEnable() || vhe_.GetEnable()) {
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
        tube_simulator_.Process(tmp_buf, tmp_buf_size);
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
    const uint32_t arr_size,
    signed char *arr
) {
    switch (param) {

        // System
        case kParamResetAllEffects: {
            VIPER_LOGD("ResetAllEffects");
            ResetAllEffects();
            break;
        }

        // Master Limiter
        case kParamMasterLimiterThreshold: {
            VIPER_LOGD("Master Limiter: threshold=%d", val1);
            software_limiters_[0].SetGate(static_cast<float>(val1) / 100.0f);
            software_limiters_[1].SetGate(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamMasterLimiterOutputVolume: {
            VIPER_LOGD("Master Limiter: output_vol=%d", val1);
            frame_scale_ = static_cast<float>(val1) / 100.0f;
            break;
        }
        case kParamMasterLimiterChannelPan: {
            VIPER_LOGD("Master Limiter: pan=%d", val1);
            const float tmp = static_cast<float>(val1) / 100.0f;
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
            VIPER_LOGD("PlaybackGain: %s", val1 ? "ON" : "OFF");
            playback_gain_.SetEnable(val1 != 0);
            break;
        }
        case kParamPlaybackGainControlStrength: {
            VIPER_LOGD("PlaybackGain: strength=%d", val1);
            playback_gain_.SetRatio(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamPlaybackGainControlMaxGain: {
            VIPER_LOGD("PlaybackGain: max_gain=%d", val1);
            playback_gain_.SetMaxGainFactor(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamPlaybackGainControlOutputThreshold: {
            VIPER_LOGD("PlaybackGain: output_threshold=%d", val1);
            playback_gain_.SetVolume(static_cast<float>(val1) / 100.0f);
            break;
        }

        // LUFS Targeting
        case kParamLufsEnable: {
            VIPER_LOGD("LUFS: %s", val1 ? "ON" : "OFF");
            lufs_targeting_.SetEnable(val1 != 0);
            break;
        }
        case kParamLufsTarget: {
            VIPER_LOGD("LUFS: target=%d", val1);
            lufs_targeting_.SetTargetLUFS(static_cast<float>(val1) / -10.0f);
            break;
        }
        case kParamLufsMaxGain: {
            VIPER_LOGD("LUFS: max_gain=%d", val1);
            lufs_targeting_.SetMaxGain(static_cast<float>(val1) / 10.0f);
            break;
        }
        case kParamLufsSpeed: {
            VIPER_LOGD("LUFS: speed=%d", val1);
            lufs_targeting_.SetSpeed(val1);
            break;
        }

        // FET Compressor
        case kParamFetCompressorEnable: {
            VIPER_LOGD("FET: %s", val1 ? "ON" : "OFF");
            fet_compressor_.SetEnable(val1 != 0);
            break;
        }
        case kParamFetCompressorThreshold: {
            VIPER_LOGD("FET: threshold=%d", val1);
            fet_compressor_.SetThreshold(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFetCompressorRatio: {
            VIPER_LOGD("FET: ratio=%d", val1);
            fet_compressor_.SetRatio(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFetCompressorKnee: {
            VIPER_LOGD("FET: knee=%d", val1);
            fet_compressor_.SetKnee(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFetCompressorKneeAuto: {
            VIPER_LOGD("FET: knee_auto=%d", val1);
            fet_compressor_.SetKneeAuto(val1 != 0);
            break;
        }
        case kParamFetCompressorGain: {
            VIPER_LOGD("FET: gain=%d", val1);
            fet_compressor_.SetGain(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFetCompressorGainAuto: {
            VIPER_LOGD("FET: gain_auto=%d", val1);
            fet_compressor_.SetGainAuto(val1 != 0);
            break;
        }
        case kParamFetCompressorAttack: {
            VIPER_LOGD("FET: attack=%d", val1);
            fet_compressor_.SetAttack(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFetCompressorAttackAuto: {
            VIPER_LOGD("FET: attack_auto=%d", val1);
            fet_compressor_.SetAttackAuto(val1 != 0);
            break;
        }
        case kParamFetCompressorRelease: {
            VIPER_LOGD("FET: release=%d", val1);
            fet_compressor_.SetRelease(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFetCompressorReleaseAuto: {
            VIPER_LOGD("FET: release_auto=%d", val1);
            fet_compressor_.SetReleaseAuto(val1 != 0);
            break;
        }
        case kParamFetCompressorKneeMulti: {
            VIPER_LOGD("FET: knee_multi=%d", val1);
            fet_compressor_.SetKneeMulti(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFetCompressorMaxAttack: {
            VIPER_LOGD("FET: max_attack=%d", val1);
            fet_compressor_.SetMaxAttack(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFetCompressorMaxRelease: {
            VIPER_LOGD("FET: max_release=%d", val1);
            fet_compressor_.SetMaxRelease(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFetCompressorCrest: {
            VIPER_LOGD("FET: crest=%d", val1);
            fet_compressor_.SetCrest(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFetCompressorAdapt: {
            VIPER_LOGD("FET: adapt=%d", val1);
            fet_compressor_.SetAdapt(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFetCompressorNoClip: {
            VIPER_LOGD("FET: no_clip=%d", val1);
            fet_compressor_.SetNoClip(val1 != 0);
            break;
        }

        // Bass
        case kParamBassEnable: {
            VIPER_LOGD("Bass: %s", val1 ? "ON" : "OFF");
            viper_bass_.SetEnable(val1 != 0);
            break;
        }
        case kParamBassMode: {
            VIPER_LOGD("Bass: mode=%d", val1);
            viper_bass_.SetProcessMode(static_cast<ViPERBass::ProcessMode>(val1));
            break;
        }
        case kParamBassFrequency: {
            VIPER_LOGD("Bass: freq=%d", val1);
            viper_bass_.SetFrequency(static_cast<uint32_t>(val1));
            break;
        }
        case kParamBassGain: {
            VIPER_LOGD("Bass: gain=%d", val1);
            viper_bass_.SetBassFactor(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamBassAntiPop: {
            VIPER_LOGD("Bass: anti_pop=%s", val1 ? "ON" : "OFF");
            viper_bass_.SetAntiPop(val1 != 0);
            break;
        }

        // Bass Mono
        case kParamBassMonoEnable: {
            VIPER_LOGD("BassMono: %s", val1 ? "ON" : "OFF");
            viper_bass_mono_.SetEnable(val1 != 0);
            break;
        }
        case kParamBassMonoMode: {
            VIPER_LOGD("BassMono: mode=%d", val1);
            viper_bass_mono_.SetProcessMode(
                static_cast<ViPERBassMono::ProcessMode>(val1)
            );
            break;
        }
        case kParamBassMonoFrequency: {
            VIPER_LOGD("BassMono: freq=%d", val1);
            viper_bass_mono_.SetFrequency(static_cast<uint32_t>(val1));
            break;
        }
        case kParamBassMonoGain: {
            VIPER_LOGD("BassMono: gain=%d", val1);
            viper_bass_mono_.SetBassFactor(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamBassMonoAntiPop: {
            VIPER_LOGD("BassMono: anti_pop=%s", val1 ? "ON" : "OFF");
            viper_bass_mono_.SetAntiPop(val1 != 0);
            break;
        }

        // Psychoacoustic Bass
        case kParamPsychoacousticBassEnable: {
            VIPER_LOGD("PsychoBass: %s", val1 ? "ON" : "OFF");
            psychoacoustic_bass_.SetEnable(val1 != 0);
            break;
        }
        case kParamPsychoacousticBassCutoff: {
            VIPER_LOGD("PsychoBass: cutoff=%d", val1);
            psychoacoustic_bass_.SetCutoff(static_cast<uint32_t>(val1));
            break;
        }
        case kParamPsychoacousticBassIntensity: {
            VIPER_LOGD("PsychoBass: intensity=%d", val1);
            psychoacoustic_bass_.SetIntensity(static_cast<uint32_t>(val1));
            break;
        }
        case kParamPsychoacousticBassHarmonicOrder: {
            VIPER_LOGD("PsychoBass: harmonic_order=%d", val1);
            psychoacoustic_bass_.SetHarmonicOrder(static_cast<uint32_t>(val1));
            break;
        }
        case kParamPsychoacousticBassOriginalLevel: {
            VIPER_LOGD("PsychoBass: original_level=%d", val1);
            psychoacoustic_bass_.SetOriginalBassLevel(static_cast<uint32_t>(val1));
            break;
        }

        // Spectrum Extension
        case kParamSpectrumExtensionEnable: {
            VIPER_LOGD("SpecExt: %s", val1 ? "ON" : "OFF");
            spectrum_extend_.SetEnable(val1 != 0);
            break;
        }
        case kParamSpectrumExtensionStrength: {
            VIPER_LOGD("SpecExt: strength=%d", val1);
            spectrum_extend_.SetReferenceFrequency(val1);
            break;
        }
        case kParamSpectrumExtensionExciter: {
            VIPER_LOGD("SpecExt: exciter=%d", val1);
            spectrum_extend_.SetExciter(static_cast<float>(val1) / 100.0f);
            break;
        }

        // Equalizer (IIR Filter)
        case kParamEqualizerEnable: {
            VIPER_LOGD("EQ: %s", val1 ? "ON" : "OFF");
            iir_filter_.SetEnable(val1 != 0);
            break;
        }
        case kParamEqualizerBandLevel: {
            VIPER_LOGD("EQ: band=%d level=%d", val1, val2);
            iir_filter_.SetBandLevel(
                static_cast<uint32_t>(val1), static_cast<float>(val2) / 100.0f
            );
            break;
        }
        case kParamEqualizerBandLevels: {
            VIPER_LOGD("EQ: bands_levels=%u", arr_size);
            iir_filter_.SetBandLevels(reinterpret_cast<float *>(arr), arr_size);
            break;
        }
        case kParamEqualizerBandCount: {
            VIPER_LOGD("EQ: band_count=%d", val1);
            iir_filter_.SetBandCount(static_cast<uint32_t>(val1));
            break;
        }

        // Convolver
        case kParamConvolverEnable: {
            VIPER_LOGD("Convolver: %s", val1 ? "ON" : "OFF");
            convolver_.SetEnable(val1 != 0);
            break;
        }
        case kParamConvolverSetKernel: {
            if (arr_size > 0 && arr != nullptr) {
                char path[256] = {};
                memcpy(path, arr, arr_size < 255 ? arr_size : 255);
                VIPER_LOGD("Convolver: SetKernel path=%s", path);
                convolver_.SetKernel(path);
            }
            break;
        }
        case kParamConvolverPrepareBuffer: {
            VIPER_LOGD(
                "Convolver: PrepareBuffer buf_size=%d ch=%d reset=%d", val1, val2, val3
            );
            convolver_.PrepareKernelBuffer(val1, val2, val3 != 0);
            break;
        }
        case kParamConvolverSetBuffer: {
            VIPER_LOGD("Convolver: SetBuffer size=%u", arr_size);
            convolver_.SetKernelBuffer(reinterpret_cast<float *>(arr), arr_size);
            break;
        }
        case kParamConvolverCommitBuffer: {
            VIPER_LOGD(
                "Convolver: CommitBuffer channels=%d frames=%d sr=%d", val1, val2, val3
            );
            convolver_.CommitKernelBuffer(val1, val2, val3);
            break;
        }
        case kParamConvolverCrossChannel: {
            VIPER_LOGD("Convolver: cross_ch=%d%%", val1);
            convolver_.SetCrossChannel(static_cast<float>(val1) / 100.0f);
            break;
        }

        // DDC
        case kParamDdcEnable: {
            VIPER_LOGD("DDC: %s", val1 ? "ON" : "OFF");
            viper_ddc_.SetEnable(val1 != 0);
            break;
        }
        case kParamDdcCoefficients: {
            VIPER_LOGD("DDC: SetCoeffs arr_size=%u", arr_size);
            viper_ddc_.SetCoeffs(
                arr_size,
                reinterpret_cast<float *>(arr),
                reinterpret_cast<float *>(arr + arr_size * sizeof(float))
            );
            break;
        }

        // Field Surround (Colorful Music)
        case kParamFieldSurroundEnable: {
            VIPER_LOGD("FieldSurr: %s", val1 ? "ON" : "OFF");
            colorful_music_.SetEnable(val1 != 0);
            break;
        }
        case kParamFieldSurroundWidening: {
            VIPER_LOGD("FieldSurr: widen=%d", val1);
            colorful_music_.SetWidenValue(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFieldSurroundMidImage: {
            VIPER_LOGD("FieldSurr: mid_image=%d", val1);
            colorful_music_.SetMidImageValue(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamFieldSurroundDepth: {
            VIPER_LOGD("FieldSurr: depth=%d", val1);
            colorful_music_.SetDepthValue(val1);
            break;
        }

        // Differential Surround
        case kParamDiffSurroundEnable: {
            VIPER_LOGD("DiffSurr: %s", val1 ? "ON" : "OFF");
            diff_surround_.SetEnable(val1 != 0);
            break;
        }
        case kParamDiffSurroundDelay: {
            VIPER_LOGD("DiffSurr: delay=%d", val1);
            diff_surround_.SetDelayTime(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamDiffSurroundReverse: {
            VIPER_LOGD("DiffSurr: reverse=%s", val1 ? "ON" : "OFF");
            diff_surround_.SetReverse(val1 != 0);
            break;
        }
        case kParamDiffSurroundWetDryMix: {
            VIPER_LOGD("DiffSurr: wet_dry_mix=%d", val1);
            diff_surround_.SetWetDryMix(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamDiffSurroundLpCutoff: {
            VIPER_LOGD("DiffSurr: lp_cutoff=%d", val1);
            diff_surround_.SetLPCutoff(static_cast<float>(val1));
            break;
        }

        // Stereo Imager
        case kParamStereoImagerEnable: {
            VIPER_LOGD("StereoImg: %s", val1 ? "ON" : "OFF");
            stereo_imager_.SetEnable(val1 != 0);
            break;
        }
        case kParamStereoImagerLowWidth: {
            VIPER_LOGD("StereoImg: low_width=%d", val1);
            stereo_imager_.SetLowWidth(static_cast<float>(val1));
            break;
        }
        case kParamStereoImagerMidWidth: {
            VIPER_LOGD("StereoImg: mid_width=%d", val1);
            stereo_imager_.SetMidWidth(static_cast<float>(val1));
            break;
        }
        case kParamStereoImagerHighWidth: {
            VIPER_LOGD("StereoImg: high_width=%d", val1);
            stereo_imager_.SetHighWidth(static_cast<float>(val1));
            break;
        }
        case kParamStereoImagerLowCrossover: {
            VIPER_LOGD("StereoImg: low_crossover=%d", val1);
            stereo_imager_.SetLowCrossover(static_cast<float>(val1));
            break;
        }
        case kParamStereoImagerHighCrossover: {
            VIPER_LOGD("StereoImg: high_crossover=%d", val1);
            stereo_imager_.SetHighCrossover(static_cast<float>(val1));
            break;
        }

        // Headphone Surround (VHE)
        case kParamHeadphoneSurroundEnable: {
            VIPER_LOGD("VHE: %s", val1 ? "ON" : "OFF");
            vhe_.SetEnable(val1 != 0);
            break;
        }
        case kParamHeadphoneSurroundQuality: {
            VIPER_LOGD("VHE: quality=%d", val1);
            vhe_.SetEffectLevel(val1);
            break;
        }

        // Reverb
        case kParamReverbEnable: {
            VIPER_LOGD("Reverb: %s", val1 ? "ON" : "OFF");
            reverberation_.SetEnable(val1 != 0);
            break;
        }
        case kParamReverbRoomSize: {
            VIPER_LOGD("Reverb: room_size=%d", val1);
            reverberation_.SetRoomSize(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamReverbWidth: {
            VIPER_LOGD("Reverb: width=%d", val1);
            reverberation_.SetWidth(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamReverbDamp: {
            VIPER_LOGD("Reverb: damp=%d", val1);
            reverberation_.SetDamp(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamReverbWet: {
            VIPER_LOGD("Reverb: wet=%d", val1);
            reverberation_.SetWet(static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamReverbDry: {
            VIPER_LOGD("Reverb: dry=%d", val1);
            reverberation_.SetDry(static_cast<float>(val1) / 100.0f);
            break;
        }

        // Dynamic System
        case kParamDynamicSystemEnable: {
            VIPER_LOGD("DynSys: %s", val1 ? "ON" : "OFF");
            dynamic_system_.SetEnable(val1 != 0);
            break;
        }
        case kParamDynamicSystemXLow: {
            VIPER_LOGD("DynSys: x_low=%d", val1);
            dynamic_system_.SetXCoeffs(val1, -1);
            break;
        }
        case kParamDynamicSystemXHigh: {
            VIPER_LOGD("DynSys: x_high=%d", val1);
            dynamic_system_.SetXCoeffs(-1, val1);
            break;
        }
        case kParamDynamicSystemYLow: {
            VIPER_LOGD("DynSys: y_low=%d", val1);
            dynamic_system_.SetYCoeffs(val1, -1);
            break;
        }
        case kParamDynamicSystemYHigh: {
            VIPER_LOGD("DynSys: y_high=%d", val1);
            dynamic_system_.SetYCoeffs(-1, val1);
            break;
        }
        case kParamDynamicSystemSideGainLow: {
            VIPER_LOGD("DynSys: side_gain_low=%d", val1);
            dynamic_system_.SetSideGain(static_cast<float>(val1) / 100.0f, -1.0f);
            break;
        }
        case kParamDynamicSystemSideGainHigh: {
            VIPER_LOGD("DynSys: side_gain_high=%d", val1);
            dynamic_system_.SetSideGain(-1.0f, static_cast<float>(val1) / 100.0f);
            break;
        }
        case kParamDynamicSystemStrength: {
            VIPER_LOGD("DynSys: strength=%d", val1);
            dynamic_system_.SetBassGain(static_cast<float>(val1) / 100.0f);
            break;
        }

        // Clarity
        case kParamClarityEnable: {
            VIPER_LOGD("Clarity: %s", val1 ? "ON" : "OFF");
            viper_clarity_.SetEnable(val1 != 0);
            break;
        }
        case kParamClarityMode: {
            VIPER_LOGD("Clarity: mode=%d", val1);
            viper_clarity_.SetProcessMode(static_cast<ViPERClarity::ClarityMode>(val1));
            break;
        }
        case kParamClarityGain: {
            VIPER_LOGD("Clarity: gain=%d", val1);
            viper_clarity_.SetClarityGain(static_cast<float>(val1) / 100.0f);
            break;
        }

        // Cure (Crossfeed)
        case kParamCureEnable: {
            VIPER_LOGD("Cure: %s", val1 ? "ON" : "OFF");
            cure_.SetEnable(val1 != 0);
            break;
        }
        case kParamCureCrossfeedPreset: {
            VIPER_LOGD("Cure: crossfeed_preset=%d", val1);
            cure_.SetPreset(val1);
            break;
        }

        // Tube Simulator
        case kParamTubeSimulatorEnable: {
            VIPER_LOGD("TubeSim: %s", val1 ? "ON" : "OFF");
            tube_simulator_.SetEnable(val1 != 0);
            break;
        }

        // AnalogX
        case kParamAnalogXEnable: {
            VIPER_LOGD("AnalogX: %s", val1 ? "ON" : "OFF");
            analog_x_.SetEnable(val1 != 0);
            break;
        }
        case kParamAnalogXMode: {
            VIPER_LOGD("AnalogX: mode=%d", val1);
            analog_x_.SetProcessingModel(val1);
            break;
        }

        // Speaker Correction
        case kParamSpeakerCorrectionEnable: {
            VIPER_LOGD("SpkCorr: %s", val1 ? "ON" : "OFF");
            speaker_correction_.SetEnable(val1 != 0);
            break;
        }

        // Multiband Compressor
        case kParamMultibandCompressorEnable: {
            VIPER_LOGD("MBComp: %s", val1 ? "ON" : "OFF");
            multiband_compressor_.SetEnable(val1 != 0);
            break;
        }
        case kParamMultibandCompressorBandCount: {
            VIPER_LOGD("MBComp: band_count=%d", val1);
            multiband_compressor_.SetBandCount(val1);
            break;
        }
        case kParamMultibandCompressorCrossoverFrequency: {
            VIPER_LOGD("MBComp: crossover[%d]=%d", val1, val2);
            multiband_compressor_.SetCrossoverFrequency(val1, static_cast<float>(val2));
            break;
        }
        case kParamMultibandCompressorBandThreshold: {
            VIPER_LOGD("MBComp: band[%d] threshold=%d", val1, val2);
            multiband_compressor_.SetBandThreshold(
                val1, static_cast<float>(val2) / 100.0f
            );
            break;
        }
        case kParamMultibandCompressorBandRatio: {
            VIPER_LOGD("MBComp: band[%d] ratio=%d", val1, val2);
            multiband_compressor_.SetBandRatio(val1, static_cast<float>(val2) / 100.0f);
            break;
        }
        case kParamMultibandCompressorBandKnee: {
            VIPER_LOGD("MBComp: band[%d] knee=%d", val1, val2);
            multiband_compressor_.SetBandKnee(val1, static_cast<float>(val2) / 100.0f);
            break;
        }
        case kParamMultibandCompressorBandKneeAuto: {
            VIPER_LOGD("MBComp: band[%d] knee_auto=%d", val1, val2);
            multiband_compressor_.SetBandKneeAuto(val1, val2 != 0);
            break;
        }
        case kParamMultibandCompressorBandGain: {
            VIPER_LOGD("MBComp: band[%d] gain=%d", val1, val2);
            multiband_compressor_.SetBandGain(val1, static_cast<float>(val2) / 100.0f);
            break;
        }
        case kParamMultibandCompressorBandGainAuto: {
            VIPER_LOGD("MBComp: band[%d] gain_auto=%d", val1, val2);
            multiband_compressor_.SetBandGainAuto(val1, val2 != 0);
            break;
        }
        case kParamMultibandCompressorBandAttack: {
            VIPER_LOGD("MBComp: band[%d] attack=%d", val1, val2);
            multiband_compressor_.SetBandAttack(val1, static_cast<float>(val2) / 100.0f);
            break;
        }
        case kParamMultibandCompressorBandAttackAuto: {
            VIPER_LOGD("MBComp: band[%d] attack_auto=%d", val1, val2);
            multiband_compressor_.SetBandAttackAuto(val1, val2 != 0);
            break;
        }
        case kParamMultibandCompressorBandRelease: {
            VIPER_LOGD("MBComp: band[%d] release=%d", val1, val2);
            multiband_compressor_.SetBandRelease(val1, static_cast<float>(val2) / 100.0f);
            break;
        }
        case kParamMultibandCompressorBandReleaseAuto: {
            VIPER_LOGD("MBComp: band[%d] release_auto=%d", val1, val2);
            multiband_compressor_.SetBandReleaseAuto(val1, val2 != 0);
            break;
        }
        case kParamMultibandCompressorBandKneeMulti: {
            VIPER_LOGD("MBComp: band[%d] knee_multi=%d", val1, val2);
            multiband_compressor_.SetBandKneeMulti(
                val1, static_cast<float>(val2) / 100.0f
            );
            break;
        }
        case kParamMultibandCompressorBandMaxAttack: {
            VIPER_LOGD("MBComp: band[%d] max_attack=%d", val1, val2);
            multiband_compressor_.SetBandMaxAttack(
                val1, static_cast<float>(val2) / 100.0f
            );
            break;
        }
        case kParamMultibandCompressorBandMaxRelease: {
            VIPER_LOGD("MBComp: band[%d] max_release=%d", val1, val2);
            multiband_compressor_.SetBandMaxRelease(
                val1, static_cast<float>(val2) / 100.0f
            );
            break;
        }
        case kParamMultibandCompressorBandCrest: {
            VIPER_LOGD("MBComp: band[%d] crest=%d", val1, val2);
            multiband_compressor_.SetBandCrest(val1, static_cast<float>(val2) / 100.0f);
            break;
        }
        case kParamMultibandCompressorBandAdapt: {
            VIPER_LOGD("MBComp: band[%d] adapt=%d", val1, val2);
            multiband_compressor_.SetBandAdapt(val1, static_cast<float>(val2) / 100.0f);
            break;
        }
        case kParamMultibandCompressorBandNoClip: {
            VIPER_LOGD("MBComp: band[%d] no_clip=%d", val1, val2);
            multiband_compressor_.SetBandNoClip(val1, val2 != 0);
            break;
        }
        case kParamMultibandCompressorBandEnable: {
            VIPER_LOGD("MBComp: band[%d] enable=%d", val1, val2);
            multiband_compressor_.SetBandEnable(val1, val2 != 0);
            break;
        }

        // Dynamic EQ
        case kParamDynamicEqEnable: {
            VIPER_LOGD("DynEQ: %s", val1 ? "ON" : "OFF");
            dynamic_eq_.SetEnable(val1 != 0);
            break;
        }
        case kParamDynamicEqBandCount: {
            VIPER_LOGD("DynEQ: band_count=%d", val1);
            dynamic_eq_.SetBandCount(val1);
            break;
        }
        case kParamDynamicEqBandFrequency: {
            VIPER_LOGD("DynEQ: band[%d] freq=%d", val1, val2);
            dynamic_eq_.SetBandFrequency(val1, static_cast<float>(val2));
            break;
        }
        case kParamDynamicEqBandQ: {
            VIPER_LOGD("DynEQ: band[%d] Q=%d", val1, val2);
            dynamic_eq_.SetBandQ(val1, static_cast<float>(val2) / 100.0f);
            break;
        }
        case kParamDynamicEqBandGain: {
            VIPER_LOGD("DynEQ: band[%d] gain=%d", val1, val2);
            dynamic_eq_.SetBandGain(val1, static_cast<float>(val2) / 10.0f);
            break;
        }
        case kParamDynamicEqBandThreshold: {
            VIPER_LOGD("DynEQ: band[%d] threshold=%d", val1, val2);
            dynamic_eq_.SetBandThreshold(val1, static_cast<float>(val2) / 10.0f);
            break;
        }
        case kParamDynamicEqBandAttack: {
            VIPER_LOGD("DynEQ: band[%d] attack=%d", val1, val2);
            dynamic_eq_.SetBandAttack(val1, static_cast<float>(val2));
            break;
        }
        case kParamDynamicEqBandRelease: {
            VIPER_LOGD("DynEQ: band[%d] release=%d", val1, val2);
            dynamic_eq_.SetBandRelease(val1, static_cast<float>(val2));
            break;
        }
        case kParamDynamicEqBandFilterType: {
            VIPER_LOGD("DynEQ: band[%d] filter_type=%d", val1, val2);
            dynamic_eq_.SetBandFilterType(val1, val2);
            break;
        }

        default: {
            VIPER_LOGD("Unknown param: 0x%X val1=%d val2=%d", param, val1, val2);
            break;
        }
    }
}

void ViPER::RequestEffectsReset() {
    pending_effects_reset_.store(true, std::memory_order_release);
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

    tube_simulator_.SetSamplingRate(sampling_rate_);
    tube_simulator_.Reset();

    analog_x_.SetSamplingRate(sampling_rate_);
    analog_x_.Reset();

    speaker_correction_.SetSamplingRate(sampling_rate_);
    speaker_correction_.Reset();

    for (auto &software_limiter : software_limiters_) {
        software_limiter.Reset();
    }
}

void ViPER::RequestBuffersReset() {
    pending_buffers_reset_.store(true, std::memory_order_release);
}

void ViPER::ResetBuffers() {
    adaptive_buffer_.FlushBuffer();
    wave_buffer_.Reset();
    reverberation_.Reset();
}

void ViPER::ApplyParams(const viper::ViPERParams &params) {
    ApplyMasterLimiter(params.master_limiter);
    ApplyPlaybackGainControl(params.playback_gain_control);
    ApplyLufs(params.lufs);
    ApplyFetCompressor(params.fet_compressor);
    ApplyBass(params.bass);
    ApplyBassMono(params.bass_mono);
    ApplyPsychoacousticBass(params.psychoacoustic_bass);
    ApplySpectrumExtension(params.spectrum_extension);
    ApplyEqualizer(params.equalizer);
    ApplyConvolver(params.convolver);
    ApplyDdc(params.ddc);
    ApplyFieldSurround(params.field_surround);
    ApplyDiffSurround(params.diff_surround);
    ApplyStereoImager(params.stereo_imager);
    ApplyHeadphoneSurround(params.headphone_surround);
    ApplyReverb(params.reverb);
    ApplyDynamicSystem(params.dynamic_system);
    ApplyClarity(params.clarity);
    ApplyCure(params.cure);
    ApplyTubeSimulator(params.tube_simulator);
    ApplyAnalogX(params.analog_x);
    ApplySpeakerCorrection(params.speaker_correction);
    ApplyMultibandCompressor(params.multiband_compressor);
    ApplyDynamicEq(params.dynamic_eq);
}

void ViPER::ApplyMasterLimiter(const viper::MasterLimiterParams &p) {
    software_limiters_[0].SetGate(p.threshold);
    software_limiters_[1].SetGate(p.threshold);
    frame_scale_ = p.output_volume;
    if (p.channel_pan < 0.0f) {
        left_pan_ = 1.0f;
        right_pan_ = 1.0f + p.channel_pan;
    } else {
        left_pan_ = 1.0f - p.channel_pan;
        right_pan_ = 1.0f;
    }
    last_applied_.master_limiter = p;
}

void ViPER::ApplyPlaybackGainControl(const viper::PlaybackGainControlParams &p) {
    playback_gain_.SetEnable(p.enable);
    playback_gain_.SetRatio(p.strength);
    playback_gain_.SetMaxGainFactor(p.max_gain);
    playback_gain_.SetVolume(p.output_threshold);
    last_applied_.playback_gain_control = p;
}

void ViPER::ApplyLufs(const viper::LufsParams &p) {
    lufs_targeting_.SetEnable(p.enable);
    lufs_targeting_.SetTargetLUFS(p.target);
    lufs_targeting_.SetMaxGain(p.max_gain);
    lufs_targeting_.SetSpeed(p.speed);
    last_applied_.lufs = p;
}

void ViPER::ApplyFetCompressor(const viper::FetCompressorParams &p) {
    fet_compressor_.SetEnable(p.enable);
    fet_compressor_.SetThreshold(p.threshold);
    fet_compressor_.SetRatio(p.ratio);
    fet_compressor_.SetKnee(p.knee);
    fet_compressor_.SetKneeAuto(p.knee_auto);
    fet_compressor_.SetGain(p.gain);
    fet_compressor_.SetGainAuto(p.gain_auto);
    fet_compressor_.SetAttack(p.attack);
    fet_compressor_.SetAttackAuto(p.attack_auto);
    fet_compressor_.SetRelease(p.release);
    fet_compressor_.SetReleaseAuto(p.release_auto);
    fet_compressor_.SetKneeMulti(p.knee_multi);
    fet_compressor_.SetMaxAttack(p.max_attack);
    fet_compressor_.SetMaxRelease(p.max_release);
    fet_compressor_.SetCrest(p.crest);
    fet_compressor_.SetAdapt(p.adapt);
    fet_compressor_.SetNoClip(p.no_clip);
    last_applied_.fet_compressor = p;
}

void ViPER::ApplyBass(const viper::BassParams &p) {
    viper_bass_.SetEnable(p.enable);
    viper_bass_.SetProcessMode(static_cast<ViPERBass::ProcessMode>(p.mode));
    viper_bass_.SetFrequency(p.frequency);
    viper_bass_.SetBassFactor(p.gain);
    viper_bass_.SetAntiPop(p.anti_pop);
    last_applied_.bass = p;
}

void ViPER::ApplyBassMono(const viper::BassMonoParams &p) {
    viper_bass_mono_.SetEnable(p.enable);
    viper_bass_mono_.SetProcessMode(static_cast<ViPERBassMono::ProcessMode>(p.mode));
    viper_bass_mono_.SetFrequency(p.frequency);
    viper_bass_mono_.SetBassFactor(p.gain);
    viper_bass_mono_.SetAntiPop(p.anti_pop);
    last_applied_.bass_mono = p;
}

void ViPER::ApplyPsychoacousticBass(const viper::PsychoacousticBassParams &p) {
    psychoacoustic_bass_.SetEnable(p.enable);
    psychoacoustic_bass_.SetCutoff(p.cutoff);
    psychoacoustic_bass_.SetIntensity(p.intensity);
    psychoacoustic_bass_.SetHarmonicOrder(p.harmonic_order);
    psychoacoustic_bass_.SetOriginalBassLevel(p.original_level);
    last_applied_.psychoacoustic_bass = p;
}

void ViPER::ApplySpectrumExtension(const viper::SpectrumExtensionParams &p) {
    spectrum_extend_.SetEnable(p.enable);
    spectrum_extend_.SetReferenceFrequency(p.strength);
    spectrum_extend_.SetExciter(p.exciter);
    last_applied_.spectrum_extension = p;
}

void ViPER::ApplyEqualizer(const viper::EqualizerParams &p) {
    iir_filter_.SetEnable(p.enable);
    iir_filter_.SetBandCount(p.band_count);
    for (uint32_t i = 0; i < p.band_count && i < p.band_levels.size(); i++) {
        iir_filter_.SetBandLevel(i, p.band_levels[i]);
    }
    last_applied_.equalizer = p;
}

void ViPER::ApplyConvolver(const viper::ConvolverParams &p) {
    convolver_.SetEnable(p.enable);
    convolver_.SetCrossChannel(p.cross_channel);
    last_applied_.convolver = p;
}

void ViPER::ApplyDdc(const viper::DdcParams &p) {
    viper_ddc_.SetEnable(p.enable);
    last_applied_.ddc = p;
}

void ViPER::ApplyFieldSurround(const viper::FieldSurroundParams &p) {
    colorful_music_.SetEnable(p.enable);
    colorful_music_.SetWidenValue(p.widening);
    colorful_music_.SetMidImageValue(p.mid_image);
    colorful_music_.SetDepthValue(p.depth);
    last_applied_.field_surround = p;
}

void ViPER::ApplyDiffSurround(const viper::DiffSurroundParams &p) {
    diff_surround_.SetEnable(p.enable);
    diff_surround_.SetDelayTime(p.delay);
    diff_surround_.SetReverse(p.reverse);
    diff_surround_.SetWetDryMix(p.wet_dry_mix);
    diff_surround_.SetLPCutoff(p.lp_cutoff);
    last_applied_.diff_surround = p;
}

void ViPER::ApplyStereoImager(const viper::StereoImagerParams &p) {
    stereo_imager_.SetEnable(p.enable);
    stereo_imager_.SetLowWidth(p.low_width);
    stereo_imager_.SetMidWidth(p.mid_width);
    stereo_imager_.SetHighWidth(p.high_width);
    stereo_imager_.SetLowCrossover(p.low_crossover);
    stereo_imager_.SetHighCrossover(p.high_crossover);
    last_applied_.stereo_imager = p;
}

void ViPER::ApplyHeadphoneSurround(const viper::HeadphoneSurroundParams &p) {
    vhe_.SetEnable(p.enable);
    vhe_.SetEffectLevel(p.quality);
    last_applied_.headphone_surround = p;
}

void ViPER::ApplyReverb(const viper::ReverbParams &p) {
    reverberation_.SetEnable(p.enable);
    reverberation_.SetRoomSize(p.room_size);
    reverberation_.SetWidth(p.width);
    reverberation_.SetDamp(p.damp);
    reverberation_.SetWet(p.wet);
    reverberation_.SetDry(p.dry);
    last_applied_.reverb = p;
}

void ViPER::ApplyDynamicSystem(const viper::DynamicSystemParams &p) {
    dynamic_system_.SetEnable(p.enable);
    dynamic_system_.SetXCoeffs(p.x_coeff_low, p.x_coeff_high);
    dynamic_system_.SetYCoeffs(p.y_coeff_low, p.y_coeff_high);
    dynamic_system_.SetSideGain(p.side_gain_low, p.side_gain_high);
    dynamic_system_.SetBassGain(p.strength);
    last_applied_.dynamic_system = p;
}

void ViPER::ApplyClarity(const viper::ClarityParams &p) {
    viper_clarity_.SetEnable(p.enable);
    viper_clarity_.SetProcessMode(static_cast<ViPERClarity::ClarityMode>(p.mode));
    viper_clarity_.SetClarityGain(p.gain);
    last_applied_.clarity = p;
}

void ViPER::ApplyCure(const viper::CureParams &p) {
    cure_.SetEnable(p.enable);
    cure_.SetPreset(p.crossfeed_preset);
    last_applied_.cure = p;
}

void ViPER::ApplyTubeSimulator(const viper::TubeSimulatorParams &p) {
    tube_simulator_.SetEnable(p.enable);
    last_applied_.tube_simulator = p;
}

void ViPER::ApplyAnalogX(const viper::AnalogXParams &p) {
    analog_x_.SetEnable(p.enable);
    analog_x_.SetProcessingModel(p.mode);
    last_applied_.analog_x = p;
}

void ViPER::ApplySpeakerCorrection(const viper::SpeakerCorrectionParams &p) {
    speaker_correction_.SetEnable(p.enable);
    last_applied_.speaker_correction = p;
}

void ViPER::ApplyMultibandCompressor(const viper::MultibandCompressorParams &p) {
    multiband_compressor_.SetEnable(p.enable);
    multiband_compressor_.SetBandCount(p.band_count);
    for (uint32_t i = 0; i < p.band_count && i < p.crossover_frequencies.size(); i++) {
        multiband_compressor_.SetCrossoverFrequency(i, p.crossover_frequencies[i]);
    }
    for (uint32_t i = 0; i < p.band_count && i < p.bands.size(); i++) {
        const auto &b = p.bands[i];
        multiband_compressor_.SetBandEnable(i, b.enable);
        multiband_compressor_.SetBandThreshold(i, b.threshold);
        multiband_compressor_.SetBandRatio(i, b.ratio);
        multiband_compressor_.SetBandKnee(i, b.knee);
        multiband_compressor_.SetBandKneeAuto(i, b.knee_auto);
        multiband_compressor_.SetBandGain(i, b.gain);
        multiband_compressor_.SetBandGainAuto(i, b.gain_auto);
        multiband_compressor_.SetBandAttack(i, b.attack);
        multiband_compressor_.SetBandAttackAuto(i, b.attack_auto);
        multiband_compressor_.SetBandRelease(i, b.release);
        multiband_compressor_.SetBandReleaseAuto(i, b.release_auto);
        multiband_compressor_.SetBandKneeMulti(i, b.knee_multi);
        multiband_compressor_.SetBandMaxAttack(i, b.max_attack);
        multiband_compressor_.SetBandMaxRelease(i, b.max_release);
        multiband_compressor_.SetBandCrest(i, b.crest);
        multiband_compressor_.SetBandAdapt(i, b.adapt);
        multiband_compressor_.SetBandNoClip(i, b.no_clip);
    }
    last_applied_.multiband_compressor = p;
}

void ViPER::ApplyDynamicEq(const viper::DynamicEqParams &p) {
    dynamic_eq_.SetEnable(p.enable);
    dynamic_eq_.SetBandCount(p.band_count);
    for (uint32_t i = 0; i < p.band_count && i < p.bands.size(); i++) {
        const auto &b = p.bands[i];
        dynamic_eq_.SetBandFrequency(i, b.frequency);
        dynamic_eq_.SetBandQ(i, b.q);
        dynamic_eq_.SetBandGain(i, b.gain);
        dynamic_eq_.SetBandThreshold(i, b.threshold);
        dynamic_eq_.SetBandAttack(i, b.attack);
        dynamic_eq_.SetBandRelease(i, b.release);
        dynamic_eq_.SetBandFilterType(i, b.filter_type);
    }
    last_applied_.dynamic_eq = p;
}

std::optional<uint32_t> ViPER::LoadConvolverKernel(
    const float *samples,
    const uint32_t frame_count,
    const uint32_t channels,
    uint32_t kernel_id
) {
    if (samples == nullptr) return std::nullopt;
    if (channels < 1 || channels > 2) return std::nullopt;
    if (frame_count < 16) return std::nullopt;

    const uint32_t total_floats = frame_count * channels;
    if (total_floats == 0) return std::nullopt;

    convolver_.PrepareKernelBuffer(total_floats, channels, false);

    uint32_t written = 0;
    while (written < total_floats) {
        const uint32_t remaining = total_floats - written;
        const uint32_t chunk =
            remaining < kKernelChunkFloats ? remaining : kKernelChunkFloats;
        convolver_.SetKernelBuffer(samples + written, chunk);
        written += chunk;
    }

    const uint32_t crc =
        Crc32(reinterpret_cast<const uint8_t *>(samples), total_floats * sizeof(float));

    convolver_.CommitKernelBuffer(total_floats, crc, kernel_id);

    if (convolver_.GetKernelID() != kernel_id) {
        return std::nullopt;
    }
    return kernel_id;
}

void ViPER::UnloadConvolverKernel() {
    convolver_.PrepareKernelBuffer(0, 0, true);
}

void ViPER::LoadDdcCoefficients(
    const viper::BiquadSection *sections44100,
    const viper::BiquadSection *sections48000,
    const uint32_t section_count
) {
    if (section_count == 0) {
        viper_ddc_.SetCoeffs(0, nullptr, nullptr);
    }

    static_assert(
        sizeof(viper::BiquadSection) == 5 * sizeof(float),
        "BiquadSection must be tightly packed for reinterpret_cast"
    );
    const uint32_t total_floats = section_count * 5;
    viper_ddc_.SetCoeffs(
        total_floats,
        reinterpret_cast<const float *>(sections44100),
        reinterpret_cast<const float *>(sections48000)
    );
}
