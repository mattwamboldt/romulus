#include "romulus.h"
#include "nes/nes.h"
#include "nes/input/inputBus.h"

// TODO: Allocate this properly
static NES nes;

// NOTE: for now all of these things are forwarded to the NES enumlator
// But having the application be a higher layer up will let other backends be
// used as they come online

void DEBUG_renderMouse(InputState* input, ScreenBuffer screen)
{
    // TEMP: Really bad code to draw a box on the cursor
    if (input->mouse.xPosition > 0 && input->mouse.yPosition > 0
        && input->mouse.xPosition < screen.width && input->mouse.yPosition < screen.height)
    {
        uint8* row = (uint8*)screen.memory + (screen.pitch * input->mouse.yPosition);
        for (int32 yOffset = 0; yOffset < 5; ++yOffset)
        {
            if (input->mouse.yPosition + yOffset >= screen.height)
            {
                break;
            }

            uint32* pixel = (uint32*)row + input->mouse.xPosition;
            for (int32 xOffset = 0; xOffset < 5; ++xOffset)
            {
                if (input->mouse.xPosition + xOffset >= screen.width)
                {
                    break;
                }

                if (input->mouse.left.isPressed)
                {
                    *pixel++ = 0xFF00FF00;
                }
                else
                {
                    *pixel++ = 0xFFFF0000;
                }
            }

            row += screen.pitch;
        }
    }
}

void updateAndRender(InputState* input, ScreenBuffer screen)
{
    // TODO: Handle Mouse for any custom UI that might be useful later

    nes.processInput(input);
    nes.update(input->elapsedMs);
    nes.render(screen);

    // DEBUG_renderMouse(input, screen);
    // TODO: FPS Counter
    // TODO: Memory/debugging view
}

bool loadROM(const char* filePath)
{
    return nes.loadRom(filePath);
}

void unloadROM()
{
    nes.unloadRom();
}

void consoleReset()
{
    nes.reset();
}

void outputAudio(int16* buffer, int numSamples)
{
    nes.outputAudio(buffer, numSamples);
}

void consoleShutdown()
{
    if (nes.isRunning)
    {
        nes.powerOff();
    }
}

InputType getMapping(int port)
{
    Port p = nes.inputBus.ports[port];
    if (p.device == ZAPPER)
    {
        return InputType::SOURCE_ZAPPER;
    }

    if (nes.inputBus.controllers[p.index].sourceDeviceId < 0)
    {
        return InputType::SOURCE_KEYBOARD;
    }

    return InputType::SOURCE_GAMEPAD;
}

void setMapping(int port, InputType type)
{
    if (type == SOURCE_ZAPPER)
    {
        nes.inputBus.ports[port].device = ZAPPER;
    }
    else
    {
        nes.inputBus.ports[port].device = STANDARD_CONTROLLER;
        if (type == SOURCE_KEYBOARD)
        {
            nes.inputBus.ports[port].index = 1;
        }
        else
        {
            nes.inputBus.ports[port].index = 0;
        }
    }
}
