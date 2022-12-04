#pragma once
#include "envelope.h"

// https://www.nesdev.org/wiki/APU_Noise
struct NoiseChannel
{
    uint8 isEnabled;
    Envelope envelope;
};