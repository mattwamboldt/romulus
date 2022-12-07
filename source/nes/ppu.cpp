#include "ppu.h"
#include <windows.h>

// Starting from scratch

void PPU::tick()
{
   // Step 1: Report status correctly with frame timing for intterupts
}

bool PPU::isNMIFlagSet()
{
    return nmiEnabled && nmiRequested;
}

void PPU::setControl(uint8 value)
{
    nmiEnabled = value & 0x80;
    // NOTE: Bit 6 is tied to ground on the NES. So its safe to ignore
    useTallSprites = value & 0x20;
    backgroundPatternBaseAddress = (value & 0x10) ? 0x1000 : 0; // PERF: Could do shifts here to not branch
    spritePatternBaseAddress = (value & 0x08) ? 0x1000 : 0;
    vramAddressIncrement = (value & 0x04) ? 32 : 1;
    baseNametableAddress = 0x2000 + (0x0400 * (value & 0x03));
}

void PPU::setMask(uint8 value)
{
    OutputDebugStringA("[PPU] setMask\n");
}

uint8 PPU::getStatus()
{
    OutputDebugStringA("[PPU] getStatus\n");
    return 0;
}

void PPU::setOamAddress(uint8 value)
{
    OutputDebugStringA("[PPU] setOamAddress\n");
}

void PPU::setOamData(uint8 value)
{
    OutputDebugStringA("[PPU] setOamData\n");
}

uint8 PPU::getOamData()
{
    OutputDebugStringA("[PPU] getOamData\n");
    return 0;
}

void PPU::setScroll(uint8 value)
{
    OutputDebugStringA("[PPU] setScroll\n");
}

void PPU::setAddress(uint8 value)
{
    OutputDebugStringA("[PPU] setAddress\n");
}

void PPU::setData(uint8 value)
{
    OutputDebugStringA("[PPU] setData\n");
}

uint8 PPU::getData()
{
    OutputDebugStringA("[PPU] getData\n");
    return 0;
}
