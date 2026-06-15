#pragma once

#include <array>

#ifndef VERSION_CODE
#define VERSION_CODE 00000000
#endif
#ifndef VERSION_NAME
#define VERSION_NAME "0.0.0"
#endif

#define COMMAND_CODE_GET 0x01
#define COMMAND_CODE_SET 0x02

namespace viper::params {

constexpr int kParamResetAllEffects = 0x10101;

constexpr int kParamMasterLimiterThreshold = 0x10110;
constexpr int kParamMasterLimiterOutputVolume = 0x10111;
constexpr int kParamMasterLimiterChannelPan = 0x10112;

constexpr int kParamPlaybackGainControlEnable = 0x10120;
constexpr int kParamPlaybackGainControlStrength = 0x10121;
constexpr int kParamPlaybackGainControlMaxGain = 0x10122;
constexpr int kParamPlaybackGainControlOutputThreshold = 0x10123;

constexpr int kParamLufsEnable = 0x10130;
constexpr int kParamLufsTarget = 0x10131;
constexpr int kParamLufsMaxGain = 0x10132;
constexpr int kParamLufsSpeed = 0x10133;

constexpr int kParamFetCompressorEnable = 0x10140;
constexpr int kParamFetCompressorThreshold = 0x10141;
constexpr int kParamFetCompressorRatio = 0x10142;
constexpr int kParamFetCompressorKnee = 0x10143;
constexpr int kParamFetCompressorKneeAuto = 0x10144;
constexpr int kParamFetCompressorGain = 0x10145;
constexpr int kParamFetCompressorGainAuto = 0x10146;
constexpr int kParamFetCompressorAttack = 0x10147;
constexpr int kParamFetCompressorAttackAuto = 0x10148;
constexpr int kParamFetCompressorRelease = 0x10149;
constexpr int kParamFetCompressorReleaseAuto = 0x1014A;
constexpr int kParamFetCompressorKneeMulti = 0x1014B;
constexpr int kParamFetCompressorMaxAttack = 0x1014C;
constexpr int kParamFetCompressorMaxRelease = 0x1014D;
constexpr int kParamFetCompressorCrest = 0x1014E;
constexpr int kParamFetCompressorAdapt = 0x1014F;
constexpr int kParamFetCompressorNoClip = 0x10150;

constexpr int kParamBassEnable = 0x10160;
constexpr int kParamBassMode = 0x10161;
constexpr int kParamBassFrequency = 0x10162;
constexpr int kParamBassGain = 0x10163;
constexpr int kParamBassAntiPop = 0x10164;

constexpr int kParamBassMonoEnable = 0x10170;
constexpr int kParamBassMonoMode = 0x10171;
constexpr int kParamBassMonoFrequency = 0x10172;
constexpr int kParamBassMonoGain = 0x10173;
constexpr int kParamBassMonoAntiPop = 0x10174;

constexpr int kParamPsychoacousticBassEnable = 0x10180;
constexpr int kParamPsychoacousticBassCutoff = 0x10181;
constexpr int kParamPsychoacousticBassIntensity = 0x10182;
constexpr int kParamPsychoacousticBassHarmonicOrder = 0x10183;
constexpr int kParamPsychoacousticBassOriginalLevel = 0x10184;

constexpr int kParamSpectrumExtensionEnable = 0x10190;
constexpr int kParamSpectrumExtensionStrength = 0x10191;
constexpr int kParamSpectrumExtensionExciter = 0x10192;

constexpr int kParamEqualizerEnable = 0x101A0;
constexpr int kParamEqualizerBandLevel = 0x101A1;
constexpr int kParamEqualizerBandCount = 0x101A2;

constexpr int kParamConvolverEnable = 0x101B0;
constexpr int kParamConvolverSetKernel = 0x101B1;
constexpr int kParamConvolverPrepareBuffer = 0x101B2;
constexpr int kParamConvolverSetBuffer = 0x101B3;
constexpr int kParamConvolverCommitBuffer = 0x101B4;
constexpr int kParamConvolverCrossChannel = 0x101B5;

constexpr int kParamDdcEnable = 0x101C0;
constexpr int kParamDdcCoefficients = 0x101C1;

constexpr int kParamFieldSurroundEnable = 0x101D0;
constexpr int kParamFieldSurroundWidening = 0x101D1;
constexpr int kParamFieldSurroundMidImage = 0x101D2;
constexpr int kParamFieldSurroundDepth = 0x101D3;

constexpr int kParamDiffSurroundEnable = 0x101E0;
constexpr int kParamDiffSurroundDelay = 0x101E1;
constexpr int kParamDiffSurroundReverse = 0x101E2;
constexpr int kParamDiffSurroundWetDryMix = 0x101E3;
constexpr int kParamDiffSurroundLpCutoff = 0x101E4;

constexpr int kParamStereoImagerEnable = 0x101F0;
constexpr int kParamStereoImagerLowWidth = 0x101F1;
constexpr int kParamStereoImagerMidWidth = 0x101F2;
constexpr int kParamStereoImagerHighWidth = 0x101F3;
constexpr int kParamStereoImagerLowCrossover = 0x101F4;
constexpr int kParamStereoImagerHighCrossover = 0x101F5;

constexpr int kParamHeadphoneSurroundEnable = 0x10200;
constexpr int kParamHeadphoneSurroundQuality = 0x10201;

constexpr int kParamReverbEnable = 0x10210;
constexpr int kParamReverbRoomSize = 0x10211;
constexpr int kParamReverbWidth = 0x10212;
constexpr int kParamReverbDamp = 0x10213;
constexpr int kParamReverbWet = 0x10214;
constexpr int kParamReverbDry = 0x10215;

constexpr int kParamDynamicSystemEnable = 0x10220;
constexpr int kParamDynamicSystemXCoefficients = 0x10221;
constexpr int kParamDynamicSystemYCoefficients = 0x10222;
constexpr int kParamDynamicSystemSideGain = 0x10223;
constexpr int kParamDynamicSystemStrength = 0x10224;

constexpr int kParamClarityEnable = 0x10230;
constexpr int kParamClarityMode = 0x10231;
constexpr int kParamClarityGain = 0x10232;

constexpr int kParamCureEnable = 0x10240;
constexpr int kParamCureCrossfeedPreset = 0x10241;

constexpr int kParamTubeSimulatorEnable = 0x10250;

constexpr int kParamAnalogXEnable = 0x10260;
constexpr int kParamAnalogXMode = 0x10261;

constexpr int kParamSpeakerCorrectionEnable = 0x10270;

constexpr int kParamMultibandCompressorEnable = 0x10280;
constexpr int kParamMultibandCompressorBandCount = 0x10281;
constexpr int kParamMultibandCompressorCrossoverFrequency = 0x10282;
constexpr int kParamMultibandCompressorBandThreshold = 0x10283;
constexpr int kParamMultibandCompressorBandRatio = 0x10284;
constexpr int kParamMultibandCompressorBandKnee = 0x10285;
constexpr int kParamMultibandCompressorBandKneeAuto = 0x10286;
constexpr int kParamMultibandCompressorBandGain = 0x10287;
constexpr int kParamMultibandCompressorBandGainAuto = 0x10288;
constexpr int kParamMultibandCompressorBandAttack = 0x10289;
constexpr int kParamMultibandCompressorBandAttackAuto = 0x1028A;
constexpr int kParamMultibandCompressorBandRelease = 0x1028B;
constexpr int kParamMultibandCompressorBandReleaseAuto = 0x1028C;
constexpr int kParamMultibandCompressorBandKneeMulti = 0x1028D;
constexpr int kParamMultibandCompressorBandMaxAttack = 0x1028E;
constexpr int kParamMultibandCompressorBandMaxRelease = 0x1028F;
constexpr int kParamMultibandCompressorBandCrest = 0x10290;
constexpr int kParamMultibandCompressorBandAdapt = 0x10291;
constexpr int kParamMultibandCompressorBandNoClip = 0x10292;
constexpr int kParamMultibandCompressorBandEnable = 0x10293;

constexpr int kParamDynamicEqEnable = 0x102A0;
constexpr int kParamDynamicEqBandCount = 0x102A1;
constexpr int kParamDynamicEqBandFrequency = 0x102A2;
constexpr int kParamDynamicEqBandQ = 0x102A3;
constexpr int kParamDynamicEqBandGain = 0x102A4;
constexpr int kParamDynamicEqBandThreshold = 0x102A5;
constexpr int kParamDynamicEqBandAttack = 0x102A6;
constexpr int kParamDynamicEqBandRelease = 0x102A7;
constexpr int kParamDynamicEqBandFilterType = 0x102A8;

} // namespace viper::params

