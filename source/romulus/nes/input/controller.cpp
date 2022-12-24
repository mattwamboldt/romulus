#include "controller.h"

// Notes about input on the NES
// You write to 4016 to tell connected devices to update state
// Then you turn it off so they are primed and ready
// After that you read the state ONE BIT AT A TIME!
// 
// For controllers thats a sequence of buttons on or off, and if
// you don't turn off strobe you always get the same one
// Thats because it loads it all into a "shift register", which
// shifts one bit out on each read.

uint8 StandardController::read()
{
    if (strobeActive)
    {
        strobeState = currentState;
        shiftCount = 0;
    }

    // TODO: Theres mention of this being 1 forever after the first 8 bits but that caused me issues
    if (shiftCount >= 8)
    {
        return 0;
    }

    uint8 result = strobeState & 0x01;
    strobeState >>= 1;
    ++shiftCount;
    return result;
}

void StandardController::setStrobe(bool active)
{
    strobeActive = active;
    if (strobeActive)
    {
        strobeState = currentState;
        shiftCount = 0;
    }
}

void StandardController::update(GamePad gamepad)
{
    currentState = 0;

    uint8 currentBit = BIT_0;
    for (int i = 0; i < NUM_BUTTONS; ++i)
    {
        if (gamepad.buttons[buttonMap[i]].isPressed)
        {
            currentState |= currentBit;
        }

        currentBit <<= 1;
    }

    // Prevent simultaneous Up and Down
    if ((currentState & 0x30) == 0x30)
    {
        currentState &= 0xCF;
    }

    // Prevent simultaneous Left and Right
    if ((currentState & 0xC0) == 0xC0)
    {
        currentState &= 0x3F;
    }
}

void StandardController::setButton(uint8 index, bool active)
{
    uint8 desiredBit = 1 << index;
    if (active)
    {
        currentState |= desiredBit;
    }
    else
    {
        currentState &= ~desiredBit;
    }
}