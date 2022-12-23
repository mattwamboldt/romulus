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

// NOTE: Shouldn't shift around, but keep this in sync with GamePad::Buttons
WORD xinputToGenericButtonMap[GamePad::NUM_BUTTONS] =
{
    XINPUT_GAMEPAD_DPAD_UP,
    XINPUT_GAMEPAD_DPAD_DOWN,
    XINPUT_GAMEPAD_DPAD_LEFT,
    XINPUT_GAMEPAD_DPAD_RIGHT,
    XINPUT_GAMEPAD_START,
    XINPUT_GAMEPAD_BACK,
    XINPUT_GAMEPAD_A,
    XINPUT_GAMEPAD_B,
    XINPUT_GAMEPAD_X,
    XINPUT_GAMEPAD_Y,
    XINPUT_GAMEPAD_LEFT_SHOULDER,
    XINPUT_GAMEPAD_RIGHT_SHOULDER
};

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
            gamePad->deviceId = i;
            gamePad->isConnected = true;

            XINPUT_GAMEPAD pad = controllerState.Gamepad;

            for (int b = 0; b < GamePad::NUM_BUTTONS; ++b)
            {
                gamePad->buttons[b].wasPressed = gamePad->buttons[b].isPressed;
                gamePad->buttons[b].isPressed = (pad.wButtons & xinputToGenericButtonMap[b]) != 0;
            }
        }
        else
        {
            gamePad->isConnected = false;
        }
    }
}
