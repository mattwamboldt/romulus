#pragma once
#include "romulus.h"

static uint32 palette[] = {
    0x00545454, 0x00001E74, 0x00081090, 0x00300088, 0x00440064, 0x005C0030, 0x00540400, 0x003C1800, 0x00202A00, 0x00083A00, 0x00004000, 0x00003C00, 0x0000323C, 0x00000000, 0x00000000, 0x00000000,
    0x00989698, 0x00084CC4, 0x003032EC, 0x005C1EE4, 0x008814B0, 0x00A01464, 0x00982220, 0x00783C00, 0x00545A00, 0x00287200, 0x00087C00, 0x00007640, 0x00006678, 0x00000000, 0x00000000, 0x00000000,
    0x00ECEEEC, 0x004C9AEC, 0x00787CEC, 0x00B062EC, 0x00E454EC, 0x00EC58B4, 0x00EC6A64, 0x00D48820, 0x00A0AA00, 0x0074C400, 0x004CD020, 0x0038CC6C, 0x0038B4CC, 0x003C3C3C, 0x00000000, 0x00000000,
    0x00ECEEEC, 0x00A8CCEC, 0x00BCBCEC, 0x00D4B2EC, 0x00ECAEEC, 0x00ECAED4, 0x00ECB4B0, 0x00E4C490, 0x00CCD278, 0x00B4DE78, 0x00A8E290, 0x0098E2B4, 0x00A0D6E4, 0x00A0A2A0, 0x00000000, 0x00000000
};

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
