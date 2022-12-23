#pragma once
#include <windows.h>
#include "../../romulus/platform.h"

struct AudioData
{
    int samplesPerSecond;
    int bytesPerSample;
    int numChannels;
    uint32 sampleIndex;
    DWORD bytesPerSecond;
    DWORD bytesPerFrame;
    DWORD bufferSize;
    DWORD padding;
};

bool InitDirectSound(HWND window, DWORD samplesPerSecond, DWORD bufferSize);
void PlayDirectSound();
void PauseDirectSound();

typedef void (*SampleCallback)(int16*, int);

void UpdateDirectSound(AudioData* audioSpec, int16* sampleBuffer, SampleCallback getSamples);
