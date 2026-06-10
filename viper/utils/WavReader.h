#pragma once

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>

struct WavData {
    float *samples;
    uint32_t frame_count;
    uint32_t channels;
    uint32_t sample_rate;
};

static bool ReadWavFile(const char *path, WavData *out) {
    if (path == nullptr || out == nullptr) return false;

    FILE *fp = fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }

    out->samples = nullptr;
    out->frame_count = 0;
    out->channels = 0;
    out->sample_rate = 0;

    uint8_t header[44];
    if (fread(header, 1, 44, fp) != 44) {
        fclose(fp);
        return false;
    }

    if (memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0) {
        fclose(fp);
        return false;
    }

    if (memcmp(header + 12, "fmt ", 4) != 0) {
        fclose(fp);
        return false;
    }

    uint32_t fmt_size;
    memcpy(&fmt_size, header + 16, 4);

    uint16_t audio_format;
    memcpy(&audio_format, header + 20, 2);

    uint16_t num_channels;
    memcpy(&num_channels, header + 22, 2);

    uint32_t sample_rate;
    memcpy(&sample_rate, header + 24, 4);

    uint16_t bits_per_sample;
    memcpy(&bits_per_sample, header + 34, 2);

    if (audio_format != 1 && audio_format != 3) {
        fclose(fp);
        return false;
    }

    const bool is_float = (audio_format == 3);
    const uint32_t bytes_per_sample = bits_per_sample / 8;

    if (bytes_per_sample == 0 || num_channels == 0) {
        fclose(fp);
        return false;
    }

    const int32_t pos = 12 + 8 + static_cast<int32_t>(fmt_size);
    fseek(fp, pos, SEEK_SET);

    uint8_t chunk_header[8];
    uint32_t data_size = 0;
    while (fread(chunk_header, 1, 8, fp) == 8) {
        memcpy(&data_size, chunk_header + 4, 4);
        if (memcmp(chunk_header, "data", 4) == 0) {
            break;
        }
        fseek(fp, data_size, SEEK_CUR);
        data_size = 0;
    }

    if (data_size == 0) {
        fclose(fp);
        return false;
    }

    const uint32_t total_samples = data_size / bytes_per_sample;
    const uint32_t frame_count = total_samples / num_channels;

    const auto samples = new (std::nothrow) float[total_samples];
    if (samples == nullptr) {
        fclose(fp);
        return false;
    }

    if (is_float && bits_per_sample == 32) {
        if (fread(samples, sizeof(float), total_samples, fp) != total_samples) {
            delete[] samples;
            fclose(fp);
            return false;
        }
    } else if (!is_float && bits_per_sample == 16) {
        const auto tmp = new (std::nothrow) int16_t[total_samples];
        if (tmp == nullptr) {
            delete[] samples;
            fclose(fp);
            return false;
        }
        if (fread(tmp, sizeof(int16_t), total_samples, fp) != total_samples) {
            delete[] tmp;
            delete[] samples;
            fclose(fp);
            return false;
        }
        for (uint32_t i = 0; i < total_samples; i++) {
            samples[i] = static_cast<float>(tmp[i]) / 32768.0f;
        }
        delete[] tmp;
    } else if (!is_float && bits_per_sample == 24) {
        const auto tmp = new (std::nothrow) uint8_t[total_samples * 3];
        if (tmp == nullptr) {
            delete[] samples;
            fclose(fp);
            return false;
        }
        if (fread(tmp, 3, total_samples, fp) != total_samples) {
            delete[] tmp;
            delete[] samples;
            fclose(fp);
            return false;
        }
        for (uint32_t i = 0; i < total_samples; i++) {
            int32_t val =
                (tmp[i * 3] << 8) | (tmp[i * 3 + 1] << 16) | (tmp[i * 3 + 2] << 24);
            val >>= 8;
            samples[i] = static_cast<float>(val) / 8388608.0f;
        }
        delete[] tmp;
    } else if (!is_float && bits_per_sample == 32) {
        const auto tmp = new (std::nothrow) int32_t[total_samples];
        if (tmp == nullptr) {
            delete[] samples;
            fclose(fp);
            return false;
        }
        if (fread(tmp, sizeof(int32_t), total_samples, fp) != total_samples) {
            delete[] tmp;
            delete[] samples;
            fclose(fp);
            return false;
        }
        for (uint32_t i = 0; i < total_samples; i++) {
            samples[i] = static_cast<float>(tmp[i]) / 2147483648.0f;
        }
        delete[] tmp;
    } else {
        delete[] samples;
        fclose(fp);
        return false;
    }

    fclose(fp);

    out->samples = samples;
    out->frame_count = frame_count;
    out->channels = num_channels;
    out->sample_rate = sample_rate;
    return true;
}
