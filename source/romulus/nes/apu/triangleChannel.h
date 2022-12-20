#pragma once
#include "romulus.h"

// https://www.nesdev.org/wiki/APU_Triangle
struct TriangleChannel
{
    void reset();

    void setEnabled(uint8 enable);
    void setLinearCounter(uint8 value);
    void setTimerLo(uint8 value);
    void setTimerHi(uint8 value);

    void tick();
    void tickLengthCounter();
    void tickLinearCounter();

    uint8 getOutput();
    
    uint8 isEnabled;

    uint8 lengthCounter;

    // Also the lengthCounter halt per the wiki
    bool isControlFlagSet;
    bool isLinearReloadFlagSet;

    uint8 linearCounter;
    uint8 linearReloadValue;

    // Clocks sequencer
    uint16 timerLength;
    uint16 timerCurrentTick;

    uint8 sequenceIndex;
};
