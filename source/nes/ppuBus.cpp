#include "ppuBus.h"

void PPUBus::connect(PPU* ppu, Cartridge* cart)
{
    this->cart = cart;
}

// This read and write is based on mapper000, will expand later
uint8 PPUBus::read(uint16 address)
{
    address &= 0x3FFF;

    if (address < 0x2000)
    {
        return cart->chrRead(address);
    }

    if (address < 0x3F00)
    {
        address &= 0x2FFF;

        // TODO: Handle cases where there is more than 2k (certain mappers)
        // We extract the bits for the specific nametable and then set the top bit for
        // specific half of vram based on the mirroring config
        uint16 ciramAddress = address & 0x03FF;
        
        // TODO: handle mapper mirroring config
        // V => CIRAM 10 = PPU 10
        // H => CIRAM 10 = PPU 11
        // https://www.nesdev.org/wiki/INES
        if (cart->useVerticalMirroring)
        {
            ciramAddress |= address & 0x0400;
        }
        else
        {
            ciramAddress |= (address & 0x0800) >> 1;
        }

        return vram[ciramAddress];
    }

    // palettes http://wiki.nesdev.com/w/index.php/PPU_palettes
    address &= 0x001F;
    switch (address)
    {
        case 0x10: return paletteRam[0x00];
        case 0x14: return paletteRam[0x04];
        case 0x18: return paletteRam[0x08];
        case 0x1C: return paletteRam[0x0C];
        default: return paletteRam[address];
    }
}

void PPUBus::write(uint16 address, uint8 value)
{
    address &= 0x3FFF;

    if (address < 0x2000)
    {
        cart->chrWrite(address, value);
        return;
    }

    if (address < 0x3F00)
    {
        address &= 0x2FFF;
        // TODO: Handle cases where there is more than 2k (certain mappers)
        // We extract the bits for the specific nametable and then set the top bit for
        // specific half of vram based on the mirroring config
        uint16 ciramAddress = address & 0x03FF;

        // TODO: handle mapper mirroring config
        if (cart->useVerticalMirroring)
        {
            ciramAddress |= address & 0x0400;
        }
        else
        {
            ciramAddress |= (address & 0x0800) >> 1;
        }

        vram[ciramAddress] = value;
        return;
    }

    // palettes http://wiki.nesdev.com/w/index.php/PPU_palettes
    address &= 0x001F;
    switch (address)
    {
        case 0x10: paletteRam[0x00] = value;
        case 0x14: paletteRam[0x04] = value;
        case 0x18: paletteRam[0x08] = value;
        case 0x1C: paletteRam[0x0C] = value;
        default: paletteRam[address] = value;
    }
}