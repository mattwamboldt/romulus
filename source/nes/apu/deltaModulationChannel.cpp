#include "deltaModulationChannel.h"

uint16 dmcRateTable[] =
{
    428, 380, 340, 320, 286, 254, 226, 214, 190, 160, 142, 128, 106,  84,  72,  54
};

void DeltaModulationChannel::tick()
{
    if (timerCurrentTick == 0)
    {
        // End of an "output cycle"
        if (bitsRemaining == 0)
        {
            bitsRemaining = 8;
            if (!sampleBufferFilled)
            {
                silenceActive = true;
            }
            else
            {
                silenceActive = false;
                outputShiftRegister = sampleBuffer;
                sampleBufferFilled = false;
            }
        }

        if (!silenceActive)
        {
            // output veles are constrained to the range of 0-127
            bool isIncrement = outputShiftRegister & BIT_0;
            if (isIncrement && outputLevel <= 125)
            {
                outputLevel += 2;
            }
            else if (!isIncrement && outputLevel >= 2)
            {
                outputLevel -= 2;
            }
        }

        outputShiftRegister >>= 1;
        --bitsRemaining;

        timerCurrentTick = timerLength;
    }
    else
    {
        --timerCurrentTick;
    }
}

void DeltaModulationChannel::setEnabled(uint8 enabled)
{
    isEnabled = enabled > 0;
    if (isEnabled)
    {
        if (bytesRemaining == 0)
        {
            currentAddress = sampleAddress;
            bytesRemaining = sampleLength;
        }
    }
    else
    {
        bytesRemaining = 0;
    }

    isInterruptFlagSet = false;
}

void DeltaModulationChannel::setControls(uint8 value)
{
    irqEnabled = (value & BIT_7) > 0;
    isLooping = (value & BIT_6) > 0;
    timerLength = dmcRateTable[value & 0x0F];
}

void DeltaModulationChannel::setOutputLevel(uint8 value)
{
    outputLevel = value & 0x7F;
}

void DeltaModulationChannel::setSampleAddress(uint8 value)
{
    sampleAddress = 0xC000 + ((uint16)value << 6);
}

void DeltaModulationChannel::setSampleLength(uint8 value)
{
    sampleLength = ((uint16)value << 4) + 1;
}

bool DeltaModulationChannel::readRequired()
{
    return isEnabled && !sampleBufferFilled && bytesRemaining > 0;
}

void DeltaModulationChannel::loadSample(uint8 sample)
{
    sampleBufferFilled = true;
    sampleBuffer = sample;

    // Handle advancing to the next instruction
    ++currentAddress;
    if (currentAddress == 0)
    {
        currentAddress = 0x8000;
    }

    --bytesRemaining;
    if (bytesRemaining == 0)
    {
        if (isLooping)
        {
            currentAddress = sampleAddress;
            bytesRemaining = sampleLength;
        }
        else if (irqEnabled)
        {
            isInterruptFlagSet = true;
        }
    }
}