namespace viper {

struct MasterLimiterParams {
    float threshold = 1.0f;     // 0..1
    float output_volume = 1.0f; // 0..1
    float channel_pan = 0.0f;   // -1..+1

    bool operator==(const MasterLimiterParams &other) const {
        return threshold == other.threshold && output_volume == other.output_volume
               && channel_pan == other.channel_pan;
    }
};

struct PlaybackGainControlParams {
    bool enable = false;
    float strength = 0.0f;         // 0..1
    float max_gain = 0.0f;         // 0..1
    float output_threshold = 0.0f; // 0..1

    bool operator==(const PlaybackGainControlParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable && strength == other.strength
               && max_gain == other.max_gain
               && output_threshold == other.output_threshold;
    }
};

struct LufsParams {
    bool enable = false;
    float target = 0.0f;
    float max_gain = 0.0f; // dB
    int speed = 0;

    bool operator==(const LufsParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable && target == other.target
               && max_gain == other.max_gain && speed == other.speed;
    }
};

struct FetCompressorParams {
    bool enable = false;
    float threshold = 0.0f; // 0..1
    float ratio = 0.0f;     // 0..1
    float knee = 0.0f;      // 0..1
    bool knee_auto = false;
    float gain = 0.0f; // 0..1
    bool gain_auto = false;
    float attack = 0.0f; // 0..1
    bool attack_auto = false;
    float release = 0.0f; // 0..1
    bool release_auto = false;
    float knee_multi = 0.0f;
    float max_attack = 0.0f;
    float max_release = 0.0f;
    float crest = 0.0f;
    float adapt = 0.0f;
    bool no_clip = false;

