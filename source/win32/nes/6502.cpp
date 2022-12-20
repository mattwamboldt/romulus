#include "6502.h"

// TEMP
#include "windows.h"
#include <stdio.h>

const uint16 NMI_VECTOR = 0xFFFA;
const uint16 RESET_VECTOR = 0xFFFC;
const uint16 IRQ_VECTOR = 0xFFFE;

void MOS6502::start()
{
    stack = 0;
    accumulator = 0;
    x = 0;
    y = 0;
    status = 0;

    reset();
}

void MOS6502::reset()
{
    isHalted = false;
    isResetRequested = true;
    stage = 0;
    pc = 0;
}

bool MOS6502::isExecuting()
{
    return isResetRequested || stage > 0;
}

bool MOS6502::tick()
{
    if (isHalted)
    {
        return false;
    }

    if (stage == 0)
    {
        if (interruptPending || isResetRequested)
        {
            sequence = HANDLE_INTERRUPT;
            interruptPending = false;
            // Still does a dummy read and takes 7 cycles
            bus->read(pc);
        }
        else
        {
            instAddr = pc;
            inst = bus->read(pc++);
            Operation operation = operations[inst];

            // TODO: From what I've found there are no true "KILL" instructions, just undocumented ones that could be problematic
            if (operation.opCode == KILL)
            {
                --pc;
                instAddr = pc;
                KillUnimplemented("Illegal opcode");
                return false;
            }

            // NOTE: I'm using enums and switches here, these could be function pointers
            // Will see if that becomes appealing later on
            switch (operation.opCode)
            {
                case BRK:
                    isBreakRequested = true;
                    sequence = HANDLE_INTERRUPT;
                    break;

                case PHA:
                case PHP:
                    sequence = PUSH_REGISTER;
                    break;

                case PLA:
                case PLP:
                    sequence = PULL_REGISTER;
                    break;

                case RTI: sequence = RETURN_INTERRUPT; break;
                case RTS: sequence = RETURN_SUBROUTINE; break;
                case JSR: sequence = JUMP_SUBROUTINE; break;

                default:
                {
                    switch (operation.addressMode)
                    {
                        case Accumulator:
                        case Implied:
                        case Immediate:
                            sequence = TWO_CYCLE_MODES;
                            break;

                        case ZeroPage: sequence = SEQ_ZEROPAGE; break;
                        case Absolute: sequence = SEQ_ABSOLUTE; break;

                        case ZeroPageX:
                        case ZeroPageY:
                            sequence = SEQ_ZEROPAGE_INDEXED;
                            break;

                        case AbsoluteX:
                        case AbsoluteY:
                            sequence = SEQ_ABSOLUTE_INDEXED;
                            break;

                        case Relative: sequence = SEQ_RELATIVE; break;
                        case IndirectX: sequence = SEQ_INDIRECTX; break;
                        case IndirectY: sequence = SEQ_INDIRECTY; break;

                        // Only used by the JMP instruction, but here in case its on some undocumented one or something
                        case Indirect: sequence = SEQ_INDIRECT; break;
                    }
                }
            }
        }

        ++stage;
    }
    else
    {
        switch (sequence)
        {
            case HANDLE_INTERRUPT: tickInterrupt(); break; // BRK/NMI/IRQ/RESET
            case RETURN_INTERRUPT: tickReturnInterrupt(); break; // RTI
            case RETURN_SUBROUTINE: tickReturnSubroutine(); break; // RTS
            case PUSH_REGISTER: tickPushRegister(); break; // PHA, PHX
            case PULL_REGISTER: tickPullRegister(); break; // PLA, PLX
            case JUMP_SUBROUTINE: tickJumpSubroutine(); break; // JSR
            case TWO_CYCLE_MODES: tickTwoCycleInstruction(); break; // Absolute, Immediate, and Implied all do basically the same thing
            case SEQ_ABSOLUTE: tickAbsoluteInstruction(); break;
            case SEQ_ZEROPAGE: tickZeropageInstruction(); break;
            case SEQ_ZEROPAGE_INDEXED: tickZeropageIndexedInstruction(); break;
            case SEQ_ABSOLUTE_INDEXED: tickAbsoluteIndexedInstruction(); break;
            case SEQ_RELATIVE: tickRelativeInstruction(); break;
            case SEQ_INDIRECTX: tickIndirectXInstruction(); break; // These two seem similar but are quite differenct conceptually, (Indirect Indexed, and Indexed Indirect in some docs)
            case SEQ_INDIRECTY: tickIndirectYInstruction(); break;
            case SEQ_INDIRECT: tickIndirectInstruction(); break;
            default:
            {
                isHalted = true;
                return false;
            }
        }
    }

    return true;
}

