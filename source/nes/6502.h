#pragma once
#include "bus.h"

// References
// https://www.nesdev.org/obelisk-6502-guide/reference.html
enum OpCode
{
    ADC, // ADd with Carry
    ALR, // * 
    ANC, // *
    AND, // bitwise AND with accumulator
    ARR, // *
    ASL, // Arithmetic Shift Left
    AXS, // *
    ATX, // *
    BCC, // Branch is Carry Clear
    BCS, // Branch if Carry Set
    BEQ, // Branch if EQual (Zero flag)
    BIT, // BIT Test (mask Memory with A and set flags based on result)
    BMI, // Branch if MInus (Negative flag)
    BNE, // Branch if Not Equal
    BPL, // Branch if Positive
    BRK, // BReaK (Force Interrupt)
    BVC, // Branch if oVerflow Clear
    BVS, // Branch if oVerflow Set
    CLC, // CLear Carry flag
    CLD, // CLear Decimal mode
    CLI, // CLear Interrupt disable
    CLV, // CLear oVerflow flag
    CMP, // CoMPare (A-M, set flags as appropriate)
    CPX, // ComPare X (X-M, set flags as appropriate)
    CPY, // ComPare Y (Y-M, set flags as appropriate)
    DCP, // *
    DEC, // DECrement memory
    DEX, // DEcrement X
    DEY, // DEcrement Y
    EOR, // Exclusive OR (A^M)
    INC, // INCrement memory
    INX, // INcrement X
    INY, // INcrement Y
    ISC, // * Increase and Subtract with Carry (A-(M+1))
    JMP, // JuMP
    JSR, // Jump SubRoutine
    LAX, // * 
    LDA, // LoaD Accumulator
    LDX, // LoaD X
    LDY, // LoaD Y
    LSR, // Logical Shift Right
    NOP, // No OPeration
    ORA, // OR Accumulator (A|M)
    PHA, // PusH Accumulator
    PHP, // PusH Processor status
    PLA, // PulL Accumulator
    PLP, // PulL Processor status
    ROL, // ROtate Left
    ROR, // ROtate Right
    RLA, // *
    RRA, // *
    RTI, // ReTurn from Interrupt
    RTS, // ReTurn from Subroutine
    SAX, // *
    SBC, // SuBtract with Carry
    SEC, // SEt Carry flag
    SED, // SEt Decimal flag
    SEI, // SEt Interrupt disable
    SLO, // * Shift Left Or (Shift one bit in memory, then A | M)
    SRE, // * Shift Right Exlusive or (Shift right one bit in memory, then A ^ M)
    STA, // STore Accumulator (M = A)
    STX, // STore X (M = X)
    STY, // STore Y (M = Y)
    SHX, // * Store X And (AND X register with the high byte of the target address of the argument +1. Store the result in memory)
    TAX, // Transfer Accumulator to X (X=A)
    TAY, // Transfer Accumulator to Y (Y=A)
    TSX, // Transfer Stack pointer to X (X=S)
    TXA, // Transfer X to Accumulator (A=X)
    TXS, // Transfer X to Stack pointer (S=X)
    TYA, // Transfer Y to Accumulator (A=Y)
    KILL, // * Catch all for any Illegal Opcode that crashes the chip/is unhandled
};

static const char* opCodeNames[] =
{
    "ADC",
    "ALR",
    "ANC",
    "AND",
    "ARR",
    "ASL",
    "AXS",
    "ATX",
    "BCC",
    "BCS",
    "BEQ",
    "BIT",
    "BMI",
    "BNE",
    "BPL",
    "BRK",
    "BVC",
    "BVS",
    "CLC",
    "CLD",
    "CLI",
    "CLV",
    "CMP",
    "CPX",
    "CPY",
    "DCP",
    "DEC",
    "DEX",
    "DEY",
    "EOR",
    "INC",
    "INX",
    "INY",
    "ISB", // = ISC, using this alias to make nestest happy
    "JMP",
    "JSR",
    "LAX",
    "LDA",
    "LDX",
    "LDY",
    "LSR",
    "NOP",
    "ORA",
    "PHA",
    "PHP",
    "PLA",
    "PLP",
    "ROL",
    "ROR",
    "RLA",
    "RRA",
    "RTI",
    "RTS",
    "SAX",
    "SBC",
    "SEC",
    "SED",
    "SEI",
    "SLO",
    "SRE",
    "STA",
    "STX",
    "STY",
    "SHX",
    "TAX",
    "TAY",
    "TSX",
    "TXA",
    "TXS",
    "TYA",
    "KILL" // Illegal opcodes that'll kill the machine,
};

