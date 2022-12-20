#pragma once
#include "romulus.h"

void openStream(const char* filename, uint16 numChannels, uint32 samplesPerSecond);
void write(int16* buffer, uint32 length);
void finalizeStream();
