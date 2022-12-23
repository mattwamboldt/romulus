#include "wavefile.h"

#include <stdio.h>

enum WaveFormats
{
	WAVE_PCM = 1,
	WAVE_FLOAT = 3,
	WAVE_ALAW = 6,
	WAVE_MULAW = 7,
	WAVE_EXTENSIBLE = 0xFFFE
};

#pragma pack(push, 1)
struct WaveHeader
{
	uint32 riffChunkId;
	uint32 chunkSize;
	uint32 waveChunkId;
	uint32 formatChunkId;
	uint32 formatChunkSize; // 16, 18 or 40 depending on version
	uint16 format;
	uint16 numChannels;
	uint32 samplesPerSecond;
	uint32 bytesPerSec;
	uint16 blockAlign;
	uint16 bitsPerSample;
	uint32 dataChunkId;
	uint32 dataChunkSize;
};
#pragma pack(pop)

FILE* fileHandle;
WaveHeader header;

void openStream(const char* filename, uint16 numChannels, uint32 samplesPerSecond)
{
	fileHandle = fopen(filename, "wb");

	header = {};
	header.riffChunkId = fourCC('R', 'I', 'F', 'F');
	header.chunkSize = 36;
	header.waveChunkId = fourCC('W', 'A', 'V', 'E');

	header.formatChunkId = fourCC('f', 'm', 't', ' ');
	header.formatChunkSize = 16;
	header.format = WAVE_PCM;
	header.numChannels = numChannels;
	header.samplesPerSecond = samplesPerSecond;
	header.blockAlign = sizeof(int16) * numChannels;
	header.bytesPerSec = header.samplesPerSecond * header.blockAlign;
	header.bitsPerSample = 16;

	header.dataChunkId = fourCC('d', 'a', 't', 'a');

	fwrite(&header, sizeof(WaveHeader), 1, fileHandle);
}

// Writes the buffer to the file, Direct write so ignores channels
void write(int16* buffer, uint32 length)
{
	fwrite(buffer, sizeof(int16), length, fileHandle);
	header.dataChunkSize += length * sizeof(int16);
}

// Update the appropriate sizes and close the file handle
void finalizeStream()
{
	fseek(fileHandle, 0, SEEK_SET);
	header.chunkSize = 36 + header.dataChunkSize;
	fwrite(&header, sizeof(WaveHeader), 1, fileHandle);
	fclose(fileHandle);
}
