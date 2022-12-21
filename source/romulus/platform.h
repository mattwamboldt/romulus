#pragma once

// TODO: controls for various things that will become settings later
#define SHOW_DEBUG_VIEWS 0

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

// Platform agnostic data structures

struct ScreenBuffer
{
    int width;
    int height;
    int pitch;
    void* memory;
};

// This is modelled on the 360 controller for now, just to have a consistent
// definition of what bottom face button means, etc.
struct GamePad
{
    uint32 deviceId;
    bool isConnected;

    bool upPressed;
    bool downPressed;
    bool leftPressed;
    bool rightPressed;

    bool aPressed;
    bool bPressed;
    bool xPressed;
    bool yPressed;

    bool startPressed;
    bool selectPressed;
};

struct Mouse
{
    // Relative to the upper left of the screen so can be negative
    int32 xPosition;
    int32 yPosition;

    bool leftPressed;
    bool rightPressed;
};

struct InputState
{
    Mouse mouse;
    GamePad controllers[2];
};

// Functions that the emulator exposes to the platform side

// Simulates forward by the given amount and renders whatever the end result was
void updateAndRender(real32 secondsElapsed, InputState* input, ScreenBuffer screen);
// Copies the amount of audio requested from the currently playing sources
void outputAudio(int16* buffer, int numSamples);

// Menu Commands and the like
// 
// TODO: Providing these directly for now to finish the separation but may be worth having a
// command queue/event mechanism so the api doesn't bloat too much

// Detects the file type, loads the appropriate emulator, and starts it
// TODO: May want a facility to not start immediately
void loadROM(const char* filePath);

// Triggers the equivalent of hitting the reset button on the given console
// TODO: Decide what to do if that doesn't exist. Maybe just a hard reboot to the loaded rom
void consoleReset();

// Shuts down the running console and does any last minute battery saves
void consoleShutdown();
