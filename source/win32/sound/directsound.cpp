#include "directsound.h"

// AUDIO
// This audio implementation is initially going to use DirectSound for the low level playback
// I've used higher level libs in the past, but would rather have a better control and understanding
// of how these systems work. Using DSound before on another project was fairly straightforward so
// this will be an evolution of that code.

#include <dsound.h>

static bool soundIsValid = false;

static LPDIRECTSOUNDBUFFER directSoundOutputBuffer;

typedef HRESULT WINAPI DirectSoundCreateFn(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter);

// TODO: There are examples around of how to do device enumeration, which would just involve calling a bunch of this same code with
// the guid of the selected device in our dsoundcreate function. Maybe look into that as a future feature
bool InitDirectSound(HWND window, DWORD samplesPerSecond, DWORD bufferSize)
{
    // We use the version of direct sound that's installed on the system, rather than link directly, more compatibility
    HMODULE directSoundLib = LoadLibraryA("dsound.dll");
    if (!directSoundLib)
    {
        OutputDebugStringA("[Audio::DirectSound] Failed to load dll\n");
        return false;
    }

    DirectSoundCreateFn* directSoundCreate = (DirectSoundCreateFn*)GetProcAddress(directSoundLib, "DirectSoundCreate");
    LPDIRECTSOUND directSound;
    if (!directSoundCreate || !SUCCEEDED(directSoundCreate(0, &directSound, 0)))
    {
        OutputDebugStringA("[Audio::DirectSound] Failed to create Primary Device object\n");
        return false;
    }

    WAVEFORMATEX waveFormat = {};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = 2;
    waveFormat.wBitsPerSample = 16;
    waveFormat.nSamplesPerSec = samplesPerSecond;
    waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

    // These chnages to the primary buffer shouldn't prevent playback as far as I understand.
    // They just set priority and prevent resampling that would cause performace issues if formats didn't match
    if (SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
    {
        DSBUFFERDESC bufferDesc = {};
        bufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
        bufferDesc.dwSize = sizeof(DSBUFFERDESC);

        LPDIRECTSOUNDBUFFER primaryBuffer;
        if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDesc, &primaryBuffer, 0)))
        {
            if (!SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
            {
                OutputDebugStringA("[Audio::DirectSound] Failed to set primary buffer format\n");
            }
        }
        else
        {
            OutputDebugStringA("[Audio::DirectSound] Failed to create primary buffer\n");
        }
    }
    else
    {
        OutputDebugStringA("[Audio::DirectSound] Failed to set cooperative level\n");
    }

    DSBUFFERDESC bufferDesc = {};
    bufferDesc.dwSize = sizeof(DSBUFFERDESC);
    bufferDesc.dwBufferBytes = bufferSize;
    bufferDesc.lpwfxFormat = &waveFormat;
    // TODO: Consider DSBCAPS_GETCURRENTPOSITION2 for more accurate cursor position, currently just using code I know works
    // TODO: Consider other flags that may improve quality of playback on some systems

    // We need this to succeed so that we have a buffer to write the program audio into
    if (!SUCCEEDED(directSound->CreateSoundBuffer(&bufferDesc, &directSoundOutputBuffer, 0)))
    {
        OutputDebugStringA("[Audio::DirectSound] Failed to create secondary buffer\n");
        return false;
    }

    // We need to clear the audio buffer so we don't get any weird sounds until we want sounds
    void* region1;
    DWORD region1Size;
    void* region2;
    DWORD region2Size;

    if (SUCCEEDED(directSoundOutputBuffer->Lock(0, bufferSize,
        &region1, &region1Size, &region2, &region2Size, 0)))
    {
        uint8* currentSample = (uint8*)region1;
        for (DWORD i = 0; i < region1Size; ++i)
        {
            *currentSample++ = 0;
        }

        // Realistically we're asking for the whole buffer so the first loop will catch everything
        // This is just here for converage/safety's sake
        currentSample = (uint8*)region2;
        for (DWORD i = 0; i < region2Size; ++i)
        {
            *currentSample++ = 0;
        }

        directSoundOutputBuffer->Unlock(region1, region1Size, region2, region2Size);
    }

    // This starts the background process and has to be called so we can stream into it later
    directSoundOutputBuffer->Play(0, 0, DSBPLAY_LOOPING);
    return true;
}

void PlayDirectSound()
{
    directSoundOutputBuffer->Play(0, 0, DSBPLAY_LOOPING);
}

void PauseDirectSound()
{
    directSoundOutputBuffer->Stop();
}

void FillDirectSoundBuffer(AudioData* audio, DWORD startPosition, DWORD size, int16* sourceBuffer)
{
    void* region1;
    DWORD region1Size;
    void* region2;
    DWORD region2Size;

    if (SUCCEEDED(directSoundOutputBuffer->Lock(
        startPosition, size,
        &region1, &region1Size,
        &region2, &region2Size,
        0)))
    {
        int16* sourceSample = sourceBuffer;
        int16* currentSample = (int16*)region1;
        DWORD region1SampleCount = region1Size / (audio->bytesPerSample * audio->numChannels);
        for (DWORD i = 0; i < region1SampleCount; ++i)
        {
            *(currentSample++) = *(sourceSample++); //LEFT channel
            *(currentSample++) = *(sourceSample++); //RIGHT channel
            ++(audio->sampleIndex);
        }

        currentSample = (int16*)region2;
        DWORD region2SampleCount = region2Size / (audio->bytesPerSample * audio->numChannels);
        for (DWORD i = 0; i < region2SampleCount; ++i)
        {
            *(currentSample++) = *(sourceSample++); //LEFT channel
            *(currentSample++) = *(sourceSample++); //RIGHT channel
            ++(audio->sampleIndex);
        }

        directSoundOutputBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

void UpdateDirectSound(AudioData* audioSpec, int16* sampleBuffer, SampleCallback getSamples)
{
    // Audio output (Initial implementation from another project, will tweak as we go)
    //
    // This approach runs in the main thread, but some systems use a callback that has no
    // concept of the "game" loop. They just keep asking for the next block when they need more.
    // Main thread is fine where we're doing so much synth but there may be a better way.

    DWORD playCursor = 0;
    DWORD writeCursor = 0;
    if (directSoundOutputBuffer->GetCurrentPosition(&playCursor, &writeCursor) == DS_OK)
    {
        if (!soundIsValid)
        {
            audioSpec->sampleIndex = writeCursor / (audioSpec->bytesPerSample * audioSpec->numChannels);
            soundIsValid = true;
        }

        DWORD writeEndByte = writeCursor + audioSpec->bytesPerFrame + audioSpec->padding;
        writeEndByte %= audioSpec->bufferSize;

        DWORD bytesToWrite = 0;
        DWORD byteToLock = (audioSpec->sampleIndex * audioSpec->bytesPerSample * audioSpec->numChannels) % audioSpec->bufferSize;
        if (byteToLock > writeEndByte)
        {
            bytesToWrite = (audioSpec->bufferSize - byteToLock) + writeEndByte;
        }
        else
        {
            bytesToWrite = writeEndByte - byteToLock;
        }

        // Mix down application audio into a buffer
        // TODO: Pass in a generic audio spec/"device" so we can control things like sample rate
        // and such from the platform side
        getSamples(sampleBuffer, bytesToWrite / audioSpec->bytesPerSample);

        FillDirectSoundBuffer(audioSpec, byteToLock, bytesToWrite, sampleBuffer);
    }
    else
    {
        soundIsValid = false;
    }
}