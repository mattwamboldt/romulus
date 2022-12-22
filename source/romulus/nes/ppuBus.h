#pragma once
#include "bus.h"
#include "ppu/ppu.h"
#include "cartridge.h"

class PPUBus : public IBus
{
public:
    void connect(Cartridge* cart);

    uint8 read(uint16 address);
    void write(uint16 address, uint8 value);

    // Ensures reads have no side effects (Used for logging and debug views)
    void setReadOnly(bool enable) { cart->isReadOnly = enable; };

private:
    Cartridge* cart;

    uint8 vram[2 * 1024] = {};
    uint8 paletteRam[32];
};