enum AddressingMode
{
    Absolute,
    AbsoluteX,
    AbsoluteY,
    Accumulator,
    Immediate,
    Implied,
    Indirect,
    IndirectX,
    IndirectY, // Very different mechanisms, but easier to keep straight this way
    Relative,
    ZeroPage,
    ZeroPageX,
    ZeroPageY,
};

static const char* addressModeNames[] =
{
    "Absolute",
    "AbsoluteX",
    "AbsoluteY",
    "Accumulator",
    "Immediate",
    "Implied",
    "Indirect",
    "IndirectX",
    "IndirectY", // Very different mechanisms, but easier to keep straight this way
    "Relative",
    "ZeroPage",
    "ZeroPageX",
    "ZeroPageY",
};

struct Operation
{
    uint8 instruction;
    uint8 cycleCount;
    OpCode opCode;
    AddressingMode addressMode;
    bool isUnofficial;
};

static Operation operations[] = {
    { 0x00, 7, BRK, Implied, false },
    { 0x01, 6, ORA, IndirectX, false },
    { 0x02, 0, KILL, Implied, true },
    { 0x03, 8, SLO, IndirectX, true },
    { 0x04, 2, NOP, ZeroPage, true },
    { 0x05, 3, ORA, ZeroPage, false },
    { 0x06, 5, ASL, ZeroPage, false },
    { 0x07, 5, SLO, ZeroPage, true },
    { 0x08, 3, PHP, Implied, false },
    { 0x09, 2, ORA, Immediate, false },
    { 0x0A, 2, ASL, Accumulator, false },
    { 0x0B, 2, ANC, Immediate, true },
    { 0x0C, 4, NOP, Absolute, true },
    { 0x0D, 4, ORA, Absolute, false },
    { 0x0E, 6, ASL, Absolute, false },
    { 0x0F, 6, SLO, Absolute, true },
    { 0x10, 2, BPL, Relative, false },
    { 0x11, 5, ORA, IndirectY, false },
    { 0x12, 0, KILL, Implied, true },
    { 0x13, 8, SLO, IndirectY, true },
    { 0x14, 4, NOP, ZeroPageX, true },
    { 0x15, 4, ORA, ZeroPageX, false },
    { 0x16, 6, ASL, ZeroPageX, false },
    { 0x17, 6, SLO, ZeroPageX, true },
    { 0x18, 2, CLC, Implied, false },
    { 0x19, 4, ORA, AbsoluteY, false },
    { 0x1A, 2, NOP, Implied, true },
    { 0x1B, 7, SLO, AbsoluteY, true },
    { 0x1C, 4, NOP, AbsoluteX, true },
    { 0x1D, 4, ORA, AbsoluteX, false },
    { 0x1E, 7, ASL, AbsoluteX, false },
    { 0x1F, 7, SLO, AbsoluteX, true },
    { 0x20, 6, JSR, Absolute, false },
    { 0x21, 6, AND, IndirectX, false },
    { 0x22, 0, KILL, Implied, true },
    { 0x23, 8, RLA, IndirectX, true },
    { 0x24, 3, BIT, ZeroPage, false },
    { 0x25, 3, AND, ZeroPage, false },
    { 0x26, 5, ROL, ZeroPage, false },
    { 0x27, 5, RLA, ZeroPage, true },
    { 0x28, 4, PLP, Implied, false },
    { 0x29, 2, AND, Immediate, false },
    { 0x2A, 2, ROL, Accumulator, false },
    { 0x2B, 2, ANC, Immediate, true },
    { 0x2C, 4, BIT, Absolute, false },
    { 0x2D, 4, AND, Absolute, false },
    { 0x2E, 6, ROL, Absolute, false },
    { 0x2F, 6, RLA, Absolute, true },
    { 0x30, 2, BMI, Relative, false },
    { 0x31, 5, AND, IndirectY, false },
    { 0x32, 0, KILL, Implied, true },
    { 0x33, 8, RLA, IndirectY, true },
    { 0x34, 4, NOP, ZeroPageX, true },
    { 0x35, 4, AND, ZeroPageX, false },
    { 0x36, 6, ROL, ZeroPageX, false },
    { 0x37, 6, RLA, ZeroPageX, true },
    { 0x38, 2, SEC, Implied, false },
    { 0x39, 4, AND, AbsoluteY, false },
    { 0x3A, 2, NOP, Implied, true },
    { 0x3B, 7, RLA, AbsoluteY, true },
    { 0x3C, 4, NOP, AbsoluteX, true },
    { 0x3D, 4, AND, AbsoluteX, false },
    { 0x3E, 7, ROL, AbsoluteX, false },
    { 0x3F, 7, RLA, AbsoluteX, true },
    { 0x40, 6, RTI, Implied, false },
    { 0x41, 6, EOR, IndirectX, false },
    { 0x42, 0, KILL, Implied, true },
    { 0x43, 8, SRE, IndirectX, true },
    { 0x44, 2, NOP, ZeroPage, true },
    { 0x45, 3, EOR, ZeroPage, false },
    { 0x46, 5, LSR, ZeroPage, false },
    { 0x47, 5, SRE, ZeroPage, true },
    { 0x48, 3, PHA, Implied, false },
    { 0x49, 2, EOR, Immediate, false },
    { 0x4A, 2, LSR, Accumulator, false },
    { 0x4B, 2, ALR, Immediate, true },
    { 0x4C, 3, JMP, Absolute, false },
    { 0x4D, 4, EOR, Absolute, false },
    { 0x4E, 6, LSR, Absolute, false },
    { 0x4F, 6, SRE, Absolute, true },
    { 0x50, 2, BVC, Relative, false },
    { 0x51, 5, EOR, IndirectY, false },
    { 0x52, 0, KILL, Implied, true },
    { 0x53, 8, SRE, IndirectY, true },
    { 0x54, 4, NOP, ZeroPageX, true },
    { 0x55, 4, EOR, ZeroPageX, false },
    { 0x56, 6, LSR, ZeroPageX, false },
    { 0x57, 6, SRE, ZeroPageX, true },
    { 0x58, 2, CLI, Implied, false },
    { 0x59, 4, EOR, AbsoluteY, false },
    { 0x5A, 2, NOP, Implied, true },
    { 0x5B, 7, SRE, AbsoluteY, true },
    { 0x5C, 4, NOP, AbsoluteX, true },
    { 0x5D, 4, EOR, AbsoluteX, false },
    { 0x5E, 7, LSR, AbsoluteX, false },
    { 0x5F, 7, SRE, AbsoluteX, true },
    { 0x60, 6, RTS, Implied, false },
    { 0x61, 6, ADC, IndirectX, false },
    { 0x62, 0, KILL, Implied, true },
    { 0x63, 8, RRA, IndirectX, true },
    { 0x64, 0, NOP, ZeroPage, true },
    { 0x65, 3, ADC, ZeroPage, false },
    { 0x66, 5, ROR, ZeroPage, false },
    { 0x67, 5, RRA, ZeroPage, true },
    { 0x68, 4, PLA, Implied, false },
    { 0x69, 2, ADC, Immediate, false },
    { 0x6A, 2, ROR, Accumulator, false },
    { 0x6B, 2, ARR, Immediate, true },
    { 0x6C, 5, JMP, Indirect, false },
    { 0x6D, 4, ADC, Absolute, false },
    { 0x6E, 6, ROR, Absolute, false },
    { 0x6F, 6, RRA, Absolute, true },
    { 0x70, 2, BVS, Relative, false },
    { 0x71, 5, ADC, IndirectY, false },
    { 0x72, 0, KILL, Implied, true },
    { 0x73, 8, RRA, IndirectY, true },
    { 0x74, 4, NOP, ZeroPageX, true },
    { 0x75, 4, ADC, ZeroPageX, false },
    { 0x76, 6, ROR, ZeroPageX, false },
    { 0x77, 6, RRA, ZeroPageX, true },
    { 0x78, 2, SEI, Implied, false },
    { 0x79, 4, ADC, AbsoluteY, false },
    { 0x7A, 2, NOP, Implied, true },
    { 0x7B, 7, RRA, AbsoluteY, true },
    { 0x7C, 4, NOP, AbsoluteX, true },
    { 0x7D, 4, ADC, AbsoluteX, false },
    { 0x7E, 7, ROR, AbsoluteX, false },
    { 0x7F, 7, RRA, AbsoluteX, true },
    { 0x80, 2, NOP, Immediate, true },
    { 0x81, 6, STA, IndirectX, false },
    { 0x82, 2, NOP, Immediate, true },
    { 0x83, 4, SAX, IndirectX, true },
    { 0x84, 3, STY, ZeroPage, false },
    { 0x85, 3, STA, ZeroPage, false },
    { 0x86, 3, STX, ZeroPage, false },
    { 0x87, 4, SAX, ZeroPage, true },
    { 0x88, 2, DEY, Implied, false },
    { 0x89, 2, NOP, Immediate, true },
    { 0x8A, 2, TXA, Implied, false },
    { 0x8B, 0, KILL, Implied, true },
    { 0x8C, 4, STY, Absolute, false },
    { 0x8D, 4, STA, Absolute, false },
    { 0x8E, 4, STX, Absolute, false },
    { 0x8F, 4, SAX, Absolute, true },
    { 0x90, 2, BCC, Relative, false },
    { 0x91, 6, STA, IndirectY, false },
    { 0x92, 0, KILL, Implied, true },
    { 0x93, 0, KILL, Implied, true },
    { 0x94, 4, STY, ZeroPageX, false },
    { 0x95, 4, STA, ZeroPageX, false },
    { 0x96, 4, STX, ZeroPageY, false },
    { 0x97, 4, SAX, ZeroPageY, true },
    { 0x98, 2, TYA, Implied, false },
    { 0x99, 5, STA, AbsoluteY, false },
    { 0x9A, 2, TXS, Implied, false },
    { 0x9B, 0, KILL, Implied, true },
    { 0x9C, 4, NOP, AbsoluteX, true },
    { 0x9D, 5, STA, AbsoluteX, false },
    { 0x9E, 5, SHX, AbsoluteY, true },
    { 0x9F, 0, KILL, Implied, true },
    { 0xA0, 2, LDY, Immediate, false },
    { 0xA1, 6, LDA, IndirectX, false },
    { 0xA2, 2, LDX, Immediate, false },
    { 0xA3, 6, LAX, IndirectX, true },
    { 0xA4, 3, LDY, ZeroPage, false },
    { 0xA5, 3, LDA, ZeroPage, false },
    { 0xA6, 3, LDX, ZeroPage, false },
    { 0xA7, 3, LAX, ZeroPage, true },
    { 0xA8, 2, TAY, Implied, false },
    { 0xA9, 2, LDA, Immediate, false },
    { 0xAA, 2, TAX, Implied, false },
    { 0xAB, 2, ATX, Implied, true },
    { 0xAC, 4, LDY, Absolute, false },
    { 0xAD, 4, LDA, Absolute, false },
    { 0xAE, 4, LDX, Absolute, false },
    { 0xAF, 4, LAX, Absolute, true },
    { 0xB0, 2, BCS, Relative, false },
    { 0xB1, 5, LDA, IndirectY, false },
    { 0xB2, 0, KILL, Implied, true },
    { 0xB3, 5, LAX, IndirectY, true },
    { 0xB4, 4, LDY, ZeroPageX, false },
    { 0xB5, 4, LDA, ZeroPageX, false },
    { 0xB6, 4, LDX, ZeroPageY, false },
    { 0xB7, 4, LAX, ZeroPageY, true },
    { 0xB8, 2, CLV, Implied, false },
    { 0xB9, 4, LDA, AbsoluteY, false },
    { 0xBA, 2, TSX, Implied, false },
    { 0xBB, 0, KILL, Implied, true },
    { 0xBC, 4, LDY, AbsoluteX, false },
    { 0xBD, 4, LDA, AbsoluteX, false },
    { 0xBE, 4, LDX, AbsoluteY, false },
    { 0xBF, 4, LAX, AbsoluteY, true },
    { 0xC0, 2, CPY, Immediate, false },
    { 0xC1, 6, CMP, IndirectX, false },
    { 0xC2, 2, NOP, Immediate, true },
    { 0xC3, 8, DCP, IndirectX, true },
    { 0xC4, 3, CPY, ZeroPage, false },
    { 0xC5, 3, CMP, ZeroPage, false },
    { 0xC6, 5, DEC, ZeroPage, false },
    { 0xC7, 5, DCP, ZeroPage, true },
    { 0xC8, 2, INY, Implied, false },
    { 0xC9, 2, CMP, Immediate, false },
    { 0xCA, 2, DEX, Implied, false },
    { 0xCB, 2, AXS, Immediate, true },
    { 0xCC, 4, CPY, Absolute, false },
    { 0xCD, 4, CMP, Absolute, false },
    { 0xCE, 6, DEC, Absolute, false },
    { 0xCF, 6, DCP, Absolute, true },
    { 0xD0, 2, BNE, Relative, false },
    { 0xD1, 5, CMP, IndirectY, false },
    { 0xD2, 0, KILL, Implied, true },
    { 0xD3, 8, DCP, IndirectY, true },
    { 0xD4, 4, NOP, ZeroPageX, true },
    { 0xD5, 4, CMP, ZeroPageX, false },
    { 0xD6, 6, DEC, ZeroPageX, false },
    { 0xD7, 6, DCP, ZeroPageX, true },
    { 0xD8, 2, CLD, Implied, false },
    { 0xD9, 4, CMP, AbsoluteY, false },
    { 0xDA, 2, NOP, Implied, true },
    { 0xDB, 7, DCP, AbsoluteY, true },
    { 0xDC, 4, NOP, AbsoluteX, true },
    { 0xDD, 4, CMP, AbsoluteX, false },
    { 0xDE, 7, DEC, AbsoluteX, false },
    { 0xDF, 7, DCP, AbsoluteX, true },
    { 0xE0, 2, CPX, Immediate, false },
    { 0xE1, 6, SBC, IndirectX, false },
    { 0xE2, 2, NOP, Immediate, true },
    { 0xE3, 8, ISC, IndirectX, true },
    { 0xE4, 3, CPX, ZeroPage, false },
    { 0xE5, 3, SBC, ZeroPage, false },
    { 0xE6, 5, INC, ZeroPage, false },
    { 0xE7, 5, ISC, ZeroPage, true },
    { 0xE8, 2, INX, Implied, false },
    { 0xE9, 2, SBC, Immediate, false },
    { 0xEA, 2, NOP, Implied, false },
    { 0xEB, 2, SBC, Immediate, true },
    { 0xEC, 4, CPX, Absolute, false },
    { 0xED, 4, SBC, Absolute, false },
    { 0xEE, 6, INC, Absolute, false },
    { 0xEF, 6, ISC, Absolute, true },
    { 0xF0, 2, BEQ, Relative, false },
    { 0xF1, 5, SBC, IndirectY, false },
    { 0xF2, 0, KILL, Implied, true },
    { 0xF3, 8, ISC, IndirectY, true },
    { 0xF4, 4, NOP, ZeroPageX, true },
    { 0xF5, 4, SBC, ZeroPageX, false },
    { 0xF6, 6, INC, ZeroPageX, false },
    { 0xF7, 6, ISC, ZeroPageX, true },
    { 0xF8, 2, SED, Implied, false },
    { 0xF9, 4, SBC, AbsoluteY, false },
    { 0xFA, 2, NOP, Implied, true },
    { 0xFB, 7, ISC, AbsoluteY, true },
    { 0xFC, 4, NOP, AbsoluteX, true },
    { 0xFD, 4, SBC, AbsoluteX, false },
    { 0xFE, 7, INC, AbsoluteX, false },
    { 0xFF, 7, ISC, AbsoluteX, true },
};

