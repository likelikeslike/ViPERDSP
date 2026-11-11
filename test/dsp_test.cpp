#include "viper/ViPER.h"
#include "viper/effects/SoftwareLimiter.h"
#include "viper/effects/TubeSimulator.h"
#include "viper/effects/ViPERBass.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void Check(bool condition, const std::string &message) {
    if (!condition) throw std::runtime_error(message);
}

void CheckNear(
    float actual, float expected, float tolerance, const std::string &message
) {
    if (std::fabs(actual - expected) > tolerance) {
        throw std::runtime_error(
            message + ": actual=" + std::to_string(actual)
            + " expected=" + std::to_string(expected)
        );
    }
}

std::vector<float> ProcessEngine(const std::vector<float> &input, uint32_t block_size) {
    ViPER engine;
    std::vector<float> output(input.size());
    const uint32_t frames = static_cast<uint32_t>(input.size() / 2);
    for (uint32_t offset = 0; offset < frames; offset += block_size) {
        const uint32_t count = std::min(block_size, frames - offset);
        std::vector<float> block(
            input.begin() + offset * 2, input.begin() + (offset + count) * 2
        );
        engine.Process(block, count);
        Check(block.size() == count * 2, "engine changed buffer size");
        std::copy(block.begin(), block.end(), output.begin() + offset * 2);
    }
    Check(engine.GetProcessedFrames() == frames, "engine frame counter is incorrect");
    return output;
}

void TestEngineBlocks() {
    constexpr uint32_t frames = 8192;
    std::vector<float> input(frames * 2);
    for (uint32_t i = 0; i < frames; ++i) {
        input[i * 2] = 0.2f * std::sin(0.031f * static_cast<float>(i));
        input[i * 2 + 1] = 0.15f * std::cos(0.019f * static_cast<float>(i));
    }

    const auto reference = ProcessEngine(input, 512);
    float energy = 0.0f;
    for (float sample : reference)
        energy += sample * sample;
    Check(energy > 1.0f, "engine produced no signal");

    for (uint32_t block_size : {1u, 64u, 511u, 2048u, 4096u}) {
        const auto output = ProcessEngine(input, block_size);
        for (size_t i = 0; i < output.size(); ++i) {
            Check(std::isfinite(output[i]), "engine produced a non-finite sample");
            CheckNear(
                output[i],
                reference[i],
                1e-6f,
                "engine output depends on block size at sample " + std::to_string(i)
            );
        }
    }
}

void TestEngineReset() {
    ViPER engine;
    std::vector<float> block(128 * 2, 0.0f);
    block[0] = 0.5f;
    engine.Process(block, 128);
    engine.RequestEffectsReset();
    std::fill(block.begin(), block.end(), 0.0f);
    engine.Process(block, 128);
    for (float sample : block) {
        CheckNear(sample, 0.0f, 0.0f, "effect reset left delayed audio");
    }
}

void TestLimiter() {
    SoftwareLimiter limiter;
    limiter.SetGate(0.5f);
    std::vector<float> output(1024);
    output[0] = limiter.Process(0.25f);
    for (size_t i = 1; i < output.size(); ++i)
        output[i] = limiter.Process(0.0f);
    for (size_t i = 0; i < output.size(); ++i) {
        CheckNear(
            output[i],
            i == 256 ? 0.25f : 0.0f,
            1e-6f,
            "limiter impulse response differs at sample " + std::to_string(i)
        );
    }

    limiter.Reset();
    for (uint32_t i = 0; i < 4096; ++i) {
        const float sample = limiter.Process(2.0f);
        Check(
            std::isfinite(sample) && std::fabs(sample) <= 0.500001f,
            "limiter exceeded its gate"
        );
    }
}

std::vector<float> ProcessTube(
    const std::vector<float> &input, int mode, uint32_t block_size
) {
    TubeSimulator tube;
    tube.SetSamplingRate(48000);
    tube.SetMode(mode);
    tube.SetTubeType(1);
    tube.SetDrive(75);
    tube.SetEnable(true);
    auto output = input;
    const uint32_t frames = static_cast<uint32_t>(output.size() / 2);
    for (uint32_t offset = 0; offset < frames; offset += block_size) {
        tube.Process(output.data() + offset * 2, std::min(block_size, frames - offset));
    }
    return output;
}

