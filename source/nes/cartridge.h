#pragma once
#include "../common.h"

class Cartridge
{
public:
    void load(const char* file);

    uint8 prgRead(uint16 address);
    bool prgWrite(uint16 address, uint8 value);
    uint8 chrRead(uint16 address);
    bool chrWrite(uint16 address, uint8 value);

    int mapperNumber;

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

    uint8 prgRomSize;
    uint8* prgRom;
    uint8* prgRomBank1;
    uint8* prgRomBank2;

    uint8* chrRom;
    uint8 chrRam[kilobytes(8)];
    uint8* patternTable0;
    uint8* patternTable1;
};