#pragma once
#include "common.h"

enum MirrorMode
{
    MIRROR_HORIZONTAL,
    MIRROR_VERTICAL,
    SINGLE_SCREEN_LOWER,
    SINGLE_SCREEN_UPPER,
};

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

    MirrorMode getMirroring() { return mirrorMode; }

    int mapperNumber;

    // NSF Config
    bool isNSF;
    uint16 initAddress;
    uint16 playAddress;
    // Time between each call to the playAddress in whole ms (ex 16666 for ~60Hz)
    uint16 playSpeed;

    // Used to turn off side effects on read operations (Mainly mapper 9 at the moment)
    bool isReadOnly;

private:
    // Settings from the header
    bool useVerticalMirroring;
    bool hasPerisitantMemory;
    bool hasFullVram;

    MirrorMode mirrorMode;

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

    // MMC1
    // TODO: MMC1, Handle CHR RAM variants like SNROM, SOROM, etc.

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

    // MMC2

    uint8 mmc2ChrLatch0;
    uint8* mmc2ChrRom0FE;
    uint8* mmc2ChrRom0FD;

    uint8 mmc2ChrLatch1;
    uint8* mmc2ChrRom1FE;
    uint8* mmc2ChrRom1FD;

    // MMC3 stuff

    uint8 mmc3BankSelect;

    // PRG ROM Bank mode bit is set
    // Means 0x8000 range is fixed, vs 0xC000 range when disabled
    // TODO: This name is hot garbage
    bool mmc3SwapPrgRomHigh;
    bool mmc3Chr2KBanksAreHigh;

    uint8 mmc3PrgRomBankMode;

    bool mmc3PrgRamEnabled;

    uint8 mmc3IrqEnabled;
    uint8 mmc3IrqReloadValue;
    bool mmc3ReloadIrqCounter;

    bool mmc3IrqPending;
    uint8 mmc3IrqCounter;

    // There are 6 pointers that can be bank switched
    // and the mode determines which ones to use where
    uint8* mmc3ChrRomBanks[6];

    // There are 4 8k chunks, the last is always fixed but all the rest
    // can be swapped around in various ways. This is indexed by bits 13 and 14
    uint8* mmc3PrgRomBanks[4];

    // a temp variable used for the bank that isn't fixed second to last
    uint8 mmc3PrgRomLowBank;

    void mmc3Reset();
    void mmc3RemapPrg();

    void mmc3PrgWrite(uint16 address, uint8 value);
};