    bool operator==(const FetCompressorParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable && threshold == other.threshold
               && ratio == other.ratio && knee == other.knee
               && knee_auto == other.knee_auto && gain == other.gain
               && gain_auto == other.gain_auto && attack == other.attack
               && attack_auto == other.attack_auto && release == other.release
               && release_auto == other.release_auto && knee_multi == other.knee_multi
               && max_attack == other.max_attack && max_release == other.max_release
               && crest == other.crest && adapt == other.adapt
               && no_clip == other.no_clip;
    }
};

struct BassParams {
    bool enable = false;
    int mode = 0;
    uint32_t frequency = 0; // Hz
    float gain = 0.0f;      // 0..1
    bool anti_pop = false;

    bool operator==(const BassParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable && mode == other.mode
               && frequency == other.frequency && gain == other.gain
               && anti_pop == other.anti_pop;
    }
};

struct BassMonoParams {
    bool enable = false;
    int mode = 0;
    uint32_t frequency = 0;
    float gain = 0.0f;
    bool anti_pop = false;

    bool operator==(const BassMonoParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable && mode == other.mode
               && frequency == other.frequency && gain == other.gain
               && anti_pop == other.anti_pop;
    }
};

struct PsychoacousticBassParams {
    bool enable = false;
    uint32_t cutoff = 0; // Hz
    uint32_t intensity = 0;
    uint32_t harmonic_order = 0;
    uint32_t original_level = 0;

