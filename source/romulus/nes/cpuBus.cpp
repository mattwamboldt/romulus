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
            case JOY1:    return readOnly ? 0 : readGamepad(0);
            case JOY2:    return readOnly ? 0 : readZapper(); // hard wiring to zapper for now
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
            case JOY1:
            {
                inputStrobeActive = (value & 0x01);
                strobeInput(0);
                strobeInput(1);
            }
            break;
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

void CPUBus::setGamepad(NESGamePad pad, int number)
{
    controllers[number].input = pad;
}

void CPUBus::strobeInput(int number)
{
    uint8 currentState = 0;
    ControllerState* controller = controllers + number;

    // NOTE: DO NOT DO THIS NORMALLY, BAD
    if (controller->input.a) currentState |= 0x01;
    if (controller->input.b) currentState |= 0x02;
    if (controller->input.select) currentState |= 0x04;
    if (controller->input.start) currentState |= 0x08;
    if (controller->input.up) currentState |= 0x10;
    if (controller->input.down) currentState |= 0x20;
    if (controller->input.left) currentState |= 0x40;
    if (controller->input.right) currentState |= 0x80;

    controller->shiftCount = 0;
    controller->shiftRegister = currentState;
}

uint8 CPUBus::readGamepad(int number)
{
    // Notes about input on the NES
    // You write to 4016 to tell connected devices to update state
    // Then you turn it off so they are primed and ready
    // After that you read the state ONE BIT AT A TIME!
    // 
    // For controllers thats a sequence of buttons on or off, and if
    // you don't turn off strobe you always get the same one
    // Thats because it loads it all into a "shift register", which
    // shifts one bit out on each read.

    ControllerState* controller = controllers + number;

    if (inputStrobeActive)
    {
        strobeInput(number);
    }

    if (controller->shiftCount >= 8)
    {
        return 0;
    }

    uint8 result = controller->shiftRegister & 0x01;
    controller->shiftRegister >>= 1;
    ++controller->shiftCount;
    return result;
}

void CPUBus::setMouse(Mouse newMouse)
{
    if (!this->mouse.leftPressed && newMouse.leftPressed)
    {
        // TODO: May have to put prevention in for clicking within the existing timer
        // But who could realistically click faster than 80ms?
        zapperCounterMs = 0.08f;
    }

    this->mouse = newMouse;
}
 
bool CPUBus::zapperDetectsLight()
{
    // We assume darkness when pointed away from the screen
    if (mouse.xPosition < 0 || mouse.xPosition >= NES_SCREEN_WIDTH
        || mouse.yPosition < 0 || mouse.yPosition >= NES_SCREEN_HEIGHT)
    {
        return false;
    }

    // The ppu hasn't rendered this out yet so the light sensor would be dead
    if (mouse.yPosition > ppu->scanline
        || (mouse.yPosition == ppu->scanline && mouse.xPosition >= ppu->cycle))
    {
        return false;
    }

    // Detect how long it's been "on" if on at all.
    // ---------------------------------------------
    // The wiki mentions a couple different values for how long it stays active from the point of detection
    // For simplicity (ie I only care about the duck hunt level of "yes/no" detection working for now). I'll
    // stick with around the pure white to light grey numbers.
    if (ppu->scanline - mouse.yPosition > 25)
    {
        return false;
    }

    // TODO: Some roms use a different palette because they're meant for other systems, like vs and playchoice 10
    uint8 paletteIndex = ppu->backbuffer[mouse.xPosition + (mouse.yPosition * NES_SCREEN_WIDTH)];
    return palette[paletteIndex] > 0;
}

// For now hard wiring Zapper to port 2 and gamepad to port 1
uint8 CPUBus::readZapper()
{
    uint8 output = 0;
    if (!zapperDetectsLight())
    {
        output = 0x08;
    }

    if (zapperCounterMs > 0)
    {
        output |= 0x10;
    }

    return output;
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
