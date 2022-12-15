#pragma once
#include "common.h"

class Cartridge
{
public:
    bool load(const char* file);
    bool loadNSF(uint8* buffer, uint32 length);
    bool loadINES(uint8* buffer, uint32 length);

    uint8 prgRead(uint16 address);
    bool prgWrite(uint16 address, uint8 value);
    uint8 chrRead(uint16 address);
    bool chrWrite(uint16 address, uint8 value);

    int mapperNumber;
    bool useVerticalMirroring;

    // NSF Config
    bool isNSF;
    uint16 initAddress;
    uint16 playAddress;
    // Time between each call to the playAddress in whole ms (ex 16666 for ~60Hz)
    uint16 playSpeed;

private:
    // TODO: This stuff will change when we get multiple mappers
    uint8 cartRam[kilobytes(8)] = {};
    uint8 backingRom[kilobytes(32)] = {}; // Used to support NSF for now

    // Number of PRG ROM chips (16KB each)
    uint8 prgRomSize;
    uint8* prgRom;

    bool hasPerisitantMemory;
    bool hasFullVram;

    uint8* prgRomBank1;
    uint8* prgRomBank2;

    // Number of CHR ROM chips (8KB each)
    uint8 chrRomSize;
    uint8* chrRom;

    uint8 chrRam[kilobytes(8)];
    uint8* patternTable0;
    uint8* patternTable1;
};