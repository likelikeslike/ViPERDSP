#include "ViPER.h"
#include "../include/ViPERParams.h"
#include "../include/log.h"
#include "constants.h"
#include "utils/Crc32.h"
#include <type_traits>

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

bool ViPER::DispatchParamValue(const int param, const viper::ParamValue &value) {
    auto require_bool = [&] { return value.type == viper::ParamValueType::kBool; };
    auto require_int = [&] { return value.type == viper::ParamValueType::kInt; };
    auto require_float = [&] { return value.type == viper::ParamValueType::kFloat; };
    auto require_float_array = [&] {
        return value.type == viper::ParamValueType::kFloatArray
               && value.float_array != nullptr;
    };
    auto require_bytes = [&] {
        return value.type == viper::ParamValueType::kBytes && value.bytes != nullptr;
    };
    auto require_int_array = [&] {
        return value.type == viper::ParamValueType::kIntArray
               && value.int_array != nullptr;
    };

    switch (param) {
        case kParamResetAllEffects: {
            VIPER_LOGD("ResetAllEffects");
            ResetAllEffects();
            break;
        }

        // Master Limiter
        case kParamMasterLimiterThreshold: {
            if (!require_float()) return false;
            VIPER_LOGD("Master Limiter: threshold=%f", value.float_value);
            software_limiters_[0].SetGate(value.float_value);
            software_limiters_[1].SetGate(value.float_value);
            break;
        }
        case kParamMasterLimiterOutputVolume: {
            if (!require_float()) return false;
            VIPER_LOGD("Master Limiter: output_vol=%f", value.float_value);
            frame_scale_ = value.float_value;
            break;
        }
        case kParamMasterLimiterChannelPan: {
            if (!require_float()) return false;
            VIPER_LOGD("Master Limiter: pan=%f", value.float_value);
            if (value.float_value < 0.0f) {
                left_pan_ = 1.0f;
                right_pan_ = 1.0f + value.float_value;
            } else {
                left_pan_ = 1.0f - value.float_value;
                right_pan_ = 1.0f;
            }
            break;
        }

        // Playback Gain Control
        case kParamPlaybackGainControlEnable: {
            if (!require_bool()) return false;
            VIPER_LOGD("PlaybackGain: %s", value.bool_value ? "ON" : "OFF");
            playback_gain_.SetEnable(value.bool_value);
            break;
        }
        case kParamPlaybackGainControlStrength: {
            if (!require_float()) return false;
            VIPER_LOGD("PlaybackGain: strength=%f", value.float_value);
            playback_gain_.SetRatio(value.float_value);
            break;
        }
        case kParamPlaybackGainControlMaxGain: {
            if (!require_float()) return false;
            VIPER_LOGD("PlaybackGain: max_gain=%f", value.float_value);
            playback_gain_.SetMaxGainFactor(value.float_value);
            break;
        }
        case kParamPlaybackGainControlOutputThreshold: {
            if (!require_float()) return false;
            VIPER_LOGD("PlaybackGain: output_threshold=%f", value.float_value);
            playback_gain_.SetVolume(value.float_value);
            break;
        }

        // LUFS
        case kParamLufsEnable: {
            if (!require_bool()) return false;
            VIPER_LOGD("LUFS: %s", value.bool_value ? "ON" : "OFF");
            lufs_targeting_.SetEnable(value.bool_value);
            break;
        }
        case kParamLufsTarget: {
            if (!require_float()) return false;
            VIPER_LOGD("LUFS: target=%f", value.float_value);
            lufs_targeting_.SetTargetLUFS(value.float_value);
            break;
        }
        case kParamLufsMaxGain: {
            if (!require_float()) return false;
            VIPER_LOGD("LUFS: max_gain=%f", value.float_value);
            lufs_targeting_.SetMaxGain(value.float_value);
            break;
        }
        case kParamLufsSpeed: {
            if (!require_int()) return false;
            VIPER_LOGD("LUFS: speed=%d", value.int_value);
            lufs_targeting_.SetSpeed(value.int_value);
            break;
        }

        // FET Compressor
        case kParamFetCompressorEnable: {
            if (!require_bool()) return false;
            VIPER_LOGD("FET: %s", value.bool_value ? "ON" : "OFF");
            fet_compressor_.SetEnable(value.bool_value);
            break;
        }
        case kParamFetCompressorThreshold: {
            if (!require_float()) return false;
            VIPER_LOGD("FET: threshold=%f", value.float_value);
            fet_compressor_.SetThreshold(value.float_value);
            break;
        }
        case kParamFetCompressorRatio: {
            if (!require_float()) return false;
            VIPER_LOGD("FET: ratio=%f", value.float_value);
            fet_compressor_.SetRatio(value.float_value);
            break;
        }
        case kParamFetCompressorKnee: {
            if (!require_float()) return false;
            VIPER_LOGD("FET: knee=%f", value.float_value);
            fet_compressor_.SetKnee(value.float_value);
            break;
        }
        case kParamFetCompressorKneeAuto: {
            if (!require_bool()) return false;
            VIPER_LOGD("FET: knee_auto=%d", value.bool_value);
            fet_compressor_.SetKneeAuto(value.bool_value);
            break;
        }
        case kParamFetCompressorGain: {
            if (!require_float()) return false;
            VIPER_LOGD("FET: gain=%f", value.float_value);
            fet_compressor_.SetGain(value.float_value);
            break;
        }
        case kParamFetCompressorGainAuto: {
            if (!require_bool()) return false;
            VIPER_LOGD("FET: gain_auto=%d", value.bool_value);
            fet_compressor_.SetGainAuto(value.bool_value);
            break;
        }
        case kParamFetCompressorAttack: {
            if (!require_float()) return false;
            VIPER_LOGD("FET: attack=%f", value.float_value);
            fet_compressor_.SetAttack(value.float_value);
            break;
        }
        case kParamFetCompressorAttackAuto: {
            if (!require_bool()) return false;
            VIPER_LOGD("FET: attack_auto=%d", value.bool_value);
            fet_compressor_.SetAttackAuto(value.bool_value);
            break;
        }
        case kParamFetCompressorRelease: {
            if (!require_float()) return false;
            VIPER_LOGD("FET: release=%f", value.float_value);
            fet_compressor_.SetRelease(value.float_value);
            break;
        }
        case kParamFetCompressorReleaseAuto: {
            if (!require_bool()) return false;
            VIPER_LOGD("FET: release_auto=%d", value.bool_value);
            fet_compressor_.SetReleaseAuto(value.bool_value);
            break;
        }
        case kParamFetCompressorKneeMulti: {
            if (!require_float()) return false;
            VIPER_LOGD("FET: knee_multi=%f", value.float_value);
            fet_compressor_.SetKneeMulti(value.float_value);
            break;
        }
        case kParamFetCompressorMaxAttack: {
            if (!require_float()) return false;
            VIPER_LOGD("FET: max_attack=%f", value.float_value);
            fet_compressor_.SetMaxAttack(value.float_value);
            break;
        }
        case kParamFetCompressorMaxRelease: {
            if (!require_float()) return false;
            VIPER_LOGD("FET: max_release=%f", value.float_value);
            fet_compressor_.SetMaxRelease(value.float_value);
            break;
        }
        case kParamFetCompressorCrest: {
            if (!require_float()) return false;
            VIPER_LOGD("FET: crest=%f", value.float_value);
            fet_compressor_.SetCrest(value.float_value);
            break;
        }
        case kParamFetCompressorAdapt: {
            if (!require_float()) return false;
            VIPER_LOGD("FET: adapt=%f", value.float_value);
            fet_compressor_.SetAdapt(value.float_value);
            break;
        }
        case kParamFetCompressorNoClip: {
            if (!require_bool()) return false;
            VIPER_LOGD("FET: no_clip=%d", value.bool_value);
            fet_compressor_.SetNoClip(value.bool_value);
            break;
        }

        // Bass
        case kParamBassEnable: {
            if (!require_bool()) return false;
            VIPER_LOGD("Bass: %s", value.bool_value ? "ON" : "OFF");
            viper_bass_.SetEnable(value.bool_value);
            break;
        }
        case kParamBassMode: {
            if (!require_int()) return false;
            VIPER_LOGD("Bass: mode=%d", value.int_value);
            viper_bass_.SetProcessMode(
                static_cast<ViPERBass::ProcessMode>(value.int_value)
            );
            break;
        }
        case kParamBassFrequency: {
            if (!require_int()) return false;
            VIPER_LOGD("Bass: freq=%d", value.int_value);
            viper_bass_.SetFrequency(static_cast<uint32_t>(value.int_value));
            break;
        }
        case kParamBassGain: {
            if (!require_float()) return false;
            VIPER_LOGD("Bass: gain=%f", value.float_value);
            viper_bass_.SetBassFactor(value.float_value);
            break;
        }
        case kParamBassAntiPop: {
            if (!require_bool()) return false;
            VIPER_LOGD("Bass: anti_pop=%s", value.bool_value ? "ON" : "OFF");
            viper_bass_.SetAntiPop(value.bool_value);
            break;
        }

        // Bass Mono
        case kParamBassMonoEnable: {
            if (!require_bool()) return false;
            VIPER_LOGD("BassMono: %s", value.bool_value ? "ON" : "OFF");
            viper_bass_mono_.SetEnable(value.bool_value);
            break;
        }
        case kParamBassMonoMode: {
            if (!require_int()) return false;
            VIPER_LOGD("BassMono: mode=%d", value.int_value);
            viper_bass_mono_.SetProcessMode(
                static_cast<ViPERBassMono::ProcessMode>(value.int_value)
            );
            break;
        }
        case kParamBassMonoFrequency: {
            if (!require_int()) return false;
            VIPER_LOGD("BassMono: freq=%d", value.int_value);
            viper_bass_mono_.SetFrequency(static_cast<uint32_t>(value.int_value));
            break;
        }
        case kParamBassMonoGain: {
            if (!require_float()) return false;
            VIPER_LOGD("BassMono: gain=%f", value.float_value);
            viper_bass_mono_.SetBassFactor(value.float_value);
            break;
        }
        case kParamBassMonoAntiPop: {
            if (!require_bool()) return false;
            VIPER_LOGD("BassMono: anti_pop=%s", value.bool_value ? "ON" : "OFF");
            viper_bass_mono_.SetAntiPop(value.bool_value);
            break;
        }

        // Psychoacoustic Bass
        case kParamPsychoacousticBassEnable: {
            if (!require_bool()) return false;
            VIPER_LOGD("PsychoBass: %s", value.bool_value ? "ON" : "OFF");
            psychoacoustic_bass_.SetEnable(value.bool_value);
            break;
        }
        case kParamPsychoacousticBassCutoff: {
            if (!require_int()) return false;
            VIPER_LOGD("PsychoBass: cutoff=%d", value.int_value);
            psychoacoustic_bass_.SetCutoff(static_cast<uint32_t>(value.int_value));
            break;
        }
        case kParamPsychoacousticBassIntensity: {
            if (!require_float()) return false;
            VIPER_LOGD("PsychoBass: intensity=%f", value.float_value);
            psychoacoustic_bass_.SetIntensity(value.float_value);
            break;
        }
        case kParamPsychoacousticBassHarmonicOrder: {
            if (!require_int()) return false;
            VIPER_LOGD("PsychoBass: harmonic_order=%d", value.int_value);
            psychoacoustic_bass_.SetHarmonicOrder(static_cast<uint32_t>(value.int_value));
            break;
        }
        case kParamPsychoacousticBassOriginalLevel: {
            if (!require_float()) return false;
            VIPER_LOGD("PsychoBass: original_level=%f", value.float_value);
            psychoacoustic_bass_.SetOriginalBassLevel(value.float_value);
            break;
        }

        // Spectrum Extension
        case kParamSpectrumExtensionEnable: {
            if (!require_bool()) return false;
            VIPER_LOGD("SpecExt: %s", value.bool_value ? "ON" : "OFF");
            spectrum_extend_.SetEnable(value.bool_value);
            break;
        }
        case kParamSpectrumExtensionStrength: {
            if (!require_int()) return false;
            VIPER_LOGD("SpecExt: strength=%d", value.int_value);
            spectrum_extend_.SetReferenceFrequency(value.int_value);
            break;
        }
        case kParamSpectrumExtensionExciter: {
            if (!require_float()) return false;
            VIPER_LOGD("SpecExt: exciter=%f", value.float_value);
            spectrum_extend_.SetExciter(value.float_value);
            break;
        }

        // Equalizer (IIR Filter)
        case kParamEqualizerEnable: {
            if (!require_bool()) return false;
            VIPER_LOGD("EQ: %s", value.bool_value ? "ON" : "OFF");
            iir_filter_.SetEnable(value.bool_value);
            break;
        }
        case kParamEqualizerBandLevel: {
            if (!require_float() || value.index < 0) return false;
            VIPER_LOGD("EQ: band=%d level=%f", value.index, value.float_value);
            iir_filter_.SetBandLevel(
                static_cast<uint32_t>(value.index), value.float_value
            );
            break;
        }
        case kParamEqualizerBandLevels: {
            if (!require_float_array()) return false;
            VIPER_LOGD("EQ: bands_levels=%zu", value.float_count);
            iir_filter_.SetBandLevels(
                value.float_array, static_cast<uint32_t>(value.float_count)
            );
            break;
        }
        case kParamEqualizerBandCount: {
            if (!require_int()) return false;
            VIPER_LOGD("EQ: band_count=%d", value.int_value);
            iir_filter_.SetBandCount(static_cast<uint32_t>(value.int_value));
            break;
        }

        // Convolver
        case kParamConvolverEnable: {
            if (!require_bool()) return false;
            VIPER_LOGD("Convolver: %s", value.bool_value ? "ON" : "OFF");
            convolver_.SetEnable(value.bool_value);
            break;
        }
        case kParamConvolverSetKernel: {
            if (!require_bytes()) return false;
            if (value.byte_count > 0) {
                char path[256] = {};
                memcpy(
                    path, value.bytes, value.byte_count < 255 ? value.byte_count : 255
                );
                VIPER_LOGD("Convolver: SetKernel path=%s", path);
                convolver_.SetKernel(path);
            }
            break;
        }
        case kParamConvolverPrepareBuffer: {
            if (!require_int_array() || value.int_count != 3) return false;
            VIPER_LOGD(
                "Convolver: PrepareBuffer buf_size=%d ch=%d reset=%d",
                value.int_array[0],
                value.int_array[1],
                value.int_array[2]
            );
            convolver_.PrepareKernelBuffer(
                value.int_array[0], value.int_array[1], value.int_array[2] != 0
            );
            break;
        }
        case kParamConvolverSetBuffer: {
            if (!require_float_array()) return false;
            VIPER_LOGD("Convolver: SetBuffer size=%zu", value.float_count);
            convolver_.SetKernelBuffer(
                value.float_array, static_cast<uint32_t>(value.float_count)
            );
            break;
        }
        case kParamConvolverCommitBuffer: {
            if (!require_int_array() || value.int_count != 3) return false;
            VIPER_LOGD(
                "Convolver: CommitBuffer channels=%d frames=%d sr=%d",
                value.int_array[0],
                value.int_array[1],
                value.int_array[2]
            );
            convolver_.CommitKernelBuffer(
                value.int_array[0], value.int_array[1], value.int_array[2]
            );
            break;
        }
        case kParamConvolverCrossChannel: {
            if (!require_float()) return false;
            VIPER_LOGD("Convolver: cross_ch=%f", value.float_value);
            convolver_.SetCrossChannel(value.float_value);
            break;
        }

        // DDC
        case kParamDdcEnable: {
            if (!require_bool()) return false;
            VIPER_LOGD("DDC: %s", value.bool_value ? "ON" : "OFF");
            viper_ddc_.SetEnable(value.bool_value);
            break;
        }
        case kParamDdcCoefficients: {
            if (!require_float_array()) return false;
            VIPER_LOGD("DDC: SetCoeffs arr_size=%zu", value.float_count);
            viper_ddc_.SetCoeffs(
                static_cast<uint32_t>(value.float_count / 2),
                value.float_array,
                value.float_array + value.float_count / 2
            );
            break;
        }

        // Field Surround (Colorful Music)
        case kParamFieldSurroundEnable: {
            if (!require_bool()) return false;
            VIPER_LOGD("FieldSurr: %s", value.bool_value ? "ON" : "OFF");
            colorful_music_.SetEnable(value.bool_value);
            break;
        }
        case kParamFieldSurroundWidening: {
            if (!require_float()) return false;
            VIPER_LOGD("FieldSurr: widen=%f", value.float_value);
            colorful_music_.SetWidenValue(value.float_value);
            break;
        }
        case kParamFieldSurroundMidImage: {
            if (!require_float()) return false;
            VIPER_LOGD("FieldSurr: mid_image=%f", value.float_value);
            colorful_music_.SetMidImageValue(value.float_value);
            break;
        }
        case kParamFieldSurroundDepth: {
            if (!require_int()) return false;
            VIPER_LOGD("FieldSurr: depth=%d", value.int_value);
            colorful_music_.SetDepthValue(static_cast<uint32_t>(value.int_value));
            break;
        }

        // Differential Surround
        case kParamDiffSurroundEnable: {
            if (!require_bool()) return false;
            VIPER_LOGD("DiffSurr: %s", value.bool_value ? "ON" : "OFF");
            diff_surround_.SetEnable(value.bool_value);
            break;
        }
        case kParamDiffSurroundDelay: {
            if (!require_float()) return false;
            VIPER_LOGD("DiffSurr: delay=%f", value.float_value);
            diff_surround_.SetDelayTime(value.float_value);
            break;
        }
        case kParamDiffSurroundReverse: {
            if (!require_bool()) return false;
            VIPER_LOGD("DiffSurr: reverse=%s", value.bool_value ? "ON" : "OFF");
            diff_surround_.SetReverse(value.bool_value);
            break;
        }
        case kParamDiffSurroundWetDryMix: {
            if (!require_float()) return false;
            VIPER_LOGD("DiffSurr: wet_dry_mix=%f", value.float_value);
            diff_surround_.SetWetDryMix(value.float_value);
            break;
        }
        case kParamDiffSurroundLpCutoff: {
            if (!require_int()) return false;
            VIPER_LOGD("DiffSurr: lp_cutoff=%d", value.int_value);
            diff_surround_.SetLPCutoff(static_cast<float>(value.int_value));
            break;
        }

        // Stereo Imager
        case kParamStereoImagerEnable: {
            if (!require_bool()) return false;
            VIPER_LOGD("StereoImg: %s", value.bool_value ? "ON" : "OFF");
            stereo_imager_.SetEnable(value.bool_value);
            break;
        }
        case kParamStereoImagerLowWidth: {
            if (!require_float()) return false;
            VIPER_LOGD("StereoImg: low_width=%f", value.float_value);
            stereo_imager_.SetLowWidth(value.float_value);
            break;
        }
        case kParamStereoImagerMidWidth: {
            if (!require_float()) return false;
            VIPER_LOGD("StereoImg: mid_width=%f", value.float_value);
            stereo_imager_.SetMidWidth(value.float_value);
            break;
        }
        case kParamStereoImagerHighWidth: {
            if (!require_float()) return false;
            VIPER_LOGD("StereoImg: high_width=%f", value.float_value);
            stereo_imager_.SetHighWidth(value.float_value);
            break;
        }
        case kParamStereoImagerLowCrossover: {
            if (!require_int()) return false;
            VIPER_LOGD("StereoImg: low_crossover=%d", value.int_value);
            stereo_imager_.SetLowCrossover(static_cast<float>(value.int_value));
            break;
        }
        case kParamStereoImagerHighCrossover: {
            if (!require_int()) return false;
            VIPER_LOGD("StereoImg: high_crossover=%d", value.int_value);
            stereo_imager_.SetHighCrossover(static_cast<float>(value.int_value));
            break;
        }

        // Headphone Surround (VHE)
        case kParamHeadphoneSurroundEnable: {
            if (!require_bool()) return false;
            VIPER_LOGD("VHE: %s", value.bool_value ? "ON" : "OFF");
            vhe_.SetEnable(value.bool_value);
            break;
        }
        case kParamHeadphoneSurroundQuality: {
            if (!require_int()) return false;
            VIPER_LOGD("VHE: quality=%d", value.int_value);
            vhe_.SetEffectLevel(value.int_value);
            break;
        }

        // Reverb
        case kParamReverbEnable: {
            if (!require_bool()) return false;
            VIPER_LOGD("Reverb: %s", value.bool_value ? "ON" : "OFF");
            reverberation_.SetEnable(value.bool_value);
            break;
        }
        case kParamReverbRoomSize: {
            if (!require_float()) return false;
            VIPER_LOGD("Reverb: room_size=%f", value.float_value);
            reverberation_.SetRoomSize(value.float_value);
            break;
        }
        case kParamReverbWidth: {
            if (!require_float()) return false;
            VIPER_LOGD("Reverb: width=%f", value.float_value);
            reverberation_.SetWidth(value.float_value);
            break;
        }
        case kParamReverbDamp: {
            if (!require_float()) return false;
            VIPER_LOGD("Reverb: damp=%f", value.float_value);
            reverberation_.SetDamp(value.float_value);
            break;
        }
        case kParamReverbWet: {
            if (!require_float()) return false;
            VIPER_LOGD("Reverb: wet=%f", value.float_value);
            reverberation_.SetWet(value.float_value);
            break;
        }
        case kParamReverbDry: {
            if (!require_float()) return false;
            VIPER_LOGD("Reverb: dry=%f", value.float_value);
            reverberation_.SetDry(value.float_value);
            break;
        }

        // Dynamic System
        case kParamDynamicSystemEnable: {
            if (!require_bool()) return false;
            VIPER_LOGD("DynSys: %s", value.bool_value ? "ON" : "OFF");
            dynamic_system_.SetEnable(value.bool_value);
            break;
        }
        case kParamDynamicSystemXLow: {
            if (!require_int()) return false;
            VIPER_LOGD("DynSys: x_low=%d", value.int_value);
            dynamic_system_.SetXCoeffs(value.int_value, -1);
            break;
        }
        case kParamDynamicSystemXHigh: {
            if (!require_int()) return false;
            VIPER_LOGD("DynSys: x_high=%d", value.int_value);
            dynamic_system_.SetXCoeffs(-1, value.int_value);
            break;
        }
        case kParamDynamicSystemYLow: {
            if (!require_int()) return false;
            VIPER_LOGD("DynSys: y_low=%d", value.int_value);
            dynamic_system_.SetYCoeffs(value.int_value, -1);
            break;
        }
        case kParamDynamicSystemYHigh: {
            if (!require_int()) return false;
            VIPER_LOGD("DynSys: y_high=%d", value.int_value);
            dynamic_system_.SetYCoeffs(-1, value.int_value);
            break;
        }
        case kParamDynamicSystemSideGainLow: {
            if (!require_float()) return false;
            VIPER_LOGD("DynSys: side_gain_low=%f", value.float_value);
            dynamic_system_.SetSideGain(value.float_value, -1.0f);
            break;
        }
        case kParamDynamicSystemSideGainHigh: {
            if (!require_float()) return false;
            VIPER_LOGD("DynSys: side_gain_high=%f", value.float_value);
            dynamic_system_.SetSideGain(-1.0f, value.float_value);
            break;
        }
        case kParamDynamicSystemStrength: {
            if (!require_float()) return false;
            VIPER_LOGD("DynSys: strength=%f", value.float_value);
            dynamic_system_.SetBassGain(value.float_value);
            break;
        }

        // Clarity
        case kParamClarityEnable: {
            if (!require_bool()) return false;
            VIPER_LOGD("Clarity: %s", value.bool_value ? "ON" : "OFF");
            viper_clarity_.SetEnable(value.bool_value);
            break;
        }
        case kParamClarityMode: {
            if (!require_int()) return false;
            VIPER_LOGD("Clarity: mode=%d", value.int_value);
            viper_clarity_.SetProcessMode(
                static_cast<ViPERClarity::ClarityMode>(value.int_value)
            );
            break;
        }
        case kParamClarityGain: {
            if (!require_float()) return false;
            VIPER_LOGD("Clarity: gain=%f", value.float_value);
            viper_clarity_.SetClarityGain(value.float_value);
            break;
        }

        // Cure (Crossfeed)
        case kParamCureEnable: {
            if (!require_bool()) return false;
            VIPER_LOGD("Cure: %s", value.bool_value ? "ON" : "OFF");
            cure_.SetEnable(value.bool_value);
            break;
        }
        case kParamCureCrossfeedPreset: {
            if (!require_int()) return false;
            VIPER_LOGD("Cure: crossfeed_preset=%d", value.int_value);
            cure_.SetPreset(value.int_value);
            break;
        }

        // Tube Simulator
        case kParamTubeSimulatorEnable: {
            if (!require_bool()) return false;
            VIPER_LOGD("TubeSim: %s", value.bool_value ? "ON" : "OFF");
            tube_simulator_.SetEnable(value.bool_value);
            break;
        }

        // AnalogX
        case kParamAnalogXEnable: {
            if (!require_bool()) return false;
            VIPER_LOGD("AnalogX: %s", value.bool_value ? "ON" : "OFF");
            analog_x_.SetEnable(value.bool_value);
            break;
        }
        case kParamAnalogXMode: {
            if (!require_int()) return false;
            VIPER_LOGD("AnalogX: mode=%d", value.int_value);
            analog_x_.SetProcessingModel(value.int_value);
            break;
        }

        // Speaker Correction
        case kParamSpeakerCorrectionEnable: {
            if (!require_bool()) return false;
            VIPER_LOGD("SpkCorr: %s", value.bool_value ? "ON" : "OFF");
            speaker_correction_.SetEnable(value.bool_value);
            break;
        }

        // Multiband Compressor
        case kParamMultibandCompressorEnable: {
            if (!require_bool()) return false;
            VIPER_LOGD("MBComp: %s", value.bool_value ? "ON" : "OFF");
            multiband_compressor_.SetEnable(value.bool_value);
            break;
        }
        case kParamMultibandCompressorBandCount: {
            if (!require_int()) return false;
            VIPER_LOGD("MBComp: band_count=%d", value.int_value);
            multiband_compressor_.SetBandCount(value.int_value);
            break;
        }
        case kParamMultibandCompressorCrossoverFrequency: {
            if (!require_int() || value.index < 0) return false;
            VIPER_LOGD("MBComp: crossover[%d]=%d", value.index, value.int_value);
            multiband_compressor_.SetCrossoverFrequency(
                value.index, static_cast<float>(value.int_value)
            );
            break;
        }
        case kParamMultibandCompressorBandThreshold: {
            if (!require_float() || value.index < 0) return false;
            VIPER_LOGD("MBComp: band[%d] threshold=%f", value.index, value.float_value);
            multiband_compressor_.SetBandThreshold(value.index, value.float_value);
            break;
        }
        case kParamMultibandCompressorBandRatio: {
            if (!require_float() || value.index < 0) return false;
            VIPER_LOGD("MBComp: band[%d] ratio=%f", value.index, value.float_value);
            multiband_compressor_.SetBandRatio(value.index, value.float_value);
            break;
        }
        case kParamMultibandCompressorBandKnee: {
            if (!require_float() || value.index < 0) return false;
            VIPER_LOGD("MBComp: band[%d] knee=%f", value.index, value.float_value);
            multiband_compressor_.SetBandKnee(value.index, value.float_value);
            break;
        }
        case kParamMultibandCompressorBandKneeAuto: {
            if (!require_bool() || value.index < 0) return false;
            VIPER_LOGD("MBComp: band[%d] knee_auto=%d", value.index, value.bool_value);
            multiband_compressor_.SetBandKneeAuto(value.index, value.bool_value);
            break;
        }
        case kParamMultibandCompressorBandGain: {
            if (!require_float() || value.index < 0) return false;
            VIPER_LOGD("MBComp: band[%d] gain=%f", value.index, value.float_value);
            multiband_compressor_.SetBandGain(value.index, value.float_value);
            break;
        }
        case kParamMultibandCompressorBandGainAuto: {
            if (!require_bool() || value.index < 0) return false;
            VIPER_LOGD("MBComp: band[%d] gain_auto=%d", value.index, value.bool_value);
            multiband_compressor_.SetBandGainAuto(value.index, value.bool_value);
            break;
        }
        case kParamMultibandCompressorBandAttack: {
            if (!require_float() || value.index < 0) return false;
            VIPER_LOGD("MBComp: band[%d] attack=%f", value.index, value.float_value);
            multiband_compressor_.SetBandAttack(value.index, value.float_value);
            break;
        }
        case kParamMultibandCompressorBandAttackAuto: {
            if (!require_bool() || value.index < 0) return false;
            VIPER_LOGD("MBComp: band[%d] attack_auto=%d", value.index, value.bool_value);
            multiband_compressor_.SetBandAttackAuto(value.index, value.bool_value);
            break;
        }
        case kParamMultibandCompressorBandRelease: {
            if (!require_float() || value.index < 0) return false;
            VIPER_LOGD("MBComp: band[%d] release=%f", value.index, value.float_value);
            multiband_compressor_.SetBandRelease(value.index, value.float_value);
            break;
        }
        case kParamMultibandCompressorBandReleaseAuto: {
            if (!require_bool() || value.index < 0) return false;
            VIPER_LOGD("MBComp: band[%d] release_auto=%d", value.index, value.bool_value);
            multiband_compressor_.SetBandReleaseAuto(value.index, value.bool_value);
            break;
        }
        case kParamMultibandCompressorBandKneeMulti: {
            if (!require_float() || value.index < 0) return false;
            VIPER_LOGD("MBComp: band[%d] knee_multi=%f", value.index, value.float_value);
            multiband_compressor_.SetBandKneeMulti(value.index, value.float_value);
            break;
        }
        case kParamMultibandCompressorBandMaxAttack: {
            if (!require_float() || value.index < 0) return false;
            VIPER_LOGD("MBComp: band[%d] max_attack=%f", value.index, value.float_value);
            multiband_compressor_.SetBandMaxAttack(value.index, value.float_value);
            break;
        }
        case kParamMultibandCompressorBandMaxRelease: {
            if (!require_float() || value.index < 0) return false;
            VIPER_LOGD("MBComp: band[%d] max_release=%f", value.index, value.float_value);
            multiband_compressor_.SetBandMaxRelease(value.index, value.float_value);
            break;
        }
        case kParamMultibandCompressorBandCrest: {
            if (!require_float() || value.index < 0) return false;
            VIPER_LOGD("MBComp: band[%d] crest=%f", value.index, value.float_value);
            multiband_compressor_.SetBandCrest(value.index, value.float_value);
            break;
        }
        case kParamMultibandCompressorBandAdapt: {
            if (!require_float() || value.index < 0) return false;
            VIPER_LOGD("MBComp: band[%d] adapt=%f", value.index, value.float_value);
            multiband_compressor_.SetBandAdapt(value.index, value.float_value);
            break;
        }
        case kParamMultibandCompressorBandNoClip: {
            if (!require_bool() || value.index < 0) return false;
            VIPER_LOGD("MBComp: band[%d] no_clip=%d", value.index, value.bool_value);
            multiband_compressor_.SetBandNoClip(value.index, value.bool_value);
            break;
        }
        case kParamMultibandCompressorBandEnable: {
            if (!require_bool() || value.index < 0) return false;
            VIPER_LOGD("MBComp: band[%d] enable=%d", value.index, value.bool_value);
            multiband_compressor_.SetBandEnable(value.index, value.bool_value);
            break;
        }

        // Dynamic EQ
        case kParamDynamicEqEnable: {
            if (!require_bool()) return false;
            VIPER_LOGD("DynEQ: %s", value.bool_value ? "ON" : "OFF");
            dynamic_eq_.SetEnable(value.bool_value);
            break;
        }
        case kParamDynamicEqBandCount: {
            if (!require_int()) return false;
            VIPER_LOGD("DynEQ: band_count=%d", value.int_value);
            dynamic_eq_.SetBandCount(value.int_value);
            break;
        }
        case kParamDynamicEqBandFrequency: {
            if (!require_int() || value.index < 0) return false;
            VIPER_LOGD("DynEQ: band[%d] freq=%d", value.index, value.int_value);
            dynamic_eq_.SetBandFrequency(
                value.index, static_cast<float>(value.int_value)
            );
            break;
        }
        case kParamDynamicEqBandQ: {
            if (!require_float() || value.index < 0) return false;
            VIPER_LOGD("DynEQ: band[%d] Q=%f", value.index, value.float_value);
            dynamic_eq_.SetBandQ(value.index, value.float_value);
            break;
        }
        case kParamDynamicEqBandGain: {
            if (!require_float() || value.index < 0) return false;
            VIPER_LOGD("DynEQ: band[%d] gain=%f", value.index, value.float_value);
            dynamic_eq_.SetBandGain(value.index, value.float_value);
            break;
        }
        case kParamDynamicEqBandThreshold: {
            if (!require_float() || value.index < 0) return false;
            VIPER_LOGD("DynEQ: band[%d] threshold=%f", value.index, value.float_value);
            dynamic_eq_.SetBandThreshold(value.index, value.float_value);
            break;
        }
        case kParamDynamicEqBandAttack: {
            if (!require_float() || value.index < 0) return false;
            VIPER_LOGD("DynEQ: band[%d] attack=%f", value.index, value.float_value);
            dynamic_eq_.SetBandAttack(value.index, value.float_value);
            break;
        }
        case kParamDynamicEqBandRelease: {
            if (!require_float() || value.index < 0) return false;
            VIPER_LOGD("DynEQ: band[%d] release=%f", value.index, value.float_value);
            dynamic_eq_.SetBandRelease(value.index, value.float_value);
            break;
        }
        case kParamDynamicEqBandFilterType: {
            if (!require_int() || value.index < 0) return false;
            VIPER_LOGD("DynEQ: band[%d] filter_type=%d", value.index, value.int_value);
            dynamic_eq_.SetBandFilterType(value.index, value.int_value);
            break;
        }

        default: {
            VIPER_LOGD("Unknown param: 0x%X", param);
            return false;
        }
    }
    return true;
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
