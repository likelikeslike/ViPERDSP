#include "PConvSingle.h"
#include "pffft.h"
#include <cstdlib>
#include <cstring>

PConvSingle::PConvSingle() {
    this->instanceUsable = false;
    this->segmentCount = 0;
    this->segmentSize = 0;
    this->fftSize = 0;
    this->fftSetup = nullptr;
    this->fftWork = nullptr;
    this->filterSegments = nullptr;
    this->inputHistory = nullptr;
    this->overlapBuffer = nullptr;
    this->fftBuffer = nullptr;
    this->accumBuffer = nullptr;
    this->monoBuffer = nullptr;
    this->delayLineIndex = 0;
}

PConvSingle::~PConvSingle() {
    ReleaseResources();
}

void PConvSingle::Convolve(float *buffer) {
    ConvSegment(buffer, false, 0);
}

void PConvSingle::ConvolveInterleaved(float *buffer, int channel) {
    ConvSegment(buffer, true, channel);
}

void PConvSingle::ConvSegment(float *buffer, bool interleaved, int channel) {
    if (!this->instanceUsable) return;

    float *input;
    if (interleaved) {
        for (int i = 0; i < this->segmentSize; i++) {
            this->monoBuffer[i] = buffer[i * 2 + channel];
        }
        input = this->monoBuffer;
    } else {
        input = buffer;
    }

    // Overlap-save: form [previous_overlap | current_input] in fftBuffer
    memcpy(this->fftBuffer, this->overlapBuffer, this->segmentSize * sizeof(float));
    memcpy(this->fftBuffer + this->segmentSize, input, this->segmentSize * sizeof(float));

    // Save current input as overlap for next call
    memcpy(this->overlapBuffer, input, this->segmentSize * sizeof(float));

    // Forward FFT the combined buffer into the current delay line slot
    pffft_transform(
        this->fftSetup,
        this->fftBuffer,
        this->inputHistory[this->delayLineIndex],
        this->fftWork,
        PFFFT_FORWARD
    );

    // Frequency-domain multiply-accumulate across all kernel segments
    memset(this->accumBuffer, 0, this->fftSize * sizeof(float));
    for (int k = 0; k < this->segmentCount; k++) {
        int idx = (this->delayLineIndex - k + this->segmentCount) % this->segmentCount;
        pffft_zconvolve_accumulate(
            this->fftSetup,
            this->inputHistory[idx],
            this->filterSegments[k],
            this->accumBuffer,
            1.0f
        );
    }

    // Inverse FFT
    pffft_transform(
        this->fftSetup, this->accumBuffer, this->fftBuffer, this->fftWork, PFFFT_BACKWARD
    );

    // Scale by 1/N (pffft does not normalize)
    float scale = 1.0f / (float) this->fftSize;
    for (int i = 0; i < this->fftSize; i++) {
        this->fftBuffer[i] *= scale;
    }

    // Output the second half (valid overlap-save region)
    float *output = this->fftBuffer + this->segmentSize;

    if (interleaved) {
        for (int i = 0; i < this->segmentSize; i++) {
            buffer[i * 2 + channel] = output[i];
        }
    } else {
        memcpy(buffer, output, this->segmentSize * sizeof(float));
    }

    // Advance delay line ring buffer
    this->delayLineIndex = (this->delayLineIndex + 1) % this->segmentCount;
}

int PConvSingle::GetFFTSize() {
    return this->segmentSize * 2;
}

int PConvSingle::GetSegmentCount() {
    return this->segmentCount;
}

int PConvSingle::GetSegmentSize() {
    return this->segmentSize;
}

bool PConvSingle::InstanceUsable() {
    return this->instanceUsable;
}

int PConvSingle::LoadKernel(const float *kernel, int kernelSize, int segmentSize) {
    if (kernel != nullptr && kernelSize >= 2 && segmentSize >= 2
        && (segmentSize & (segmentSize - 1)) == 0) {
        this->instanceUsable = false;
        ReleaseResources();
        this->segmentSize = segmentSize;
        int n = ProcessKernel(kernel, kernelSize, 1);
        if (n != 0) {
            this->instanceUsable = true;
            return n;
        }
        ReleaseResources();
    }
    return 0;
}

int PConvSingle::LoadKernel(
    const float *kernel, float gain, int kernelSize, int segmentSize
) {
    if (kernel != nullptr && kernelSize >= 2 && segmentSize >= 2
        && (segmentSize & (segmentSize - 1)) == 0) {
        this->instanceUsable = false;
        ReleaseResources();
        this->segmentSize = segmentSize;
        int n = ProcessKernel(kernel, gain, kernelSize, 1);
        if (n != 0) {
            this->instanceUsable = true;
            return n;
        }
        ReleaseResources();
    }
    return 0;
}

