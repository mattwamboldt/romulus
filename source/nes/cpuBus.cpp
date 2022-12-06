#include "cpuBus.h"

// Reference
// https://www.nesdev.org/wiki/CPU_memory_map

void CPUBus::connect(PPU* ppu, APU* apu, Cartridge* cart)
{
    this->ppu = ppu;
    this->apu = apu;
    this->cart = cart;
}

void CPUBus::addWriteCallback(WriteCallback cb)
{
    writeCallback = cb;
}

uint8 CPUBus::read(uint16 address)
{
    if (address < 0x2000)
    {
        // map to the internal 2kb ram
        return ram[address & 0x07FF];
    }

    if (address < 0x4000)
    {
        return ppu->readRegister(address);
    }

    if (address < 0x4020)
    {
        // map to the apu/io registers
        // http://wiki.nesdev.com/w/index.php/2A03
        // TODO: docs mention these as readonly, remove if that matters
        // TODO: It's actualy write only, have to handle whetever Open Bus means for read. Also these mappings are ALL wrong
        switch (address)
        {
            // TODO: Handle Open Bus
            case 0x4014: return ppu->readRegister(address);
            case 0x4015: return apu->getStatus();
            case 0x4016: return readGamepad(0);
            case 0x4017: return readGamepad(1);
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
        if (writeCallback) writeCallback(address, value);
    }
    else if (address < 0x4000)
    {
        ppu->writeRegister(address, value);
        if (writeCallback) writeCallback(address, value);
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
            case 0x4014: ppu->writeRegister(address, value); break;
            case 0x4015: apu->writeControl(value); break;
            case 0x4016: inputStrobeActive = (value & 0x01); break;
            case 0x4017: apu->writeFrameCounterControl(value); break;
            default: return;
        }

        if (writeCallback) writeCallback(address, value);
    }
    // Cartridge space (logic depends on the mapper)
    else if (cart->prgWrite(address, value))
    {
        if (writeCallback) writeCallback(address, value);
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
