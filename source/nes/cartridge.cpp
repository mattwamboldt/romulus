#include "cartridge.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>

// PRG = cartridge side connected to the cpu
// CHR = cartridge side connected to the ppu

#pragma region NSF (Music format, not the main one we use)

#pragma pack(push, 1)
struct NSFHeader
{
    uint8 formatMarker[5]; // 0x4d53454e1A ("NESM" followed by MS-DOS end-of-file)
    uint8 version;
    uint8 totalSongs;
    uint8 startingSong;
    uint16 loadAddress;
    uint16 initAddress;
    uint16 playAddress;
    uint8 songName[32];
    uint8 artistName[32];
    uint8 copyrightHolder[32];
    uint16 playSpeedNtsc;
    uint8 bankSwitchInit[8];
    uint16 playSpeedPAL;
    uint8 regionFlags;
    uint8 extraChipSupportFlags;
    uint8 nsf2Placeholder;
    uint8 programDataLength[3];
};
#pragma pack(pop)

bool Cartridge::loadNSF(uint8* buffer, uint32 length)
{
    isNSF = true;

    // NSF File
    NSFHeader* header = (NSFHeader*)buffer;
    buffer += sizeof(NSFHeader);

    initAddress = header->initAddress;
    playAddress = header->playAddress;
    playSpeed = header->playSpeedNtsc;

    // Non zero means theres extra stuff to parse for NSF 2.0 and that doesn't matter yet
    // TODO: assert(header->programDataLength == 0);
    memcpy(backingRom + (header->loadAddress - 0x8000), buffer, length - sizeof(NSFHeader));
    return true;
}

#pragma endregion

// Don't get to use unions often. Fancy..
union INESHeader
{
    struct
    {
        uint32 formatMarker; // 0x4E45531A ("NES" followed by MS-DOS end-of-file)
        uint8 prgRomSize; // Size of PRG ROM in 16 KB units
        uint8 chrRomSize; // Size of CHR ROM in 8 KB units (Value 0 means the board uses CHR RAM)
        uint8 flags6;
        uint8 flags7;
        uint8 flags8;
        uint8 flags9;
        uint8 flags10;
        uint8 reserved[5];
    };
    uint8 rawBytes[15];
};

