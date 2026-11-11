#include "viper/utils/AdaptiveBuffer.h"
#include "viper/utils/Biquad.h"
#include "viper/utils/CAllPassFilter.h"
#include "viper/utils/CCombFilter.h"
#include "viper/utils/CRevModel.h"
#include "viper/utils/Crc32.h"
#include "viper/utils/Crossfeed.h"
#include "viper/utils/DepthSurround.h"
#include "viper/utils/DynamicBass.h"
#include "viper/utils/FIR.h"
#include "viper/utils/Harmonic.h"
#include "viper/utils/HiFi.h"
#include "viper/utils/HighShelf.h"
#include "viper/utils/IIR_1st.h"
#include "viper/utils/IIR_NOrder_BW_BP.h"
#include "viper/utils/IIR_NOrder_BW_LH.h"
#include "viper/utils/MinPhaseIIRCoeffs.h"
#include "viper/utils/MultiBiquad.h"
#include "viper/utils/NoiseSharpening.h"
#include "viper/utils/PConvSingle.h"
#include "viper/utils/PassFilter.h"
#include "viper/utils/PolesFilter.h"
#include "viper/utils/Polyphase.h"
#include "viper/utils/QuadricTube.h"
#include "viper/utils/Stereo3DSurround.h"
#include "viper/utils/Subwoofer.h"
#include "viper/utils/TimeConstDelay.h"
#include "viper/utils/WavReader.h"
#include "viper/utils/WaveBuffer.h"
#include "viper/utils/pffft.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr uint32_t kFrames = 8192;

void Check(bool condition, const std::string &message) {
    if (!condition) throw std::runtime_error(message);
}

