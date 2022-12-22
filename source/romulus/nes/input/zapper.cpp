#include "zapper.h"
#include <nes/constants.h>

// TODO: Implement playchoice/vs system version that goes through the strobing mechanic like a normal controller

uint8 Zapper::read(PPU* ppu)
{
    uint8 output = 0;
    if (!lightDetected(ppu))
    {
        output = 0x08;
    }

    if (activeCounterMs > 0)
    {
        output |= 0x10;
    }

    return output;
}

void Zapper::update(Mouse mouse, real32 elapsedMs)
{
    x = mouse.xPosition;
    y = mouse.yPosition;

    if (!mouse.left.wasPressed && mouse.left.isPressed)
    {
        // TODO: May have to put prevention in for clicking within the existing timer
        // But who could realistically click faster than 80ms?
        activeCounterMs = 0.08f;
    }
    else if (activeCounterMs > 0)
    {
        activeCounterMs -= elapsedMs;
    }
}

bool Zapper::lightDetected(PPU* ppu)
{
    // We assume darkness when pointed away from the screen
    if (x < 0 || x >= NES_SCREEN_WIDTH || y < 0 || y >= NES_SCREEN_HEIGHT)
    {
        return false;
    }

    // The ppu hasn't rendered this out yet so the light sensor would be dead
    if (y > ppu->scanline || (y == ppu->scanline && x >= ppu->cycle))
    {
        return false;
    }

    // Detect how long it's been "on" if on at all.
    // ---------------------------------------------
    // The wiki mentions a couple different values for how long it stays active from the point of detection
    // For simplicity (ie I only care about the duck hunt level of "yes/no" detection working for now). I'll
    // stick with around the pure white to light grey numbers.
    if (ppu->scanline - y > 25)
    {
        return false;
    }

    // TODO: Some roms use a different palette because they're meant for other systems, like vs and playchoice 10
    uint8 paletteIndex = ppu->backbuffer[x + (y * NES_SCREEN_WIDTH)];
    return palette[paletteIndex] > 0;
}
