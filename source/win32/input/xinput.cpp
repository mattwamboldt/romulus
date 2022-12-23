#include <windows.h>
#include <xinput.h>
#include "xinput.h"

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

void UpdateXInputState(InputState* inputState)
{
    for (DWORD i = 0; i < 2; ++i)
    {
        GamePad* gamePad = inputState->controllers + i;

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
}
