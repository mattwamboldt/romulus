#include <windows.h>
#include <xinput.h>

#include "nes\nes.h"

// TODO: New main objective - get parity with the console app
// Then APU since that's easier and will enable more tests, then the PPU

/* Current objective: Ability to run nestest visually
- Add text rendering
- Recreate debug views
- Add pattern table renderer, including ability to show and switch palettes
- Add a nametable render view that overlays a box based on current scroll position
  - This will help with making sure the lookups are correct and establish some util functions
- Do a performance pass once the general rendering is complete to make sure bit operations are good
- If nestest renders:
    - Add controller input for joysticks
    - Add DirectSound
    - Add APU
    - Tweak until nestest runs in full nametable mode
- Otherwise:
    - reevaluate this priority list
*/
/* Objectives:
- Run nestest correctly (Just the visible screen and fixing bugs)
- Run through the basic test roms
- Run NEStress
- Run through more complicated test roms
- Run an actual game
- GET CREATIVE!!! (Remove all green)
*/

// TODO: Better error handling/logging. For now using OutputDebugStringA everywhere

// AUDIO
// This audio implementation is initially going to use DirectSound for the low level playback
// I've used higher level libs in the past, but would rather have a better control and understanding
// of how these systems work. Using DSound before on another project was fairly straightforward so
// this will be an evolution of that code.

#include <dsound.h>

// TODO: Will pull this into a separate file/class maybe, later
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

// Dynamic Loading of XInput
typedef DWORD WINAPI XInputGetStateFn(DWORD dwUserIndex, XINPUT_STATE* pState);
typedef DWORD WINAPI XInputSetStateFn(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration);
static XInputGetStateFn* pfnXInputGetState;
static XInputSetStateFn* pfnXInputSetState;
#define XInputGetState pfnXInputGetState
#define XInputSetState pfnXInputSetState

