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

    // NOTE: DO NOT DO THIS NORMALLY, BAD
    // TODO: Proper mapping (You'll note a and b are swapped, cause xbox controller
    if (gamepad.bPressed) currentState |= 0x01;
    if (gamepad.aPressed) currentState |= 0x02;
    if (gamepad.selectPressed) currentState |= 0x04;
    if (gamepad.startPressed) currentState |= 0x08;
    if (gamepad.upPressed) currentState |= 0x10;
    if (gamepad.downPressed) currentState |= 0x20;
    if (gamepad.leftPressed) currentState |= 0x40;
    if (gamepad.rightPressed) currentState |= 0x80;

    // Noticed this as a setting on other emulators and realized I forgot to block this
    // Probably never caught it cause I use a gamepad but trivial to do on keyboard
    if (gamepad.upPressed && gamepad.downPressed)
    {
        currentState &= 0xCF;
    }

    if (gamepad.leftPressed && gamepad.rightPressed)
    {
        currentState &= 0x3F;
    }
}
