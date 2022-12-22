#pragma once
#include "bus.h"
#include "ppu/ppu.h"
#include "apu/apu.h"
#include "cartridge.h"
#include "input/zapper.h"
#include "input/controller.h"

// TODO: Pull DMA into a new CPU that wraps the apu features and the dma controller, like it is on the nes
// TODO: Pull Input handling out into its own module and let this and the ppubus be subsumed into the cartridge / mapper stuff

class CPUBus : public IBus
{
public:
    void connect(PPU* ppu, APU* apu, Cartridge* cart);

    uint8 read(uint16 address);
    void write(uint16 address, uint8 value);
    void setGamepad(GamePad pad, int number);
    void setMouse(Mouse mouse, real32 elapsedMs);

    void setReadOnly(bool enable) { readOnly = enable; cart->isReadOnly = enable; };

    void tickDMA();

    // DMA Data
    bool isDmaActive;
    uint16 dmaAddress;
    uint16 dmaCycleCount;
    uint8 dmaReadValue;

private:
    PPU* ppu;
    APU* apu;
    Cartridge* cart;

    uint8 ram[2 * 1024];

    StandardController controllers[2];
    Zapper zapper;

    uint8 ppuOpenBusValue;

    // Debug implementation detail, for logging
    bool readOnly;
};