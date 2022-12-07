#pragma once
#include "../common.h"
#include "bus.h"

class PPU
{
public:
    void connect(IBus* bus) { this->bus = bus; }
    void tick();

    bool isNMIFlagSet();

    // CPU <=> PPU Bus functions

    void setControl(uint8 value);
    void setMask(uint8 value);
    
    uint8 getStatus();

    void setOamAddress(uint8 value);
    void setOamData(uint8 value);
    uint8 getOamData();

    void setScroll(uint8 value);
    void setAddress(uint8 value);

    void setData(uint8 value);
    uint8 getData();

    // TODO: This probably doens't make sense to happen in the PPU actually
    // Should probably happen a layer above
    void setOamDma(uint8 value) {}

    uint32 cycle;
    uint32 scanline;

private:
    IBus* bus;

    bool nmiRequested;
    bool nmiEnabled;

    bool useTallSprites;
    uint16 baseNametableAddress;
    uint16 vramAddressIncrement;
    uint16 spritePatternBaseAddress;
    uint16 backgroundPatternBaseAddress;
};