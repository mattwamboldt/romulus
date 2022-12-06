#include "pulseChannel.h"

// NOTE: Could be a simpler way, but its the most direct approach without lots of ifs
uint8 dutySequenceTables[4][8] =
{
    {0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 1, 1},
    {0, 0, 0, 0, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 0, 0}
};

uint8 lengthCounterLookup[32] =
{
    // 0x00 - 0x0F
    10,254, 20,  2, 40,  4, 80,  6, 160,  8, 60, 10, 14, 12, 26, 14,
    // 0x10 - 0x1F
    12, 16, 24, 18, 48, 20, 96, 22, 192, 24, 72, 26, 16, 28, 32, 30
};

void PulseChannel::setEnabled(uint8 enable)
{
    isEnabled = enable;
    if (!isEnabled)
    {
        lengthCounter = 0;
    }
}

void PulseChannel::setDutyEnvelope(uint8 value)
{
    dutyCycle = value >> 6;
    envelope.setControl(value);
    isLengthCounterHalted = value & 0b00100000;
}

void PulseChannel::setSweep(uint8 value)
{
    sweep.isEnabled = value & 0x80;
    sweep.dividerPeriod = (value >> 4) & 0x07;
    sweep.isNegateFlagSet = value & 0x08;
    sweep.shiftCount = value & 0x07;
    sweep.isReloadFlagSet = true;
}

void PulseChannel::setTimerLo(uint8 value)
{
    timerLength &= 0xFF00;
    timerLength |= value;
}

// Also sets length counter and triggers some side effects
void PulseChannel::setTimerHi(uint8 value)
{
    timerLength &= 0x00FF;
    timerLength |= ((uint16)(value & 0b00000111)) << 8;

    if (isEnabled)
    {
        uint8 lengthCounterLoad = value >> 3;
        lengthCounter = lengthCounterLookup[lengthCounterLoad];
    }

    dutySequenceIndex = 0;
    envelope.restart();
}

uint16 PulseChannel::getSweepTargetPeriod()
{
    uint16 targetPeriod = timerLength;
    uint16 changeAmount = timerLength >> sweep.shiftCount;
    if (sweep.isNegateFlagSet)
    {
        targetPeriod = timerLength - changeAmount;
    }
    else
    {
        targetPeriod = timerLength + changeAmount;
    }

    return targetPeriod;
}

bool PulseChannel::isSweepMuting()
{
    if (timerLength < 8)
    {
        return true;
    }

    return getSweepTargetPeriod() > 0x07FF;
}

void PulseChannel::tickSweep()
{
    if (sweep.dividerValue == 0 && sweep.isEnabled && !isSweepMuting())
    {
        timerLength = getSweepTargetPeriod();
    }

    if (sweep.dividerValue == 0 || sweep.isReloadFlagSet)
    {
        sweep.dividerValue = sweep.dividerPeriod;
    }
    else
    {
        --sweep.dividerValue;
    }

    // Doing this in here for now since it happens at the same rate
    // may refactor later
    if (lengthCounter != 0 && !isLengthCounterHalted)
    {
        --lengthCounter;
        if (lengthCounter == 0)
        {
            isEnabled = 0;
        }
    }
}

void PulseChannel::tick()
{
    // ticks the squence timer per apu frame, not sure if this should happen before or after sweep/envelope stuff
    if (timerCurrentTick == 0)
    {
        if (dutySequenceIndex == 0)
        {
            dutySequenceIndex = 7;
        }
        else
        {
            --dutySequenceIndex;
        }

        timerCurrentTick = timerLength;
    }
    else
    {
        --timerCurrentTick;
    }
}

uint8 PulseChannel::getOutput()
{
    if (!isEnabled)
    {
        return 0;
    }

    if (isSweepMuting())
    {
        return 0;
    }

    if (!dutySequenceTables[dutyCycle][dutySequenceIndex])
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
