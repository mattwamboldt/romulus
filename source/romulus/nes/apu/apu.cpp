#include "apu.h"

const bool DEBUG_PULSE1_MUTE = 0;
const bool DEBUG_PULSE2_MUTE = 0;
const bool DEBUG_TRIANGLE_MUTE = 0;
const bool DEBUG_NOISE_MUTE = 0;
const bool DEBUG_DMC_MUTE = 0;

void APU::reset()
{
    pulse1.reset();
    pulse2.reset();
    noise.reset();
    triangle.reset();
    dmc.reset();

    frameCounter = 0;
    frameCounterResetRequested = false;
    isFrameInteruptFlagSet = false;
    isFiveStepMode = false;
    isInterruptInhibited = false;
}

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
    pulse1.tickSweep(0);
    pulse2.tickSweep(1);
    triangle.lengthCounter.tick();
    noise.lengthCounter.tick();
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
    if (pulse1.lengthCounter.active())   result |= BIT_0;
    if (pulse2.lengthCounter.active())   result |= BIT_1;
    if (triangle.lengthCounter.active()) result |= BIT_2;
    if (noise.lengthCounter.active())    result |= BIT_3;
    if (dmc.getBytesRemaining())         result |= BIT_4;
    if (isFrameInteruptFlagSet)          result |= BIT_6;
    if (dmc.isInterruptFlagSet)          result |= BIT_7;

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
    pulse1.setEnabled(value & BIT_0);
    pulse2.setEnabled(value & BIT_1);
    triangle.setEnabled(value & BIT_2);
    noise.setEnabled(value & BIT_3);
    dmc.setEnabled(value & BIT_4);
}

void APU::writeFrameCounterControl(uint8 value)
{
    isFiveStepMode = (value & BIT_7) > 0;
    isInterruptInhibited = (value & BIT_6) > 0;

    if (isInterruptInhibited)
    {
        isFrameInteruptFlagSet = false;
    }

    frameCounterResetRequested = true;
}

real32 APU::getOutput()
{
    // Using the linear approximation for now
    // https://www.nesdev.org/wiki/APU_Mixer#Linear_Approximation

    real32 pulseOutput = 0.0f;
    
    if (!DEBUG_PULSE1_MUTE)
    {
        pulseOutput += pulse1.getOutput();
    }

    if (!DEBUG_PULSE2_MUTE)
    {
        pulseOutput += pulse2.getOutput();
    }

    pulseOutput *= 0.00752f;
    
    real32 tndOut = 0.0f;

    if (!DEBUG_TRIANGLE_MUTE)
    {
        tndOut += 0.00851f * triangle.getOutput();
    }

    if (!DEBUG_NOISE_MUTE)
    {
        tndOut += 0.00494f * noise.getOutput();
    }

    if (!DEBUG_DMC_MUTE)
    {
        tndOut += 0.00335f * dmc.getOutput();
    }

    if (pulseOutput + tndOut > 1.0)
    {
        return 1.0f;
    }

    return pulseOutput + tndOut;
}