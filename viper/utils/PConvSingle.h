#pragma once

typedef struct PFFFT_Setup PFFFT_Setup;

class PConvSingle {
public:
    PConvSingle();
    ~PConvSingle();

    void Reset();

    int GetFFTSize();
    int GetSegmentCount();
    int GetSegmentSize();
    bool InstanceUsable();

    void Convolve(float *buffer);
    void ConvolveInterleaved(float *buffer, int channel);
    void ConvSegment(float *buffer, bool interleaved, int channel);

    int LoadKernel(const float *kernel, int kernelSize, int segmentSize);
    int LoadKernel(const float *kernel, float gain, int kernelSize, int segmentSize);

    int ProcessKernel(const float *kernel, int kernelSize, int unused);
    int ProcessKernel(const float *kernel, float gain, int kernelSize, int unused);

    void ReleaseResources();
    void UnloadKernel();

    bool instanceUsable;
    int segmentCount;
    int segmentSize;

private:
    int fftSize;
    PFFFT_Setup *fftSetup;
    float *fftWork;
    float **filterSegments;
    float **inputHistory;
    float *overlapBuffer;
    float *fftBuffer;
    float *accumBuffer;
    float *monoBuffer;
    int delayLineIndex;
};
