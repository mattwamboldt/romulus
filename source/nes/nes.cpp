#include "nes.h"
#include "cpuTrace.h"
#include <math.h>
#include <string.h>

#include "../wavefile.h"

NES::NES()
{
    cpu.connect(&cpuBus);
    ppu.connect(&ppuBus);
    cpuBus.connect(&ppu, &apu, &cartridge);
    ppuBus.connect(&ppu, &cartridge);
    traceEnabled = false;
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

    // TODO: calculate this rate properly and preserve extra cycles in a variable to run later
    // cpu and ppu per needed timings, cpu and apu every 12, ppu every 4

    // TODO: There s a note on the wiki that mentions having a hard coded clock rate
    // "Emulator authors may wish to emulate the NTSC NES/Famicom CPU at 21441960 Hz ((341�262-0.5)�4�60) to ensure a synchronised/stable 60 frames per second."
    // I don't understand how this works, so for now, I'll just calculate it
    int32 masterClockHz = 21477272;
    int32 masterCycles = (int32)(secondsPerFrame * masterClockHz);
    int32 cyclesPerSample = masterCycles / (secondsPerFrame * 48000);
    int32 audioCounter = 0;
    for (int i = 0; i < masterCycles; ++i)
    {
        if (i % 12 == 0)
        {
            cpuStep();
            if (i % 24 == 0)
            {
                apu.tick();
            }
        }

        ++audioCounter;
        if (audioCounter >= cyclesPerSample)
        {
            uint32 output = apu.getOutput(); // Testing pulse only, values in range of 0-30 for now;
            float range = output / 30.0f;
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
    memset(outputBuffer, 0, length);
    if (isRunning)
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