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
    // TODO: Implement
    void setMask(uint8 value);
    
    uint8 getStatus();

    // TODO: Implement
    void setOamAddress(uint8 value);
    // TODO: Implement
    void setOamData(uint8 value);
    // TODO: Implement
    uint8 getOamData();

    // TODO: Implement
    void setScroll(uint8 value);
    // TODO: Implement
    void setAddress(uint8 value);

    // TODO: Implement
    void setData(uint8 value);
    // TODO: Implement
    uint8 getData();

    // TODO: This probably doens't make sense to happen in the PPU actually
    // Should probably happen a layer above
    void setOamDma(uint8 value) {}

    uint32 cycle;
    uint32 scanline;

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
    uint16 baseNametableAddress;
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
};