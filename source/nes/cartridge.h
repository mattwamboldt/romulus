#pragma once
#include "common.h"

class Cartridge
{
public:
    bool load(const char* file);
    bool loadNSF(uint8* buffer, uint32 length);
    bool loadINES(uint8* buffer, uint32 length);

    // Resets variables and banks to their default positions for the loaded rom
    void reset();

    uint8 prgRead(uint16 address);
    bool prgWrite(uint16 address, uint8 value);
    uint8 chrRead(uint16 address);
    bool chrWrite(uint16 address, uint8 value);

    // TODO: Mirroring config is more involved and can even be full vram
    bool hasVerticalMirroring()
    {
        if (mapperNumber == 1)
        {
            return (mmc1Control & 0x03) == 2;
        }

        return useVerticalMirroring;
    }

    int mapperNumber;

    // NSF Config
    bool isNSF;
    uint16 initAddress;
    uint16 playAddress;
    // Time between each call to the playAddress in whole ms (ex 16666 for ~60Hz)
    uint16 playSpeed;

private:
    bool useVerticalMirroring;
    bool hasPerisitantMemory;
    bool hasFullVram;

    // Used to support NSF only
    uint8 backingRom[kilobytes(32)] = {};

    // Provides RAM in range 0x6000 - 0x7FFF
    // Can be written out to disk for games that have persistant
    // (battery backed) memory
    uint8 cartRam[kilobytes(8)] = {};

    // Number of PRG ROM chips (16KB each)
    uint8 prgRomSize;

    // Base for all prg ROM loaded from disk
    uint8* prgRom;

    // Points to the PRG bank mapped to 0x8000
    uint8* prgRomBank1;

    // Points to the PRG bank mapped to 0xC000
    // NOTE: Used for static section in MMC2
    uint8* prgRomBank2;

    // Number of CHR ROM chips (8KB each)
    uint8 chrRomSize;

    // Base for all chr ROM loaded from disk
    uint8* chrRom;

    // Points to the CHR bank mapped to 0x0000
    uint8* patternTable0;

    // Points to the CHR bank mapped to 0x1000
    uint8* patternTable1;

    // Used in place of CHR ROM when none is provided
    uint8 chrRam[kilobytes(8)];

    // Points to chrROM or chrRAM depending on the mapper
    uint8* chrBase;

    bool ignoreNextWrite;

    uint8 mmc1ShiftRegister;
    uint8 mmc1Control;
    uint8 mmc1Chr0;
    uint8 mmc1Chr1;
    uint8 mmc1PrgBank;

    // Resets the mmc1 variables to a known state and
    // resets pointers based on the loaded rom
    void mmc1Reset();
    void mmc1RemapPrg();
    void mmc1RemapChr();

    uint8 mmc2ChrLatch0;
    uint8* mmc2ChrRom0FE;
    uint8* mmc2ChrRom0FD;

    uint8 mmc2ChrLatch1;
    uint8* mmc2ChrRom1FE;
    uint8* mmc2ChrRom1FD;
};