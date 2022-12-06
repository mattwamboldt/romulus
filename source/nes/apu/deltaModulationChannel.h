#pragma once

// https://www.nesdev.org/wiki/APU_DMC
struct DeltaModulationChannel
{
    uint8 getOutput() { return 0; }

    uint8 isEnabled;
    uint32 bytesRemaining;
};