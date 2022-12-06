#include "triangleChannel.h"

extern uint8 lengthCounterLookup[32];

uint8 triangleSequence[32] =
{
    15, 14, 13, 12, 11, 10,  9,  8,  7,  6,  5,  4,  3,  2,  1,  0,
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
};

void TriangleChannel::setEnabled(uint8 enable)
{
    isEnabled = enable;
    if (!isEnabled)
    {
        lengthCounter = 0;
    }
}

void TriangleChannel::setLinearCounter(uint8 value)
{
    isControlFlagSet = (value & 0x80) > 0;
    linearReloadValue = value & 0x7F;
}

void TriangleChannel::setTimerLo(uint8 value)
{
    timerLength &= 0xFF00;
    timerLength |= value;
}

void TriangleChannel::setTimerHi(uint8 value)
{
    timerLength &= 0x00FF;
    timerLength |= ((uint16)(value & 0x07)) << 8;

    if (isEnabled)
    {
        uint8 lengthCounterLoad = value >> 3;
        lengthCounter = lengthCounterLookup[lengthCounterLoad];
    }

    isLinearReloadFlagSet = true;
}

void TriangleChannel::tick()
{
    if (timerCurrentTick == 0)
    {
        if (lengthCounter > 0 && linearCounter > 0)
        {
            ++sequenceIndex;
            if (sequenceIndex >= 32)
            {
                sequenceIndex = 0;
            }
        }

        timerCurrentTick = timerLength;
    }
    else
    {
        --timerCurrentTick;
    }
}

void TriangleChannel::tickLengthCounter()
{
    if (lengthCounter != 0 && !isControlFlagSet)
    {
        --lengthCounter;
    }
}

void TriangleChannel::tickLinearCounter()
{
    if (isLinearReloadFlagSet)
    {
        linearCounter = linearReloadValue;
    }
    else if (linearCounter != 0)
    {
        --linearCounter;
    }

    if (!isControlFlagSet)
    {
        isLinearReloadFlagSet = false;
    }
}

uint8 TriangleChannel::getOutput()
{
    return triangleSequence[sequenceIndex];
}