DWORD WINAPI XInputGetStateStub(DWORD dwUserIndex, XINPUT_STATE* pState)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputSetStateStub(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

void LoadXInput()
{
    HMODULE xInput = LoadLibraryA("xinput1_3.dll");
    if (!xInput)
    {
        xInput = LoadLibraryA("xinput1_4.dll");
    }

    if (!xInput)
    {
        xInput = LoadLibraryA("xinput9_1_0.dll");
    }

    if (xInput)
    {
        pfnXInputGetState = (XInputGetStateFn*)GetProcAddress(xInput, "XInputGetState");
        pfnXInputSetState = (XInputSetStateFn*)GetProcAddress(xInput, "XInputSetState");
    }
    else
    {
        pfnXInputGetState = &XInputGetStateStub;
        pfnXInputSetState = &XInputSetStateStub;
    }
}

struct GDIBackBuffer
{
    BITMAPINFO info = {};
    void* memory;
    int width;
    int height;
    int pitch;
};

struct WindowSize
{
    int width;
    int height;
};

WindowSize GetWindowSize(HWND window)
{
    RECT clientRect;
    GetClientRect(window, &clientRect);

    WindowSize result;
    result.width = clientRect.right - clientRect.left;
    result.height = clientRect.bottom - clientRect.top;
    return result;
}

static bool isRunning = true;
static GDIBackBuffer globalBackBuffer = {};
static int64 cpuFreq = 1;

uint32 palette[] = {
    0x00545454, 0x00001E74, 0x00081090, 0x00300088, 0x00440064, 0x005C0030, 0x00540400, 0x003C1800, 0x00202A00, 0x00083A00, 0x00004000, 0x00003C00, 0x0000323C, 0x00000000, 0x00000000, 0x00000000,
    0x00989698, 0x00084CC4, 0x003032EC, 0x005C1EE4, 0x008814B0, 0x00A01464, 0x00982220, 0x00783C00, 0x00545A00, 0x00287200, 0x00087C00, 0x00007640, 0x00006678, 0x00000000, 0x00000000, 0x00000000,
    0x00ECEEEC, 0x004C9AEC, 0x00787CEC, 0x00B062EC, 0x00E454EC, 0x00EC58B4, 0x00EC6A64, 0x00D48820, 0x00A0AA00, 0x0074C400, 0x004CD020, 0x0038CC6C, 0x0038B4CC, 0x003C3C3C, 0x00000000, 0x00000000,
    0x00ECEEEC, 0x00A8CCEC, 0x00BCBCEC, 0x00D4B2EC, 0x00ECAEEC, 0x00ECAED4, 0x00ECB4B0, 0x00E4C490, 0x00CCD278, 0x00B4DE78, 0x00A8E290, 0x0098E2B4, 0x00A0D6E4, 0x00A0A2A0, 0x00000000, 0x00000000
};

void drawRect(GDIBackBuffer buffer, uint32 x, uint32 y, uint32 width, uint32 height, uint32 color)
{
    uint8* row = (uint8*)buffer.memory + (buffer.pitch * y);
    for (int yOffset = 0; yOffset < height; ++yOffset)
    {
        uint32* pixel = (uint32*)row + x;
        for (int xOffset = 0; xOffset < width; ++xOffset)
        {
            *pixel++ = color;
        }

        row += buffer.pitch;
    }
}

void drawPalettePixel(GDIBackBuffer buffer, uint32 x, uint32 y, uint8 index)
{
    uint8* row = (uint8*)buffer.memory + (buffer.pitch * y);
    uint32* pixel = (uint32*)row + x;
    *pixel = palette[index];
}

void drawNesPalette(GDIBackBuffer buffer, NES* nes, uint32 top, uint32 left, uint16 baseAddress)
{
    for (int paletteNum = 0; paletteNum < 16; paletteNum += 4)
    {
        for (int i = 0; i < 4; ++i)
        {
            uint8 paletteIndex = nes->ppuBus.read(baseAddress + paletteNum + i);
            drawRect(buffer, left + (i * 10), top, 10, 10, palette[paletteIndex]);
        }

        left += 45;
    }
}

void renderPatternTable(GDIBackBuffer buffer, NES* nes, uint32 top, uint32 left, uint8 selectedPalette)
{
    // TODO: Render the palettes
    // put a rect over the selected rect
    const uint16 paletteRamBase = 0x3F00;
    drawNesPalette(buffer, nes, top, left, paletteRamBase);
    top += 15;
    drawNesPalette(buffer, nes, top, left, paletteRamBase + 0x10);
    top += 15;

    uint16 patternAddress = 0x0000;

    uint8* row = (uint8*)buffer.memory + (buffer.pitch * top);
    uint32* pixel = ((uint32*)row) + left;

    uint32 x = left;
    uint32 y = top;

    for (int page = 0; page < 2; ++page)
    {
        for (int yTile = 0; yTile < 16; ++yTile)
        {
            for (int xTile = 0; xTile < 16; ++xTile)
            {
                for (int tileY = 0; tileY < 8; ++tileY)
                {
                    uint8 patfield01 = nes->ppuBus.read(patternAddress);
                    uint8 patfield02 = nes->ppuBus.read(patternAddress + 8);

                    for (int bit = 0; bit < 8; ++bit)
                    {
                        uint8 paletteOffset = (patfield01 & 0b10000000) >> 7;
                        paletteOffset |= (patfield02 & 0b10000000) >> 6;
                        patfield01 <<= 1;
                        patfield02 <<= 1;
                        uint8 color = nes->ppuBus.read(paletteRamBase + paletteOffset);
                        drawPalettePixel(buffer, x++, y, color);
                    }

                    ++patternAddress;
                    x -= 8;
                    ++y;
                }

                patternAddress += 8;
                y -= 8;
                x += 8;
            }

            x -= 128;
            y += 8;
        }

        y = top;
        x = left + 128;
    }
}

void render(GDIBackBuffer buffer, NES* nes)
{
    uint8* nesBuffer = nes->ppu.screenBuffer;
    uint8* row = (uint8*)buffer.memory;

    if (!nes->cpu.hasHalted())
    {
        for (int y = 0; y < NES_SCREEN_HEIGHT; ++y)
        {
            uint32* pixel = (uint32*)row;
            for (int x = 0; x < NES_SCREEN_WIDTH; ++x)
            {
                *pixel++ = palette[*nesBuffer++];
            }

            row += buffer.pitch;
        }
    }
    else
    {
        for (int y = 0; y < NES_SCREEN_HEIGHT; ++y)
        {
            uint32* pixel = (uint32*)row;
            for (int x = 0; x < NES_SCREEN_WIDTH; ++x)
            {
                *pixel++ = 0xFF0000FF;
            }

            row += buffer.pitch;
        }
    }
}

void resizeDIBSection(GDIBackBuffer* buffer, int width, int height)
{
    if (buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->width = width;
    buffer->height = height;

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = width;
    buffer->info.bmiHeader.biHeight = -height;
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;

    buffer->memory = VirtualAlloc(0, 4 * width * height, MEM_COMMIT, PAGE_READWRITE);
    buffer->pitch = buffer->width * 4;
}

void paintToWindow(HDC deviceContext, GDIBackBuffer buffer, int windowWidth, int windowHeight)
{
    StretchDIBits(deviceContext,
        0, 0, windowWidth, windowHeight,
        0, 0, buffer.width, buffer.height,
        buffer.memory, &buffer.info,
        DIB_RGB_COLORS, SRCCOPY);
}

LRESULT windowProc(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_SIZE:
        {
            
        }
        break;

        case WM_PAINT:
        {
            PAINTSTRUCT paint;
            HDC deviceContext = BeginPaint(window, &paint);
            EndPaint(window, &paint);
        }
        break;

        case WM_DESTROY:
        case WM_CLOSE:
            isRunning = false;
            break;

        default:
            return DefWindowProcA(window, msg, wParam, lParam);
    }

    return 0;
}

LARGE_INTEGER getClockTime()
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter;
}

