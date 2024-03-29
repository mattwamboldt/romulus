﻿#include "nes.h"
#include "cpuTrace.h"
#include <string.h>
#include "constants.h"

const uint32 masterClockHz = 21477272;

NES::NES()
{
    cpu.connect(&cpuBus);
    ppu.connect(&ppuBus);
    cpuBus.connect(&ppu, &apu, &cartridge, &inputBus);
    ppuBus.connect(&cartridge);
    inputBus.init(&ppu);

    traceEnabled = false;
}

bool NES::loadRom(const char * path)
{
    if (!cartridge.load(path))
    {
        return false;
    }

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

    return true;
}

void NES::unloadRom()
{
    powerOff();
    cartridge.unload();
}

void NES::powerOn()
{
    cpu.start();
    apu.reset();
    ppu.reset();
    apu.noise.shiftRegister = 1;

    clockDivider = 0;
    currentCpuCycle = 0;
    isRunning = true;

    // This runs the reset process without the ppu active, doing this to line up with nintendulator
    // TODO: I know it was more "correct" before, but I'm trying to track down a timing issue and diffing logs is all I got...
    while (cpu.isExecuting())
    {
        cpu.tick();
        apu.tick(currentCpuCycle);
        cartridge.tickCPU();

        ++currentCpuCycle;
    }
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
    clockDivider = 0;
    currentCpuCycle = 0;

    // This runs the reset process without the ppu active, doing this to line up with nintendulator
    // TODO: I know it was more "correct" before, but I'm trying to track down a timing issue and diffing logs is all I got...
    while (cpu.isExecuting())
    {
        cpu.tick();
        apu.tick(currentCpuCycle);
        cartridge.tickCPU();

        ++currentCpuCycle;
    }
}

void NES::powerOff()
{
    cartridge.unload();
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

            cartridge.tickCPU();

            ++currentCpuCycle;
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
        // NSF is supposed to be designed to not use interrupts
        else if (clockDivider % 4 == 0)
        {
            ppu.tick();

            // Gather up all the potenial interrupt sources to assert the right status in the cpu
            cpu.setIRQ(apu.isFrameInteruptFlagSet
                || apu.dmc.isInterruptFlagSet
                || cartridge.isIrqPending());

            if (ppu.isNMISuppressed())
            {
                cpu.forceClearNMI();
            }
            else
            {
                cpu.setNMI(ppu.isNMIFlagSet());
            }
        }

        // TODO: Over sample the audio and manually downsample to reduce aliasing artifacts
        // TODO: Find a way to center the audio around 0 so it's not as quiet (High Pass Filter?)
        // TODO: Add a master volume and per channel volume controls
        ++audioOutputCounter;
        if (audioOutputCounter >= cyclesPerSample)
        {
            real32 output = apu.getOutput();
            apuBuffer[writeHead++] = (int16)(output * 32768);
            if (writeHead >= 48000)
            {
                writeHead = 0;
            }

            audioOutputCounter -= cyclesPerSample;
        }

        ++clockDivider;
        if (clockDivider >= 12)
        {
            clockDivider = 0;
        }
    }
}