void MOS6502::jumpSubroutine(uint16 address)
{
    pc = address;

    uint16 returnAddress = instAddr + 2;
    push((returnAddress & 0xFF00) >> 8);
    push(returnAddress & 0x00FF);
}

void MOS6502::addWithCarry(uint8 data)
{
    uint8 carry = status & STATUS_CARRY;
    uint8 sum = data + carry + accumulator;

    // Slightly complicated, but overflow only kicks in if signs are different
    uint8 overflow = (accumulator ^ sum) & (data ^ sum) & 0b10000000;
    status &= ~(STATUS_OVERFLOW|STATUS_CARRY);
    status |= overflow >> 1;

    if ((uint16)data + carry + accumulator > 0xFF)
    {
        status |= STATUS_CARRY;
    }

    accumulator = sum;
    updateNZ(accumulator);
}

void MOS6502::subtractWithCarry(uint8 data)
{
    addWithCarry(~data);
}

void MOS6502::andA(uint8 data)
{
    accumulator &= data;
    updateNZ(accumulator);
}

uint8 MOS6502::shiftLeft(uint8 data)
{
    status &= ~STATUS_CARRY;
    status |= (data & 0b10000000) >> 7;
    data <<= 1;
    updateNZ(data);
    return data;
}

uint8 MOS6502::shiftRight(uint8 data)
{
    status &= ~STATUS_CARRY;
    status |= data & 0b00000001;
    data >>= 1;
    updateNZ(data);
    return data;
}

void MOS6502::bitTest(uint8 data)
{
    clearFlags(STATUS_NEGATIVE|STATUS_OVERFLOW|STATUS_ZERO);
    status |= (data & (STATUS_NEGATIVE|STATUS_OVERFLOW));

    if ((data & accumulator) == 0)
    {
        setFlags(STATUS_ZERO);
    }
}

uint8 MOS6502::decrement(uint8 data)
{
    --data;
    updateNZ(data);
    return data;
}

void MOS6502::decrementX()
{
    --x;
    updateNZ(x);
}

void MOS6502::decrementY()
{
    --y;
    updateNZ(y);
}

void MOS6502::exclusiveOrA(uint8 data)
{
    accumulator ^= data;
    updateNZ(accumulator);
}

void MOS6502::orA(uint8 data)
{
    accumulator |= data;
    updateNZ(accumulator);
}

uint8 MOS6502::increment(uint8 data)
{
    ++data;
    updateNZ(data);
    return data;
}

void MOS6502::incrementX()
{
    ++x;
    updateNZ(x);
}

void MOS6502::incrementY()
{
    ++y;
    updateNZ(y);
}

void MOS6502::loadA(uint8 data)
{
    accumulator = data;
    updateNZ(accumulator);
}

void MOS6502::loadX(uint8 data)
{
    x = data;
    updateNZ(x);
}

void MOS6502::loadY(uint8 data)
{
    y = data;
    updateNZ(y);
}

void MOS6502::pushA()
{
    push(accumulator);
}

void MOS6502::pushStatus()
{
    push(status | 0b00110000);
}

void MOS6502::pullA()
{
    accumulator = pull();
    updateNZ(accumulator);
}

void MOS6502::pullStatus()
{
    status = (pull() & 0b11101111) | 0b00100000;
}

