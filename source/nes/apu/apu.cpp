#include "apu.h"

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
    // Tick the length counters and sweep units
    pulse1.tickSweep();
    pulse2.tickSweep();
    triangle.tickLengthCounter();
}

void APU::tick()
{
    if (frameCounterResetRequested)
    {
        frameCounter = 0;
    }
    else
    {
        ++frameCounter;
    }

    // Sorry for the magic numbers here but the sequence/frame timing is uneven
    // This'll get worse when PAl comes in
    if (frameCounter == 37280)
    {
        quarterClock();
    }
    else if (frameCounter == 7456)
    {
        quarterClock();
        halfClock();
    }
    else if (frameCounter == 11185)
    {
        quarterClock();
    }
    else if (frameCounter == 14914 && !isFiveStepMode)
    {
        quarterClock();
        halfClock();
        // TODO: Trigger interupt if inhibit flag is clear
        frameCounterResetRequested = true;
    }
    else if (frameCounter == 18640 && isFiveStepMode)
    {
        quarterClock();
        halfClock();
        frameCounterResetRequested = true;
    }

    pulse1.tick();
    pulse2.tick();
}

void APU::writeNoiseControls(uint8 value)
{

}

void APU::writeNoisePeriod(uint8 value)
{

}

void APU::writeNoiseLength(uint8 value)
{

}

void APU::writeDmcControls(uint8 value)
{

}

void APU::writeDmcSampleAddress(uint8 value)
{

}

void APU::writeDmcSampleLength(uint8 value)
{

}

void APU::writeDmcCounter(uint8 value)
{

}

// TODO: These map to 4015 https://www.nesdev.org/wiki/APU#Status_($4015)
uint8 APU::getStatus()
{
    uint8 result = 0;
    // Can only do this because the writes are the same bits
    result |= pulse1.isEnabled;
    result |= pulse2.isEnabled;
    result |= triangle.isEnabled;
    result |= noise.isEnabled;
    result |= dmc.isEnabled;

    // TODO: Handle FrameInterupt Flag
    // TODO: Handle DMC Interupt, true if remaining bytes > 0

    return result;
}

void APU::writeControl(uint8 value)
{
    pulse1.setEnabled(value & 0b00000001);
    pulse2.setEnabled(value & 0b00000010);
    triangle.setEnabled(value & 0b00000100);

    // TODO: Handle write side effects on units that changed
    noise.isEnabled = value & 0b00001000;
    dmc.isEnabled = value & 0b00010000;
}

void APU::writeFrameCounterControl(uint8 value)
{

}

uint32 APU::getOutput()
{
    // TODO: for now using the linear approximation from the wiki and only pulse, will come back to this
    uint32 pulse_out = pulse1.getOutput() + pulse2.getOutput() + triangle.getOutput();
    return pulse_out;
}