enum StatusFlags
{
    STATUS_CARRY       = 0b00000001,
    STATUS_ZERO        = 0b00000010,
    STATUS_INT_DISABLE = 0b00000100,
    STATUS_DECIMAL     = 0b00001000,
    STATUS_B           = 0b00110000,
    STATUS_OVERFLOW    = 0b01000000,
    STATUS_NEGATIVE    = 0b10000000
};

// From looking at docs on cycle timings and the like there are
// a set of sequences the processor goes through that mainly line
// up with addressing modes and then a few unique ones
enum MicrocodeSequence
{
    // First set is coving the unique cases
    HANDLE_INTERRUPT, // BRK/NMI/IRQ/RESET
    RETURN_INTERRUPT, // RTI
    RETURN_SUBROUTINE, // RTS
    PUSH_REGISTER, // PHA, PHX
    PULL_REGISTER, // PLA, PLX
    JUMP_SUBROUTINE, // JSR

    // Address mode related (these will have a bunch of overlapping stages
    // but its easier to maintain conceptualy for me this way)
    // TODO: Consider if it's worth doing R/W/R+W variants to reduce the branching after the initial fetch/decode
    TWO_CYCLE_MODES, // Absolute, Immediate, and Implied all do basically the same thing
    SEQ_ABSOLUTE,
    SEQ_ZEROPAGE,
    SEQ_ZEROPAGE_INDEXED,
    SEQ_ABSOLUTE_INDEXED,
    SEQ_RELATIVE,
    SEQ_INDIRECTX, // These two seem similar but are quite differenct conceptually, (Indirect Indexed, and Indexed Indirect in some docs)
    SEQ_INDIRECTY,
    SEQ_INDIRECT, // Used only by the JMP instruction