uint8 MOS6502::rotateLeft(uint8 data)
{
    uint8 carry = status & STATUS_CARRY;
    clearFlags(STATUS_CARRY);
    status |= (data & 0b10000000) >> 7;
    data = (data << 1) | carry;
    updateNZ(data);
    return data;
}

uint8 MOS6502::rotateRight(uint8 data)
{
    uint8 carry = status & STATUS_CARRY;
    clearFlags(STATUS_CARRY);
    status |= data & 0b00000001;
    data = (data >> 1) | (carry << 7);
    updateNZ(data);
    return data;
}

void MOS6502::transferAtoX()
{
    x = accumulator;
    updateNZ(x);
}

void MOS6502::transferAtoY()
{
    y = accumulator;
    updateNZ(y);
}

void MOS6502::transferStoX()
{
    x = stack;
    updateNZ(x);
}

void MOS6502::transferXtoA()
{
    accumulator = x;
    updateNZ(accumulator);
}

void MOS6502::transferXtoS()
{
    stack = x;
}

void MOS6502::transferYtoA()
{
    accumulator = y;
    updateNZ(accumulator);
}

void MOS6502::alr(uint8 data)
{
    andA(data);
    shiftRight(data);
}

void MOS6502::anc(uint8 data)
{
    andA(data);
    clearFlags(STATUS_CARRY);
    status |= (status & STATUS_NEGATIVE) >> 7;
}

void MOS6502::arr(uint8 data)
{
    // TODO: Implement
    // Similar to AND #i then ROR A, except sets the flags differently. N and Z are normal, but C is bit 6 and V is bit 6 xor bit 5
    KillUnimplemented("Illegal opcode");
}

void MOS6502::axs(uint8 data)
{
    // https://www.masswerk.at/6502/6502_instruction_set.html#SBX
    // TODO: Implement
    // "Sets X to {(A AND X) - #value without borrow}, and updates NZC." how?
    KillUnimplemented("Illegal opcode");
}

void MOS6502::atx(uint8 data)
{
    andA(data);
    transferAtoX();
}

void MOS6502::lax(uint8 data)
{
    loadA(data);
    transferAtoX();
}

void MOS6502::sxa(uint8 data)
{
    // TODO: Implement
    // AND X register with the high byte of the target address of the argument +1. Store the result in memory.
    KillUnimplemented("Illegal opcode");
}

uint8 MOS6502::dcp(uint8 data)
{
    data = decrement(data);
    compare(accumulator, data);
    return data;
}

uint8 MOS6502::isc(uint8 data)
{
    data = increment(data);
    subtractWithCarry(data);
    return data;
}

uint8 MOS6502::rla(uint8 data)
{
    data = rotateLeft(data);
    andA(data);
    return data;
}

uint8 MOS6502::rra(uint8 data)
{
    data = rotateRight(data);
    addWithCarry(data);
    return data;
}

uint8 MOS6502::slo(uint8 data)
{
    data = shiftLeft(data);
    orA(data);
    return data;
}

uint8 MOS6502::sre(uint8 data)
{
    data = shiftRight(data);
    exclusiveOrA(data);
    return data;
}

uint16 MOS6502::calcAddress(AddressingMode addressMode, uint16 address, uint8 a, uint8 b)
{
    switch (addressMode)
    {
        case ZeroPage:
            return a;

        case ZeroPageX:
            return (uint8)(a + x);

        case ZeroPageY:
            return (uint8)(a + y);

        case Absolute:
            return (uint16)b << 8 | a;

        case AbsoluteX:
            return x + ((uint16)b << 8 | a);

        case AbsoluteY:
            return y + ((uint16)b << 8 | a);

        case Indirect:
        {
            uint16 lo = bus->read((uint16)b << 8 | a);
            uint16 hi = bus->read((uint16)b << 8 | (uint8)(a + 1));
            return ((uint16)hi << 8) + lo;
        }
        case IndirectX:
        {
            uint16 lo = bus->read((uint8)(a + x));
            uint16 hi = bus->read((uint8)(a + x + 1));
            return (hi << 8) + lo;
        }
        case IndirectY:
        {
            uint8 zpReadLo = bus->read(a);
            uint8 zpReadHi = bus->read((uint8)(a + 1));

            uint16 lo = zpReadLo + (uint16)y;
            return ((uint16)zpReadHi << 8) + lo;
        }
        case Relative:
            return (address + 2) + (int8)a;
    }

    return 0;
}

