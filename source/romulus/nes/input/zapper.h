#pragma once

#include "platform.h"
#include "nes/ppu/ppu.h"

class Zapper
{
public:
    uint8 read(PPU* ppu);
    void update(Mouse mouse, real32 elapsedMs);
    
private:
    int32 x;
    int32 y;

    // Used to track how long the "half pull" state should be active
    real32 activeCounterMs;

    bool lightDetected(PPU* ppu);
};
