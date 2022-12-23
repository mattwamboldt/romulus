#pragma once
#include "platform.h"

// Reference https://www.nesdev.org/wiki/Standard_controller
struct StandardController
{
    enum Buttons
    {
        A,
        B,
        SELECT,
        START,
        UP,
        DOWN,
        LEFT,
        RIGHT
    };

    // The bit pattern as of the last update
    uint8 currentState;
    // The bit pattern as of the last "strobe"
    uint8 strobeState;
    // How many bits shifted since the last strobe so we can return 1 on completion
    uint8 shiftCount;
    // Reloads the curent state on every read while active
    bool strobeActive;

    // Gets the next bit from the serial port (ie lowest bit of the strobe state)
    uint8 read();
    
    // Sets and ticks strobe accordingly
    void setStrobe(bool active);

    // Writes the current controller state out to the bits in the register
    // TODO: Figure out how to handle mappings (Either through this class or a generic "fake" gamepad device above this)
    void update(GamePad gamepad);
};
