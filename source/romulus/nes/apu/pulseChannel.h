#pragma once

#include "envelope.h"
#include "lengthCounter.h"

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
    void tickSweep(uint16 index);

    uint8 getOutput();

    bool isSweepMuting();

    uint8 isEnabled;

    uint8 dutyCycle;
    uint8 dutySequenceIndex;

    LengthCounter lengthCounter;

    // Clocks the duty cycle sequencer
    uint16 timerLength;
    uint16 timerCurrentTick;

    Envelope envelope;
    SweepUnit sweep;
};
