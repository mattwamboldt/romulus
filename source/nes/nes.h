#pragma once
#include "6502.h"
#include "ppu.h"
#include "apu/apu.h"
#include "cpuBus.h"
#include "ppuBus.h"

class NES
{
public: // making public for now, will wrap this as needed later
    MOS6502 cpu = {};
    PPU ppu = {};
    PPUBus ppuBus = {};
    APU apu = {};
    CPUBus cpuBus = {};
    Cartridge cartridge = {};
    bool isRunning;

    NES();

    void powerOn();
    void reset();
    void powerOff();

    void loadRom(const char* path);
    void update(real32 secondsPerFrame);
    void singleStep();

    void toggleSingleStep() { singleStepMode = !singleStepMode; }
    void outputAudio(int16* outputBuffer, int length);
    void setGamepadState(NESGamePad pad, int number);

private:
    bool wasVBlankActive;
    bool traceEnabled;
    bool singleStepMode;

    void cpuStep();

    uint32 currentCpuCycle;

    int16 apuBuffer[48000];
    uint32 writeHead;
    uint32 playHead;
    uint32 audioOutputCounter;

    // Checked on subroutine return to see if nsf control is in the player side
    uint16 nsfSentinal;
    int32 totalPlayCycles;
    int32 cyclesToNextPlay;
};
