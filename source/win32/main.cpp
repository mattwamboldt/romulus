#include <windows.h>
#include <resources/resource.h>

#include "../romulus/platform.h"
#include "input/xinput.h"
#include "sound/directsound.h"

// TODO: Better error handling/logging. For now using OutputDebugStringA everywhere

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

uint32 settingToMenuItem[] =
{
    ID_CONTROLLER1_KEYBOARD,
    ID_CONTROLLER1_GAMEPAD,
    ID_CONTROLLER1_ZAPPER,
    ID_CONTROLLER2_KEYBOARD,
    ID_CONTROLLER2_GAMEPAD,
    ID_CONTROLLER2_ZAPPER
};

void RecalculateSettings()
{
    CheckMenuItem(mainMenu, ID_CONTROLLER1_KEYBOARD, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(mainMenu, ID_CONTROLLER1_GAMEPAD, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(mainMenu, ID_CONTROLLER1_ZAPPER, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(mainMenu, ID_CONTROLLER2_KEYBOARD, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(mainMenu, ID_CONTROLLER2_GAMEPAD, MF_BYCOMMAND | MF_UNCHECKED);
    CheckMenuItem(mainMenu, ID_CONTROLLER2_ZAPPER, MF_BYCOMMAND | MF_UNCHECKED);

    uint8 controller1 = getMapping(0);
    CheckMenuItem(mainMenu, settingToMenuItem[controller1], MF_BYCOMMAND | MF_CHECKED);

    uint8 controller2 = getMapping(1) + 3;
    CheckMenuItem(mainMenu, settingToMenuItem[controller2], MF_BYCOMMAND | MF_CHECKED);
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
            PauseDirectSound();
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
                        EnableMenuItem(mainMenu, MENU_FILE_CLOSE, MF_BYCOMMAND | MF_ENABLED);
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
            else if (command == MENU_FILE_CLOSE)
            {
                unloadROM();
                SetWindowTextA(window, "ROMulus");
                EnableMenuItem(mainMenu, MENU_FILE_CLOSE, MF_BYCOMMAND | MF_GRAYED);
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
            else if (command == ID_CONTROLLER1_KEYBOARD)
            {
                setMapping(0, SOURCE_KEYBOARD);

                if (getMapping(1) == SOURCE_KEYBOARD)
                {
                    setMapping(1, SOURCE_GAMEPAD);
                }

                RecalculateSettings();
            }
            else if (command == ID_CONTROLLER1_GAMEPAD)
            {
                setMapping(0, SOURCE_GAMEPAD);

                if (getMapping(1) == SOURCE_GAMEPAD)
                {
                    setMapping(1, SOURCE_KEYBOARD);
                }

                RecalculateSettings();
            }
            else if (command == ID_CONTROLLER1_ZAPPER)
            {
                setMapping(0, SOURCE_ZAPPER);

                if (getMapping(1) == SOURCE_ZAPPER)
                {
                    setMapping(1, SOURCE_KEYBOARD);
                }

                RecalculateSettings();
            }
            else if (command == ID_CONTROLLER2_KEYBOARD)
            {
                setMapping(1, SOURCE_KEYBOARD);

                if (getMapping(0) == SOURCE_KEYBOARD)
                {
                    setMapping(0, SOURCE_GAMEPAD);
                }

                RecalculateSettings();
            }
            else if (command == ID_CONTROLLER2_GAMEPAD)
            {
                setMapping(1, SOURCE_GAMEPAD);

                if (getMapping(0) == SOURCE_GAMEPAD)
                {
                    setMapping(0, SOURCE_KEYBOARD);
                }

                RecalculateSettings();
            }
            else if (command == ID_CONTROLLER2_ZAPPER)
            {
                setMapping(1, SOURCE_ZAPPER);

                if (getMapping(0) == SOURCE_ZAPPER)
                {
                    setMapping(0, SOURCE_KEYBOARD);
                }

                RecalculateSettings();
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

    RecalculateSettings();

    LARGE_INTEGER frameTime = getClockTime();
    LARGE_INTEGER displayTime = getClockTime();

    uint32 frameCount = 0;
    
    InputState input = {};
    KeyboardEvent keyboardEvents[256];
    input.keyboardEvents = keyboardEvents;
    
    while (isRunning)
    {
        input.numKeyboardEvents = 0;

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
                if (msg.message == WM_KEYDOWN || msg.message == WM_KEYUP)
                {
                    bool wasDown = (msg.lParam & 0x40000000) != 0;
                    bool isDown = (msg.lParam & 0x80000000) == 0;
                    if (isDown != wasDown)
                    {
                        KeyboardEvent* keyEvent = input.keyboardEvents + input.numKeyboardEvents++;
                        keyEvent->isPress = !wasDown;
                        keyEvent->keycode = (uint8)msg.wParam;
                    }
                }
                else
                {
                    TranslateMessage(&msg);
                    DispatchMessageA(&msg);
                }
            }
        }

        if (windowLoopStalled)
        {
            frameTime = getClockTime();
            PlayDirectSound();
            windowLoopStalled = false;
        }

        input.elapsedMs = secondsPerFrame;

        UpdateXInputState(&input);

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

        // Run the sim frame and render
        ScreenBuffer screen = {};
        screen.width = globalBackBuffer.width;
        screen.height = globalBackBuffer.height;
        screen.memory = globalBackBuffer.memory;
        screen.pitch = globalBackBuffer.pitch;

        updateAndRender(&input, screen);

        UpdateDirectSound(&audio, samples, outputAudio);
        
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
