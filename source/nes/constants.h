#pragma once
#include "../common.h"

// PPU Memory Mapped Registers
#define PPUCTRL    0x2000
#define PPUMASK    0x2001
#define PPUSTATUS  0x2002
#define OAMADDR    0x2003
#define OAMDATA    0x2004
#define PPUSCROLL  0x2005
#define PPUADDR    0x2006
#define PPUDATA    0x2007
#define OAMDMA     0x4014

// APU Memory Mapped Registers
// https://www.nesdev.org/wiki/2A03
#define SQ1_VOL    0x4000
#define SQ1_SWEEP  0x4001
#define SQ1_LO     0x4002
#define SQ1_HI     0x4003

#define SQ2_VOL    0x4004
#define SQ2_SWEEP  0x4005
#define SQ2_LO     0x4006
#define SQ2_HI     0x4007

#define TRI_LINEAR 0x4008
// 4009 is unused
#define TRI_LO     0x400A
#define TRI_HI     0x400B

#define NOISE_VOL  0x400C
// 400D is unused
#define NOISE_LO   0x400E
#define NOISE_HI   0x400F

#define DMC_FREQ   0x4010
#define DMC_RAW    0x4011
#define DMC_START  0x4012
#define DMC_LEN    0x4013

#define SND_CHN    0x4015
#define JOY1       0x4016
#define JOY2       0x4017
