#pragma once
#include "6502.h"
#include "ppu/ppu.h"
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
    void processInput(InputState* input);
    void render(ScreenBuffer buffer);
    void outputAudio(int16* outputBuffer, int length);

    // Debug views
    void renderNametable(ScreenBuffer buffer, uint32 top, uint32 left);
    void renderPatternTable(ScreenBuffer buffer, uint32 top, uint32 left, uint8 selectedPalette);

private:
    bool wasVBlankActive;
    bool traceEnabled;
    bool singleStepMode;

    void cpuStep();

    uint32 currentCpuCycle;
    uint8 clockDivider;

    int16 apuBuffer[48000];
    uint32 writeHead;
    uint32 playHead;
    uint32 audioOutputCounter;

    int16 lastSample;

    // Checked on subroutine return to see if nsf control is in the player side
    uint16 nsfSentinal;
    int32 totalPlayCycles;
    int32 cyclesToNextPlay;

    void drawPalette(ScreenBuffer buffer, uint32 top, uint32 left, uint16 baseAddress);
};
