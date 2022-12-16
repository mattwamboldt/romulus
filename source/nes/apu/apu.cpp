#include "apu.h"

const bool DEBUG_PULSE1_MUTE = 0;
const bool DEBUG_PULSE2_MUTE = 0;
const bool DEBUG_TRIANGLE_MUTE = 0;
const bool DEBUG_NOISE_MUTE = 0;
const bool DEBUG_DMC_MUTE = 1;

void APU::quarterClock()
{
    // Tick the envelopes and triangle linear counter
    pulse1.envelope.tick();
    pulse2.envelope.tick();
    noise.envelope.tick();
    triangle.tickLinearCounter();
}

void APU::halfClock()
{
    quarterClock();

    // Tick the length counters and sweep units
    pulse1.tickSweep();
    pulse2.tickSweep();
    triangle.tickLengthCounter();
    noise.tickLengthCounter();
}

void APU::tick(uint32 cpuCycleCount)
{
    triangle.tick();

    if (cpuCycleCount % 2 == 0)
    {
        return;
    }

    if (frameCounterResetRequested)
    {
        frameCounter = 0;
        frameCounterResetRequested = false;
        if (isFiveStepMode)
        {
            halfClock();
        }
    }

    bool sequenceComplete = false;

    // Sorry for the magic numbers here but the sequence/frame timing is uneven
    // This'll get worse when PAl comes in
    if (frameCounter == 3728)
    {
        quarterClock();
    }
    else if (frameCounter == 7456)
    {
        halfClock();
    }
    else if (frameCounter == 11185)
    {
        quarterClock();
    }
    else if (frameCounter == 14914 && !isFiveStepMode)
    {
        halfClock();
        isFrameInteruptFlagSet = !isInterruptInhibited;
        sequenceComplete = true;
    }
    else if (frameCounter == 18640 && isFiveStepMode)
    {
        halfClock();
        sequenceComplete = true;
    }

    pulse1.tick();
    pulse2.tick();
    noise.tick();
    dmc.tick();

    if (sequenceComplete)
    {
        frameCounter = 0;
    }
    else
    {
        ++frameCounter;
    }
}

// https://www.nesdev.org/wiki/APU#Status_($4015)
uint8 APU::getStatus(bool readOnly)
{
    uint8 result = 0;
    if (pulse1.lengthCounter > 0)   result |= 0x01;
    if (pulse2.lengthCounter > 0)   result |= 0x02;
    if (triangle.lengthCounter > 0) result |= 0x04;
    if (noise.lengthCounter > 0)    result |= 0x08;
    if (dmc.getBytesRemaining())    result |= 0x10;
    if (isFrameInteruptFlagSet)     result |= 0x40;
    if (dmc.isInterruptFlagSet)      result |= 0x80;

    if (!readOnly)
    {
        // TODO: Figure out how to handle this or if I have to, will be easier when tick is mapped to cpu tick
        // "If an interrupt flag was set at the same moment of the read, it will read back as 1 but it will not be cleared."
        isFrameInteruptFlagSet = false;
    }

    return result;
}

void APU::writeControl(uint8 value)
{
    pulse1.setEnabled(value & 0b00000001);
    pulse2.setEnabled(value & 0b00000010);
    triangle.setEnabled(value & 0b00000100);
    noise.setEnabled(value & 0b00001000);
    dmc.setEnabled(value & 0b00010000);
}

void APU::writeFrameCounterControl(uint8 value)
{
    isFiveStepMode = (value & 0x80) > 0;
    
    isInterruptInhibited = (value & 0x40) > 0;
    if (isInterruptInhibited)
    {
        isFrameInteruptFlagSet = false;
    }

    frameCounterResetRequested = true;
}

real32 APU::getOutput()
{
    // TODO: Actual mixing, for now just getting something out

    uint32 output = 0;
    real32 maxValue = 0;
    if (!DEBUG_PULSE1_MUTE)
    {
        output += pulse1.getOutput();
        maxValue += 15.0f;
    }

    if (!DEBUG_PULSE2_MUTE)
    {
        output += pulse2.getOutput();
        maxValue += 15.0f;
    }
    
    if (!DEBUG_TRIANGLE_MUTE)
    {
        output += triangle.getOutput();
        maxValue += 15.0f;
    }

    if (!DEBUG_NOISE_MUTE)
    {
        output += noise.getOutput();
        maxValue += 15.0f;
    }

    if (!DEBUG_DMC_MUTE)
    {
        output += dmc.getOutput();
        maxValue += 127.0f;
    }

    if (maxValue == 0)
    {
        return 0.0f;
    }

    real32 returnValue = output / maxValue;
    if (returnValue > 1.0)
    {
        return 1.0;
    }

    return returnValue;
}