// =========
// Utils
// =========
void MOS6502::setFlags(uint8 flags)
{
    status |= flags;
}

void MOS6502::clearFlags(uint8 flags)
{
    status &= ~flags;
}

void MOS6502::updateNZ(uint8 val)
{
    clearFlags(STATUS_NEGATIVE | STATUS_ZERO);
    status |= val & STATUS_NEGATIVE;
    if (val == 0)
    {
        setFlags(STATUS_ZERO);
    }
}

void MOS6502::compare(uint8 a, uint8 b)
{
    clearFlags(STATUS_NEGATIVE | STATUS_ZERO | STATUS_CARRY);
    if (a >= b)
    {
        setFlags(STATUS_CARRY);
    }

    updateNZ(a - b);
}


uint8 MOS6502::pull()
{
    ++stack;
    return bus->read(0x0100 | stack);
}

void MOS6502::push(uint8 v)
{
    bus->write(0x0100 | stack, v);
    stack--;
}

void MOS6502::setNMI(bool active)
{
    if (!nmiWasActive && active)
    {
        // NMI is "edge" detected, meaning it flip flops, rather than being a state
        // Should only trigger when going inactive to active
        nmiPending = true;
    }

    nmiWasActive = active;
}

void MOS6502::setIRQ(bool active)
{
    irqActive = active;
}

// This seems like it could be done at the start of every microcode tick, but some
// operate very specifically. Therefore I'm sprinkling it around as needed.
void MOS6502::pollInterrupts()
{
    interruptPending = nmiPending || (irqActive && !isFlagSet(STATUS_INT_DISABLE));
}

// TODO: Find timing of address evaluation, I'm putting it in the first vector read cycle for now
void MOS6502::tickInterrupt()
{    
    // read next instruction byte (and throw it away)
    if (stage == 1)
    {
        // dummy read
        bus->read(pc);
        if (isBreakRequested)
        {
            ++pc;
        }
    }
    else if (isResetRequested && stage <= 4)
    {
        // R/W is held read until the reset finishes so only sp can update
        --stack;
    }
    else if (stage == 2)
    {
        pushPCH();
    }
    else if (stage == 3)
    {
        pushPCL();
    }
    else if (stage == 4)
    {
        if (isBreakRequested)
        {
            // Sets the "B" flag
            push(status | 0b00110000);
        }
        else
        {
            push(status | 0b00100000);
        }
    }
    else if (stage == 5)
    {
        if (isResetRequested)
        {
            address = RESET_VECTOR;
            isResetRequested = 0;
        }
        else 
        {
            if (nmiPending)
            {
                address = NMI_VECTOR;
                nmiPending = false;
            }
            else
            {
                address = IRQ_VECTOR;
                isBreakRequested = false;
            }
        }

        setFlags(STATUS_INT_DISABLE);
        uint8 pcl = bus->read(address++);
        pc &= 0xFF00;
        pc |= pcl;
    }
    else if (stage == 6)
    {
        uint8 pch = bus->read(address);
        pc &= 0x00FF;
        pc |= ((uint16)pch << 8);
        stage = 0;
        return;
    }

    ++stage;
}

void MOS6502::tickReturnInterrupt()
{
    switch (stage)
    {
        case 1: bus->read(pc); break;
        // TODO: Treating this as a no op cycle, since the timing doc just increments sp
        // We'd have to invert the current understanding of how the stack works, but that may be necesary
        // case 2: bus->read(pc); break;
        case 3: pullStatus(); break;
        case 4: pullPCL(); break;
        case 5:
        {
            pollInterrupts();
            pullPCH();
            stage = 0;
            return;
        }
    }

    ++stage;
}