bool Cartridge::loadINES(uint8* buffer, uint32 length)
{
    INESHeader* header = (INESHeader*)buffer;
    buffer += sizeof(INESHeader);

    prgRomSize = header->prgRomSize;
    chrRomSize = header->chrRomSize;

    // Parse out flags 6
    useVerticalMirroring = (header->flags6 & 0x01) > 0;
    hasPerisitantMemory = (header->flags6 & 0x02) > 0;

    // Trainers are 512 byte sections of code that get dumped to 0x7000
    // It let people shove code into the rom, but they're useless now so we skip it
    bool hasTrainer = (header->flags6 & 0x04) > 0;
    hasFullVram = (header->flags6 & 0x08) > 0;
    mapperNumber = (header->flags6 & 0xF0) >> 4;

    // Parse out flags 7
    // bit 0 is VS Unisystem (unused)
    // bit 1 is PlayChoice-10
    uint8 versionSignature = header->flags7 & 0x0C;
    mapperNumber |= header->flags7 & 0xF0;

    // TODO: version detection (taken from the wiki, may not be fully correct)
    // From reading it sounds like some emus have an internal database that maps
    // a hash of the header to a structure of known good values to handle old dumps
    
    if (versionSignature == 0x08)
    {
        // Try to parse as 2.0 or reject it
        OutputDebugStringA("Need to handle NES 2.0\n");
        return false;
    }
    else if (versionSignature == 0x04)
    {
        // Archaic iNES could have anything in the rest of the bytes
        mapperNumber &= 0x0F;
    }
    else if (versionSignature == 0x00)
    {
        // Modern iNES validation
        for (int i = 12; i < 16; ++i)
        {
            if (header->rawBytes[i])
            {
                OutputDebugStringA("Failed to parse modern iNES:\n");
                char message[256];
                sprintf(message, " - Mapper number %d\n", mapperNumber & 0x0F);
                OutputDebugStringA(message);
                mapperNumber &= 0x0F;
                break;
            }
        }
    }
    else
    {
        // if its 0x0C then it's ancient or trash and needs special handling
        // So for now its trash
        // TODO: Output an error of some kind
        OutputDebugStringA("0.7 or archaic\n");
        mapperNumber &= 0x0F;
    }

    if (hasTrainer)
    {
        buffer += 512;
    }

    if (mapperNumber == 0)
    {
        OutputDebugStringA("Mapper: 000 NROM\n");
    }
    else if (mapperNumber == 1)
    {
        OutputDebugStringA("Mapper: 001 MMC1\n");
    }
    else if (mapperNumber == 2)
    {
        OutputDebugStringA("Mapper: 002 UxROM\n");
    }
    else if (mapperNumber == 3)
    {
        OutputDebugStringA("Mapper: 003 CNROM\n");
    }
    else if (mapperNumber == 4)
    {
        OutputDebugStringA("Mapper: 004 MMC3 (Incomplete)\n");
    }
    else if (mapperNumber == 7)
    {
        OutputDebugStringA("Mapper: 007 AxROM\n");
    }
    else if (mapperNumber == 9)
    {
        OutputDebugStringA("Mapper: 009 MMC2\n");
    }
    else
    {
        char message[256];
        sprintf(message, "Unhandled Mapper number %d\n", mapperNumber);
        OutputDebugStringA(message);
    }

    prgRom = (uint8*)buffer;

    buffer += header->prgRomSize * kilobytes(16);

    chrRom = 0;
    if (chrRomSize > 0)
    {
        chrRom = (uint8*)buffer;
        chrBase = chrRom;
    }
    else
    {
        // means we have no chr space on the cart and should use the system ram
        chrBase = chrRam;
    }

    reset();

    return true;
}

bool Cartridge::load(const char* file)
{
    FILE* testFile = fopen(file, "rb");
    fseek(testFile, 0, SEEK_END);
    size_t fileSize = ftell(testFile);
    rewind(testFile);

    // TODO: for now newing up and ignoring so we can reuse by just pointing into it
    // a couple kbs while testing wont hurt much, but this can't ship. its an obvious leak
    uint8* contents = new uint8[fileSize];
    size_t bytesRead = fread(contents, sizeof(uint8), fileSize, testFile);
    fclose(testFile);

    bool isLoaded = false;
    if (*((uint32*)contents) == 0x4d53454e)
    {
        isLoaded = loadNSF(contents, (uint32)fileSize);
    }
    else if (*((uint32*)contents) == 0x1a53454e)
    {
        isLoaded = loadINES(contents, (uint32)fileSize);
    }

    return isLoaded;
}

uint8 Cartridge::prgRead(uint16 address)
{
    if (!isReadOnly)
    {
        ignoreNextWrite = false;
    }

    if (address < 0x6000)
    {
        return 0;
    }

    if (address < 0x8000)
    {
        return cartRam[address - 0x6000];
    }

    if (isNSF)
    {
        return backingRom[address - 0x8000];
    }

    if (mapperNumber == 4)
    {
        uint8* selectedBank = mmc3PrgRomBanks[(address & 0x6000) >> 13];
        return selectedBank[address & 0x1FFF];
    }
    else if (mapperNumber == 9)
    {
        if (address < 0xA000)
        {
            return prgRomBank1[address - 0x8000];
        }

        return prgRomBank2[address - 0xA000];
    }

    if (address < 0xC000)
    {
        return prgRomBank1[address - 0x8000];
    }

    return prgRomBank2[address - 0xC000];
}

