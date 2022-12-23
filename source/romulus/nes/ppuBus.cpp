#include "ppuBus.h"

void PPUBus::connect(Cartridge* cart)
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
        switch (cart->getMirroring())
        {
            case MIRROR_HORIZONTAL:
                // H => CIRAM 10 = PPU 11
                ciramAddress |= (address & 0x0800) >> 1;
                break;

            case MIRROR_VERTICAL:
                // V => CIRAM 10 = PPU 10
                ciramAddress |= address & 0x0400;
                break;

            case SINGLE_SCREEN_UPPER:
                ciramAddress |= 0x400;
                break;

            // SINGLE_SCREEN_LOWER is what we have already
        }

        return vram[ciramAddress];
    }

    // palettes http://wiki.nesdev.com/w/index.php/PPU_palettes
    address &= 0x001F;

    uint8 value = 0;
    switch (address)
    {
        case 0x10: value = paletteRam[0x00];
        case 0x14: value = paletteRam[0x04];
        case 0x18: value = paletteRam[0x08];
        case 0x1C: value = paletteRam[0x0C];
        default: value = paletteRam[address];
    }

    // For some reason this gets set to above the range in certain carts. Maybe its another bug but... yeah
    return value & 0x3F;
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
        switch (cart->getMirroring())
        {
            case MIRROR_HORIZONTAL:
                // H => CIRAM 10 = PPU 11
                ciramAddress |= (address & 0x0800) >> 1;
                break;

            case MIRROR_VERTICAL:
                // V => CIRAM 10 = PPU 10
                ciramAddress |= address & 0x0400;
                break;

            case SINGLE_SCREEN_UPPER:
                ciramAddress |= 0x400;
                break;

                // SINGLE_SCREEN_LOWER is what we have already
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