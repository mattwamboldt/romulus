#include "ppu.h"
#include <windows.h>

const uint32 PRERENDER_LINE = 261;
const uint32 CYCLES_PER_SCANLINE = 340;

// Masks to help simplify understanding the vram address
// yyy NN YYYYY XXXXX
// ||| || ||||| +++++-- coarse X scroll
// ||| || +++++-------- coarse Y scroll
// ||| ++-------------- nametable select
// +++----------------- fine Y scroll

const uint16 COARSE_X_MASK =  0x001F; // ........ ...XXXXX
const uint16 COARSE_Y_MASK =  0x03E0; // ......YY YYY.....
const uint16 NAMETABLE_MASK = 0x0C00; // ....NN.. ........
const uint16 FINE_Y_MASK =    0x7000; // .yyy.... ........

// The key to understanding what hapens on a ppu update was a combo of these pages
// https://www.nesdev.org/wiki/PPU_scrolling#At_dot_256_of_each_scanline
// https://www.nesdev.org/wiki/PPU_rendering#Line-by-line_timing
// https://www.nesdev.org/wiki/File:Ppu.svg
void PPU::tick()
{
    // Cycle 0 is an idle cycle always
    if (cycle == 0)
    {
        ++cycle;
        return;
    }

    // This handles our vblank dead zone
    if (scanline >= 240 && scanline < PRERENDER_LINE)
    {
        if (cycle == 1 && scanline == 241)
        {
            nmiRequested = true;
        }

        ++cycle;
        if (cycle > CYCLES_PER_SCANLINE)
        {
            ++scanline;
            cycle = 0;
        }

        return;
    }

    if (cycle == 1 && scanline == PRERENDER_LINE)
    {
        nmiRequested = false;
        outputOffset = 0;
    }

    bool isSpritePhase = cycle > NES_SCREEN_WIDTH && cycle <= 320;

    // Render the background
    if (scanline != PRERENDER_LINE && cycle <= NES_SCREEN_WIDTH)
    {
        uint8 backgroundPixel = calculateBackgroundPixel();

        // Clock the background shift registers
        // PERF: See if this is needed on bg disable
        patternLoShift <<= 1;
        patternHiShift <<= 1;
        attributeLoShift <<= 1;
        attributeHiShift <<= 1;
        attributeLoShift |= attributeBit0;
        attributeHiShift |= attributeBit1;

        uint8 spritePixel = calculateSpritePixel();

        // Clock sprite counters and shift registers
        for (int i = 0; i < 8; ++i)
        {
            if (spriteXCounters[i] > 0)
            {
                --spriteXCounters[i];
            }
            else
            {
                spritePatternLoShift[i] <<= 1;
                spritePatternHiShift[i] <<= 1;
            }
        }

        uint8 pixel = 0;
        if (backgroundPixel == 0)
        {
            pixel = spritePixel;
        }
        else if (spritePixel == 0)
        {
            pixel = backgroundPixel;
        }
        else // TODO: Priority (0: Sprite, 1: BG)
        {

        }

        screenBuffer[outputOffset++] = bus->read(0x3F00 + pixel);
    }

    // Fetch data into a set of "latches" based on the current cycle
    // These will fill the shift registers every 8 shifts
    // > Start of scanline is Idle
    // ---| NT| AT|BGL|BGH|.... (Repeat for the whole scanline)
    // > 0|1 2|3 4|5 6|7 0|....
    switch (cycle % 8)
    {
        case 2: // Name Table https://www.nesdev.org/wiki/PPU_nametables
        {
            // TODO: Potentially repeat on the sprite Attribute stage for mapper timing stuff
            uint16 nameTableAddress = 0x2000 | (vramAddress & 0x0FFF);
            nameTableLatch = bus->read(nameTableAddress);
            break;
        }
        case 4: // Attribute table https://www.nesdev.org/wiki/PPU_attribute_tables
        {
            if (isSpritePhase)
            {
                
            }
            else
            {
                // From the Wiki this is the address we want
                // NN 1111 YYY XXX
                // || |||| ||| +++--- high 3 bits of coarse X(x / 4)
                // || |||| +++------- high 3 bits of coarse Y(y / 4)
                // || ++++----------- attribute offset(960 bytes)
                // ++---------------- nametable select
                uint16 coarseX = (vramAddress & COARSE_X_MASK) >> 2;
                uint16 coarseY = (vramAddress & 0x0380) >> 4;
                uint16 nameTableSelect = vramAddress & 0x0C00;
                uint16 attributeAddress = 0x23C0 | nameTableSelect | coarseY | coarseX;
                attributeLatch = bus->read(attributeAddress);
            }
        }
        break;
        case 6: // Pattern Table Lo https://www.nesdev.org/wiki/PPU_pattern_tables
        {
            if (isSpritePhase)
            {

            }
            else
            {
                // Pattern table address Scheme
                // 0H RRRR CCCC PTTT
                // || |||| |||| |+++--- T : Fine Y offset, the row number within a tile
                // || |||| |||| +------ P : Bit plane (0: "lower"; 1: "upper")
                // || |||| ++++-------- C : Tile column
                // || ++++------------- R : Tile row
                // |+------------------ H : Half of pattern table (0: "left"; 1: "right")
                // +------------------- 0 : Pattern table is at $0000 - $1FFF
                uint16 fineY = (vramAddress & FINE_Y_MASK) >> 12;
                uint16 tileOffset = ((uint16)nameTableLatch) << 4;
                uint16 bitPlane = 0;
                uint16 patternTableAddress = fineY | bitPlane | tileOffset | backgroundPatternBaseAddress;
                patternLoLatch = bus->read(patternTableAddress);
            }
        }
        break;
        case 0: // Pattern Table Hi
        {
            if (isSpritePhase)
            {

            }
            else
            {
                uint16 fineY = (vramAddress & FINE_Y_MASK) >> 12;
                uint16 tileOffset = ((uint16)nameTableLatch) << 4;
                uint16 bitPlane = BIT_3;
                uint16 patternTableAddress = fineY | bitPlane | tileOffset | backgroundPatternBaseAddress;
                patternHiLatch = bus->read(patternTableAddress);

                // Fill the shift registers
                // TODO: This may need to occur separately/in the pixel render so it can be started at
                // a certain cycle offset, for sprite overwrite or something (Shift registers require a
                // clock, where the reads would be available immediately, so assuming after the render for now)

                // TODO: Yeah the more I fill out the attribute stuff the more realize this is wasting a lot of cycles
                // and making it more complicated will definitely refactor

                // In theory the bottom 8 bits are already clear from the shift operations, so OR is fine
                patternLoShift |= patternLoLatch;
                patternHiShift |= patternHiLatch;

                // attribute table covers a 4 x 4 tile area so bit 0 doesn't matter
                // and the next 3 bits after this got us this attribute in the first place
                bool isRightAttribute = vramAddress & BIT_1;
                bool isBottomAttribute = vramAddress & BIT_6;
                if (isRightAttribute)
                {
                    if (isBottomAttribute)
                    {
                        // Bottom Right
                        attributeBit0 = (attributeLatch >> 6) & BIT_0;
                        attributeBit1 = attributeLatch >> 7;
                    }
                    else
                    {
                        // Top Right
                        attributeBit0 = (attributeLatch >> 2) & BIT_0;
                        attributeBit1 = (attributeLatch >> 3) & BIT_0;
                    }
                }
                else
                {
                    if (isBottomAttribute)
                    {
                        // Bottom Left
                        attributeBit0 = (attributeLatch >> 4) & BIT_0;
                        attributeBit1 = (attributeLatch >> 5) & BIT_0;
                    }
                    else
                    {
                        // Top Left
                        attributeBit0 = attributeLatch & BIT_0;
                        attributeBit1 = (attributeLatch >> 1) & BIT_0;
                    }
                }
            }

        }
        break;
    }

    // Render the sprites

    // Handle Updating of the vram address, but only if rendering is enabled
    if (isBackgroundEnabled)
    {
        // Increment X Scroll
        if ((cycle < NES_SCREEN_WIDTH || cycle > 320) && cycle % 8 == 0)
        {
            // Handle overflow into the next horizontal nametable
            if ((vramAddress & COARSE_X_MASK) == 31)
            {
                vramAddress &= ~COARSE_X_MASK;
                vramAddress ^= 0x0400; // XOR will cause a toggle
            }
            else
            {
                ++vramAddress;
            }
        }
        // Increment Y Scroll
        if (cycle == NES_SCREEN_WIDTH)
        {
            // Check if we can safely increment fine y
            if ((vramAddress & FINE_Y_MASK) != FINE_Y_MASK)
            {
                vramAddress += 0x1000;
            }
            else
            {
                // Clear fine y and increment coarse y to go to the next tile
                // Since coarse y can be set out of bounds through v it has some special handling
                vramAddress &= ~FINE_Y_MASK;

                uint16 coarseY = (vramAddress & COARSE_Y_MASK) >> 5;
                if (coarseY == 29)
                {
                    coarseY = 0;
                    vramAddress ^= 0x0800;
                }
                else if (coarseY == 31)
                {
                    coarseY = 0;
                }
                else
                {
                    ++coarseY;
                }

                vramAddress = (vramAddress & ~COARSE_Y_MASK) | (coarseY << 5);
            }
        }
        // Copy horizontal components of t to v: horiz (v) = horiz (t)
        // v: ....A.. ...BCDEF <- t: ....A.. ...BCDEF
        else if (cycle == 257)
        {
            vramAddress &= ~0x041F;
            vramAddress |= tempVramAddress & 0x041F;
        }
        // Copy vertical components of t to v: vert (v) = vert (t)
        // v: GHIA.BC DEF..... <- t: GHIA.BC DEF.....
        else if (scanline == PRERENDER_LINE && cycle >= 280 && cycle <= 304)
        {
            vramAddress &= 0x041F;
            vramAddress |= tempVramAddress & ~0x041F;
        }
    }

    // Finish the cycle and advance
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

void PPU::setMask(uint8 mask)
{
    shouldRenderGreyscale =     (mask & BIT_0) > 0;
    showBackgroundInLeftEdge =  (mask & BIT_1) > 0;
    showSpritesInLeftEdge =     (mask & BIT_2) > 0;
    isBackgroundEnabled =       (mask & BIT_3) > 0;
    areSpritesEnabled =         (mask & BIT_4) > 0;
    shouldEmphasizeRed =        (mask & BIT_5) > 0;
    shouldEmphasizeGreen =      (mask & BIT_6) > 0;
    shouldEmphasizeBlue =       (mask & BIT_7) > 0;
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
    oamAddress = value;
}

void PPU::setOamData(uint8 value)
{
    oam[oamAddress++] = value;
}

uint8 PPU::getOamData()
{
    // TODO: This is supposed to increment in certain circumstances I believe
    return oam[oamAddress];
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


// Util functions

uint8 PPU::calculateBackgroundPixel()
{
    if (!isBackgroundEnabled)
    {
        return 0;
    }

    if (!showBackgroundInLeftEdge && cycle <= 8)
    {
        return 0;
    }

    // Need to flip this around and do shifts since we don't have mux
    uint8 xOffset = 15 - fineX; 
    uint8 bit0 = (patternLoShift >> xOffset) & 0x01;
    uint8 bit1 = (patternHiShift >> xOffset) & 0x01;

    uint8 attributeXOffset = 7 - fineX;
    uint8 bit2 = (attributeLoShift >> attributeXOffset) & 0x01;
    uint8 bit3 = (attributeHiShift >> attributeXOffset) & 0x01;

    return (bit3 << 3) | (bit2 << 2) | (bit1 << 1) | bit0;
}

uint8 PPU::calculateSpritePixel()
{
    if (!areSpritesEnabled)
    {
        return 0;
    }

    if (!showSpritesInLeftEdge && cycle <= 8)
    {
        return 0;
    }

    for (int i = 0; i < 8; ++i)
    {
        if (spriteXCounters[i] == 0)
        {
            // TODO: Need to handle sprite filpping
            // Probably in sprite eval when filling these units
            uint8 bit0 = (spritePatternLoShift[i] & BIT_7) >> 7;
            uint8 bit1 = (spritePatternHiShift[i] & BIT_7) >> 6;
            uint8 spritePixel = spriteAttributeLatches[0] | bit1 | bit0;

            // Priority is given to first non transparent pixel
            if (spritePixel)
            {
                renderedSpriteIndex = i;
                return spritePixel;
            }
        }
    }

    // May not need this but its an extra way to know we rendered nothing
    renderedSpriteIndex = 8;
    return 0;
}