bool Cartridge::prgWrite(uint16 address, uint8 value)
{
    if (address < 0x6000)
    {
        return false;
    }

    if (address < 0x8000)
    {
        cartRam[address - 0x6000] = value;
        return true;
    }

    // Mappers take advantage of the fact that the upper section
    // of memory is (typically) ROM and thus unwritable. The write request
    // is intercepted and shoved off to another set of chips that
    // can change the configuration of the mapper.

    // MMC1 
    if (mapperNumber == 1)
    {
        if (ignoreNextWrite)
        {
            return false;
        }

        ignoreNextWrite = true;

        if (value & BIT_7)
        {
            mmc1ShiftRegister = BIT_4;
            mmc1Control |= 0x0C;
            mmc1RemapPrg();
        }
        else 
        {
            bool isFifthWrite = mmc1ShiftRegister & BIT_0;

            // Every write we load bit 0 into the left side of the register
            mmc1ShiftRegister >>= 1;
            mmc1ShiftRegister |= (value & BIT_0) << 4;

            // Load into the right internal register based on bits 13 and 14 of the address
            if (isFifthWrite)
            {
                uint16 selectBits = (address >> 13) & 0x0003;
                if (selectBits == 0)
                {
                    mmc1Control = mmc1ShiftRegister;

                    uint8 mirroring = mmc1Control & 0x03;
                    if (mirroring == 0)
                    {
                        mirrorMode = SINGLE_SCREEN_LOWER;
                    }
                    else if (mirroring == 1)
                    {
                        mirrorMode = SINGLE_SCREEN_UPPER;
                    }
                    else if (mirroring == 2)
                    {
                        mirrorMode = MIRROR_VERTICAL;
                    }
                    else
                    {
                        mirrorMode = MIRROR_HORIZONTAL;
                    }

                    // Need to reset all the pointers in case the mode has changed (Could check if its
                    // changed but that just more overhead potentially)
                    mmc1RemapPrg();
                    mmc1RemapChr();
                }
                else if (selectBits == 1)
                {
                    mmc1Chr0 = mmc1ShiftRegister;
                    mmc1RemapChr();
                }
                else if (selectBits == 2)
                {
                    mmc1Chr1 = mmc1ShiftRegister;
                    mmc1RemapChr();
                }
                else if (selectBits == 3)
                {
                    mmc1PrgBank = (mmc1ShiftRegister & 0x0F);
                    mmc1RemapPrg();
                }

                mmc1ShiftRegister = BIT_4;
            }
        }
    }
    // UxROM varieties are a direct mapping
    else if (mapperNumber == 2)
    {
        prgRomBank1 = prgRom + kilobytes(16) * value;
    }
    else if (mapperNumber == 3)
    {
        // TODO: Theres a note on the wiki mentioning larger size variants, may be another mapper?
        patternTable0 = chrRom + (kilobytes(8) * (value & 0x03));
        patternTable1 = patternTable0 + kilobytes(4);
    }
    else if (mapperNumber == 4)
    {
        mmc3PrgWrite(address, value);
    }
    // AxROM
    else if (mapperNumber == 7)
    {
        uint8 bank = value & 0x07;
        prgRomBank1 = prgRom + (bank * kilobytes(32));
        prgRomBank2 = prgRomBank1 + kilobytes(16);

        if (value & BIT_4)
        {
            mirrorMode = SINGLE_SCREEN_UPPER;
        }
        else
        {
            mirrorMode = SINGLE_SCREEN_LOWER;
        }
    }
    // MMC2
    else if (mapperNumber == 9)
    {
        uint16 registerSelect = address & 0xF000;
        if (registerSelect == 0xA000)
        {
            prgRomBank1 = prgRom + (kilobytes(8) * (value & 0x0F));
        }
        else if (registerSelect == 0xB000)
        {
            mmc2ChrRom0FD = chrRom + (kilobytes(4) * (value & 0x1F));
            patternTable0 = mmc2ChrRom0FD;
        }
        else if (registerSelect == 0xC000)
        {
            mmc2ChrRom0FE = chrRom + (kilobytes(4) * (value & 0x1F));
            patternTable0 = mmc2ChrRom0FE;
        }
        else if (registerSelect == 0xD000)
        {
            mmc2ChrRom1FD = chrRom + (kilobytes(4) * (value & 0x1F));
            if (mmc2ChrLatch1 == 0xFD)
            {
                patternTable1 = mmc2ChrRom1FD;
            }
        }
        else if (registerSelect == 0xE000)
        {
            mmc2ChrRom1FE = chrRom + (kilobytes(4) * (value & 0x1F));
            if (mmc2ChrLatch1 == 0xFE)
            {
                patternTable1 = mmc2ChrRom1FE;
            }
        }
        else if (registerSelect == 0xF000)
        {
            if (value & BIT_0)
            {
                mirrorMode = MIRROR_HORIZONTAL;
            }
            else
            {
                mirrorMode = MIRROR_VERTICAL;
            }
        }
    }

    return false;
}

