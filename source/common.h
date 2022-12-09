#pragma once

// Some common typedefs that I tend to use for uniformity
typedef signed char int8;
typedef short int16;
typedef int int32;
typedef long long int64;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long long uint64;

typedef int32 bool32;

typedef float real32;
typedef double real64;

// Converts character codes into uint32 representation, useful for file format processing
#define fourCC(A, B, C, D) \
    ((uint32)((uint8)(A) << 0) | (uint32)((uint8)(B) << 8) | \
     (uint32)((uint8)(C) << 16) | (uint32)((uint8)(D) << 24))

#ifndef FINAL
#define assert(expression) if(!(expression)) { __debugbreak(); }
#else
#define assert(expression)
#endif

// Simplifies memory and size calculations
#define kilobytes(value) ((value) * 1024)
#define megabytes(value) (kilobytes(value) * 1024)
#define gigabytes(value) (megabytes(value) * 1024)
#define terabytes(value) (gigabytes(value) * 1024)

// Lots of code is just passing around this kind of info so made a struct for it
struct Buffer
{
    uint32 size;
    void* memory;
};

typedef void(*WriteCallback)(uint16 adddress, uint8 value);

// Got tried of having to memorize bit positions
#define BIT_0 0x01
#define BIT_1 0x02
#define BIT_2 0x04
#define BIT_3 0x08
#define BIT_4 0x10
#define BIT_5 0x20
#define BIT_6 0x40
#define BIT_7 0x80
