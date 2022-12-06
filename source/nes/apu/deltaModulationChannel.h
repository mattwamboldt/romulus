#pragma once

// https://www.nesdev.org/wiki/APU_DMC
struct DeltaModulationChannel
{
    uint8 isEnabled;
    
    uint8 getOutput() { return 0; }
};