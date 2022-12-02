#pragma once
#include "6502.h"
#include "ppu.h"

int formatInstruction(char* dest, uint16 address, MOS6502* cpu, IBus* bus);
void logInstruction(const char* filename, uint16 address, MOS6502* cpu, IBus* cpuBus, PPU* ppu);
void flushLog();