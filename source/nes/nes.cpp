#include "nes.h"
#include "cpuTrace.h"
#include <string.h>

const uint32 masterClockHz = 21477272;

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
    apu.reset();
    ppu.reset();
    apu.noise.shiftRegister = 1;
    isRunning = true;
}

void NES::reset()
{
    if (!isRunning)
    {
        return;
    }

    cpu.reset();
    apu.reset();
    ppu.reset();
    isRunning = true;
}

void NES::powerOff()
{
    cpu.stop();
    isRunning = false;

    flushLog();
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
    uint32 masterCycles = (uint32)(secondsPerFrame * masterClockHz);
    uint32 cyclesPerSample = (uint32)(masterCycles / (secondsPerFrame * 48000));
    for (uint32 i = 0; i < masterCycles; ++i)
    {
        if (clockDivider == 0)
        {
            // TODO: There may be some conditions to emulate with this and the dmc cross talking? Need to look that up
            if (cpuBus.isDmaActive)
            {
                // First cycle is odd to wait for cpu writes to complete
                if (cpuBus.dmaCycleCount == 0)
                {
                    ++cpuBus.dmaCycleCount;
                }
                // Wait an extra frame if on an odd cycle
                // dma cycle 1 == cpu cycle 0 (even)
                else if (cpuBus.dmaCycleCount % 2 != currentCpuCycle % 2)
                {
                    // TODO: Ticking something in a memory access/mapping layer feels wrong
                    // The MOSS and apu together are the nes cpu, so maybe better to combine those
                    // to better map to the device
                    cpuBus.tickDMA();
                }
            }
            else if (!cartridge.isNSF || cpu.stack != nsfSentinal)
            {
                cpuStep();
            }
            
            apu.tick(currentCpuCycle);

            // TODO: Should hijack the cpu for some amount of time. Skipping for now to get initial playback
            if (apu.dmc.readRequired())
            {
                apu.dmc.loadSample(cpuBus.read(apu.dmc.getCurrentAddress()));
            }

            ++currentCpuCycle;
        }

        ++audioOutputCounter;
        if (audioOutputCounter >= cyclesPerSample)
        {
            real32 output = apu.getOutput(); // Testing pulse only, values in range of 0.0-1.0 for now;
            apuBuffer[writeHead++] = (int16)(output * 32768); // TODO: Do math
            if (writeHead >= 48000)
            {
                writeHead = 0;
            }
            
            audioOutputCounter -= cyclesPerSample;
        }

        if (clockDivider % 4 == 0)
        {
            ppu.tick();
        }

        ++clockDivider;
        clockDivider %= 12;

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
        // NSF is supposed to be designed to not use interrupts
        else
        {
            // Gather up all the potenial interrupt sources to assert the right status in the cpu
            // TODO: Only have the apu for now, other sources will come later
            cpu.setIRQ(apu.isFrameInteruptFlagSet || apu.dmc.isInterruptFlagSet);
            if (ppu.isNMISuppressed())
            {
                cpu.forceClearNMI();
            }
            else
            {
                cpu.setNMI(ppu.isNMIFlagSet());
            }
        }
    }
}

void NES::cpuStep()
{
    if (traceEnabled && !cpu.hasHalted() && !cpu.isExecuting())
    {
        logInstruction("bin/6502.log", cpu.pc, &cpu, &cpuBus, &ppu, currentCpuCycle);
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
    memset(outputBuffer, 0, length * sizeof(int16));
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