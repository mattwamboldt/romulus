#pragma once

#include "envelope.h"

struct SweepUnit
{
    // Configuration/params
    bool isEnabled;
    bool isNegateFlagSet;
    uint8 dividerPeriod;
    uint8 shiftCount;

    // Internal to the unit
    bool isReloadFlagSet;
    uint8 dividerValue;
};

// https://www.nesdev.org/wiki/APU_Pulse
class PulseChannel
{
public:
    void reset();

    void setEnabled(uint8 enable);

    void setDutyEnvelope(uint8 value);
    void setSweep(uint8 value);
    void setTimerLo(uint8 value);
    // Also sets length counter and triggers some side effects
    void setTimerHi(uint8 value);

    void tick();
    void tickSweep();

    uint8 getOutput();

    bool isSweepMuting();

    uint8 isEnabled;

    uint8 dutyCycle;
    uint8 dutySequenceIndex;

    // Determines how long the wave will continue
    // Stops when clocked and set to zero, not when it becomes zero
    uint8 lengthCounter;
    bool isLengthCounterHalted;

    // Clocks the duty cycle sequencer
    uint16 timerLength;
    uint16 timerCurrentTick;

    Envelope envelope;
    SweepUnit sweep;
};
