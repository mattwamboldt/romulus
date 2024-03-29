#pragma once
#include "romulus.h"
#include "envelope.h"
#include "pulseChannel.h"
#include "triangleChannel.h"
#include "noiseChannel.h"
#include "deltaModulationChannel.h"

// References:
// http://www.nesdev.com/wiki/2A03
// https://www.nesdev.org/wiki/APU
class APU
{
public:

    void reset();

    void tick(uint32 cpuCycleCount);

    // Read 0x4015
    uint8 getStatus(bool readOnly);

    // Write 0x4015
    void writeControl(uint8 value);

    // Write 0x4017
    void writeFrameCounterControl(uint8 value);

    // Does the mixdown of all the channels at the current moment in time
    // According to https://www.nesdev.org/wiki/APU_Mixer has a range of 0.0 - 1.0
    real32 getOutput();
    
    void quarterClock();
    void halfClock();

    PulseChannel pulse1;
    PulseChannel pulse2;
    TriangleChannel triangle;
    NoiseChannel noise;
    DeltaModulationChannel dmc;

    bool isFrameInteruptFlagSet;

    // https://www.nesdev.org/wiki/APU_Frame_Counter
    uint16 frameCounter;

    bool isFiveStepMode;
    bool isInterruptInhibited;
    bool frameCounterResetRequested;
};