    NUM_MICROCODE_SEQUENCES
};

class MOS6502
{
public:
    // Registers on the system
    // https://archive.org/details/mos_6500_mpu_preliminary_may_1976/page/n1/mode/1up
    uint16 pc;
    uint8 stack;
    uint8 status;
    uint8 accumulator;
    uint8 x;
    uint8 y;

    // Values to abstract the internal magic of the 6502
    uint8 inst; // Currently executing instruction
    uint16 instAddr; // Address of current instruction

    // Address Lines, (needed since we don't always fetch pc)
    uint16 address;

    MicrocodeSequence sequence;
    // Tells us how far into a given sequence we are
    uint8 stage;

    void connect(IBus* bus) { this->bus = bus; }

    void start();

    // -> RES
    void reset(bool isFirstBoot = false);
    void stop() { isHalted = true; };

    bool tick(); // true on start of instruction
    bool tickCycleAccurate();
    bool hasHalted() { return isHalted; }
    bool isExecuting() { return stage > 0; }

    bool isFlagSet(uint8 flags) { return status & flags; }
    bool isFlagClear(uint8 flags) { return !(status & flags); }

    uint16 calcAddress(AddressingMode addressMode, uint16 address, uint8 p1, uint8 p2);
    bool requireRead(uint8 opCode);

