#include "cpuBus.h"

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
            case 0x4014: return ppuOpenBusValue;
            case 0x4015: return apu->getStatus(readOnly);
            case 0x4016: return readOnly ? 0 : readGamepad(0);
            case 0x4017: return readOnly ? 0 : readGamepad(1);
            default: return 0;
        }
    }

    return cart->prgRead(address);
}

uint16 CPUBus::readWord(uint16 address)
{
    uint8 lo = read(address);
    uint8 hi = read(address + 1);
    return ((uint16)hi << 8) + lo;
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
        uint16 decodedAddress = address & 0x0007;
        switch (decodedAddress)
        {
            case 0x00: ppu->setControl(value); break;
            case 0x01: ppu->setMask(value); break;
            case 0x03: ppu->setOamAddress(value); break;
            case 0x04: ppu->setOamData(value); break;
            case 0x05: ppu->setScroll(value); break;
            case 0x06: ppu->setAddress(value); break;
            case 0x07: ppu->setData(value); break;
        }

        ppuOpenBusValue = value;
    }
    else if (address < 0x4020)
    {
        // map to the apu/io registers
        // http://wiki.nesdev.com/w/index.php/2A03
        switch (address)
        {
            case 0x4000: apu->pulse1.setDutyEnvelope(value); break;
            case 0x4001: apu->pulse1.setSweep(value); break;
            case 0x4002: apu->pulse1.setTimerLo(value); break;
            case 0x4003: apu->pulse1.setTimerHi(value); break;
            case 0x4004: apu->pulse2.setDutyEnvelope(value); break;
            case 0x4005: apu->pulse2.setSweep(value); break;
            case 0x4006: apu->pulse2.setTimerLo(value); break;
            case 0x4007: apu->pulse2.setTimerHi(value); break;
            case 0x4008: apu->triangle.setLinearCounter(value); break;
            case 0x400A: apu->triangle.setTimerLo(value); break;
            case 0x400B: apu->triangle.setTimerHi(value); break;
            case 0x400C: apu->noise.setEnvelope(value); break;
            case 0x400E: apu->noise.setPeriod(value); break;
            case 0x400F: apu->noise.setLengthCounter(value); break;
            case 0x4010: apu->writeDmcControls(value); break;
            case 0x4011: apu->writeDmcCounter(value); break;
            case 0x4012: apu->writeDmcSampleAddress(value); break;
            case 0x4013: apu->writeDmcSampleLength(value); break;
            case 0x4014: ppu->setOamDma(value); break;
            case 0x4015: apu->writeControl(value); break;
            case 0x4016: inputStrobeActive = (value & 0x01); break;
            case 0x4017: apu->writeFrameCounterControl(value); break;
            default: return;
        }
    }
    else
    {
        // Cartridge space (logic depends on the mapper)
        cart->prgWrite(address, value);
    }
}

void CPUBus::setInput(NESGamePad pad, int number)
{
    controllers[number].input = pad;
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
        uint8 currentState = 0;

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

    if (controller->shiftCount >= 8)
    {
        return 0x01;
    }

    uint8 result = controller->shiftRegister & 0x01;
    controller->shiftRegister >>= 1;
    ++controller->shiftCount;
    return result;
}
