#include "viper/effects/AnalogX.h"
#include "viper/effects/ColorfulMusic.h"
#include "viper/effects/Convolver.h"
#include "viper/effects/Cure.h"
#include "viper/effects/DiffSurround.h"
#include "viper/effects/DynamicEQ.h"
#include "viper/effects/DynamicSystem.h"
#include "viper/effects/FETCompressor.h"
#include "viper/effects/IIRFilter.h"
#include "viper/effects/LUFSTargeting.h"
#include "viper/effects/MultibandCompressor.h"
#include "viper/effects/PlaybackGain.h"
#include "viper/effects/PsychoacousticBass.h"
#include "viper/effects/Reverberation.h"
#include "viper/effects/SoftwareLimiter.h"
#include "viper/effects/SpeakerCorrection.h"
#include "viper/effects/SpectrumExtend.h"
#include "viper/effects/StereoImager.h"
#include "viper/effects/TubeSimulator.h"
#include "viper/effects/VHE.h"
#include "viper/effects/ViPERBass.h"
#include "viper/effects/ViPERBassMono.h"
#include "viper/effects/ViPERClarity.h"
#include "viper/effects/ViPERDDC.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr uint32_t kFrames = 24000;
constexpr uint32_t kBlock = 512;

void Check(bool condition, const std::string &message) {
    if (!condition) throw std::runtime_error(message);
}

std::vector<float> Signal() {
    std::vector<float> signal(kFrames * 2);
    for (uint32_t i = 0; i < kFrames; ++i) {
        signal[i * 2] = 0.15f * std::sin(0.019f * i) + 0.04f * std::sin(0.37f * i);
        signal[i * 2 + 1] = 0.11f * std::cos(0.023f * i);
    }
    return signal;
}

template <typename Process>
void RunBlocks(std::vector<float> &buffer, Process process) {
    for (uint32_t offset = 0; offset < kFrames; offset += kBlock) {
        process(buffer.data() + offset * 2, std::min(kBlock, kFrames - offset));
    }
}

void CheckOutput(
    const std::string &name,
    const std::vector<float> &input,
    const std::vector<float> &output
) {
    double difference = 0.0;
    double energy = 0.0;
    for (size_t i = 0; i < output.size(); ++i) {
        Check(std::isfinite(output[i]), name + " produced a non-finite sample");
        const double delta = static_cast<double>(output[i]) - input[i];
        difference += delta * delta;
        energy += static_cast<double>(output[i]) * output[i];
    }
    Check(difference > 1e-5, name + " did not change the signal");
    Check(energy > 1e-5, name + " produced silence");
}

template <typename Effect, typename Configure>
void Exercise(const std::string &name, Configure configure) {
    Effect effect;
    effect.SetEnable(false);
    const auto input = Signal();
    auto bypass = input;
    RunBlocks(bypass, [&](float *data, uint32_t size) { effect.Process(data, size); });
    Check(bypass == input, name + " changed the signal while disabled");

    configure(effect);
    effect.SetEnable(true);
    auto output = input;
    RunBlocks(output, [&](float *data, uint32_t size) { effect.Process(data, size); });
    CheckOutput(name, input, output);
}

void TestConvolver() {
    Convolver effect;
    std::array<float, 16> kernel{};
    kernel[0] = 0.5f;
    kernel[1] = 0.5f;
    effect.SetKernel(kernel.data(), kernel.size());
    effect.SetEnable(true);
    const auto input = Signal();
    auto output = input;
    RunBlocks(output, [&](float *data, uint32_t size) {
        Check(effect.Process(data, data, size) == size, "Convolver frame count changed");
    });
    CheckOutput("Convolver", input, output);
}

void TestVhe() {
    VHE effect;
    effect.SetSamplingRate(48000);
    effect.SetEffectLevel(0);
    effect.SetEnable(true);
    const auto input = Signal();
    auto output = input;
    RunBlocks(output, [&](float *data, uint32_t size) {
        Check(effect.Process(data, data, size) == size, "VHE frame count changed");
    });
    CheckOutput("VHE", input, output);
}

void TestLimiter() {
    SoftwareLimiter effect;
    effect.SetGate(0.5f);
    for (uint32_t i = 0; i < 1024; ++i) {
        const float output = effect.Process(2.0f);
        Check(
            std::isfinite(output) && std::fabs(output) <= 0.500001f,
            "SoftwareLimiter exceeded its gate"
        );
    }
}

