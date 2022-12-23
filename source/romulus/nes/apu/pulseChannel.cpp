#include "pulseChannel.h"

// NOTE: Could be a simpler way, but its the most direct approach without lots of ifs
uint8 dutySequenceTables[4][8] =
{
    {0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 1, 1},
    {0, 0, 0, 0, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 0, 0}
};

void PulseChannel::reset()
{
    isEnabled = false;
    lengthCounter.clear();
}

void PulseChannel::setEnabled(uint8 enable)
{
    isEnabled = enable;
    if (!isEnabled)
    {
        lengthCounter.clear();
    }
}

void PulseChannel::setDutyEnvelope(uint8 value)
{
    dutyCycle = value >> 6;
    envelope.setControl(value);
    lengthCounter.isHalted = value & 0b00100000;
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
        lengthCounter.setValue(value);
    }

    dutySequenceIndex = 0;
    envelope.restart();
}

bool PulseChannel::isSweepMuting()
{
    if (timerLength < 8)
    {
        return true;
    }

    // Don't check for negative in case a wraparound or some weird edge case causes it to go sideways
    // a zero lero length timer just means a super high frequency
    if (sweep.isNegateFlagSet)
    {
        return false;
    }

    return (timerLength + (timerLength >> sweep.shiftCount)) > 0x07FF;
}

void PulseChannel::tickSweep(uint16 index)
{
    // Doing this in here for now since it happens at the same rate
    // may refactor later
    lengthCounter.tick();

    // Minor thing missed in the wiki was that the shift count should be non zero
    if (sweep.dividerValue == 0 && sweep.isEnabled && !isSweepMuting() && sweep.shiftCount != 0)
    {
        uint16 changeAmount = timerLength >> sweep.shiftCount;
        if (sweep.isNegateFlagSet)
        {
            timerLength -= changeAmount;
            timerLength -= index;
        }
        else
        {
            timerLength += changeAmount;
        }
    }

    // TODO: it's very unclear if this stuff should still be happening in all the previous if condition's scenarios
    if (sweep.dividerValue == 0 || sweep.isReloadFlagSet)
    {
        sweep.dividerValue = sweep.dividerPeriod;
        sweep.isReloadFlagSet = false;
    }
    else
    {
        --sweep.dividerValue;
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

    if (!lengthCounter.active())
    {
        return 0;
    }

    return envelope.getOutput();
}
