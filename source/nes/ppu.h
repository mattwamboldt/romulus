#pragma once
#include "../common.h"
#include "bus.h"

// TODO: Replace raw masks values with constants to better document the code

#define NES_SCREEN_HEIGHT 240
#define NES_SCREEN_WIDTH  256

class PPU
{
public:
    void connect(IBus* bus) { this->bus = bus; }
    // TODO: Implement
    void tick();

    bool isNMIFlagSet();

    // CPU <=> PPU Bus functions

    void setControl(uint8 value);
    void setMask(uint8 value);
    
    // Readonly prevents side effects. for debug specifically
    uint8 getStatus(bool readOnly);

    void setOamAddress(uint8 value);
    void setOamData(uint8 value);
    uint8 getOamData();

    void setScroll(uint8 value);
    void setAddress(uint8 value);
    void setData(uint8 value);
    uint8 getData(bool readOnly);

    // Used for timing and debugging

    uint32 cycle;
    uint32 scanline;
    uint32 pixel;

    // Temp for now, will have a screen abstraction later
    // Stores the pallette data while it's being written
    uint8 screenBuffer[NES_SCREEN_WIDTH * NES_SCREEN_HEIGHT];
    uint16 outputOffset;

private:
    IBus* bus;

    // Set at the start of vblank: dot 1 of scanline 241
    // Cleared at the end of vblank: dot 1 of pre-render
    bool nmiRequested;
    bool nmiEnabled;

    // TODO: Implement
    bool useTallSprites;
    uint8 spriteHeight;

    uint16 vramAddressIncrement;
    // TODO: Implement
    uint16 spritePatternBaseAddress;
    uint16 backgroundPatternBaseAddress;

    // TODO: Implement
    // Apparently complicated do to bugs? https://www.nesdev.org/wiki/PPU_sprite_evaluation
    bool isSpriteOverflowFlagSet;
    // TODO: Implement
    bool isSpriteZeroHit;

    // Settings extracted from PPUMASK
    bool shouldRenderGreyscale; // TODO: Implement
    bool showBackgroundInLeftEdge; // TODO: Implement
    bool showSpritesInLeftEdge; // TODO: Implement
    bool isBackgroundEnabled; // TODO: Implement
    bool areSpritesEnabled; // TODO: Implement
    bool shouldEmphasizeRed; // TODO: Implement
    bool shouldEmphasizeGreen; // TODO: Implement
    bool shouldEmphasizeBlue; // TODO: Implement

    // ======================
    // Registers
    // https://www.nesdev.org/wiki/PPU_scrolling#PPU_internal_registers
    // ======================

    // v: The current pointer of the ppu into vram
    uint16 vramAddress;

    // t: Address of the top left of the screen
    uint16 tempVramAddress;

    // TODO: Implement
    // x: Fine X scroll (3 bits)
    uint8 fineX;

    // w: Single bit used to get two 8 bit values into t
    bool isWriteLatchActive;

    // =======================
    // Internal storage for background render
    // NOTE: Latch might not be the right term here, its a temp variable
    // But seems like thats whats in the wiki as the physical euqivalent
    // ======================

    uint8 nameTableLatch;
    uint8 attributeLatch;
    uint8 patternLoLatch;
    uint8 patternHiLatch;

    // TODO: This could be optimized and potentially simplified by precomputing arrays
    // and using an index instead of shift registers, but I want to get this
    // "Correct" before I get it fast and more readable
    uint16 patternLoShift; // Bit plane 0 entry
    uint16 patternHiShift; // Bit Plane 1 entry

    // Finally understand how these work and will definitely be computing some arrays
    // Essentially There are two 1 bit values, determined by the attribute read.
    // The attribute holds 4 tiles worth of data for the top two bits of pallette
    // depending on which quadrant the tile is in. Those are loaded into these latches
    // which are shifted in to these registers multiple times to line up with the pattern data
    // It's done this way so you can have the data for the nest tile shifted in which
    // may have a different attribute, and you only have to combine the outputs 
    uint8 attributeLoShift; // Attribute Bit 0
    uint8 attributeHiShift; // Attribute Bit 1

    uint8 attributeBit0;
    uint8 attributeBit1;

    // ======================
    // Internal Storage for the Sprites
    // See https://www.nesdev.org/wiki/PPU_sprite_evaluation
    // TODO: Implement
    // ======================

    uint8 oamAddress;

    // OAM (Object Attribute Memory) holds sprite data
    uint8 oam[256];

    // Data for the 8 (or fewer) sprites to render on this scanline
    uint8 oamSecondary[32];

    uint8 spritePatternLoShift[8];
    uint8 spritePatternHiShift[8];
    uint8 spriteAttributeLatches[8];
    uint8 spriteXCounters[8];

    // This is a temp variable to keep track of which sprite had priority
    uint8 renderedSpriteIndex;

    // Temp variables for sprite evaluation
    bool isSecondaryOAMWriteDisabled;
    bool isCopyingSprite;
    uint8 secondaryOamAddress;
    uint8 numSpritesChecked;
    uint8 n;
    uint8 m;

    // =====================
    // Internal Utility Functions
    // =====================

    uint8 calculateBackgroundPixel();
    uint8 calculateSpritePixel();
};