#include "envelope.h"

void Envelope::setControl(uint8 value)
{
    volume = value & 0b00001111;
    useConstantVolume = value & 0b00010000;
    isLoopFlagSet = value & 0b00100000;
}

void Envelope::restart()
{
    isStartFlagSet = true;
}

void Envelope::tick()
{
    if (isStartFlagSet)
    {
        isStartFlagSet = false;
        decay = 15;
        divider = volume;
        return;
    }

    if (divider != 0)
    {
        --divider;
        return;
    }

    divider = volume;
    if (decay > 0)
    {
        --decay;
    }
    else if (isLoopFlagSet)
    {
        decay = 15;
    }
}

uint8 Envelope::getOutput()
{
    if (useConstantVolume)
    {
        return volume;
    }

    return decay;
}