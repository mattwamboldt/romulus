#include "romulus.h"
#include "nes/nes.h"

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
        for (uint32 yOffset = 0; yOffset < 5; ++yOffset)
        {
            if (input->mouse.yPosition + yOffset >= screen.height)
            {
                break;
            }

            uint32* pixel = (uint32*)row + input->mouse.xPosition;
            for (uint32 xOffset = 0; xOffset < 5; ++xOffset)
            {
                if (input->mouse.xPosition + xOffset >= screen.width)
                {
                    break;
                }

                if (input->mouse.leftPressed)
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

void updateAndRender(real32 secondsElapsed, InputState* input, ScreenBuffer screen)
{
    // TODO: Handle Mouse for any custom UI that might be useful later

    nes.processInput(input);
    nes.update(secondsElapsed);
    nes.render(screen);

    // DEBUG_renderMouse(input, screen);
}

bool loadROM(const char* filePath)
{
    return nes.loadRom(filePath);
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