int PConvSingle::ProcessKernel(const float *kernel, int kernelSize, int unused) {
    this->fftSize = this->segmentSize * 2;
    this->segmentCount = (kernelSize + this->segmentSize - 1) / this->segmentSize;

    this->fftSetup = pffft_new_setup(this->fftSize, PFFFT_REAL);
    if (this->fftSetup == nullptr) return 0;

    this->fftWork = (float *) pffft_aligned_malloc(this->fftSize * sizeof(float));
    this->fftBuffer = (float *) pffft_aligned_malloc(this->fftSize * sizeof(float));
    this->accumBuffer = (float *) pffft_aligned_malloc(this->fftSize * sizeof(float));
    this->overlapBuffer =
        (float *) pffft_aligned_malloc(this->segmentSize * sizeof(float));
    this->monoBuffer = (float *) pffft_aligned_malloc(this->segmentSize * sizeof(float));

    if (!this->fftWork || !this->fftBuffer || !this->accumBuffer || !this->overlapBuffer
        || !this->monoBuffer) {
        return 0;
    }

    memset(this->overlapBuffer, 0, this->segmentSize * sizeof(float));
    memset(this->monoBuffer, 0, this->segmentSize * sizeof(float));

    this->filterSegments = new float *[this->segmentCount];
    this->inputHistory = new float *[this->segmentCount];
    for (int i = 0; i < this->segmentCount; i++) {
        this->filterSegments[i] =
            (float *) pffft_aligned_malloc(this->fftSize * sizeof(float));
        this->inputHistory[i] =
            (float *) pffft_aligned_malloc(this->fftSize * sizeof(float));
        memset(this->inputHistory[i], 0, this->fftSize * sizeof(float));
    }

    // Split kernel into segments, zero-pad each to fftSize, and forward FFT
    for (int i = 0; i < this->segmentCount; i++) {
        memset(this->fftBuffer, 0, this->fftSize * sizeof(float));
        int offset = i * this->segmentSize;
        int remaining = kernelSize - offset;
        int count = remaining < this->segmentSize ? remaining : this->segmentSize;
        memcpy(this->fftBuffer, kernel + offset, count * sizeof(float));
        pffft_transform(
            this->fftSetup,
            this->fftBuffer,
            this->filterSegments[i],
            this->fftWork,
            PFFFT_FORWARD
        );
    }

    this->delayLineIndex = 0;
    return this->segmentCount;
}

int PConvSingle::ProcessKernel(
    const float *kernel, float gain, int kernelSize, int unused
) {
    // Scale kernel by gain factor before processing
    float *scaled = (float *) pffft_aligned_malloc(kernelSize * sizeof(float));
    if (!scaled) return 0;
    for (int i = 0; i < kernelSize; i++) {
        scaled[i] = kernel[i] * gain;
    }
    int result = ProcessKernel(scaled, kernelSize, unused);
    pffft_aligned_free(scaled);
    return result;
}

void PConvSingle::ReleaseResources() {
    if (this->filterSegments != nullptr) {
        for (int i = 0; i < this->segmentCount; i++) {
            pffft_aligned_free(this->filterSegments[i]);
        }
        delete[] this->filterSegments;
        this->filterSegments = nullptr;
    }

    if (this->inputHistory != nullptr) {
        for (int i = 0; i < this->segmentCount; i++) {
            pffft_aligned_free(this->inputHistory[i]);
        }
        delete[] this->inputHistory;
        this->inputHistory = nullptr;
    }

    if (this->overlapBuffer != nullptr) {
        pffft_aligned_free(this->overlapBuffer);
        this->overlapBuffer = nullptr;
    }

    if (this->fftBuffer != nullptr) {
        pffft_aligned_free(this->fftBuffer);
        this->fftBuffer = nullptr;
    }

    if (this->accumBuffer != nullptr) {
        pffft_aligned_free(this->accumBuffer);
        this->accumBuffer = nullptr;
    }

    if (this->monoBuffer != nullptr) {
        pffft_aligned_free(this->monoBuffer);
        this->monoBuffer = nullptr;
    }

    if (this->fftWork != nullptr) {
        pffft_aligned_free(this->fftWork);
        this->fftWork = nullptr;
    }

    if (this->fftSetup != nullptr) {
        pffft_destroy_setup(this->fftSetup);
        this->fftSetup = nullptr;
    }

    this->instanceUsable = false;
    this->segmentCount = 0;
    this->segmentSize = 0;
    this->fftSize = 0;
    this->delayLineIndex = 0;
}

void PConvSingle::Reset() {
    if (!this->instanceUsable) return;

    for (int i = 0; i < this->segmentCount; i++) {
        memset(this->inputHistory[i], 0, this->fftSize * sizeof(float));
    }
    memset(this->overlapBuffer, 0, this->segmentSize * sizeof(float));
    this->delayLineIndex = 0;
}

void PConvSingle::UnloadKernel() {
    this->instanceUsable = false;
    ReleaseResources();
}