    // Operations (individual functions w/ data param helps debugging)
    void addWithCarry(uint8 data);
    void subtractWithCarry(uint8 data);

    uint8 shiftLeft(uint8 data);
    uint8 shiftRight(uint8 data);

    void branchCarryClear(uint8 data);
    void branchCarrySet(uint8 data);
    void branchEqual(uint8 data);
    void branchNegative(uint8 data);
    void branchNotZero(uint8 data);
    void branchPositive(uint8 data);
    void branchOverflowSet(uint8 data);
    void branchOverflowClear(uint8 data);
    void bitTest(uint8 data);

    void clearCarry();
    void clearDecimal();
    void clearInteruptDisable();
    void clearOverflow();
    void setCarry();
    void setDecimal();
    void setInteruptDisable();

    void compareA(uint8 data);
    void compareX(uint8 data);
    void compareY(uint8 data);

    uint8 decrement(uint8 data);
    void decrementX();
    void decrementY();

    void andA(uint8 data);
    void exclusiveOrA(uint8 data);
    void orA(uint8 data);

    uint8 increment(uint8 data);
    void incrementX();
    void incrementY();

    void jump(uint16 address);
    void jumpSubroutine(uint16 address);

    void loadA(uint8 data);
    void loadX(uint8 data);
    void loadY(uint8 data);