uint8 Cartridge::chrRead(uint16 address)
{
    if (mapperNumber == 4)
    {
        bool isPatternTableHi = address & 0x1000;
        mmc3SetAddress(address);

        // TODO: THIS IS GROSS. WHY DID I DO THIS?
        // Answer: Cause its the dumb way that you know will work and you
        // can clean it up later
        // Don't direct compare! Since these are both masked out values it may not work?
        if ((mmc3Chr2KBanksAreHigh && isPatternTableHi)
            || (!mmc3Chr2KBanksAreHigh && !isPatternTableHi))
        {
            return mmc3ChrRomBanks[(address & 0x0800) >> 11][address & 0x07FF];
        }
        
        return mmc3ChrRomBanks[((address & 0x0C00) >> 10) + 2][address & 0x03FF];
    }

    uint8 result = 0;
    if (address < 0x1000)
    {
        result = patternTable0[address];
    }
    else
    {
        result = patternTable1[address - 0x1000];
    }

    // Check has to happen after the read
    if (!isReadOnly && mapperNumber == 9)
    {
        if (address == 0x0FD8)
        {
            mmc2ChrLatch0 = 0xFD;
            patternTable0 = mmc2ChrRom0FD;
        }
        else if (address == 0x0FE8)
        {
            mmc2ChrLatch0 = 0xFE;
            patternTable0 = mmc2ChrRom0FE;
        }
        else if (address >= 0x1FD8 && address <= 0x1FDF)
        {
            mmc2ChrLatch1 = 0xFD;
            patternTable1 = mmc2ChrRom1FD;
        }
        else if (address >= 0x1FE8 && address <= 0x1FEF)
        {
            mmc2ChrLatch1 = 0xFE;
            patternTable1 = mmc2ChrRom1FE;
        }
    }

    return result;
}

bool Cartridge::chrWrite(uint16 address, uint8 value)
{
    mmc3SetAddress(address);
    if (address < 0x1000)
    {
        patternTable0[address] = value;
    }
    else
    {
        patternTable1[address - 0x1000] = value;
    }

    return true;
}

void Cartridge::reset()
{
    isReadOnly = false;

    // NOTE: These default pointers seem to work for several mappers
 
    // 0x8000 on the first bank and 0xC000 mapped to the last one
    prgRomBank1 = prgRom;
    prgRomBank2 = prgRom + kilobytes(16) * (prgRomSize - 1);

    // Mapped as if there's no switching
    patternTable0 = chrBase;
    patternTable1 = chrBase + kilobytes(4);

    mirrorMode = MIRROR_HORIZONTAL;
    if (useVerticalMirroring)
    {
        mirrorMode = MIRROR_VERTICAL;
    }

    if (mapperNumber == 1)
    {
        mmc1Reset();
    }
    else if (mapperNumber == 4)
    {
        mmc3Reset();
    }
    else if (mapperNumber == 7)
    {
        // Mapper 7 switches on 32 kb instead of 16
        prgRomBank1 = prgRom;
        prgRomBank2 = prgRomBank1 + kilobytes(16);

        mirrorMode = SINGLE_SCREEN_LOWER;
    }
    else if (mapperNumber == 9)
    {
        prgRomBank2 = prgRom + (kilobytes(16) * prgRomSize) - (kilobytes(8) * 3);

        mmc2ChrLatch0 = 0xFE;
        mmc2ChrLatch1 = 0xFE;

        // Default these to use the same banks for now
        mmc2ChrRom0FD = chrRom;
        mmc2ChrRom1FD = chrRom + kilobytes(4);

        mmc2ChrRom0FE = chrRom;
        mmc2ChrRom1FE = chrRom + kilobytes(4);
    }
}