void Run(const std::string &name) {
    if (name == "AnalogX") {
        Exercise<AnalogX>(name, [](auto &effect) { effect.SetProcessingModel(1); });
    } else if (name == "ColorfulMusic") {
        Exercise<ColorfulMusic>(name, [](auto &effect) {
            effect.SetDepthValue(50);
            effect.SetWidenValue(0.6f);
        });
    } else if (name == "Convolver") {
        TestConvolver();
    } else if (name == "Cure") {
        Exercise<Cure>(name, [](auto &effect) { effect.SetPreset(1); });
    } else if (name == "DiffSurround") {
        Exercise<DiffSurround>(name, [](auto &effect) {
            effect.SetDelayTime(10.0f);
            effect.SetWetDryMix(0.5f);
        });
    } else if (name == "DynamicEQ") {
        Exercise<DynamicEQ>(name, [](auto &effect) {
            effect.SetBandCount(1);
            effect.SetBandFrequency(0, 1000.0f);
            effect.SetBandQ(0, 1.0f);
            effect.SetBandGain(0, 9.0f);
            effect.SetBandThreshold(0, -60.0f);
        });
    } else if (name == "DynamicSystem") {
        Exercise<DynamicSystem>(name, [](auto &effect) {
            effect.SetBassGain(1.0f);
            effect.SetXCoeffs(50, 180);
            effect.SetYCoeffs(60, 200);
            effect.SetSideGain(1.0f, 1.0f);
        });
    } else if (name == "FETCompressor") {
        Exercise<FETCompressor>(name, [](auto &effect) {
            effect.SetThreshold(0.5f);
            effect.SetRatio(0.8f);
        });
    } else if (name == "IIRFilter") {
        IIRFilter effect(10);
        effect.SetEnable(false);
        auto input = Signal();
        auto bypass = input;
        RunBlocks(bypass, [&](float *data, uint32_t size) {
            effect.Process(data, size);
        });
        Check(bypass == input, "IIRFilter changed the signal while disabled");
        effect.SetBandLevel(0, 9.0f);
        effect.SetEnable(true);
        RunBlocks(input, [&](float *data, uint32_t size) { effect.Process(data, size); });
        CheckOutput(name, bypass, input);
    } else if (name == "LUFSTargeting") {
        Exercise<LUFSTargeting>(name, [](auto &effect) {
            effect.SetTargetLUFS(-24.0f);
            effect.SetMaxGain(6.0f);
        });
    } else if (name == "MultibandCompressor") {
        Exercise<MultibandCompressor>(name, [](auto &effect) {
            effect.SetBandCount(2);
            effect.SetCrossoverFrequency(0, 400.0f);
            effect.SetBandEnable(0, true);
            effect.SetBandThreshold(0, 0.5f);
            effect.SetBandRatio(0, 0.8f);
        });
    } else if (name == "PlaybackGain") {
        Exercise<PlaybackGain>(name, [](auto &effect) {
            effect.SetRatio(0.8f);
            effect.SetVolume(0.5f);
        });
    } else if (name == "PsychoacousticBass") {
        Exercise<PsychoacousticBass>(name, [](auto &effect) {
            effect.SetCutoff(150);
            effect.SetIntensity(80);
        });
    } else if (name == "Reverberation") {
        Exercise<Reverberation>(name, [](auto &effect) {
            effect.SetWet(0.5f);
            effect.SetDry(0.5f);
        });
    } else if (name == "SoftwareLimiter") {
        TestLimiter();
    } else if (name == "SpeakerCorrection") {
        Exercise<SpeakerCorrection>(name, [](auto &) {});
    } else if (name == "SpectrumExtend") {
        Exercise<SpectrumExtend>(name, [](auto &effect) { effect.SetExciter(1.0f); });
    } else if (name == "StereoImager") {
        Exercise<StereoImager>(name, [](auto &effect) {
            effect.SetLowWidth(0.3f);
            effect.SetMidWidth(0.3f);
            effect.SetHighWidth(0.3f);
        });
    } else if (name == "TubeSimulator") {
        Exercise<TubeSimulator>(name, [](auto &effect) {
            effect.SetDrive(0.75f);
            effect.SetMix(1.0f);
        });
    } else if (name == "VHE") {
        TestVhe();
    } else if (name == "ViPERBass") {
        Exercise<ViPERBass>(name, [](auto &effect) {
            effect.SetFrequency(80);
            effect.SetBassFactor(1.0f);
            effect.SetAntiPop(false);
        });
    } else if (name == "ViPERBassMono") {
        Exercise<ViPERBassMono>(name, [](auto &effect) {
            effect.SetFrequency(80);
            effect.SetBassFactor(1.0f);
            effect.SetAntiPop(false);
        });
    } else if (name == "ViPERClarity") {
        Exercise<ViPERClarity>(name, [](auto &effect) {
            effect.SetProcessMode(ViPERClarity::OZONE);
            effect.SetClarityGain(0.8f);
        });
    } else if (name == "ViPERDDC") {
        constexpr float coefficients[5] = {0.5f, 0.0f, 0.0f, 0.0f, 0.0f};
        Exercise<ViPERDDC>(name, [&](auto &effect) {
            effect.SetCoeffs(5, coefficients, coefficients);
        });
    } else {
        throw std::runtime_error("missing effect test: " + name);
    }
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    try {
        Run(argv[1]);
    } catch (const std::exception &error) {
        std::fprintf(stderr, "%s: %s\n", argv[1], error.what());
        return 1;
    }
    return 0;
}