void MOS6502::tickReturnSubroutine()
{
    switch (stage)
    {
        case 1: bus->read(pc); break;
        // TODO: Treating this as a no op cycle, since the timing doc just increments sp
        // We'd have to invert the current understanding of how the stack works, but that may be necesary
        // case 2: bus->read(pc); break;
        case 3: pullPCL(); break;
        case 4: pullPCH(); break;
        case 5:
        {
            pollInterrupts();
            bus->read(pc++); stage = 0;
            return;
        }
    }

    ++stage;
}

void MOS6502::tickPushRegister()
{
    if (stage == 1)
    {
        bus->read(pc);
        ++stage;
    }
    else
    {
        pollInterrupts();

        Operation operation = operations[inst];
        if (operation.opCode == PHA)
        {
            pushA();
        }
        else
        {
            pushStatus();
        }

        stage = 0;
    }
}

void MOS6502::tickPullRegister()
{
    if (stage == 1)
    {
        bus->read(pc);
        ++stage;
    }
    else if (stage == 2)
    {
        ++stage;
    }
    else
    {
        pollInterrupts();

        Operation operation = operations[inst];
        if (operation.opCode == PLA)
        {
            pullA();
        }
        else
        {
            pullStatus();
        }

        stage = 0;
    }
}

void MOS6502::tickJumpSubroutine()
{
    // NOTE: The odd pattern in this and RTS is to hop over p2
    switch (stage)
    {
        case 1: p1 = bus->read(pc++); break;
        case 3: pushPCH(); break;
        case 4: pushPCL(); break;
        case 5:
        {
            pollInterrupts();
            // "copy low address byte to PCL, fetch high address byte to PCH"
            pc = ((uint16)bus->read(pc) << 8) | p1;
            stage = 0;
            return;
        }
    }

    ++stage;
}

void MOS6502::tickTwoCycleInstruction()
{
    pollInterrupts();

    // NOTE: This one almost doesn't feel worth a function, but I kinda want to isolate
    // things as much as possible cause its already a lot to keep in mind at once
    Operation operation = operations[inst];
    if (operation.addressMode == Immediate)
    {
        tempData = bus->read(pc++);
    }
    else
    {
        bus->read(pc);
        tempData = accumulator;
    }

    if (executeReadInstruction(operation.opCode))
    {
        stage = 0;
        return;
    }

    switch (operation.opCode)
    {
        // RMW Instructions (Handled explicitly to keep write uncluttered)
        case ASL: accumulator = shiftLeft(tempData); break;
        case ROL: accumulator = rotateLeft(tempData); break;
        case ROR: accumulator = rotateRight(tempData); break;
        case LSR: accumulator = shiftRight(tempData); break;

        // Implied (No Params)
        case CLC: clearFlags(STATUS_CARRY); break;
        case CLD: clearFlags(STATUS_DECIMAL); break;
        case CLI: clearFlags(STATUS_INT_DISABLE); break;
        case CLV: clearFlags(STATUS_OVERFLOW); break;

        case DEX: decrementX(); break;
        case DEY: decrementY(); break;
        case INX: incrementX(); break;
        case INY: incrementY(); break;

        case SEC: setFlags(STATUS_CARRY); break;
        case SED: setFlags(STATUS_DECIMAL); break;
        case SEI: setFlags(STATUS_INT_DISABLE); break;

        case TAX: transferAtoX(); break;
        case TAY: transferAtoY(); break;
        case TSX: transferStoX(); break;
        case TXA: transferXtoA(); break;
        case TXS: transferXtoS(); break;
        case TYA: transferYtoA(); break;

        default:
            KillUnimplemented("Unhandled opcode");
    }

    stage = 0;
}

