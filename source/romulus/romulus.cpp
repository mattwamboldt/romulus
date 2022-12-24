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

// Evrything here is debug so definitely some platform specifics that should get pulled out
#include <intrin.h>

// Only go 64 layers deep on timers, avoid recursion
// TODO: Make this system more robust, Can use aggregation for repeat runs through blocks and the like
// The perf monitoring in visual studio leaves a lot to be desired. only gives some rough "relative" metrics
// so I want something specific. Probably make this more of a tree than a stack. works for general use for now
static TimerBlock timerStack[64];
static uint32 timerCount = 0;

// NOTE: Timers are an area where you can use object constructor destructor pairs with scopes to
// get possible wins, but begin end function pairs feels a bit better because its explicit
void beginTimer(const char* blockName)
{
    timerStack[timerCount].name = blockName;
    timerStack[timerCount].cycleCount = __rdtsc();
    ++timerCount;
}

void endTimer()
{
    --timerCount;
    uint64 cycleCount = __rdtsc() - timerStack[timerCount].cycleCount;
    logInfo("[DBG] %s took %I64u\n", timerStack[timerCount].name, cycleCount);
}