    void pushA();
    void pushStatus();
    void pullA();
    void pullStatus();

    uint8 rotateLeft(uint8 data);
    uint8 rotateRight(uint8 data);

    void returnIntertupt();
    void returnSubroutine();

    void transferAtoX();
    void transferAtoY();
    void transferStoX();
    void transferXtoA();
    void transferXtoS();
    void transferYtoA();

    // illegal opcodes
    void alr(uint8 data);
    void anc(uint8 data);
    void arr(uint8 data);
    void axs(uint8 data);
    void lax(uint8 data);
    void atx(uint8 data);
    void sxa(uint8 data);

    uint8 dcp(uint8 data);
    uint8 isc(uint8 data);
    uint8 rla(uint8 data);
    uint8 rra(uint8 data);
    uint8 slo(uint8 data);
    uint8 sre(uint8 data);

    void forceBreak();

    // -> NMI
    // Called externally to set the state of /NMI
    // See: https://www.nesdev.org/wiki/NMI
    void setNMI(bool active);

    // -> IRQ
    // Called externally to set the state of /IRQ
    // See: https://www.nesdev.org/wiki/IRQ
    void setIRQ(bool active);

private:
    bool isHalted;

    bool nmiRequested;
    bool nmiWasActive;
    bool interruptRequested;
    bool isResetRequested;
    bool isBreakRequested;

