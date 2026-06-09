#pragma once

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
