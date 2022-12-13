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
                OutputDebugStringA("Failed to parse modern iNES\n");
                return false;
            }
        }
    }
    else
    {
        // if its 0x0C then it's ancient or trash and needs special handling
        // So for now its trash
        // TODO: Output an error of some kind
        OutputDebugStringA("0.7 or archaic\n");
        return false;
    }

    if (hasTrainer)
    {
        buffer += 512;
    }

    if (mapperNumber != 0)
    {
        char message[256];
        sprintf(message, "Unhandled Mapper number %d\n", mapperNumber);
        OutputDebugStringA(message);
    }

    prgRom = prgRomBank1 = (uint8*)buffer;
    prgRomBank2 = prgRom + kilobytes(16) * (prgRomSize - 1);

    buffer += header->prgRomSize * kilobytes(16);

    chrRom = 0;
    if (chrRomSize > 0)
    {
        chrRom = patternTable0 = (uint8*)buffer;
        patternTable1 = chrRom + kilobytes(4);
    }
    else
    {
        patternTable0 = chrRam;
        patternTable1 = chrRam + kilobytes(4);
    }

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

    if (address < 0xC000)
    {
        return prgRomBank1[address - 0x8000];
    }

    return prgRomBank2[address - 0xC000];
}

bool Cartridge::prgWrite(uint16 address, uint8 value)
{
    if (address > 0x6000 && address < 0x8000)
    {
        cartRam[address - 0x6000] = value;
        return true;
    }

    return false;
}

uint8 Cartridge::chrRead(uint16 address)
{
    if (address < 0x1000)
    {
        return patternTable0[address];
    }

    return patternTable1[address - 0x1000];
}

bool Cartridge::chrWrite(uint16 address, uint8 value)
{
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