real32 getSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end)
{
    int64 elapsed = end.QuadPart - start.QuadPart;
    return (real32)elapsed / (real32)cpuFreq;
}

int WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int showCmd)
{
    LoadXInput();

    LARGE_INTEGER counterFrequency;
    QueryPerformanceFrequency(&counterFrequency);
    cpuFreq = counterFrequency.QuadPart;

    WNDCLASSA windowClass = {};
    windowClass.style = CS_HREDRAW|CS_VREDRAW;
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIcon((HINSTANCE)NULL, IDI_APPLICATION);
    windowClass.lpszClassName = "ROMulusWndClass";
    windowClass.hCursor = LoadCursorA(instance, IDC_ARROW);

    UINT desiredSchedulerMS = 1;
    bool useSleep = timeBeginPeriod(desiredSchedulerMS) == TIMERR_NOERROR;

    if (!RegisterClassA(&windowClass))
    {
        return 0;
    }

    HWND window = CreateWindowExA(
        0,
        windowClass.lpszClassName,
        "ROMulus",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        0, 0, instance, 0
    );

    if (!window)
    {
        return 0;
    }

    resizeDIBSection(&globalBackBuffer, 720, 480);

    real32 framesPerSecond = 30.0f;
    real32 secondsPerFrame = 1.0f / framesPerSecond;

    AudioData audio = {};
    audio.samplesPerSecond = 48000;
    audio.numChannels = 2;
    audio.bytesPerSample = sizeof(int16);
    audio.bytesPerSecond = audio.samplesPerSecond * audio.bytesPerSample * audio.numChannels;
    audio.bufferSize = audio.bytesPerSecond; // only using a 1 second long buffer for now, but keeping this flexible
    audio.bytesPerFrame = (DWORD)((real32)audio.bytesPerSecond / framesPerSecond);
    audio.padding = (DWORD)((real32)audio.bytesPerFrame / 2.0f);

    // A generic buffer for the application to write audio into on update
    int16* samples = (int16*)VirtualAlloc(0, audio.bufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

    InitDirectSound(window, audio.samplesPerSecond, audio.bufferSize);

    LARGE_INTEGER frameTime = getClockTime();
    LARGE_INTEGER displayTime = getClockTime();

    uint32 frameCount = 0;
    bool soundIsValid = false;

    NES nes;
    nes.loadRom("data/nestest.nes");
    nes.cpu.instAddr = nes.cpu.pc = 0xC000;
    
    while (isRunning)
    {
        // Handle Input
        MSG msg;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                isRunning = false;
            }

            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        /*
        if (input == 'q')
        {
            isRunning = false;
        }

        if (input == 'd')
        {
            // debug.nextpage();
        }

        if (input == 'a')
        {
            // debug.prevpage();
        }

        if (input == 'c')
        {
            //debug.toggleAsciiMode();
        }

        if (input == 'p')
        {
            nes.toggleSingleStep();
        }

        if (input == ' ')
        {
            nes.singleStep();
        }

        if (input == '`')
        {
            nes.reset();
        }
        */

        for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
        {
            XINPUT_STATE controllerState;
            if (XInputGetState(i, &controllerState) == ERROR_SUCCESS)
            {
                // connected
                // TODO: handle packet number
                XINPUT_GAMEPAD pad = controllerState.Gamepad;

                bool up = pad.wButtons & XINPUT_GAMEPAD_DPAD_UP;
                bool down = pad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                bool left = pad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                bool right = pad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
                bool start = pad.wButtons & XINPUT_GAMEPAD_START;
                bool back = pad.wButtons & XINPUT_GAMEPAD_BACK;
                bool leftShoulder = pad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                bool rightShoulder = pad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
                bool aButton = pad.wButtons & XINPUT_GAMEPAD_A;
                bool bButton = pad.wButtons & XINPUT_GAMEPAD_B;
                bool xButton = pad.wButtons & XINPUT_GAMEPAD_X;
                bool yButton = pad.wButtons & XINPUT_GAMEPAD_Y;
            }
            else
            {
                // not connected
            }
        }

        // TODO: This constant frame time update means that we can't miss, or the whole thing will spiral, audio will get wonky, etc.
        // Run the sim frame and render
        nes.update(secondsPerFrame);

        render(globalBackBuffer, &nes);
        renderPatternTable(globalBackBuffer, &nes, 0, 300, 0);

        // TODO: FPS Counter
        // TODO: Memory/debugging view

        // Audio output (Initial implementation from another project, will tweak as we go)
        //
        // This approach runs in the main thread, but some systems use a callback that has no
        // concept of the "game" loop. They just keep asking for the next block when they need more.
        // Main thread is fine where we're doing so much synth but there may be a better way.
        LARGE_INTEGER preAudioTime = getClockTime();
        real32 preAudioElapsedSeconds = getSecondsElapsed(displayTime, preAudioTime);

        DWORD playCursor = 0;
        DWORD writeCursor = 0;
        if (directSoundOutputBuffer->GetCurrentPosition(&playCursor, &writeCursor) == DS_OK)
        {
            if (!soundIsValid)
            {
                audio.sampleIndex = writeCursor / (audio.bytesPerSample * audio.numChannels);
                soundIsValid = true;
            }

            real32 secondsUntilDisplay = secondsPerFrame - preAudioElapsedSeconds;
            DWORD bytesUntilDisplay = (DWORD)((secondsUntilDisplay / secondsPerFrame) * (real32)audio.bytesPerFrame);

            // Where we think the play cursor will be by the end of this frame
            DWORD displayCursor = playCursor + bytesUntilDisplay;

            DWORD safeWriteCursor = writeCursor;
            if (writeCursor < playCursor)
            {
                // We treat the cursor as if the buffer isn't circular, purely for comparison
                safeWriteCursor += audio.bufferSize;
            }

            safeWriteCursor += audio.padding;

            DWORD writeEndByte;
            if (safeWriteCursor < displayCursor)
            {
                // Gives us perfect audio sync if possible
                writeEndByte = displayCursor + audio.bytesPerFrame;
            }
            else
            {
                // safe end location on latent audio cards
                // TODO: This will result in us asking for more audio than we have generated from the emulator
                // It would be fine in a game engine where we can just grab more of sound effect or piece of music,
                // even in a normal synth where you can generate ahead and let a note change happen a frame later on input.
                // In this case the apu wouldn't have run yet to that point on the first frame for example.
                // NOTE: I think this is probably unique to an emulator, where the apu can change state mid frame.
                // Thinking on it even the sdl approach of grabbing chunks at a time would potentially overtake
                // the generated code. Might have to institute a delay/preamble of silence of some kind.
                // For now I'm just going to roll with this and see how bad the side effects are.
                writeEndByte = writeCursor + audio.bytesPerFrame + audio.padding;
            }

            writeEndByte %= audio.bufferSize;

            DWORD bytesToWrite = 0;

            DWORD byteToLock = (audio.sampleIndex * audio.bytesPerSample * audio.numChannels) % audio.bufferSize;
            if (byteToLock > writeEndByte)
            {
                bytesToWrite = (audio.bufferSize - byteToLock) + writeEndByte;
            }
            else
            {
                bytesToWrite = writeEndByte - byteToLock;
            }

            // Mix down application audio into a buffer
            nes.outputAudio(samples, bytesToWrite / audio.bytesPerSample);

            FillDirectSoundBuffer(&audio, byteToLock, bytesToWrite, samples);
        }
        else
        {
            soundIsValid = false;
        }
        
        // Sleep until the frame should display for proper frame pacing
        real32 frameElapsed = getSecondsElapsed(frameTime, getClockTime());
        if (frameElapsed < secondsPerFrame)
        {
            if (useSleep)
            {
                DWORD sleepMs = (DWORD)(1000.0f * (secondsPerFrame - frameElapsed));
                if (sleepMs > 0)
                {
                    Sleep(sleepMs);
                }
            }

            // NOTE: Sleep can only get ms accuracy, so we run a busy loop to eat up anything thats left
            // Try to find something less bad
            while (frameElapsed < secondsPerFrame)
            {
                frameElapsed = getSecondsElapsed(frameTime, getClockTime());
            }
        }
        else
        {
            OutputDebugStringA("FrameMissed\n");
        }

        frameTime = getClockTime();
        ++frameCount;

        // Swap Backbuffer / end frame
        HDC deviceContext = GetDC(window);
        WindowSize size = GetWindowSize(window);
        paintToWindow(deviceContext, globalBackBuffer, size.width, size.height);
        ReleaseDC(window, deviceContext);

        displayTime = getClockTime();
    }

    return 0;
}
