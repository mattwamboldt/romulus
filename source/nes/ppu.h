#pragma once
#include "../common.h"
#include "bus.h"

class PPU
{
public:
    void connect(IBus* bus) { this->bus = bus; }
    void tick();

    // These are used by the mapper to write to the internal registers wince the write has side effects
    // http://wiki.nesdev.com/w/index.php/PPU_registers
    void writeRegister(uint16 address, uint8 value);
    uint8 readRegister(uint16 address);

private:
    IBus* bus;
};