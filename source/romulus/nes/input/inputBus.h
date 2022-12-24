#pragma once
#include "controller.h"
#include "zapper.h"

enum NESDeviceType
{
    DISCONNECTED = 0,
    STANDARD_CONTROLLER,
    ZAPPER,
};

struct Port
{
    NESDeviceType device;
    int8 index;
};

class InputBus
{
public:
    StandardController controllers[2];
    Zapper zapper;

    void init(PPU* ppu);

    uint8 read(int portNumber);
    void write(uint8 value);

    void update(InputState* rawInput);

    Port ports[2];

private:
    PPU* ppu;

    GamePad keyboardVirtualGamepad;
};