void Near(double actual, double expected, double tolerance, const std::string &message) {
    Check(std::isfinite(actual) && std::fabs(actual - expected) <= tolerance, message);
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
void CheckStereoChange(const std::string &name, Process process) {
    const auto input = Signal();
    auto output = input;
    for (uint32_t offset = 0; offset < kFrames; offset += 512) {
        process(output.data() + offset * 2, 512);
    }
    double difference = 0.0;
    double energy = 0.0;
    for (size_t i = 0; i < output.size(); ++i) {
        Check(std::isfinite(output[i]), name + " produced a non-finite sample");
        const double delta = static_cast<double>(output[i]) - input[i];
        difference += delta * delta;
        energy += static_cast<double>(output[i]) * output[i];
    }
    Check(difference > 1e-7 && energy > 1e-7, name + " did not process the signal");
}

template <typename Process>
void CheckMonoChange(const std::string &name, Process process) {
    double difference = 0.0;
    double energy = 0.0;
    for (uint32_t i = 0; i < kFrames; ++i) {
        const double input = 0.3 * std::sin(0.017 * i);
        const double output = process(input);
        Check(std::isfinite(output), name + " produced a non-finite sample");
        difference += (output - input) * (output - input);
        energy += output * output;
    }
    Check(difference > 1e-7 && energy > 1e-7, name + " did not process the signal");
}

void TestAdaptiveBuffer() {
    AdaptiveBuffer buffer(2, 2);
    const float input[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    Check(buffer.PushFrames(input, 2) == 1, "AdaptiveBuffer push failed");
    buffer.PanFrames(0.5f, 1.0f);
    buffer.ScaleFrames(2.0f);
    float output[4] = {};
    Check(buffer.PopFrames(output, 2) == 1, "AdaptiveBuffer pop failed");
    Near(output[0], 1.0, 0.0, "AdaptiveBuffer left gain incorrect");
    Near(output[1], 4.0, 0.0, "AdaptiveBuffer right gain incorrect");
    Check(buffer.GetBufferOffset() == 0, "AdaptiveBuffer offset incorrect");
    Check(
        buffer.PushZero(3) == 1 && buffer.GetBufferLength() >= 3,
        "AdaptiveBuffer growth failed"
    );
    buffer.FlushBuffer();
    Check(buffer.GetBufferOffset() == 0, "AdaptiveBuffer flush failed");
}

void TestWaveBuffer() {
    WaveBuffer buffer(2, 1);
    const float input[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    Check(buffer.PushSamples(input, 2) == 1, "WaveBuffer push failed");
    Check(buffer.GetBufferSize() >= 2, "WaveBuffer did not grow");
    float output[2] = {};
    Check(buffer.PopSamples(output, 1, false) == 1, "WaveBuffer pop failed");
    Near(output[0], 1.0, 0.0, "WaveBuffer first channel incorrect");
    Near(buffer.GetBuffer()[0], 3.0, 0.0, "WaveBuffer did not shift remaining data");
    buffer.Reset();
    Check(buffer.GetBufferOffset() == 0, "WaveBuffer reset failed");
}

void TestFIR() {
    FIR filter;
    const float coefficients[2] = {0.5f, 0.5f};
    Check(filter.LoadCoefficients(coefficients, 2, 4) == 1, "FIR load failed");
    float samples[8] = {1.0f, 7.0f, 0.0f, 7.0f, 0.0f, 7.0f, 0.0f, 7.0f};
    filter.FilterSamplesInterleaved(samples, 4, 2);
    Near(samples[0], 0.5, 1e-6, "FIR first tap incorrect");
    Near(samples[2], 0.5, 1e-6, "FIR second tap incorrect");
    Near(samples[1], 7.0, 0.0, "FIR changed another channel");
}

void TestPConvSingle() {
    PConvSingle filter;
    std::array<float, 16> kernel{};
    kernel[0] = 0.5f;
    kernel[1] = 0.5f;
    Check(
        filter.LoadKernel(kernel.data(), kernel.size(), 64) != 0,
        "PConvSingle rejected a valid kernel"
    );
    std::vector<float> samples(256, 0.0f);
    samples[0] = 1.0f;
    filter.Convolve(samples.data(), samples.size());
    Near(samples[0], 0.5, 1e-4, "PConvSingle first tap incorrect");
    Near(samples[1], 0.5, 1e-4, "PConvSingle second tap incorrect");
    filter.UnloadKernel();
    Check(!filter.InstanceUsable(), "PConvSingle unload failed");
}

void TestTimeConstDelay() {
    TimeConstDelay delay;
    delay.SetParameters(1000, 0.004f);
    Near(delay.ProcessSample(1.0f), 0.0, 0.0, "delay returned input early");
    for (int i = 0; i < 3; ++i) {
        Near(delay.ProcessSample(0.0f), 0.0, 0.0, "delay returned input early");
    }
    Near(delay.ProcessSample(0.0f), 1.0, 0.0, "delay returned wrong sample");
}

void TestCAllPassFilter() {
    CAllPassFilter filter;
    float buffer[2] = {};
    filter.SetBuffer(buffer, 2);
    filter.SetFeedback(0.0f);
    Near(filter.Process(1.0f), -1.0, 0.0, "all-pass direct path incorrect");
    Near(filter.Process(0.0f), 0.0, 0.0, "all-pass delay incorrect");
    Near(filter.Process(0.0f), 1.0, 0.0, "all-pass delayed path incorrect");
}

void TestCCombFilter() {
    CCombFilter filter;
    float buffer[2] = {};
    filter.SetBuffer(buffer, 2);
    filter.SetFeedback(0.0f);
    filter.SetDamp(0.0f);
    Near(filter.Process(1.0f), 0.0, 0.0, "comb returned input early");
    Near(filter.Process(0.0f), 0.0, 0.0, "comb returned input early");
    Near(filter.Process(0.0f), 1.0, 0.0, "comb delayed path incorrect");
}

void TestCRevModel() {
    CRevModel model;
    model.SetWet(0.5f);
    model.SetDry(0.0f);
    CheckStereoChange("CRevModel", [&](float *data, uint32_t size) {
        model.ProcessReplace(data, data + 1, size);
    });
}

void TestCrc32() {
    constexpr uint8_t data[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
    Check(Crc32(data, sizeof(data)) == 0xcbf43926u, "CRC32 standard vector failed");
}

void TestWavReader() {
    const auto directory = std::filesystem::current_path() / "tmp" / "test";
    std::filesystem::create_directories(directory);
    const auto path = directory / "wav_reader.wav";
    std::array<uint8_t, 48> data{};
    std::memcpy(data.data(), "RIFF", 4);
    std::memcpy(data.data() + 8, "WAVEfmt ", 8);
    std::memcpy(data.data() + 36, "data", 4);
    const auto put16 = [&](size_t offset, uint16_t value) {
        data[offset] = static_cast<uint8_t>(value);
        data[offset + 1] = static_cast<uint8_t>(value >> 8);
    };
    const auto put32 = [&](size_t offset, uint32_t value) {
        for (size_t i = 0; i < 4; ++i)
            data[offset + i] = static_cast<uint8_t>(value >> (8 * i));
    };
    put32(4, 40);
    put32(16, 16);
    put16(20, 1);
    put16(22, 1);
    put32(24, 48000);
    put32(28, 96000);
    put16(32, 2);
    put16(34, 16);
    put32(40, 4);
    put16(44, 16384);
    put16(46, 32768);
    {
        std::ofstream output(path, std::ios::binary);
        output.write(reinterpret_cast<const char *>(data.data()), data.size());
    }
    WavData wav{};
    const bool loaded = ReadWavFile(path.string().c_str(), &wav);
    std::filesystem::remove(path);
    Check(loaded, "WavReader rejected valid PCM data");
    Check(
        wav.channels == 1 && wav.frame_count == 2 && wav.sample_rate == 48000,
        "WavReader metadata incorrect"
    );
    Near(wav.samples[0], 0.5, 1e-6, "WavReader first sample incorrect");
    Near(wav.samples[1], -1.0, 1e-6, "WavReader second sample incorrect");
    delete[] wav.samples;
}

void TestPffft() {
    constexpr int size = 64;
    PFFFT_Setup *setup = pffft_new_setup(size, PFFFT_REAL);
    Check(setup != nullptr, "pffft setup failed");
    auto *input = static_cast<float *>(pffft_aligned_malloc(size * sizeof(float)));
    auto *spectrum = static_cast<float *>(pffft_aligned_malloc(size * sizeof(float)));
    auto *output = static_cast<float *>(pffft_aligned_malloc(size * sizeof(float)));
    Check(
        input != nullptr && spectrum != nullptr && output != nullptr,
        "pffft aligned allocation failed"
    );
    std::fill(input, input + size, 0.0f);
    input[3] = 1.0f;
    pffft_transform(setup, input, spectrum, nullptr, PFFFT_FORWARD);
    pffft_transform(setup, spectrum, output, nullptr, PFFFT_BACKWARD);
    const float actual = output[3] / size;
    const float other = output[4] / size;
    pffft_aligned_free(input);
    pffft_aligned_free(spectrum);
    pffft_aligned_free(output);
    pffft_destroy_setup(setup);
    Near(actual, 1.0, 1e-5, "pffft round trip lost impulse");
    Near(other, 0.0, 1e-5, "pffft round trip leaked impulse");
}

void Run(const std::string &name) {
    if (name == "AdaptiveBuffer") {
        TestAdaptiveBuffer();
    } else if (name == "Biquad") {
        Biquad filter;
        filter.SetLowPassParameter(1000.0f, 48000, 0.707f);
        CheckMonoChange(name, [&](double x) { return filter.ProcessSample(x); });
    } else if (name == "CAllPassFilter") {
        TestCAllPassFilter();
    } else if (name == "CCombFilter") {
        TestCCombFilter();
    } else if (name == "CRevModel") {
        TestCRevModel();
    } else if (name == "Crc32") {
        TestCrc32();
    } else if (name == "Crossfeed") {
        Crossfeed filter;
        filter.Reset();
        CheckStereoChange(name, [&](float *data, uint32_t size) {
            filter.ProcessFrames(data, size);
        });
    } else if (name == "DepthSurround") {
        DepthSurround filter;
        filter.SetStrength(600);
        CheckStereoChange(name, [&](float *data, uint32_t size) {
            filter.Process(data, size);
        });
    } else if (name == "DynamicBass") {
        DynamicBass filter;
        filter.SetBassGain(2.0f);
        CheckStereoChange(name, [&](float *data, uint32_t size) {
            filter.FilterSamples(data, size);
        });
    } else if (name == "FIR") {
        TestFIR();
    } else if (name == "Harmonic") {
        Harmonic filter;
        const float coefficients[10] = {0.5f, 0.5f};
        filter.SetHarmonics(coefficients);
        CheckMonoChange(name, [&](double x) { return filter.Process(x); });
    } else if (name == "HiFi") {
        HiFi filter;
        filter.SetClarity(0.8f);
        CheckStereoChange(name, [&](float *data, uint32_t size) {
            filter.Process(data, size);
        });
    } else if (name == "HighShelf") {
        HighShelf filter;
        filter.SetFrequency(1000.0f);
        filter.SetGain(2.0f);
        filter.SetSamplingRate(48000);
        CheckMonoChange(name, [&](double x) { return filter.Process(x); });
    } else if (name == "IIR_1st") {
        IIR_1st filter;
        filter.SetLowPassFilterBW(1000.0f, 48000);
        CheckMonoChange(name, [&](double x) { return Filter(&filter, x); });
    } else if (name == "IIR_NOrder_BW_BP") {
        IIR_NOrder_BW_BP filter(3);
        filter.SetBPF(100.0f, 2000.0f, 48000);
        CheckMonoChange(name, [&](double x) { return FilterBP(&filter, x); });
    } else if (name == "IIR_NOrder_BW_LH") {
        IIR_NOrder_BW_LH filter(3);
        filter.SetLPF(1000.0f, 48000);
        CheckMonoChange(name, [&](double x) { return FilterLH(filter, x); });
    } else if (name == "MinPhaseIIRCoeffs") {
        MinPhaseIIRCoeffs filter;
        Check(filter.UpdateCoeffs(10, 48000) == 1, "coefficient update failed");
        Check(filter.GetCoefficients() != nullptr, "coefficient array missing");
        Near(
            filter.GetIndexFrequency(5),
            1000.0,
            0.0,
            "coefficient center frequency incorrect"
        );
        for (uint32_t i = 0; i < 40; ++i) {
            Check(
                std::isfinite(filter.GetCoefficients()[i]), "coefficient is non-finite"
            );
        }
    } else if (name == "MultiBiquad") {
        MultiBiquad filter;
        filter.RefreshFilter(MultiBiquad::LOW_PASS, 0.0f, 1000.0f, 48000, 0.707f, false);
        CheckMonoChange(name, [&](double x) { return filter.ProcessSample(x); });
    } else if (name == "NoiseSharpening") {
        NoiseSharpening filter;
        filter.SetGain(0.8f);
        CheckStereoChange(name, [&](float *data, uint32_t size) {
            filter.Process(data, size);
        });
    } else if (name == "PConvSingle") {
        TestPConvSingle();
    } else if (name == "PassFilter") {
        PassFilter filter;
        CheckStereoChange(name, [&](float *data, uint32_t size) {
            filter.ProcessFrames(data, size);
        });
    } else if (name == "PolesFilter") {
        PolesFilter filter;
        filter.SetPassFilter(100, 8000);
        double energy = 0.0;
        for (uint32_t i = 0; i < kFrames; ++i) {
            float low, high, band;
            filter.FilterLeft(i == 0 ? 1.0f : 0.0f, &low, &high, &band);
            Check(
                std::isfinite(low) && std::isfinite(high) && std::isfinite(band),
                "PolesFilter produced non-finite output"
            );
            energy += low * low + high * high + band * band;
        }
        Check(energy > 1e-6, "PolesFilter lost the impulse");
    } else if (name == "Polyphase") {
        Polyphase filter(2);
        std::vector<float> samples(1008 * 2, 0.0f);
        samples[0] = 1.0f;
        Check(
            filter.Process(samples.data(), 1008) == 1008,
            "Polyphase returned wrong frame count"
        );
        double energy = 0.0;
        for (float sample : samples) {
            Check(std::isfinite(sample), "Polyphase produced non-finite output");
            energy += sample * sample;
        }
        Check(energy > 1e-7 && filter.GetLatency() > 0, "Polyphase lost the impulse");
    } else if (name == "QuadricTube") {
        QuadricTube filter;
        filter.Configure(1, 0.75);
        CheckMonoChange(name, [&](double x) { return filter.Process(x); });
    } else if (name == "Stereo3DSurround") {
        Stereo3DSurround filter;
        filter.SetStereoWiden(0.5f);
        CheckStereoChange(name, [&](float *data, uint32_t size) {
            filter.Process(data, size);
        });
    } else if (name == "Subwoofer") {
        Subwoofer filter;
        filter.SetBassGain(48000, 2.0f);
        CheckStereoChange(name, [&](float *data, uint32_t size) {
            filter.Process(data, size);
        });
    } else if (name == "TimeConstDelay") {
        TestTimeConstDelay();
    } else if (name == "WaveBuffer") {
        TestWaveBuffer();
    } else if (name == "WavReader") {
        TestWavReader();
    } else if (name == "pffft") {
        TestPffft();
    } else {
        throw std::runtime_error("missing util test: " + name);
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
