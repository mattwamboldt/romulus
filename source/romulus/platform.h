#pragma once

// This header defines the API that the application needs from the platform and
// that is made available to the platform. Try to minimize the amount of cross talk
// where possible, though obviously that depends on the needs of both

// NOTE TO SELF: Avoid using stl or standard headers on the generic side if possible.
// Mainly because it pollutes intellisense with garbage so not that serious

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
// TODO: This may be compiler dependant, if it is look at more general ways to manually breakpoint
#define assert(expression) if(!(expression)) { __debugbreak(); }
#else
#define assert(expression)
#endif

// Simplifies memory and size calculations
#define kilobytes(value) ((value) * 1024)
#define megabytes(value) (kilobytes(value) * 1024)
#define gigabytes(value) (megabytes(value) * 1024)
#define terabytes(value) (gigabytes(value) * 1024)



int doStuff(int x);