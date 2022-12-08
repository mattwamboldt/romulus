#include "ppu.h"
#include <windows.h>

// Starting from scratch

const uint32 PRERENDER_LINE = 261;
const uint32 CYCLES_PER_SCANLINE = 340;

void PPU::tick()
{
    // Step 1: Report status correctly with frame timing for interupts

    // Cycle 0 is an idle cycle
    if (cycle == 0)
    {
        ++cycle;
        return;
    }

    if (cycle == 1)
    {
        if (scanline == 241)
        {
            nmiRequested = true;
        }
        else if (scanline == PRERENDER_LINE)
        {
            nmiRequested = false;
        }
    }

    ++cycle;

    if (cycle > CYCLES_PER_SCANLINE)
    {
        ++scanline;
        cycle = 0;

        if (scanline > PRERENDER_LINE)
        {
            scanline = 0;
        }
    }
}

bool PPU::isNMIFlagSet()
{
    return nmiEnabled && nmiRequested;
}

void PPU::setControl(uint8 value)
{
    nmiEnabled = value & BIT_7;
    // NOTE: Bit 6 is tied to ground on the NES. So its safe to ignore
    useTallSprites = value & BIT_5;
    backgroundPatternBaseAddress = (value & BIT_4) ? 0x1000 : 0; // PERF: Could do shifts here to not branch
    spritePatternBaseAddress = (value & BIT_3) ? 0x1000 : 0;
    vramAddressIncrement = (value & BIT_2) ? 32 : 1;

    // Base Nametable address is set via t, so could get wiped by setting address directly
    // t: ...GH.. ........ <- d: ......GH
    tempVramAddress &= 0xF3FF;
    tempVramAddress |= (uint16)(value & 0x03) << 10;
}

void PPU::setMask(uint8 value)
{
    // TOOD: Adding in case this needs to be split up
    mask = value;
}

uint8 PPU::getStatus(bool readOnly)
{
    uint8 status = 0;
    if (isSpriteOverflowFlagSet)
    {
        status |= BIT_5;
    }

    if (isSpriteZeroHit)
    {
        status |= BIT_6;
    }

    if (nmiRequested)
    {
        status |= BIT_7;
    }

    if (readOnly)
    {
        return status;
    }

    // TODO: Might have to worry about this from the wiki?
    // Race Condition Warning: Reading PPUSTATUS within two cycles of the start
    // of vertical blank will return 0 in bit 7 but clear the latch anyway, causing
    // NMI to not occur that frame. See NMI and PPU_frame_timing for details.
    nmiRequested = false;
    isWriteLatchActive = false;

    return status;
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
    // First write
    if (!isWriteLatchActive)
    {
        // t: ....... ...ABCDE <- d: ABCDE...
        tempVramAddress &= 0xFFE0;
        tempVramAddress |= (value & 0xF8) >> 3;

        // x:              FGH < -d : .....FGH
        fineX = (value & 0x07);
    }
    // Second write
    else
    {
        // t: FGH..AB CDE..... <- d: ABCDEFGH
        tempVramAddress &= 0x8C1F;
        tempVramAddress |= (uint16)(value & 0x07) << 12; // FGH
        tempVramAddress |= (uint16)(value & 0xF8) << 2; // ABCDE
    }

    isWriteLatchActive = !isWriteLatchActive;
}

void PPU::setAddress(uint8 value)
{
    // First write
    if (!isWriteLatchActive)
    {
        // t: .CDEFGH ........ <- d: ..CDEFGH
        // t : Z...... ........ < -0 (bit Z is cleared)
        tempVramAddress &= 0x00FF;
        tempVramAddress |= (uint16)(value & 0x3F) << 8;
    }
    // Second write
    else
    {
        // t: .......ABCDEFGH < -d : ABCDEFGH
        tempVramAddress &= 0xFF00;
        tempVramAddress |= value;

        vramAddress = tempVramAddress;
    }

    isWriteLatchActive = !isWriteLatchActive;
}

void PPU::setData(uint8 value)
{
    bus->write(vramAddress, value);
    vramAddress += vramAddressIncrement;
}

uint8 PPU::getData(bool readOnly)
{
    // TODO: The PPUDATA read buffer needs to potentially be handled
    // https://www.nesdev.org/wiki/PPU_registers#The_PPUDATA_read_buffer_(post-fetch)
    uint8 result = bus->read(vramAddress);
    if (readOnly)
    {
        return result;
    }

    vramAddress += vramAddressIncrement;
    return result;
}
