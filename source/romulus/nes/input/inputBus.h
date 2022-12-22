#pragma once
#include "controller.h"
#include "zapper.h"

class InputBus
{
public:
    StandardController controllers[2];
    Zapper zapper;

    void init(PPU* ppu);

    uint8 read(int portNumber);
    void write(uint8 value);

    void update(InputState* rawInput);

private:
    PPU* ppu;
};
