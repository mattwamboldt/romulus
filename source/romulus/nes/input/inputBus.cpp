#include "inputBus.h"

// TODO: implement the controller and button mapping at this level

void InputBus::init(PPU* ppu)
{
    this->ppu = ppu;
    // For now hard wiring port 0 to the controller and 1 to the zapper
    ports[0] = STANDARD_CONTROLLER;
    ports[1] = ZAPPER;
}

uint8 InputBus::read(int portNumber)
{
    switch (ports[portNumber])
    {
        case STANDARD_CONTROLLER:
            return controllers[portNumber].read();

        case ZAPPER:
            return zapper.read(ppu);
    }

    return 0;
}

void InputBus::write(uint8 value)
{
    bool strobeActive = (value & 0x01) > 0;
    controllers[0].setStrobe(strobeActive);
    controllers[1].setStrobe(strobeActive);
}

void InputBus::update(InputState* rawInput)
{
    // TODO: Come up with some kind of mechanism for custom input mapping that doesn't suck
    // TODO: Handle keyboard

    for (int i = 0; i < 2; ++i)
    {
        controllers[i].update(rawInput->controllers[i]);
    }

    zapper.update(rawInput->mouse, rawInput->elapsedMs);
}