bool MOS6502::executeReadInstruction(OpCode opcode)
{
    switch (opcode)
    {
        case LDA: loadA(tempData); break;
        case LDX: loadX(tempData); break;
        case LDY: loadY(tempData); break;

        case EOR: exclusiveOrA(tempData); break;
        case AND: andA(tempData); break;
        case ORA: orA(tempData); break;

        case ADC: addWithCarry(tempData); break;
        case SBC: subtractWithCarry(tempData); break;

        case CMP: compare(accumulator, tempData); break;
        case CPX: compare(x, tempData); break;
        case CPY: compare(y, tempData); break;

        case BIT: bitTest(tempData); break;

        case NOP: break;

        // Illegal Instructions:
        case ANC: anc(tempData); break;
        case ALR: alr(tempData); break;
        case LAX: lax(tempData); break;
        case ARR: arr(tempData); break;
        case ATX: atx(tempData); break;
        case AXS: axs(tempData); break;

        default: return false;
    }

    return true;
}

bool MOS6502::executeWriteInstruction(OpCode opcode)
{
    switch (opcode)
    {
        case STA: bus->write(address, accumulator); break;
        case STX: bus->write(address, x); break;
        case STY: bus->write(address, y); break;
        // Undocumented Instructions
        case SAX: bus->write(address, accumulator & x); break;
        // case SHA: curently treated as a kill
        case SHX: KillUnimplemented("Illegal opcode"); break; // illegal opcode unhandled
        // case SHY: Currently treaded as a NOP, instruction 0x9C

        default: return false;
    }

    return true;
}

// always the last/longest in a sequence so not bool like the other two
void MOS6502::executeModifyInstruction(OpCode opcode)
{
    switch (opcode)
    {
        case ASL: bus->write(address, shiftLeft(tempData)); break;
        case LSR: bus->write(address, shiftRight(tempData)); break;
        case ROL: bus->write(address, rotateLeft(tempData)); break;
        case ROR: bus->write(address, rotateRight(tempData)); break;
        case INC: bus->write(address, increment(tempData)); break;
        case DEC: bus->write(address, decrement(tempData)); break;

        // Undocumented Instructions
        case SLO: bus->write(address, slo(tempData)); break;
        case SRE: bus->write(address, sre(tempData)); break;
        case RLA: bus->write(address, rla(tempData)); break;
        case RRA: bus->write(address, rra(tempData)); break;
        case ISC: bus->write(address, isc(tempData)); break;
        case DCP: bus->write(address, dcp(tempData)); break;

        default:
            KillUnimplemented("Unknown Opcode");
    }
}

void MOS6502::tickAbsoluteInstruction()
{
    pollInterrupts();

    Operation operation = operations[inst];

    switch (stage)
    {
        case 1: p1 = bus->read(pc++); break;
        case 2:
        {
            if (operation.opCode == JMP)
            {
                // "copy low address byte to PCL, fetch high address byte to PCH"
                pc = ((uint16)bus->read(pc) << 8) | p1;
                stage = 0;
                return;
            }

            p2 = bus->read(pc++);
            address = ((uint16)p2 << 8) | p1;
            break;
        }
        case 3:
        {
            if (executeWriteInstruction(operation.opCode))
            {
                stage = 0;
                return;
            }

            tempData = bus->read(address);
            if (executeReadInstruction(operation.opCode))
            {
                stage = 0;
                return;
            }

            break;
        }
        case 4:
        {
            // dummy write
            bus->write(address, tempData);
            break;
        }
        case 5:
        {
            executeModifyInstruction(operation.opCode);
            stage = 0;
            return;
        }
    }

    ++stage;
}

void MOS6502::tickZeropageInstruction()
{
    pollInterrupts();

    Operation operation = operations[inst];

    switch (stage)
    {
        case 1: address = bus->read(pc++); break;
        case 2:
        {
            if (executeWriteInstruction(operation.opCode))
            {
                stage = 0;
                return;
            }

            tempData = bus->read(address);
            if (executeReadInstruction(operation.opCode))
            {
                stage = 0;
                return;
            }

            break;
        }
        case 3:
        {
            // dummy write
            bus->write(address, tempData);
            break;
        }
        case 4:
        {
            executeModifyInstruction(operation.opCode);
            stage = 0;
            return;
        }
    }

    ++stage;
}