void Cartridge::mmc1Reset()
{
    ignoreNextWrite = false;
    mmc1ShiftRegister = BIT_4;

    // Horizontal by default
    mmc1Control = 3;
    if (mirrorMode == MIRROR_VERTICAL)
    {
        mmc1Control = 2;
    }

    // Going to default the ROM bank mode to be similar to UxROM for now
    // That means first bank movable, second bank fixed on the last ROM chip
    mmc1Control |= 0x0C;
    mmc1PrgBank = 0;

    // CHR config can stay on the zero induced default of 8kb for now
    mmc1Chr0 = 0;
    mmc1Chr1 = 0;

    // Reset the pointers based on the rules
    mmc1RemapPrg();
    mmc1RemapChr();
}

void Cartridge::mmc1RemapPrg()
{
    assert(mmc1PrgBank < prgRomSize)

    uint8 mode = (mmc1Control >> 2) & 0x03;
    if (mode == 2)
    {
        prgRomBank1 = prgRom;
        prgRomBank2 = prgRom + (mmc1PrgBank * kilobytes(16));
    }
    else if (mode == 3)
    {
        prgRomBank1 = prgRom + (mmc1PrgBank * kilobytes(16));
        prgRomBank2 = prgRom + (kilobytes(16) * (prgRomSize - 1));
    }
    else
    {
        prgRomBank1 = prgRom + ((mmc1PrgBank & 0xFE) * kilobytes(16));
        prgRomBank2 = prgRomBank1 + kilobytes(16);
    }
}

void Cartridge::mmc1RemapChr()
{
    // CHR ROM mode (0: single 8kb bank, 1: 2 x 4kb banks)
    if ((mmc1Control & BIT_4) == 0)
    {
        // low bit ignored in 8kb mode to avoid overflow
        patternTable0 = chrBase + ((mmc1Chr0 & 0xFE) * kilobytes(4));
        patternTable1 = patternTable0 + kilobytes(4);
    }
    else
    {
        patternTable0 = chrBase + (mmc1Chr0 * kilobytes(4));
        patternTable1 = chrBase + (mmc1Chr1 * kilobytes(4));
    }
}

void Cartridge::mmc3Reset()
{
    // The default configuration puts:
    // - 2 x 2k banks on nameTable 0
    // - 4 x 1k banks on nameTable 1
    // - 0x8000-0x9FFF swappable
    // - 0xA000-0xBFFF swappable (default to bank 2 for now)
    // - 0xC000-0xDFFF fixed to second to last bank
    // - 0xE000-0xFFFF fixed to last bank

    // 0xE000 can't be changed
    // 0xA000 is always directly swappable
    // Other two can change roles between swappable and fixed based on mode
    // which name table is the 2k banks and which is the 4 k is set by mode

    mmc3ChrRomBanks[0] = chrRom;
    mmc3ChrRomBanks[1] = chrRom + kilobytes(2);
    mmc3ChrRomBanks[2] = chrRom + kilobytes(4);
    mmc3ChrRomBanks[3] = chrRom + kilobytes(5);
    mmc3ChrRomBanks[4] = chrRom + kilobytes(6);
    mmc3ChrRomBanks[5] = chrRom + kilobytes(7);

    mmc3PrgRomBanks[1] = prgRom + kilobytes(8);
    mmc3PrgRomBanks[3] = prgRom + (kilobytes(8) * (prgRomSize * 2 - 1));
}

