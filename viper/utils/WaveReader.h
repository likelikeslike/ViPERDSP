#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

struct WavData {
    float *samples;
    uint32_t frameCount;
    uint32_t channels;
    uint32_t sampleRate;
};

static bool ReadWavFile(const char *path, WavData *out) {
    if (path == nullptr || out == nullptr) return false;

    FILE *fp = fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }

    out->samples = nullptr;
    out->frameCount = 0;
    out->channels = 0;
    out->sampleRate = 0;

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

    uint32_t fmtSize;
    memcpy(&fmtSize, header + 16, 4);

    uint16_t audioFormat;
    memcpy(&audioFormat, header + 20, 2);

    uint16_t numChannels;
    memcpy(&numChannels, header + 22, 2);

    uint32_t sampleRate;
    memcpy(&sampleRate, header + 24, 4);

    uint16_t bitsPerSample;
    memcpy(&bitsPerSample, header + 34, 2);

    if (audioFormat != 1 && audioFormat != 3) {
        fclose(fp);
        return false;
    }

    bool isFloat = (audioFormat == 3);
    uint32_t bytesPerSample = bitsPerSample / 8;

    if (bytesPerSample == 0 || numChannels == 0) {
        fclose(fp);
        return false;
    }

    int32_t pos = 12 + 8 + (int32_t) fmtSize;
    fseek(fp, pos, SEEK_SET);

    uint8_t chunkHeader[8];
    uint32_t dataSize = 0;
    while (fread(chunkHeader, 1, 8, fp) == 8) {
        memcpy(&dataSize, chunkHeader + 4, 4);
        if (memcmp(chunkHeader, "data", 4) == 0) {
            break;
        }
        fseek(fp, dataSize, SEEK_CUR);
        dataSize = 0;
    }

    if (dataSize == 0) {
        fclose(fp);
        return false;
    }

    uint32_t totalSamples = dataSize / bytesPerSample;
    uint32_t frameCount = totalSamples / numChannels;

    float *samples = new (std::nothrow) float[totalSamples];
    if (samples == nullptr) {
        fclose(fp);
        return false;
    }

    if (isFloat && bitsPerSample == 32) {
        if (fread(samples, sizeof(float), totalSamples, fp) != totalSamples) {
            delete[] samples;
            fclose(fp);
            return false;
        }
    } else if (!isFloat && bitsPerSample == 16) {
        int16_t *tmp = new (std::nothrow) int16_t[totalSamples];
        if (tmp == nullptr) {
            delete[] samples;
            fclose(fp);
            return false;
        }
        if (fread(tmp, sizeof(int16_t), totalSamples, fp) != totalSamples) {
            delete[] tmp;
            delete[] samples;
            fclose(fp);
            return false;
        }
        for (uint32_t i = 0; i < totalSamples; i++) {
            samples[i] = (float) tmp[i] / 32768.0f;
        }
        delete[] tmp;
    } else if (!isFloat && bitsPerSample == 24) {
        uint8_t *tmp = new (std::nothrow) uint8_t[totalSamples * 3];
        if (tmp == nullptr) {
            delete[] samples;
            fclose(fp);
            return false;
        }
        if (fread(tmp, 3, totalSamples, fp) != totalSamples) {
            delete[] tmp;
            delete[] samples;
            fclose(fp);
            return false;
        }
        for (uint32_t i = 0; i < totalSamples; i++) {
            int32_t val =
                (tmp[i * 3] << 8) | (tmp[i * 3 + 1] << 16) | (tmp[i * 3 + 2] << 24);
            val >>= 8;
            samples[i] = (float) val / 8388608.0f;
        }
        delete[] tmp;
    } else if (!isFloat && bitsPerSample == 32) {
        int32_t *tmp = new (std::nothrow) int32_t[totalSamples];
        if (tmp == nullptr) {
            delete[] samples;
            fclose(fp);
            return false;
        }
        if (fread(tmp, sizeof(int32_t), totalSamples, fp) != totalSamples) {
            delete[] tmp;
            delete[] samples;
            fclose(fp);
            return false;
        }
        for (uint32_t i = 0; i < totalSamples; i++) {
            samples[i] = (float) tmp[i] / 2147483648.0f;
        }
        delete[] tmp;
    } else {
        delete[] samples;
        fclose(fp);
        return false;
    }

    fclose(fp);

    out->samples = samples;
    out->frameCount = frameCount;
    out->channels = numChannels;
    out->sampleRate = sampleRate;
    return true;
}
