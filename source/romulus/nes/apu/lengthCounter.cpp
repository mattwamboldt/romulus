#include "lengthCounter.h"

uint8 lengthCounterLookup[32] =
{
    // 0x00 - 0x0F
    10,254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    // 0x10 - 0x1F
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

void LengthCounter::tick()
{
    if (value != 0 && !isHalted)
    {
        --value;
    }
}

void LengthCounter::setValue(uint8 v)
{
    value = lengthCounterLookup[v >> 3];
}