void NES::cpuStep()
{
    if (isRunning && traceEnabled && !cpu.hasHalted() && !cpu.isExecuting())
    {
        logInstruction("data/6502.log", cpu.pc, &cpu, &cpuBus, &ppu, currentCpuCycle);
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
    // TODO: Consider a fade in/out so the system startup doesn't cause a click
    // (May not be needed once we center the audio with filters)

    if (isRunning) 
    {
        uint32 bytesAvailable = writeHead - playHead;
        if (playHead > writeHead)
        {
            bytesAvailable = writeHead + (48000 - playHead);
        }

        if (bytesAvailable > length * sizeof(int16))
        {
            int32 i = 0;
            while (i < length)
            {
                lastSample = apuBuffer[playHead++];
                playHead %= 48000;
                outputBuffer[i] = lastSample;
                outputBuffer[i + 1] = lastSample;
                i += 2;
            }
        }
        else
        {
            int32 i = 0;
            while (i < length)
            {
                outputBuffer[i] = lastSample;
                outputBuffer[i + 1] = lastSample;
                i += 2;
            }
        }
    }
    else
    {
        memset(outputBuffer, 0, length * sizeof(int16));
    }
}

void NES::processInput(InputState* input)
{
    if (!isRunning)
    {
        return;
    }

    inputBus.update(input);
}

// Rendering functions that belong on the app side

void drawRect(ScreenBuffer buffer, uint32 x, uint32 y, uint32 width, uint32 height, uint32 color)
{
    uint8* row = (uint8*)buffer.memory + (buffer.pitch * y);
    for (uint32 yOffset = 0; yOffset < height; ++yOffset)
    {
        uint32* pixel = (uint32*)row + x;
        for (uint32 xOffset = 0; xOffset < width; ++xOffset)
        {
            *pixel++ = color;
        }

        row += buffer.pitch;
    }
}

void drawPalettePixel(ScreenBuffer buffer, uint32 x, uint32 y, uint8 index)
{
    uint8* row = (uint8*)buffer.memory + (buffer.pitch * y);
    uint32* pixel = (uint32*)row + x;
    *pixel = palette[index];
}

void NES::drawPalette(ScreenBuffer buffer, uint32 top, uint32 left, uint16 baseAddress)
{
    // TODO: put a rect over the selected pallete and allow selecting it to swap in the pattern table view
    for (uint16 paletteNum = 0; paletteNum < 16; paletteNum += 4)
    {
        for (uint16 i = 0; i < 4; ++i)
        {
            uint8 paletteIndex = ppuBus.read(baseAddress + paletteNum + i);
            drawRect(buffer, left + (i * 10), top, 10, 10, palette[paletteIndex]);
        }

        left += 45;
    }
}

void NES::renderPatternTable(ScreenBuffer buffer, uint32 top, uint32 left, uint8 selectedPalette)
{
    // TODO: Have more involved settings for this as a dedicated view (possibly window)
    const uint16 paletteRamBase = 0x3F00;
    drawPalette(buffer, top, left, paletteRamBase);
    top += 15;
    drawPalette(buffer, top, left, paletteRamBase + 0x10);
    top += 15;

    uint16 patternAddress = 0x0000;
    uint32 x = left;
    uint32 y = top;

    for (int page = 0; page < 2; ++page)
    {
        for (int yTile = 0; yTile < 16; ++yTile)
        {
            for (int xTile = 0; xTile < 16; ++xTile)
            {
                for (int tileY = 0; tileY < 8; ++tileY)
                {
                    uint8 patfield01 = ppuBus.read(patternAddress);
                    uint8 patfield02 = ppuBus.read(patternAddress + 8);

                    for (int bit = 0; bit < 8; ++bit)
                    {
                        uint8 paletteOffset = (patfield01 & 0b10000000) >> 7;
                        paletteOffset |= (patfield02 & 0b10000000) >> 6;
                        patfield01 <<= 1;
                        patfield02 <<= 1;
                        uint8 color = ppuBus.read(paletteRamBase + paletteOffset);
                        drawPalettePixel(buffer, x++, y, color);
                    }

                    ++patternAddress;
                    x -= 8;
                    ++y;
                }

                patternAddress += 8;
                y -= 8;
                x += 8;
            }

            x -= 128;
            y += 8;
        }

        y = top;
        x = left + 128;
    }
}

void NES::renderNametable(ScreenBuffer buffer, uint32 top, uint32 left)
{
    uint16 nametableAddress = 0x2000;

    uint32 x = left;
    uint32 y = top;

    uint8 universalBackground = ppuBus.read(0x3F00);

    for (uint16 page = 0; page < 4; ++page)
    {
        uint16 nameTableSelect = nametableAddress & 0x0C00;

        for (int yTile = 0; yTile < 30; ++yTile)
        {
            for (int xTile = 0; xTile < 32; ++xTile)
            {
                uint16 patternByte = ppuBus.read(nametableAddress);
                uint16 patternAddress = (patternByte << 4) + ppu.backgroundPatternBaseAddress;

                uint16 coarseX = (nametableAddress & 0x001F) >> 2;
                uint16 coarseY = (nametableAddress & 0x0380) >> 4;
                uint16 attributeAddress = 0x23C0 | nameTableSelect | coarseY | coarseX;
                uint8 attributeByte = ppuBus.read(attributeAddress);

                // attribute table covers a 4 x 4 tile area so bit 0 doesn't matter
                // and the next 3 bits after this got us this attribute in the first place
                bool isRightAttribute = nametableAddress & BIT_1;
                bool isBottomAttribute = nametableAddress & BIT_6;

                uint16 attributeValue = 0x3F00;

                if (isRightAttribute)
                {
                    if (isBottomAttribute)
                    {
                        // Bottom Right
                        attributeValue += (attributeByte & 0xC0) >> 4;
                    }
                    else
                    {
                        // Top Right
                        attributeValue += attributeByte & 0x0C;
                    }
                }
                else
                {
                    if (isBottomAttribute)
                    {
                        // Bottom Left
                        attributeValue += (attributeByte & 0x30) >> 2;
                    }
                    else
                    {
                        // Top Left
                        attributeValue += (attributeByte & 0x03) << 2;
                    }
                }

                for (int tileY = 0; tileY < 8; ++tileY)
                {
                    uint8 patfield01 = ppuBus.read(patternAddress);
                    uint8 patfield02 = ppuBus.read(patternAddress + 8);
                    uint8* row = (uint8*)buffer.memory + (buffer.pitch * y);
                    uint32* pixel = (uint32*)row + x;

                    for (int bit = 0; bit < 8; ++bit)
                    {
                        uint8 paletteOffset = (patfield01 & 0b10000000) >> 7;
                        paletteOffset |= (patfield02 & 0b10000000) >> 6;
                        patfield01 <<= 1;
                        patfield02 <<= 1;

                        if (paletteOffset == 0)
                        {
                            *pixel++ = palette[universalBackground];
                        }
                        else
                        {
                            uint8 color = ppuBus.read(attributeValue + paletteOffset);
                            *pixel++ = palette[color];
                        }
                    }

                    ++patternAddress;
                    ++y;
                }

                ++nametableAddress;
                y -= 8;
                x += 8;
            }

            x -= 256;
            y += 8;
        }

        switch (page)
        {
        case 0:
            y = top;
            x = left + 256;
            nametableAddress = 0x2400;
            break;

        case 1:
            y = top + 240;
            x = left;
            nametableAddress = 0x2800;
            break;

        case 2:
            y = top + 240;
            x = left + 256;
            nametableAddress = 0x2C00;
            break;
        }
    }

    // TODO: Locate temp address, since it contains the scroll position
    // NOTE: this is highly unstable but It may help with some debugging

    uint16 coarseX = ppu.tempVramAddress & 0x001F;
    x = left + (coarseX * 8) + ppu.fineX;

    uint16 coarseY = (ppu.tempVramAddress & 0x03E0) >> 5;
    uint16 fineY = (ppu.tempVramAddress & 0x7000) >> 12;
    y = top + (coarseY * 8) + fineY;

    uint8* row = (uint8*)buffer.memory + (buffer.pitch * y);
    uint32* pixel = ((uint32*)row) + x;

    *pixel = 0x0000FFFF;
}

void NES::render(ScreenBuffer buffer)
{
    uint8* nesBuffer = ppu.frontBuffer;
    uint8* row = (uint8*)buffer.memory;

    if (!cpu.hasHalted())
    {
        for (int y = 0; y < NES_SCREEN_HEIGHT; ++y)
        {
            uint32* pixel = (uint32*)row;
            for (int x = 0; x < NES_SCREEN_WIDTH; ++x)
            {
                *pixel++ = palette[*nesBuffer++];
            }

            row += buffer.pitch;
        }
    }
    else
    {
        for (int y = 0; y < NES_SCREEN_HEIGHT; ++y)
        {
            uint32* pixel = (uint32*)row;
            for (int x = 0; x < NES_SCREEN_WIDTH; ++x)
            {
                *pixel++ = 0xFF0000FF;
            }

            row += buffer.pitch;
        }
    }

#if SHOW_DEBUG_VIEWS
    if (!cartridge.isNSF && isRunning)
    {
        ppuBus.setReadOnly(true);
        renderPatternTable(buffer, 250, 0, 0);
        renderNametable(buffer, 10, 266);
        ppuBus.setReadOnly(false);
    }
#endif
}
