#pragma once
#include "common.h"

// https://www.nesdev.org/wiki/APU_DMC
class DeltaModulationChannel
{
public:
    bool isInterruptFlagSet;

    void reset();
    void tick();

    void setEnabled(uint8 enabled);

    void setControls(uint8 value);
    void setOutputLevel(uint8 value);
    void setSampleAddress(uint8 value);
    void setSampleLength(uint8 value);

    bool readRequired();
    uint16 getCurrentAddress() { return currentAddress; }
    void loadSample(uint8 sample);

    uint16 getBytesRemaining() { return bytesRemaining; }
    uint8 getOutput() { return outputLevel; }

private:
    uint8 isEnabled;
    
    // Settings from registers
    bool irqEnabled;
    bool isLooping;
    uint16 sampleAddress;
    uint16 sampleLength;

    // Can be set directly or is driven by memory
    uint8 outputLevel;

    uint16 currentAddress;
    uint32 bytesRemaining;
    uint8 sampleBuffer;
    bool sampleBufferFilled;

    uint8 outputShiftRegister;
    uint8 bitsRemaining;
    bool silenceActive;

    // Maps to rate intput
    uint16 timerLength;
    uint16 timerCurrentTick;
};