#pragma once
#include "../common.h"
#include "bus.h"

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

    // TODO: Implement
    void setOamAddress(uint8 value);
    // TODO: Implement
    void setOamData(uint8 value);
    // TODO: Implement
    uint8 getOamData();

    void setScroll(uint8 value);
    void setAddress(uint8 value);
    // TODO: Implement
    void setData(uint8 value);
    // TODO: Implement
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

    // TODO: Implement
    // Set at the start of vblank: dot 1 of scanline 241
    // Cleared at the end of vblank: dot 1 of pre-render
    bool nmiRequested;
    bool nmiEnabled;

    // TODO: Implement
    bool useTallSprites;
    // TODO: Implement
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


    // TODO: Implement
    // The bits of this control whetehr certain things render or not, I'm assuming bitmasking
    // is involved so I'm leaving it as a single value until we get there
    uint8 mask;

    // The following are the actual device registers
    // https://www.nesdev.org/wiki/PPU_scrolling#PPU_internal_registers

    // TODO: Implement
    // v: The current pointer of the ppu into vram
    uint16 vramAddress;

    // TODO: Implement
    // t: Used to buffer the address writes from the cpu bus:
    uint16 tempVramAddress;

    // TODO: Implement
    // x: Fine X scroll (3 bits)
    uint8 fineX;

    // TODO: Implement
    // w: Single bit used to get two 8 bit values into t
    bool isWriteLatchActive;
};