double ToneMagnitude(
    const std::vector<float> &samples, uint32_t start, uint32_t count, double frequency
) {
    constexpr double pi = 3.14159265358979323846;
    double real = 0.0;
    double imaginary = 0.0;
    for (uint32_t i = 0; i < count; ++i) {
        const double phase = 2.0 * pi * frequency * i / 48000.0;
        real += samples[(start + i) * 2] * std::cos(phase);
        imaginary -= samples[(start + i) * 2] * std::sin(phase);
    }
    return 2.0 * std::hypot(real, imaginary) / count;
}

void TestTube() {
    constexpr uint32_t frames = 48000;
    constexpr double pi = 3.14159265358979323846;
    std::vector<float> input(frames * 2, 0.0f);
    for (uint32_t i = 0; i < frames; ++i) {
        input[i * 2] = static_cast<float>(0.5 * std::sin(2.0 * pi * 100.0 * i / 48000.0));
    }

    for (int mode : {0, 1}) {
        const auto reference = ProcessTube(input, mode, 512);
        const auto single_frame = ProcessTube(input, mode, 1);
        for (size_t i = 0; i < reference.size(); i += 2) {
            Check(
                std::isfinite(reference[i]) && std::isfinite(reference[i + 1]),
                "tube produced a non-finite sample"
            );
            CheckNear(reference[i + 1], 0.0f, 1e-7f, "tube leaked into silent channel");
            CheckNear(
                single_frame[i], reference[i], 1e-6f, "tube output depends on block size"
            );
        }
        const double fundamental = ToneMagnitude(reference, 24000, 24000, 100.0);
        const double second_harmonic = ToneMagnitude(reference, 24000, 24000, 200.0);
        Check(
            fundamental > 0.1 && second_harmonic > fundamental * 1e-4,
            "tube did not produce an audible signal with harmonics"
        );
    }
}

double BassOutputRms(double frequency) {
    constexpr uint32_t frames = 48000;
    constexpr double pi = 3.14159265358979323846;
    ViPERBass bass;
    bass.SetSamplingRate(48000);
    bass.SetProcessMode(ViPERBass::NATURAL_BASS);
    bass.SetFrequency(80);
    bass.SetBassFactor(1.0f);
    bass.SetAntiPop(false);
    bass.SetEnable(true);

    double energy = 0.0;
    std::vector<float> block(512 * 2, 0.0f);
    for (uint32_t offset = 0; offset < frames; offset += 512) {
        const uint32_t count = std::min(512u, frames - offset);
        for (uint32_t i = 0; i < count; ++i) {
            block[i * 2] = static_cast<float>(
                0.1 * std::sin(2.0 * pi * frequency * (offset + i) / 48000.0)
            );
            block[i * 2 + 1] = 0.0f;
        }
        bass.Process(block.data(), count);
        for (uint32_t i = 0; i < count; ++i) {
            Check(std::isfinite(block[i * 2]), "bass produced a non-finite sample");
            CheckNear(block[i * 2 + 1], 0.0f, 1e-7f, "bass leaked into silent channel");
            if (offset + i >= frames / 2) {
                energy += static_cast<double>(block[i * 2]) * block[i * 2];
            }
        }
    }
    return std::sqrt(energy / (frames / 2));
}

void TestBass() {
    const double low = BassOutputRms(60.0);
    const double high = BassOutputRms(1000.0);
    Check(low > 0.08, "bass did not boost a low-frequency signal");
    Check(low > high * 1.2, "bass did not favor low frequencies");
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 2) {
        std::fprintf(stderr, "usage: viperdsp_test TEST_NAME\n");
        return 2;
    }

    try {
        const std::string name = argv[1];
        if (name == "engine_blocks")
            TestEngineBlocks();
        else if (name == "engine_reset")
            TestEngineReset();
        else if (name == "limiter")
            TestLimiter();
        else if (name == "tube")
            TestTube();
        else if (name == "bass")
            TestBass();
        else
            throw std::runtime_error("unknown test: " + name);
    } catch (const std::exception &error) {
        std::fprintf(stderr, "%s: %s\n", argv[1], error.what());
        return 1;
    }
    return 0;
}
