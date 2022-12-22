#include "noiseChannel.h"

// TODO: Repeating myself a bunch once all the channels are working I'll do a refactor pass

extern uint8 lengthCounterLookup[32];

// TODO: This has separate tables for ntsc and pal, just doing ntsc for now
uint16 noiseTimerLengthLookup[16] =
{
    4, 8, 16, 32, 64, 96, 128, 160, 202, 254, 380, 508, 762, 1016, 2034, 4068
};

void NoiseChannel::reset()
{
    isEnabled = false;
    lengthCounter = 0;
}

void NoiseChannel::setEnabled(uint8 enabled)
{
    isEnabled = enabled;
    if (!isEnabled)
    {
        lengthCounter = 0;
    }
}

void NoiseChannel::setEnvelope(uint8 value)
{
    envelope.setControl(value);
    isLengthCounterHalted = value & 0x20;
}

void NoiseChannel::setPeriod(uint8 value)
{
    mode = value >> 7;
    uint8 period = value & 0x0F;
    timerLength = noiseTimerLengthLookup[period];
}

void NoiseChannel::setLengthCounter(uint8 value)
{
    if (isEnabled)
    {
        uint8 lengthCounterLoad = value >> 3;
        lengthCounter = lengthCounterLookup[lengthCounterLoad];
    }

    envelope.restart();
}

void NoiseChannel::tick()
{
    if (timerCurrentTick == 0)
    {
        uint16 feedback = 0;
        uint16 currentBit = shiftRegister & 0x0001;

        if (mode)
        {
            feedback = currentBit ^ ((shiftRegister & 0x0040) >> 6);
        }
        else
        {
            feedback = currentBit ^ ((shiftRegister & 0x0002) >> 1);
        }

        shiftRegister >>= 1;
        shiftRegister &= 0xBFFF;
        shiftRegister |= (feedback << 14);

        timerCurrentTick = timerLength;
    }
    else
    {
        --timerCurrentTick;
    }
}

void NoiseChannel::tickLengthCounter()
{
    if (lengthCounter != 0 && !isLengthCounterHalted)
    {
        --lengthCounter;
    }
}

uint8 NoiseChannel::getOutput()
{
    if (!isEnabled)
    {
        return 0;
    }

    if (shiftRegister & 0x0001)
    {
        return 0;
    }

    // NOTE: This may be wrong, theres mention of the channel being silenced on becoming 0 rather than if zero
    if (lengthCounter == 0)
    {
        return 0;
    }

    return envelope.getOutput();
}
