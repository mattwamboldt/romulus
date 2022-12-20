#include "platform.h"
#include "nes/nes.h"

// TODO: Allocate this properly
static NES nes;

// NOTE: for now all of these things are forwarded to the NES enumlator
// But having the application be a higher layer up will let other backends be
// used as they come online

void updateAndRender(real32 secondsElapsed, InputState* input, ScreenBuffer screen)
{
    // TODO: Handle Mouse for any custom UI that might be useful later

    if (nes.isRunning)
    {
        // TODO: Come up with some kind of mechanism for custom input mapping that doesn't suck
        // TODO: Handle keyboard
        // TODO: Handle Mouse for Zapper
        for (int i = 0; i < 2; ++i)
        {
            nes.setGamepadState(input->controllers[i], i);
        }
    }

    nes.update(secondsElapsed);
    nes.render(screen);
}

void loadROM(const char* filePath)
{
    nes.loadRom(filePath);
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
