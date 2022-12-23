#pragma once
#include "envelope.h"
#include "lengthCounter.h"

// https://www.nesdev.org/wiki/APU_Noise
struct NoiseChannel
{
    void reset();

    void setEnabled(uint8 enabled);

    void setEnvelope(uint8 value);
    void setPeriod(uint8 value);
    void setLengthCounter(uint8 value);

    void tick();

    uint8 getOutput();

    uint8 isEnabled;

    LengthCounter lengthCounter;

    uint16 timerLength;
    uint16 timerCurrentTick;

    uint8 mode;
    uint16 shiftRegister;

    Envelope envelope;
};