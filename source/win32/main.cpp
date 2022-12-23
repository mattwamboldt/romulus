#include <windows.h>
#include <xinput.h>
#include <resources/resource.h>

#include "../romulus/platform.h"

// TODO: Better error handling/logging. For now using OutputDebugStringA everywhere

// AUDIO
// This audio implementation is initially going to use DirectSound for the low level playback
// I've used higher level libs in the past, but would rather have a better control and understanding
// of how these systems work. Using DSound before on another project was fairly straightforward so
// this will be an evolution of that code.

// TODO: Handle window events to pause the dsound buffer and emulator execution when dragging the window

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

static HMENU mainMenu;
static WINDOWPLACEMENT windowPosition;

void toggleFullscreen(HWND window)
{
    // Taken from Handmade hero, which took it from Raymod Chen
    // https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353

    DWORD style = GetWindowLongA(window, GWL_STYLE);
    if (style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO monitorInfo = { sizeof(monitorInfo) };
        if (GetWindowPlacement(window, &windowPosition) &&
            GetMonitorInfoA(MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY), &monitorInfo))
        {
            SetWindowLongA(window, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(window, HWND_TOP,
                monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
                monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
            SetMenu(window, 0);
        }
    }
    else
    {
        SetWindowLongA(window, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(window, &windowPosition);
        SetWindowPos(window, 0, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
            SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        SetMenu(window, mainMenu);
    }
}

// NOTE: Handles the window messages that signal DefWndProc will block
// From: https://en.sfml-dev.org/forums/index.php?topic=2459.0
//
// Mentions these as pairs to detect (on/off), we only need the on:
// - WM_ENTERMENUMOVE / WM_EXITMENUMOVE
// - WM_NCLBUTTONDOWN / WM_CAPTURECHANGED
// - WM_ENTERSIZEMOVE / WM_EXITSIZEMOVE
bool windowLoopStalled = false;

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

        case WM_NCLBUTTONDOWN:
        case WM_ENTERSIZEMOVE:
        case WM_ENTERMENULOOP:
            // Make sure we don't get janky audio by detecting the pending stall
            windowLoopStalled = true;
            directSoundOutputBuffer->Stop();
            return DefWindowProcA(window, msg, wParam, lParam);

        case WM_COMMAND:
        {
            WPARAM command = wParam & 0xFFFF;
            if (command == MENU_FILE_OPEN)
            {
                // https://learn.microsoft.com/en-us/windows/win32/dlgbox/using-common-dialog-boxes#opening-a-file
                char filename[MAX_PATH];

                OPENFILENAMEA openFileDesc = {};
                openFileDesc.lStructSize = sizeof(openFileDesc);
                openFileDesc.lpstrFile = filename;
                openFileDesc.lpstrFile[0] = '\0'; // Docs make it seem if you use a path it'll go there
                openFileDesc.hwndOwner = window;
                openFileDesc.nMaxFile = sizeof(filename);
                openFileDesc.lpstrFilter = "NES files(*.nes,*.nsf)\0*.nes;*.nsf\0";
                openFileDesc.nFilterIndex = 1;
                openFileDesc.lpstrInitialDir = NULL;
                openFileDesc.lpstrFileTitle = NULL;
                openFileDesc.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

                if (GetOpenFileNameA(&openFileDesc))
                {
                    char windowTitle[64];
                    char* titleWriter = windowTitle;

                    for (int i = 0; i < sizeof("ROMulus"); ++i)
                    {
                        *titleWriter++ = "ROMulus"[i];
                    }

                    if (loadROM(filename))
                    {
                        char* start = filename;
                        char* iter = filename;
                        char* ext = filename;
                        while (*iter)
                        {
                            if (*iter == '\\')
                            {
                                start = iter + 1;
                            }
                            else if (*iter == '.')
                            {
                                ext = iter;
                            }

                            ++iter;
                        }

                        --titleWriter;
                        *titleWriter++ = ':';
                        *titleWriter++ = ' ';

                        while (start != ext && (titleWriter - windowTitle) < 62)
                        {
                            *titleWriter++ = *start++;
                        }

                        *titleWriter = '\0';
                    }

                    SetWindowTextA(window, windowTitle);
                }
            }
            else if (command == MENU_FILE_EXIT)
            {
                DestroyWindow(window);
            }
            else if (command == MENU_CONSOLE_RESET)
            {
                consoleReset();
            }
            else if (command == MENU_VIDEO_FULLSCREEN)
            {
                toggleFullscreen(window);
                // TODO: Also add hide/show menus for when we go fullscreen
            }
            else if (command == MENU_VIDEO_HIDEMENU)
            {
                HMENU currentMenu = GetMenu(window);
                if (currentMenu)
                {
                    SetMenu(window, 0);
                }
                else
                {
                    SetMenu(window, mainMenu);;
                }
            }
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
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = windowProc;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIconA(instance, MAKEINTRESOURCE(IDI_ICON1));
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
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
        0, 0, instance, 0
    );

    if (!window)
    {
        return 0;
    }

    mainMenu = LoadMenuA(instance, MAKEINTRESOURCE(IDR_MENU_MAIN));
    SetMenu(window, mainMenu);

    HACCEL acceleratorTable = LoadAcceleratorsA(instance, MAKEINTRESOURCE(IDR_ACCELERATOR_MAIN));

#if SHOW_DEBUG_VIEWS
    resizeDIBSection(&globalBackBuffer, 800, 600);
#else
    resizeDIBSection(&globalBackBuffer, 256, 240);
#endif

    real32 framesPerSecond = 60.0f;
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
    
    InputState input = {};
    
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

            if (!TranslateAcceleratorA(window, acceleratorTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        }

        if (windowLoopStalled)
        {
            frameTime = getClockTime();
            directSoundOutputBuffer->Play(0, 0, DSBPLAY_LOOPING);
            windowLoopStalled = false;
        }

        input.elapsedMs = secondsPerFrame;

        for (DWORD i = 0; i < 2; ++i)
        {
            GamePad* gamePad = input.controllers + i;

            XINPUT_STATE controllerState;
            if (XInputGetState(i, &controllerState) == ERROR_SUCCESS)
            {
                // connected
                // TODO: handle packet number
                gamePad->isConnected = true;
                gamePad->deviceId = i;

                XINPUT_GAMEPAD pad = controllerState.Gamepad;
                gamePad->upPressed = (pad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0;
                gamePad->downPressed = (pad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0;
                gamePad->leftPressed = (pad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0;
                gamePad->rightPressed = (pad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0;
                gamePad->startPressed = (pad.wButtons & XINPUT_GAMEPAD_START) != 0;
                gamePad->selectPressed = (pad.wButtons & XINPUT_GAMEPAD_BACK) != 0;
                gamePad->aPressed = (pad.wButtons & XINPUT_GAMEPAD_A) != 0;
                gamePad->bPressed = (pad.wButtons & XINPUT_GAMEPAD_B) != 0;
            }
            else
            {
                gamePad->isConnected = false;
            }
        }

        RECT clientRect;

        {
            HDC deviceContext = GetDC(window);
            GetClientRect(window, &clientRect);
            ReleaseDC(window, deviceContext);
        }

        POINT mousePosition;
        GetCursorPos(&mousePosition);
        ScreenToClient(window, &mousePosition);
        input.mouse.xPosition = (real32)mousePosition.x * ((real32)globalBackBuffer.width / (real32)clientRect.right);
        input.mouse.yPosition = (real32)mousePosition.y * ((real32)globalBackBuffer.height / (real32)clientRect.bottom);
        input.mouse.left.wasPressed = input.mouse.left.isPressed;
        input.mouse.left.isPressed = (GetKeyState(VK_LBUTTON) & (1 << 15)) != 0;
        input.mouse.right.wasPressed = input.mouse.left.isPressed;
        input.mouse.right.isPressed = (GetKeyState(VK_RBUTTON) & (1 << 15)) != 0;

        // TODO: This constant frame time update means that we can't miss, or the whole thing will spiral, audio will get wonky, etc.
        // Run the sim frame and render
        ScreenBuffer screen = {};
        screen.width = globalBackBuffer.width;
        screen.height = globalBackBuffer.height;
        screen.memory = globalBackBuffer.memory;
        screen.pitch = globalBackBuffer.pitch;

        updateAndRender(&input, screen);

        // TODO: FPS Counter
        // TODO: Memory/debugging view

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
                audio.sampleIndex = writeCursor / (audio.bytesPerSample * audio.numChannels);
                soundIsValid = true;
            }

            DWORD writeEndByte = writeCursor + audio.bytesPerFrame + audio.padding;
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
            // TODO: Pass in a generic audio spec/"device" so we can control things like sample rate
            // and such from the platform side
            outputAudio(samples, bytesToWrite / audio.bytesPerSample);

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

    consoleShutdown();
    return 0;
}
