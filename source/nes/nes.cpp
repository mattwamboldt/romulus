#include "nes.h"
#include "cpuTrace.h"
#include <math.h>
#include <string.h>

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
    // TODO: Will likely need to change this to separate the boot up sysle from resets
    cartridge.load(path);
    cpu.start();
}

void NES::update(real32 secondsPerFrame)
{
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
            float output = apu.getOutput();
            apuBuffer[writeHead++] = (output * 65536) - 32768; // TODO: Do math
            writeHead %= 48000;
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
        logInstruction("data/6502.log", cpu.instAddr, &cpu, &cpuBus, &ppu);
    }

    if (cpu.tick() && cpu.hasHalted())
    {
        flushLog();
        singleStepMode = true;
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

// For now to test audio outpout I'm just going to use a simple oscillator, taken from the synthesizer project
#define PI     3.14159265359
#define TWO_PI 6.28318530718

double frequency = 440;
double phase = 0;
double samplingRadians = TWO_PI / 48000.0;
double increment = frequency * samplingRadians;

void NES::outputAudio(int16* outputBuffer, int length)
{
    memset(outputBuffer, 0, length);
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