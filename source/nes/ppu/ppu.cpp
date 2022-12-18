#include "ppu.h"

// TODO: This functionality is currently causing bugs on certain carts so it's
// not completely accurate yet. Disabling for now (may not even be worth it tbh)
#define ENABLE_NMI_SUPRESSION false

const uint32 PRERENDER_LINE = 261;
const uint32 CYCLES_PER_SCANLINE = 340;

// Masks to help simplify understanding the vram address components

const uint16 COARSE_X_MASK =  0x001F; // ........ ...XXXXX
const uint16 COARSE_Y_MASK =  0x03E0; // ......YY YYY.....
const uint16 NAMETABLE_MASK = 0x0C00; // ....NN.. ........
const uint16 FINE_Y_MASK =    0x7000; // .yyy.... ........

PPU::PPU()
{
    reset();
}

void PPU::reset()
{
    for (int i = 0; i < NES_SCREEN_WIDTH * NES_SCREEN_HEIGHT; ++i)
    {
        screenBufferOne[i] = 0;
        screenBufferTwo[i] = 0;
    }

    frontBuffer = screenBufferOne;
    backbuffer = screenBufferTwo;

    cycle = 0;
    scanline = 0;
    outputOffset = 0;

    nmiRequested = false;
    isSpriteOverflowFlagSet = false;
    isSpriteZeroHit = false;
    suppressNmi = false;
}

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
            nmiRequested = !suppressNmi;

            // Swap outputs
            uint8* temp = frontBuffer;
            frontBuffer = backbuffer;
            backbuffer = temp;
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
        isSpriteOverflowFlagSet = false;
        isSpriteZeroHit = false;
        suppressNmi = false;
        outputOffset = 0;
    }

    // Cycles 257 - 320
    bool isSpritePhase = cycle > NES_SCREEN_WIDTH && cycle <= 320;

    // Render
    if (scanline != PRERENDER_LINE && cycle <= NES_SCREEN_WIDTH)
    {
        // TODO: Handle rendering disabled (background pallette hack)
        uint8 backgroundPixel = calculateBackgroundPixel();
        uint8 spritePixel = calculateSpritePixel();

        bool backgroundVisible = backgroundPixel & 0x03;
        bool spriteVisible = spritePixel & 0x03;

        uint8 pixel = backgroundPixel;
        if (backgroundVisible && spriteVisible)
        {
            if (renderedSpriteIndex == 0 && spriteRenderers[renderedSpriteIndex].getX() != 0xFF && cycle != NES_SCREEN_WIDTH)
            {
                isSpriteZeroHit = true;
            }

            if (spriteRenderers[renderedSpriteIndex].getPriority())
            {
                pixel = backgroundPixel;
            }
            else
            {
                pixel = spritePixel;
            }
        }
        else if (spriteVisible)
        {
            pixel = spritePixel;
        }

        backbuffer[outputOffset++] = bus->read(0x3F00 + pixel);

        // Clock sprite counters and shift registers
        for (int i = 0; i < 8; ++i)
        {
            spriteRenderers[i].tick();
        }
    }

    // Clock the background shift registers
    // PERF: See if this is needed on bg disable
    // NOTE: have a feeling these would always be going and that the last 4 cycles
    // would have been priming the pipeline in some way, who knows.
    if (cycle <= 336)
    {
        patternLoShift <<= 1;
        patternHiShift <<= 1;
        attributeLoShift <<= 1;
        attributeHiShift <<= 1;
        attributeLoShift |= attributeBit0;
        attributeHiShift |= attributeBit1;
    }

    // Sprite Evaluation https://www.nesdev.org/wiki/PPU_sprite_evaluation
    if (cycle <= NES_SCREEN_WIDTH && cycle % 2 == 0)
    {
        // Clearing Phase;
        if (cycle == 64)
        {
            for (int i = 0; i < 32; ++i)
            {
                oamSecondary[i] = 0xFF;
            }

            secondaryOamAddress = 0;
            isSecondaryOAMWriteDisabled = false;
            numSpritesChecked = 0;
            numSpritesFound = 0;
        }
        else if (cycle > 64)
        {
            uint8 oamValue = oam[oamAddress];
            if (isCopyingSprite)
            {
                oamSecondary[secondaryOamAddress++] = oamValue;
                ++oamAddress;
                isCopyingSprite = secondaryOamAddress % 4 > 0;

                if (secondaryOamAddress >= 32)
                {
                    isSecondaryOAMWriteDisabled = true;
                    secondaryOamAddress = 0;
                }
            }
            else if (numSpritesChecked < 64)
            {
                // We haven't hit 8 sprites yet so check if there's one on this scanline
                if (!isSecondaryOAMWriteDisabled)
                {
                    // We always write this for some reason, seems to be somewhat of a sentinal value
                    oamSecondary[secondaryOamAddress] = oamValue;

                    uint32 spriteTop = oamValue;
                    uint32 spriteBottom = spriteTop + spriteHeight;

                    if (spriteTop < (NES_SCREEN_HEIGHT - 1) && scanline >= spriteTop && scanline < spriteBottom)
                    {
                        ++secondaryOamAddress;
                        ++oamAddress;
                        isCopyingSprite = true;
                        ++numSpritesFound;
                    }
                    else
                    {
                        oamAddress += 4;
                    }
                }
                // Continue checking for sprite overflow
                // There's a known bug where the address increments incorrectly once the write inhibit flag has been set
                // https://www.nesdev.org/wiki/PPU_sprite_evaluation#Sprite_overflow_bug
                // This was poorly worded and hard to follow, so I'll try to clarify
                // The stuff about n and m imply there are separate values that index directly into oam but
                // they're actually just portions of the oam address. m is not incremented with n if the y isn't in range
                // thats only true once you've hit the write inhibit flag after 8 sprites copied over
                // Treating it as separate values would mean that writing to OAMADDR after its been cleared wouldn't have negative consequences
                // I could be wrong about this, it's hard to tell when it should come up..
                else
                {
                    // "n" in this case gets incremented every time due to the bug
                    oamAddress += 4;

                    uint32 spriteTop = oamValue;
                    uint32 spriteBottom = spriteTop + spriteHeight;
                    if (spriteTop < (NES_SCREEN_HEIGHT - 1) && scanline >= spriteTop && scanline < spriteBottom)
                    {
                        isSpriteOverflowFlagSet = true;
                    }
                    else
                    {
                        // m gets incremented without the carry bit resulting in the "diagonal" fetch pattern for the y
                        uint8 m = (oamAddress & 0x03) + 1;
                        oamAddress = (oamAddress & 0xFC) | (m & 0x03);
                    }
                }

                ++numSpritesChecked;
            }
        }
    }
    else if (cycle == 257)
    {
        // NOTE: Avoid the temptation to do this in the middle of the visible frame
        numSpritesToRender = numSpritesFound;
        oamAddress = 0;

        for (int i = 0; i < 8; ++i)
        {
            spriteRenderers[i].reset();
            if (i < numSpritesToRender)
            {
                spriteRenderers[i].isEnabled = true;

                uint8 yPosition = oamSecondary[i * 4];
                uint16 tileIndex = oamSecondary[(i * 4) + 1];
                spriteRenderers[i].setAttribute(oamSecondary[(i * 4) + 2]);
                spriteRenderers[i].setX(oamSecondary[(i * 4) + 3]);

                // Calculate the shift values
                uint16 patternTableAddress = 0;

                // TODO: Test vertical flip (Hasn't been hit in debugger yet)
                uint16 fineY = scanline - yPosition;
                if (spriteRenderers[i].isVerticallyFlipped())
                {
                    fineY = (spriteHeight - 1) - fineY;
                }

                uint16 spriteBank = spritePatternBaseAddress;

                if (useTallSprites)
                {
                    // On tall sprites the last bit is the bank select and the rest is the tile index
                    spriteBank = (tileIndex & BIT_0) << 12;
                    tileIndex &= 0xFFFE;

                    // The bottom half the tile is to the right (BIT 0 of the normal style index)
                    if (fineY >= 8)
                    {
                        fineY -= 8;
                        ++tileIndex;
                    }
                }

                uint16 tileOffset = tileIndex << 4;
                patternTableAddress = fineY | tileOffset | spriteBank;
                spriteRenderers[i].patternLoShift = bus->read(patternTableAddress);
                patternTableAddress |= BIT_3;
                spriteRenderers[i].patternHiShift = bus->read(patternTableAddress);
            }
        }
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
            if (!isSpritePhase)
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
            isOddFrame = !isOddFrame;
            if (isOddFrame && isBackgroundEnabled)
            {
                cycle = 1;
            }
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

#if ENABLE_NMI_SUPRESSION
    if (nmiEnabled)
    {
        suppressNmi = false;
    }
    else if (nmiRequested && (scanline == 241 && (cycle == 2 || cycle == 3)))
    {
        suppressNmi = true;
    }
#endif

    // NOTE: Bit 6 is tied to ground on the NES. So its safe to ignore
    useTallSprites = value & BIT_5;
    if (useTallSprites)
    {
        spriteHeight = 16;
    }
    else
    {
        spriteHeight = 8;
    }

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

    nmiRequested = false;
    isWriteLatchActive = false;

#if ENABLE_NMI_SUPRESSION
    // Per the wiki NMI shouldn't happen if PPUSTATUS is read within 2 cycles of start of VBlank
    // The if check is to make sure we don't accidentally turn off suppression from another source
    if (!suppressNmi && scanline == 241)
    {
        suppressNmi = cycle == 1 || cycle == 2 || cycle == 3;
    }
#endif

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
    // Part of the Secondary OAM initialization
    bool inSpriteEvaluation = scanline > 0 && scanline < NES_SCREEN_HEIGHT&& cycle > 0 && cycle <= 64;

    if (inSpriteEvaluation)
    {
        return 0xFF;
    }

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
    if (readOnly)
    {
        if (vramAddress >= 0x3F00)
        {
            return bus->read(vramAddress);
        }

        return ppuDataReadBuffer;
    }

    uint8 result = ppuDataReadBuffer;
    ppuDataReadBuffer = bus->read(vramAddress);
    vramAddress += vramAddressIncrement;

    // pallete ram gets returned immediately
    if (vramAddress >= 0x3F00)
    {
        result = ppuDataReadBuffer;
    }

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

    for (int i = 0; i < numSpritesToRender; ++i)
    {
        uint8 spritePixel = spriteRenderers[i].calculatePixel();
        // Priority is given to first non transparent pixel
        if (spritePixel & 0x03)
        {
            renderedSpriteIndex = i;
            return spritePixel;
        }
    }

    // May not need this but its an extra way to know we rendered nothing
    renderedSpriteIndex = 8;
    return 0;
}