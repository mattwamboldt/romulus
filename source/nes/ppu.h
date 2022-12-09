#pragma once
#include "../common.h"
#include "bus.h"

// TODO: Replace raw masks values with constants to better document the code

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

    // TODO: This probably doens't make sense to happen in the PPU actually
    // Should probably happen a layer above
    void setOamDma(uint8 value) {}

    // Used for timing and debugging

    uint32 cycle;
    uint32 scanline;
    uint32 pixel;

private:
    IBus* bus;

    // Set at the start of vblank: dot 1 of scanline 241
    // Cleared at the end of vblank: dot 1 of pre-render
    bool nmiRequested;
    bool nmiEnabled;

    // TODO: Implement
    bool useTallSprites;
    uint16 vramAddressIncrement;
    // TODO: Implement
    uint16 spritePatternBaseAddress;
    // TODO: Implement
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

    uint8 oamAddress;
    // TODO: Implement
    // OAM (Object Attribute Memory) holds sprite data
    // See https://www.nesdev.org/wiki/PPU_sprite_evaluation
    uint8 oam[256];

    // Data for the 8 (or fewer) sprites to render on this scanline
    uint8 oamSecondary[32];

    // The following are the actual device registers
    // https://www.nesdev.org/wiki/PPU_scrolling#PPU_internal_registers

    // TODO: Implement
    // v: The current pointer of the ppu into vram
    uint16 vramAddress;

    // t: Address of the top left of the screen
    uint16 tempVramAddress;

    // TODO: Implement
    // x: Fine X scroll (3 bits)
    uint8 fineX;

    // w: Single bit used to get two 8 bit values into t
    bool isWriteLatchActive;
};