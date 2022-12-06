#pragma once
#include "../../common.h"
#include "envelope.h"
#include "pulseChannel.h"
#include "triangleChannel.h"
#include "noiseChannel.h"
#include "deltaModulationChannel.h"

// References:
// http://wiki.nesdev.com/wiki/2A03
// https://www.nesdev.org/wiki/APU
class APU
{
public:

    void tick();

    // Write 0x4010
    void writeDmcControls(uint8 value);
    // Write 0x4011
    void writeDmcCounter(uint8 value);
    // Write 0x4012
    void writeDmcSampleAddress(uint8 value);
    // Write 0x4013
    void writeDmcSampleLength(uint8 value);

    // Read 0x4015
    uint8 getStatus();

    // Write 0x4015
    void writeControl(uint8 value);

    // Write 0x4017
    void writeFrameCounterControl(uint8 value);

    // Does the mixdown of all the channels at the current moment in time
    // According to https://www.nesdev.org/wiki/APU_Mixer has a range of 0.0 - 1.0
    real32 getOutput();
    
    PulseChannel pulse1;
    PulseChannel pulse2;
    TriangleChannel triangle;
    NoiseChannel noise;
    DeltaModulationChannel dmc;

    bool isFrameInteruptFlagSet;
    bool isDmcInterruptFlagSet;

private:
    void quarterClock();
    void halfClock();

    // https://www.nesdev.org/wiki/APU_Frame_Counter
    // TODO: They mention half frames which makes me think its actually clocked by the cpu
    // And just doesn't run the sequencer on every one. Will need to find out
    uint16 frameCounter;
    bool isFiveStepMode;
    bool isInterruptInhibited;
    bool frameCounterResetRequested;
};