    uint8 waitCycles;

    uint8 p1; // temp storage for operands
    uint8 p2;
    uint8 tempData; // temp storage

    IBus* bus;

    // This model somewhat splits the operation in a way that could be
    // made cycle accurate in future, but for now is run all at once
    uint16 calcAddress(AddressingMode addressMode);
    void loadOperands(AddressingMode addressMode);
    void executeInstruction();

    // Status Register helpers
    void setFlags(uint8 flags);
    void clearFlags(uint8 flags);
    void updateNZ(uint8 val);
    
    // Utility functions
    void compare(uint8 a, uint8 b);

    void jumpOnFlagSet(uint8 flag, uint8 offset);
    void jumpOnFlagClear(uint8 flag, uint8 offset);
    void jumpRelative(uint8 offset);

    void handleInterrupt(uint16 vector, bool isReset = false);

    // Stack Manipulation
    uint8 pull();
    uint16 pullWord();
    void push(uint8 v);
    void pushWord(uint16 v);

    uint16 readWord(uint16 base);
    uint8 readData(AddressingMode addressMode);
    void writeData(AddressingMode addressMode, uint8 val);




    // NEW PROCESSOR FUNCS
    
    bool pageBoundaryCrossed;
    
    // SequenceHandlers
    void tickInterrupt(); // BRK/NMI/IRQ/RESET
    void tickReturnInterrupt();
    void tickReturnSubroutine();
    void tickPushRegister();
    void tickPullRegister();
    void tickJumpSubroutine();
    void tickTwoCycleInstruction();
    void tickAbsoluteInstruction();
    void tickZeropageInstruction();
    void tickZeropageIndexedInstruction();
    void tickAbsoluteIndexedInstruction();
    void tickRelativeInstruction();
    void tickIndirectXInstruction();
    void tickIndirectYInstruction();
    void tickIndirectInstruction();

    void pullPCL();
    void pushPCL();
    void pullPCH();
    void pushPCH();

    bool executeReadInstruction(OpCode opcode);
    bool executeWriteInstruction(OpCode opcode);
    void executeModifyInstruction(OpCode opcode);

    // TEMP: used to debug new per cycle execution
    void KillUnimplemented(const char* message);
};