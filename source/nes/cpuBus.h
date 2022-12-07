#pragma once
#include "bus.h"
#include "ppu.h"
#include "apu/apu.h"
#include "cartridge.h"
#include "gamepad.h"

struct ControllerState
{
    NESGamePad input;
    uint8 shiftRegister;
    // Track how many bits we're shifted so we can return 1 on completion
    uint8 shiftCount;
};

class CPUBus : public IBus
{
public:
    void connect(PPU* ppu, APU* apu, Cartridge* cart);

    uint8 read(uint16 address);
    uint16 readWord(uint16 address);
    void write(uint16 address, uint8 value);
    void setInput(NESGamePad pad, int number);

private:
    uint8 readGamepad(int number);

    PPU* ppu;
    APU* apu;
    Cartridge* cart;

    uint8 ram[2 * 1024];

    // https://www.nesdev.org/wiki/Standard_controller
    bool inputStrobeActive;
    ControllerState controllers[2];

    WriteCallback writeCallback;
};