void MOS6502::tickZeropageIndexedInstruction()
{
    pollInterrupts();

    Operation operation = operations[inst];

    switch (stage)
    {
        case 1: p1 = bus->read(pc++); break;
        case 2:
        {
            bus->read(p1);
            if (operation.addressMode == ZeroPageX)
            {
                address = (uint8)(p1 + x);
            }
            else
            {
                address = (uint8)(p1 + y);
            }

            break;
        }
        case 3:
        {
            if (executeWriteInstruction(operation.opCode))
            {
                stage = 0;
                return;
            }

            tempData = bus->read(address);
            if (executeReadInstruction(operation.opCode))
            {
                stage = 0;
                return;
            }

            break;
        }
        case 4:
        {
            bus->write(address, tempData);
            break;
        }
        case 5:
        {
            executeModifyInstruction(operation.opCode);
            stage = 0;
            return;
        }
    }

    ++stage;
}

void MOS6502::tickAbsoluteIndexedInstruction()
{
    pollInterrupts();

    Operation operation = operations[inst];

    switch (stage)
    {
        case 1: p1 = bus->read(pc++); break;
        case 2:
        {
            uint8 index = x;
            if (operation.addressMode == AbsoluteY)
            {
                index = y;
            }

            if (((uint16)p1 + (uint16)index) >= 0x0100)
            {
                pageBoundaryCrossed = true;
            }

            p2 = bus->read(pc++);
            address = ((uint16)p2 << 8) | (uint8)(p1 + index);
            break;
        }
        case 3:
        {
            // NOTE: processor "can't undo a write" so the extra fixup cycle
            // is always present unless its a read only
            tempData = bus->read(address);
            if (pageBoundaryCrossed)
            {
                address += 0x0100;
                pageBoundaryCrossed = false;
            }
            else if (executeReadInstruction(operation.opCode))
            {
                stage = 0;
                return;
            }

            break;
        }
        case 4:
        {
            if (executeWriteInstruction(operation.opCode))
            {
                stage = 0;
                return;
            }

            tempData = bus->read(address);
            if (executeReadInstruction(operation.opCode))
            {
                stage = 0;
                return;
            }

            break;
        }
        case 5:
        {
            bus->write(address, tempData);
            break;
        }
        case 6:
        {
            executeModifyInstruction(operation.opCode);
            stage = 0;
            return;
        }
    }

    ++stage;
}

void MOS6502::tickRelativeInstruction()
{
    // TODO: Fetch this on decode and keep around
    Operation operation = operations[inst];

    switch (stage)
    {
        case 1:
        {
            pollInterrupts();

            p1 = bus->read(pc++);

            bool shouldBranch = false;

            switch (operation.opCode)
            {
                case BCC: shouldBranch = isFlagClear(STATUS_CARRY); break;
                case BCS: shouldBranch = isFlagSet(STATUS_CARRY); break;
                case BVC: shouldBranch = isFlagClear(STATUS_OVERFLOW); break;
                case BVS: shouldBranch = isFlagSet(STATUS_OVERFLOW); break;
                case BNE: shouldBranch = isFlagClear(STATUS_ZERO); break;
                case BEQ: shouldBranch = isFlagSet(STATUS_ZERO); break;
                case BPL: shouldBranch = isFlagClear(STATUS_NEGATIVE); break;
                case BMI: shouldBranch = isFlagSet(STATUS_NEGATIVE); break;
            }

            if (!shouldBranch)
            {
                stage = 0;
                return;
            }

            break;
        }
        case 2:
        {
            // dummy read (TODO: don't know if necessary)
            bus->read(pc);
            
            uint16 oldHi = pc & 0xFF00;
            pc += (int8)p1;

            // TODO: Keeping this to do the dummy read next cycle. remove if unneeded
            address = oldHi | (pc & 0x00FF);

            uint16 newHi = pc & 0xFF00;
            if (oldHi == newHi)
            {
                stage = 0;
                return;
            }

            break;
        }
        case 3:
        {
            pollInterrupts();
            // dummy read (TODO: don't know if necessary), the excess cycle is for sure
            bus->read(address);
            stage = 0;
            return;
        }
    }

    ++stage;
}

