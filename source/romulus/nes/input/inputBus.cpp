#include "inputBus.h"

// TODO: implement the controller and button mapping at this level

void InputBus::init(PPU* ppu)
{
    this->ppu = ppu;

    // For now hard wiring port 0 to the controller and 1 to the zapper
    ports[0].device = STANDARD_CONTROLLER;
    ports[0].index = 1;
    ports[1].device = ZAPPER;

    // TODO: This is a garbage mapping scheme... Should probably be driven from the source devices.
    // For now code it the dumb way to get something working and clean it up once it's functional

    // Standard Gamepad Mapping
    controllers[0].sourceDeviceId = 0;
    controllers[0].buttonMap[StandardController::A] = GamePad::B;
    controllers[0].buttonMap[StandardController::B] = GamePad::A;
    controllers[0].buttonMap[StandardController::SELECT] = GamePad::SELECT;
    controllers[0].buttonMap[StandardController::START] = GamePad::START;
    controllers[0].buttonMap[StandardController::UP] = GamePad::UP;
    controllers[0].buttonMap[StandardController::DOWN] = GamePad::DOWN;
    controllers[0].buttonMap[StandardController::LEFT] = GamePad::LEFT;
    controllers[0].buttonMap[StandardController::RIGHT] = GamePad::RIGHT;

    // Standard Keyboard Mapping (Using windows VK codes for now, Once this is abstracted we'll get it loaded from the platform at launch)
    controllers[1].sourceDeviceId = -1;
    controllers[1].buttonMap[StandardController::A] = 'Z';
    controllers[1].buttonMap[StandardController::B] = 'X';
    controllers[1].buttonMap[StandardController::SELECT] = 0x20;
    controllers[1].buttonMap[StandardController::START] = 0x0D;
    controllers[1].buttonMap[StandardController::UP] = 0x26;
    controllers[1].buttonMap[StandardController::DOWN] = 0x28;
    controllers[1].buttonMap[StandardController::LEFT] = 0x25;
    controllers[1].buttonMap[StandardController::RIGHT] = 0x27;
}

uint8 InputBus::read(int portNumber)
{
    switch (ports[portNumber].device)
    {
        case STANDARD_CONTROLLER:
            return controllers[ports[portNumber].index].read();

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
        if (ports[i].device == ZAPPER)
        {
            zapper.update(rawInput->mouse, rawInput->elapsedMs);
        }
        else if (ports[i].device == STANDARD_CONTROLLER)
        {
            int8 deviceIndex = ports[i].index;
            StandardController* device = controllers + deviceIndex;
            if (device->sourceDeviceId < 0)
            {
                // THIS IS BAD!!!, I HATE ME!!! WHY AM I A PROGRAMMER
                for (int e = 0; e < rawInput->numKeyboardEvents; ++e)
                {
                    uint8 keycode = rawInput->keyboardEvents[e].keycode;
                    for (uint8 b = 0; b < StandardController::NUM_BUTTONS; ++b)
                    {
                        uint8 mappedCharacter = device->buttonMap[b];
                        if (keycode == mappedCharacter)
                        {
                            device->setButton(b, rawInput->keyboardEvents[e].isPress);
                            break;
                        }
                    }
                }
            }
            else
            {
                device->update(rawInput->controllers[device->sourceDeviceId]);
            }
        }
    }
}
