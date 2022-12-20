#pragma once
#include "romulus.h"

// The envelope unit outputs either a constant volume or a decreasing sawtooth wave
// See https://www.nesdev.org/wiki/APU_Envelope
class Envelope
{
public:
    void setControl(uint8 value);
    void restart();
    void tick();
    uint8 getOutput();

private:
    // Constant ouput, and/or sets the period for the "divider"
    // also called the "envelope parameter" in the wiki
    uint8 volume;

    // When true reloads everything to an initial state
    bool isStartFlagSet;

    // Whether to reset decay level
    bool isLoopFlagSet;

    // Selects between using volume directly, or the decay value
    bool useConstantVolume;

    // Divides the clock to set the speed of the decay
    // The terminology tripped me up for awhile
    uint8 divider;

    // Current level of the downward sawtooth wave
    uint8 decay;
};