void MOS6502::tickIndirectXInstruction()
{
    pollInterrupts();

    Operation operation = operations[inst];

    switch (stage)
    {
        case 1: p1 = bus->read(pc++); break;
        case 2:
        {
            // dummy read
            bus->read(p1);
            p1 += x;
            break;
        }
        case 3:
        {
            address = bus->read(p1++);
            break;
        }
        case 4:
        {
            address |= (uint8)bus->read(p1) << 8;
            break;
        }
        case 5:
        {
            if (executeWriteInstruction(operation.opCode))
            {
                stage = 0;
                return;
            }

            tempData = bus->read(address);
            if (executeReadInstruction(operation.opCode))
            {
                stage = 0;
                return;
            }

            break;
        }
        case 6:
        {
            // dummy write
            bus->write(address, tempData);
            break;
        }
        case 7:
        {
            executeModifyInstruction(operation.opCode);
            stage = 0;
            return;
        }
    }

    ++stage;
}

void MOS6502::tickIndirectYInstruction()
{
    pollInterrupts();

    Operation operation = operations[inst];

    switch (stage)
    {
        case 1: p1 = bus->read(pc++); break;
        case 2:
        {
            address = bus->read(p1++);
            break;
        }
        case 3:
        {
            uint8 hi = bus->read(p1);
            address += y;
            if (address >= 0x0100)
            {
                pageBoundaryCrossed = true;
                address &= 0x00FF;
            }

            address |= (uint16)hi << 8;
            break;
        }
        case 4:
        {
            tempData = bus->read(address);
            if (pageBoundaryCrossed)
            {
                address += 0x0100;
                pageBoundaryCrossed = false;
            }
            else if (executeReadInstruction(operation.opCode))
            {
                stage = 0;
                return;
            }

            break;
        }
        case 5:
        {
            if (executeWriteInstruction(operation.opCode))
            {
                stage = 0;
                return;
            }

            tempData = bus->read(address);
            if (executeReadInstruction(operation.opCode))
            {
                stage = 0;
                return;
            }

            break;
        }
        case 6:
        {
            // dummy write
            bus->write(address, tempData);
            break;
        }
        case 7:
        {
            executeModifyInstruction(operation.opCode);
            stage = 0;
            return;
        }
    }

    ++stage;
}

void MOS6502::tickIndirectInstruction()
{
    switch (stage)
    {
        case 1: p1 = bus->read(pc++); break;
        case 2: p2 = bus->read(pc++); break;
        case 3: tempData = bus->read((uint16)p2 << 8 | p1); break;
        case 4:
        {
            pollInterrupts();

            // page boundaries not handled for this mode
            uint16 hi = bus->read((uint16)p2 << 8 | (uint8)(p1 + 1));
            pc = ((uint16)hi << 8) + tempData;
            stage = 0;
            return;
        }
    }

    ++stage;
}

void MOS6502::pullPCL()
{
    pc &= 0xFF00;
    pc |= pull();
}

void MOS6502::pullPCH()
{
    pc &= 0x00FF;
    pc |= (uint16)pull() << 8;
}

void MOS6502::pushPCL()
{
    push(pc & 0x00FF);
}

void MOS6502::pushPCH()
{
    push((pc & 0xFF00) >> 8);
}

void MOS6502::KillUnimplemented(const char* message)
{
    char output[256];
    Operation operation = operations[inst];
    sprintf(
        output,
        "[CPU] %s: 0x%04X 0x%02X %s (%s, %d)\n",
        message,
        instAddr,
        inst,
        opCodeNames[operation.opCode],
        addressModeNames[operation.addressMode],
        stage);

    OutputDebugStringA(output);
    isHalted = true;
    assert(false);
}

void MOS6502::forceClearNMI()
{
    nmiPending = false;
}