    bool operator==(const PsychoacousticBassParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable && cutoff == other.cutoff
               && intensity == other.intensity && harmonic_order == other.harmonic_order
               && original_level == other.original_level;
    }
};

struct SpectrumExtensionParams {
    bool enable = false;
    int strength = 0;
    float exciter = 0.0f; // 0..1

    bool operator==(const SpectrumExtensionParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable && strength == other.strength
               && exciter == other.exciter;
    }
};

struct EqualizerParams {
    bool enable = false;
    uint32_t band_count = 0;
    std::array<float, 31> band_levels{};

    bool operator==(const EqualizerParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable && band_count == other.band_count
               && band_levels == other.band_levels;
    }
};

struct ConvolverParams {
    bool enable = false;
    float cross_channel = 0.0f; // 0..1

    bool operator==(const ConvolverParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable && cross_channel == other.cross_channel;
    }
};

using BiquadSection = std::array<float, 5>;

struct DdcParams {
    bool enable = false;

    bool operator==(const DdcParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable;
    }
};

struct FieldSurroundParams {
    bool enable = false;
    float widening = 0.0f;  // 0..1
    float mid_image = 0.0f; // 0..1
    short depth = 0;

    bool operator==(const FieldSurroundParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable && widening == other.widening
               && mid_image == other.mid_image && depth == other.depth;
    }
};

struct DiffSurroundParams {
    bool enable = false;
    float delay = 0.0f; // 0..1
    bool reverse = false;
    float wet_dry_mix = 0.0f; // 0..1
    float lp_cutoff = 0.0f;   // Hz

    bool operator==(const DiffSurroundParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable && delay == other.delay && reverse == other.reverse
               && wet_dry_mix == other.wet_dry_mix && lp_cutoff == other.lp_cutoff;
    }
};

struct StereoImagerParams {
    bool enable = false;
    float low_width = 0.0f;      // percent: 0..200
    float mid_width = 0.0f;      // percent: 0..200
    float high_width = 0.0f;     // percent: 0..200
    float low_crossover = 0.0f;  // Hz
    float high_crossover = 0.0f; // Hz

    bool operator==(const StereoImagerParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable && low_width == other.low_width
               && mid_width == other.mid_width && high_width == other.high_width
               && low_crossover == other.low_crossover
               && high_crossover == other.high_crossover;
    }
};

struct HeadphoneSurroundParams {
    bool enable = false;
    int quality = 0;

    bool operator==(const HeadphoneSurroundParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable && quality == other.quality;
    }
};

struct ReverbParams {
    bool enable = false;
    float room_size = 0.0f; // 0..1
    float width = 0.0f;     // 0..1
    float damp = 0.0f;      // 0..1
    float wet = 0.0f;       // 0..1
    float dry = 0.0f;       // 0..1

    bool operator==(const ReverbParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable && room_size == other.room_size
               && width == other.width && damp == other.damp && wet == other.wet
               && dry == other.dry;
    }
};

struct DynamicSystemParams {
    bool enable = false;
    int x_coeff_low = 0;
    int x_coeff_high = 0;
    int y_coeff_low = 0;
    int y_coeff_high = 0;
    float side_gain_low = 0.0f;  // 0..1
    float side_gain_high = 0.0f; // 0..1
    float strength = 0.0f;       // 0..1

    bool operator==(const DynamicSystemParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable && x_coeff_low == other.x_coeff_low
               && x_coeff_high == other.x_coeff_high && y_coeff_low == other.y_coeff_low
               && y_coeff_high == other.y_coeff_high
               && side_gain_low == other.side_gain_low
               && side_gain_high == other.side_gain_high && strength == other.strength;
    }
};

struct ClarityParams {
    bool enable = false;
    int mode = 0; // 0..1
    float gain = 0.0f;

    bool operator==(const ClarityParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable && mode == other.mode && gain == other.gain;
    }
};

struct CureParams {
    bool enable = false;
    int crossfeed_preset = 0; // 0..2

