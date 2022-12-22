#pragma once
#include "platform.h"

// Reference https://www.nesdev.org/wiki/Standard_controller
struct StandardController
{
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
    
    // Activates and deactivates strobe (all other bits are ignored)
    // TODO: Made a gaffe in the refactor, strobe is wired to both ports so it sits a level up
    // Will be fixed in the next round with the general input handling / mapping layer
    void write(uint8 value);

    // Writes the current controller state out to the bits in the register
    // TODO: Figure out how to handle mappings (Either through this class or a generic "fake" gamepad device above this)
    void update(GamePad gamepad);
};
