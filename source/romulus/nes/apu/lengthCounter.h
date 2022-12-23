#pragma once
#include "romulus.h"

struct LengthCounter
{
    uint8 value;
    bool isHalted;

    void setValue(uint8 value);
    void tick();

    inline void clear() { value = 0; }

    // NOTE: This may be wrong, theres mention of the channel being silenced on becoming 0 rather than if zero
    inline bool active() { return value > 0; }
};