#include "cpuBus.h"
#include "constants.h"

// Reference
// https://www.nesdev.org/wiki/CPU_memory_map
// https://www.nesdev.org/wiki/2A03

void CPUBus::connect(PPU* ppu, APU* apu, Cartridge* cart)
{
    this->ppu = ppu;
    this->apu = apu;
    this->cart = cart;
}

uint8 CPUBus::read(uint16 address)
{
    if (address < 0x2000)
    {
        // map to the internal 2kb ram
        return ram[address & 0x07FF];
    }

    // PPU Registers (Only bottom 3 bits matter)
    // 0x2000 - 0x3FF
    // See: https://www.nesdev.org/wiki/PPU_registers
    if (address < 0x4000)
    {
        uint8 result = ppuOpenBusValue;
        uint16 decodedAddress = address & 0x0007;
        if (decodedAddress == 0x02)
        {
            result &= 0x1F;
            result |= ppu->getStatus(readOnly);
        }
        else if (decodedAddress == 0x04)
        {
            result = ppu->getOamData();
        }
        else if (decodedAddress == 0x07)
        {
            result = ppu->getData(readOnly);
        }

        if (!readOnly)
        {
            ppuOpenBusValue = result;
        }

        return result;
    }

    // APU and IO Ports
    // 0x4000 - 0x401F
    // See http://wiki.nesdev.com/w/index.php/2A03
    if (address < 0x4020)
    {
        // TODO: docs mention these as readonly, remove if that matters
        // TODO: It's actualy write only, have to handle whetever Open Bus means for read. Also these mappings are ALL wrong
        switch (address)
        {
            // TODO: Handle Open Bus
            case OAMDMA:  return ppuOpenBusValue;
            case SND_CHN: return apu->getStatus(readOnly);
            case JOY1:    return readOnly ? 0 : controllers[0].read(); // TODO: open bus is special as input only ses the bottom 2-5 bits
            case JOY2:    return readOnly ? 0 : zapper.read(ppu); // hard wiring to zapper for now
            default: return 0;
        }
    }

    return cart->prgRead(address);
}

void CPUBus::write(uint16 address, uint8 value)
{
    if (address < 0x2000)
    {
        // map to the internal 2kb ram
        ram[address & 0x07FF] = value;
    }
    else if (address < 0x4000)
    {
        // map to the ppu registers (only uses the bottom 3 bits)
        // http://wiki.nesdev.com/w/index.php/2A03
        uint16 decodedAddress = address & 0x2007;
        switch (decodedAddress)
        {
            case PPUCTRL:   ppu->setControl(value); break;
            case PPUMASK:   ppu->setMask(value); break;
            case OAMADDR:   ppu->setOamAddress(value); break;
            case OAMDATA:   ppu->setOamData(value); break;
            case PPUSCROLL: ppu->setScroll(value); break;
            case PPUADDR:   ppu->setAddress(value); break;
            case PPUDATA:   ppu->setData(value); break;
        }

        ppuOpenBusValue = value;
    }
    else if (address < 0x4020)
    {
        // map to the apu/io registers
        // http://wiki.nesdev.com/w/index.php/2A03
        switch (address)
        {
            case SQ1_VOL:    apu->pulse1.setDutyEnvelope(value);    break;
            case SQ1_SWEEP:  apu->pulse1.setSweep(value);           break;
            case SQ1_LO:     apu->pulse1.setTimerLo(value);         break;
            case SQ1_HI:     apu->pulse1.setTimerHi(value);         break;
            case SQ2_VOL:    apu->pulse2.setDutyEnvelope(value);    break;
            case SQ2_SWEEP:  apu->pulse2.setSweep(value);           break;
            case SQ2_LO:     apu->pulse2.setTimerLo(value);         break;
            case SQ2_HI:     apu->pulse2.setTimerHi(value);         break;
            case TRI_LINEAR: apu->triangle.setLinearCounter(value); break;
            case TRI_LO:     apu->triangle.setTimerLo(value);       break;
            case TRI_HI:     apu->triangle.setTimerHi(value);       break;
            case NOISE_VOL:  apu->noise.setEnvelope(value);         break;
            case NOISE_LO:   apu->noise.setPeriod(value);           break;
            case NOISE_HI:   apu->noise.setLengthCounter(value);    break;
            case DMC_FREQ:   apu->dmc.setControls(value);           break;
            case DMC_RAW:    apu->dmc.setOutputLevel(value);        break;
            case DMC_START:  apu->dmc.setSampleAddress(value);      break;
            case DMC_LEN:    apu->dmc.setSampleLength(value);       break;
            case OAMDMA:
            {
                isDmaActive = true;
                dmaAddress = ((uint16)value) << 8;
                dmaCycleCount = 0;
            }
            break;
            case SND_CHN: apu->writeControl(value); break;
            case JOY1: controllers[0].write(value); break;
            case JOY2: apu->writeFrameCounterControl(value); break;
            default: return;
        }
    }
    else
    {
        // Cartridge space (logic depends on the mapper)
        cart->prgWrite(address, value);
    }
}

void CPUBus::setGamepad(GamePad pad, int number)
{
    controllers[number].update(pad);
}

void CPUBus::setMouse(Mouse mouse, real32 elapsedMs)
{
    zapper.update(mouse, elapsedMs);
}

void CPUBus::tickDMA()
{
    if (dmaCycleCount % 2 == 1)
    {
        // read cycle
        dmaReadValue = read(dmaAddress);
        ++dmaAddress;
    }
    else
    {
        // write cycle
        write(OAMDATA, dmaReadValue);
        isDmaActive = (dmaAddress & 0x00FF) != 0;
    }

    ++dmaCycleCount;
}