    bool operator==(const CureParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable && crossfeed_preset == other.crossfeed_preset;
    }
};

struct TubeSimulatorParams {
    bool enable = false;

    bool operator==(const TubeSimulatorParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable;
    }
};

struct AnalogXParams {
    bool enable = false;
    int mode = 0;

    bool operator==(const AnalogXParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable && mode == other.mode;
    }
};

struct SpeakerCorrectionParams {
    bool enable = false;

    bool operator==(const SpeakerCorrectionParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable;
    }
};

struct MultibandCompressorBandParams {
    bool enable = false;
    float threshold = 0.0f;
    float ratio = 0.0f;
    float knee = 0.0f;
    bool knee_auto = false;
    float gain = 0.0f;
    bool gain_auto = false;
    float attack = 0.0f;
    bool attack_auto = false;
    float release = 0.0f;
    bool release_auto = false;
    float knee_multi = 0.0f;
    float max_attack = 0.0f;
    float max_release = 0.0f;
    float crest = 0.0f;
    float adapt = 0.0f;
    bool no_clip = false;

    bool operator==(const MultibandCompressorBandParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable && threshold == other.threshold
               && ratio == other.ratio && knee == other.knee
               && knee_auto == other.knee_auto && gain == other.gain
               && gain_auto == other.gain_auto && attack == other.attack
               && attack_auto == other.attack_auto && release == other.release
               && release_auto == other.release_auto && knee_multi == other.knee_multi
               && max_attack == other.max_attack && max_release == other.max_release
               && crest == other.crest && adapt == other.adapt
               && no_clip == other.no_clip;
    }
};

struct MultibandCompressorParams {
    bool enable = false;
    uint32_t band_count = 0;
    std::array<float, 5> crossover_frequencies{}; // Hz
    std::array<MultibandCompressorBandParams, 5> bands{};

    bool operator==(const MultibandCompressorParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable && band_count == other.band_count
               && crossover_frequencies == other.crossover_frequencies
               && bands == other.bands;
    }
};

struct DynamicEqBandParams {
    float frequency = 0.0f; // Hz
    float q = 0.0f;
    float gain = 0.0f;      // dB
    float threshold = 0.0f; // dB
    float attack = 0.0f;    // ms
    float release = 0.0f;   // ms
    int filter_type = 0;

    bool operator==(const DynamicEqBandParams &other) const {
        return frequency == other.frequency && q == other.q && gain == other.gain
               && threshold == other.threshold && attack == other.attack
               && release == other.release && filter_type == other.filter_type;
    }
};

struct DynamicEqParams {
    bool enable = false;
    uint32_t band_count = 0;
    std::array<DynamicEqBandParams, 8> bands{};

    bool operator==(const DynamicEqParams &other) const {
        if (!enable && !other.enable) return true;
        return enable == other.enable && band_count == other.band_count
               && bands == other.bands;
    }
};

struct ViPERParams {
    MasterLimiterParams master_limiter;
    PlaybackGainControlParams playback_gain_control;
    LufsParams lufs;
    FetCompressorParams fet_compressor;
    BassParams bass;
    BassMonoParams bass_mono;
    PsychoacousticBassParams psychoacoustic_bass;
    SpectrumExtensionParams spectrum_extension;
    EqualizerParams equalizer;
    ConvolverParams convolver;
    DdcParams ddc;
    FieldSurroundParams field_surround;
    DiffSurroundParams diff_surround;
    StereoImagerParams stereo_imager;
    HeadphoneSurroundParams headphone_surround;
    ReverbParams reverb;
    DynamicSystemParams dynamic_system;
    ClarityParams clarity;
    CureParams cure;
    TubeSimulatorParams tube_simulator;
    AnalogXParams analog_x;
    SpeakerCorrectionParams speaker_correction;
    MultibandCompressorParams multiband_compressor;
    DynamicEqParams dynamic_eq;
};

} // namespace viper