void Cartridge::mmc3RemapPrg()
{
    if (mmc3SwapPrgRomHigh)
    {
        mmc3PrgRomBanks[0] = prgRom + (kilobytes(16) * (prgRomSize - 1));
        mmc3PrgRomBanks[2] = prgRom + (kilobytes(8) * mmc3PrgRomLowBank);
    }
    else
    {
        mmc3PrgRomBanks[0] = prgRom + (kilobytes(8) * mmc3PrgRomLowBank);
        mmc3PrgRomBanks[2] = prgRom + (kilobytes(16) * (prgRomSize - 1));
    }
}

void Cartridge::mmc3PrgWrite(uint16 address, uint8 value)
{
    // Extract bits 13 and 14
    uint16 registerSelect = (address & 0x6000) >> 13;
    bool isEven = (address & BIT_0) == 0;

    // 0x8000-0x9FFF
    if (registerSelect == 0)
    {
        // Bank Select
        if (isEven)
        {
            mmc3BankSelect = value & 0x07;
            mmc3Chr2KBanksAreHigh = value & BIT_7;

            if ((value & BIT_6) != mmc3SwapPrgRomHigh)
            {
                mmc3SwapPrgRomHigh = value & BIT_6;
                mmc3RemapPrg();
            }
        }
        // Bank Data
        else
        {
            switch (mmc3BankSelect)
            {
                case 0:
                case 1:
                    mmc3ChrRomBanks[mmc3BankSelect] = chrRom + (kilobytes(1) * (value & 0xFE));
                    break;

                case 2:
                case 3:
                case 4:
                case 5:
                    mmc3ChrRomBanks[mmc3BankSelect] = chrRom + (kilobytes(1) * value);
                    break;

                case 6:
                    mmc3PrgRomLowBank = value & 0x3F;
                    mmc3RemapPrg();
                    break;

                case 7:
                    mmc3PrgRomBanks[1] = prgRom + (kilobytes(8) * (value & 0x3F));
                    break;

            }
        }
    }
    // 0xA000-0xBFFF
    else if (registerSelect == 1)
    {
        // Mirroring
        if (isEven)
        {
            if (value & BIT_0)
            {
                mirrorMode = MIRROR_HORIZONTAL;
            }
            else
            {
                mirrorMode = MIRROR_VERTICAL;
            }
        }
        // PRG RAM Protect
        else
        {
            mmc3PrgRamEnabled = (value & BIT_7) > 0;
        }
    }
    // 0xC000-0xDFFF
    else if (registerSelect == 2)
    {
        // IRQ Latch
        if (isEven)
        {
            mmc3IrqReloadValue = value;
        }
        // IRQ Reload
        else
        {
            mmc3ReloadIrqCounter = true;
        }
    }
    // 0xE000-0xFFFF
    else
    {
        mmc3IrqEnabled = !isEven;
        if (!mmc3IrqEnabled)
        {
            mmc3IrqPending = false;
        }
    }
}

void Cartridge::mmc3TickIrq()
{
    if (mmc3ReloadIrqCounter)
    {
        mmc3IrqCounter = mmc3IrqReloadValue;
        mmc3ReloadIrqCounter = false;
    }
    else if (mmc3IrqCounter == 0)
    {
        mmc3IrqCounter = mmc3IrqReloadValue;
        mmc3ReloadIrqCounter = false;
    }
    else
    {
        --mmc3IrqCounter;
    }

    if (mmc3IrqCounter == 0 && mmc3IrqEnabled)
    {
        mmc3IrqPending = true;
    }
}

void Cartridge::mmc3SetAddress(uint16 address)
{
    bool isPatternTableHi = address & 0x1000;
    if (isPatternTableHi && !wasPatternHi && mmc3CpuM2Counter == 0)
    {
        mmc3TickIrq();
        mmc3CpuM2Counter = 3;
    }

    wasPatternHi = isPatternTableHi;
}

void Cartridge::tickCPU()
{
    if (isPatternTableHi)
    {
        mmc3CpuM2Counter = 3;
    }
    else if (mmc3CpuM2Counter > 0)
    {
        --mmc3CpuM2Counter;
    }
}