#include "nes.h"
#include "cpuTrace.h"
#include <string.h>

#include "../wavefile.h"

const int32 masterClockHz = 21477272;

NES::NES()
{
    cpu.connect(&cpuBus);
    ppu.connect(&ppuBus);
    cpuBus.connect(&ppu, &apu, &cartridge);
    ppuBus.connect(&ppu, &cartridge);
    traceEnabled = true;
}

void NES::loadRom(const char * path)
{
    cartridge.load(path);

    // TODO: Temp puuting this here for debugging apu output, don't worry about it
    openStream("bin/apu_raw.wav", 1, 48000);

    if (isRunning)
    {
        reset();
    }
    else
    {
        powerOn();
    }

    // NOTE: If checking for NSF on every rts becomes a perf issue, then pull this out, but should be fine.
    if (cartridge.isNSF)
    {
        real32 playPeriod = cartridge.playSpeed / 1000000.0f;
        totalPlayCycles = (int32)(playPeriod * masterClockHz);

        nsfSentinal = cpu.stack;
        cpu.jumpSubroutine(cartridge.initAddress);
    }
}

void NES::powerOn()
{
    cpu.start();
    isRunning = true;
}

void NES::reset()
{
    cpu.reset();
    isRunning = true;
}

void NES::powerOff()
{
    cpu.stop();
    isRunning = false;

    flushLog();

    write(apuBuffer, writeHead);
    finalizeStream();
}

void NES::update(real32 secondsPerFrame)
{
    if (!isRunning)
    {
        return;
    }

    if (cpu.hasHalted() || singleStepMode)
    {
        return;
    }

    // Our framerate is 30fps so we just need to call this every other frame
    if (cartridge.isNSF && cpu.stack == nsfSentinal)
    {
        cpuStep();
    }

    // TODO: calculate this rate properly and preserve extra cycles in a variable to run later
    // cpu and ppu per needed timings, cpu and apu every 12, ppu every 4

    // TODO: There s a note on the wiki that mentions having a hard coded clock rate
    // "Emulator authors may wish to emulate the NTSC NES/Famicom CPU at 21441960 Hz ((341�262-0.5)�4�60) to ensure a synchronised/stable 60 frames per second."
    // I don't understand how this works, so for now, I'll just calculate it
    int32 masterCycles = (int32)(secondsPerFrame * masterClockHz);
    int32 cyclesPerSample = masterCycles / (secondsPerFrame * 48000);
    int32 audioCounter = 0;
    for (int i = 0; i < masterCycles; ++i)
    {
        if (i % 12 == 0)
        {
            if (!cartridge.isNSF || cpu.stack != nsfSentinal)
            {
                cpuStep();
            }

            //TODO: Refactor the apu tick to happen on cpu and half step the parts that aren't clocked to cpu rate
            apu.triangle.tick();

            if (i % 24 == 0)
            {
                apu.tick();
            }
        }

        ++audioCounter;
        if (audioCounter >= cyclesPerSample)
        {
            uint32 output = apu.getOutput(); // Testing pulse only, values in range of 0-30 for now;
            float range = output / 45.0f;
            apuBuffer[writeHead++] = range * 30000; // TODO: Do math
            if (writeHead >= 48000)
            {
                write(apuBuffer, writeHead);
                writeHead = 0;
            }
            
            audioCounter -= cyclesPerSample;
        }

        if (i % 4 == 0)
        {
            // ppu.tick();
        }

        if (cartridge.isNSF)
        {
            if (cpu.stack == nsfSentinal && cyclesToNextPlay <= 0)
            {
                cyclesToNextPlay = totalPlayCycles;
                cpu.jumpSubroutine(cartridge.playAddress);
            }
            else
            {
                --cyclesToNextPlay;
            }
        }
    }

    if (ppu.getVBlankActive() && !wasVBlankActive)
    {
        // cpu.nonMaskableInterrupt();
        // flushScreenBuffer();
    }

    wasVBlankActive = ppu.getVBlankActive();
}

void NES::cpuStep()
{
    if (traceEnabled && !cpu.hasHalted() && !cpu.isExecuting())
    {
        logInstruction("bin/6502.log", cpu.instAddr, &cpu, &cpuBus, &ppu);
    }

    if (cpu.tick() && cpu.hasHalted())
    {
        powerOff();
    }
}

void NES::singleStep()
{
    cpuStep();

    while (cpu.isExecuting())
    {
        cpu.tick();
    }
}

void NES::outputAudio(int16* outputBuffer, int length)
{
    memset(outputBuffer, 0, length * 2 * sizeof(int16));
    uint32 bytesAvailable = writeHead - playHead;
    if (playHead > writeHead)
    {
        bytesAvailable = writeHead + (48000 - playHead);
    }

    if (isRunning && bytesAvailable > length * sizeof(int16))
    {
        int32 i = 0;
        while (i < length)
        {
            int16 value = apuBuffer[playHead++];
            playHead %= 48000;
            outputBuffer[i] = value;
            outputBuffer[i + 1] = value;
            i += 2;
        }
    }
}

void NES::setGamepadState(NESGamePad pad, int number)
{
    cpuBus.setInput(pad, number);
}