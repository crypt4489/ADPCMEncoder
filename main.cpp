#include "types.h"

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <string>
#include <math.h>
#include <intrin.h>
#include "convertpcm16.hpp"
static float lut[5][2] = { { 0.0, 0.0 },
							{  -60.0 / 64.0, 0.0 },
							{ -115.0 / 64.0, 52.0 / 64.0 },
							{  -98.0 / 64.0, 55.0 / 64.0 },
							{ -122.0 / 64.0, 60.0 / 64.0 } };

enum VAGFlag
{
	VAGF_NOTHING = 0,         /* Nothing*/
	VAGF_LOOP_LAST_BLOCK = 1, /* Last block to loop */
	VAGF_LOOP_REGION = 2,     /* Loop region*/
	VAGF_LOOP_END = 3,        /* Ending block of the loop */
	VAGF_LOOP_FIRST_BLOCK = 4,/* First block of looped data */
	VAGF_UNK = 5,             /* ?*/
	VAGF_LOOP_START = 6,      /* Starting block of the loop*/
	VAGF_PLAYBACK_END = 7     /* Playback ending position */
};

typedef struct enc_block_t
{
	s8 shift;
	s8 predict;
	u8 flags;
	u8 sample[14];
} EncBlock;

void CreateVagFile(std::ofstream &stream, VagFileHeader *header, u8* buffer, u32 size)
{
	stream.write((char*)header, sizeof(VagFileHeader));
	char pad[16] = {0};
	stream.write(&pad[0], 16);
	stream.write((char*)buffer, size);
}


u8* CreateVagSamples(s16* samples, u32 len, u32* outSize, u32 loopStart, u32 loopEnd, u32 loopFlag)
{
	float _hist_1 = 0.0, _hist_2 = 0.0;
	float hist_1 = 0.0, hist_2 = 0.0;

	int fullChunks = len * 0.035714;
	int remaining = len - (fullChunks * 28);
	char lastPredictAndShift = 0;

	s16* samplesLocal = samples;

	if (remaining)
	{
		fullChunks++;
	}

	int sizeOfOut = fullChunks * 16;

	u8* outBuffer = (u8*)malloc(sizeOfOut);

	u8* ret = outBuffer;

	memset(outBuffer, '\0', sizeOfOut);

	u32 globalIndex = 0;
	u32 bytesRead = 0;
	EncBlock block{ 0, 0, 0, 0 };
	for (int i = 0; i < fullChunks; i++)
	{

		int chunkSize = 28;
		if (i == fullChunks - 1)
		{
			chunkSize = remaining;
		}
		int predict = 0, shift;
		float min = 1e10;
		float s_1 = 0.0, s_2 = 0.0;
		float predictBuf[28][5];
		for (int j = 0; j < 5; j++)
		{
			float max = 0.0;

			s_1 = _hist_1;
			s_2 = _hist_2;

			for (int k = 0; k < chunkSize; k++)
			{
				float sample = samples[k];
				if (sample > 30719.0)
				{
					sample = 30719.0;
				}
				if (sample < -30720.0)
				{
					sample = -30720.0;
				}

				float ds = sample + s_1 * lut[j][0] + s_2 * lut[j][1];

				predictBuf[k][j] = ds;

				if (fabs(ds) > max)
				{
					max = fabs(ds);
				}

				s_2 = s_1;
				s_1 = sample;
			}
			if (max < min)
			{
				min = max;
				predict = j;
			}
			if (min <= 7)
			{
				predict = 0;
				break;
			}
		}
		//printf("%f %f\n", s_1, s_2);
		_hist_1 = s_1;
		_hist_2 = s_2;

		float d_samples[28];
		for (int i = 0; i < 28; i++)
		{
			d_samples[i] = predictBuf[i][predict];
		}

		int min2 = (int)min;
		int shift_mask = 0x4000;
		shift = 0;

		while (shift < 12)
		{
			if (shift_mask & (min2 + (shift_mask >> 3)))
			{
				break;
			}
			shift++;
			shift_mask >>= 1;
		}
		block.predict = predict;
		block.shift = shift;

		if (len - bytesRead > 28)
		{
			block.flags = VAGF_NOTHING;
			if (loopFlag)
			{
				block.flags = VAGF_LOOP_REGION;
				if (i == loopStart)
				{
					block.flags = VAGF_LOOP_START;
				}
				if (i == loopEnd)
				{
					block.flags = VAGF_LOOP_END;
					//quitAtTheNextIteration = true;
				}
			}
		}
		else
		{
			block.flags = VAGF_LOOP_LAST_BLOCK;
			if (loopFlag)
			{
				block.flags = VAGF_LOOP_END;
			}
		}


		s16 outBuf[28];
		for (int k = 0; k < 28; k++)
		{
			float s_double_trans = d_samples[k] + hist_1 * lut[predict][0] + hist_2 * lut[predict][1];
			float s_double = s_double_trans * (1 << shift);
			int sample = (int)(((int)s_double + 0x800) & 0xFFFFF000);

			if (sample > 32767)
			{
				sample = 32767;
			}
			if (sample < -32768)
			{
				sample = -32768;
			}

			outBuf[k] = (short)sample;

			sample >>= shift;
			hist_2 = hist_1;
			hist_1 = sample - s_double_trans;
		}

		for (int k = 0; k < 14; k++)
		{
			block.sample[k] = (u8)(((outBuf[(k * 2) + 1] >> 8) & 0xf0) | ((outBuf[k * 2] >> 12) & 0xf));
		}

		samples += 28;
		s8 lastPredictAndShift = (((block.predict << 4) & 0xF0) | (block.shift & 0x0F));
		outBuffer[globalIndex++] = lastPredictAndShift;
		outBuffer[globalIndex++] = block.flags;
		for (int h = 0; h < 14; h++)
			outBuffer[globalIndex++] = (u8)block.sample[h];
		bytesRead += chunkSize;
	}

	*outSize = sizeOfOut;
	return ret;
}

WavFile* LoadWavFile(const char* name)
{
	std::ifstream myfile(name, std::ios::binary | std::ios::ate);

	if (!myfile.is_open())
	{
		return NULL;
	}

	u32 fileSize = myfile.tellg();

	u8* bufferLoad = (u8*)malloc(fileSize);

	if (bufferLoad == NULL)
	{
		return NULL;
	}

	myfile.seekg(0, std::ios::beg);

	myfile.read((char*)bufferLoad, fileSize);

	WavFile* wavFile = (WavFile*)malloc(sizeof(WavFile));

	if (wavFile == NULL)
	{
		free(bufferLoad);
		return NULL;
	}

	u8* buffer = bufferLoad;

	memcpy(&wavFile->header, buffer, 36);

	buffer += 36;

	char list[4];

	memcpy(&list, buffer, 4);

	if (strncmp("data", list, 4) == 0)
	{
		memcpy(&wavFile->header.data_tag, list, 4);
	}
	else if (strncmp("list", list, 4) == 0)
	{
		int skip;
		buffer += 4;
		memcpy(&skip, buffer, 4);
		buffer += (skip + 4);
		memcpy(&wavFile->header.data_tag, buffer, 4);
	}
	else if (list[0] == 0 && list[1] == 0)
	{
		buffer += 6;
		int skip;
		memcpy(&skip, buffer, 4);
		buffer += (skip + 4);
		memcpy(&wavFile->header.data_tag, buffer, 4);
	}

	buffer += 4;

	memcpy(&wavFile->header.data_length, buffer, 4);

	int dataSize = wavFile->header.data_length;

	buffer += 4;

	wavFile->samples = (u8*)malloc(dataSize);

	if (wavFile->samples == NULL)
	{
		free(wavFile);
		free(bufferLoad);
		return NULL;
	}

	memcpy(wavFile->samples, buffer, dataSize);

	free(bufferLoad);

	myfile.close();

	return wavFile;
}

u8* LoadLmpFile(const char* name, int *size, int *samplerate)
{
	std::ifstream myfile(name, std::ios::binary | std::ios::ate);

	if (!myfile.is_open())
	{
		return NULL;
	}

	u32 fileSize = myfile.tellg();

	u8* bufferLoad = (u8*)malloc(fileSize);

	if (bufferLoad == NULL)
	{
		return NULL;
	}

	myfile.seekg(0, std::ios::beg);

	myfile.read((char*)bufferLoad, fileSize);

	*samplerate = 11025;

	*size = fileSize - 48;

	u8* buffer = (u8*)malloc(*size);

	memcpy(buffer, bufferLoad + 48, *size);

	free(bufferLoad);

	return buffer;
}

int main(int argc, char **argv)
{
	int size = 0;
	int sampleRate = 0;
	WavFile* file = LoadWavFile(".\\tone_c.wav");
	u8* rawsamples = file->samples;//LoadLmpFile(".\\test2.lmp", &size, &sampleRate);
	
	sampleRate = file->header.sample_rate;
	std::cout << file->header.num_channels << " " << file->header.bits_per_sample << " " << file->header.data_length << std::endl;
	s16* samples; // = new s16[shortSize];

	std::vector<float> coef = { .15f, .15f, .15f, .15f };
	
	ConvertPCM16<int16_t> *conversion = new (std::nothrow) ConvertPCM16<int16_t>(false, coef, file->header.data_length, rawsamples);

	//ConvertPCM16<uint8_t>* conversion = new (std::nothrow) ConvertPCM16<uint8_t>(true, coef, file->header.data_length, rawsamples);
	//ConvertPCM16<int32_t>* conversion = new (std::nothrow) ConvertPCM16<int32_t>(false, coef, file->header.data_length, rawsamples);
	if (conversion == nullptr)
	{
		free(file);
		return -1;
	}

	samples = conversion->convert();
	uint32_t shortSize = conversion->GetOutSize();

	std::cout << "hello" << shortSize << std::endl;
	/*u8* wavsamples = rawsamples;
	samples[0] = (u16)(wavsamples[0] - 0x80) << 8;
	samples[1] = (u16)(wavsamples[1] - 0x80) << 8;
	samples[2] = (u16)(wavsamples[2] - 0x80) << 8;
	for (int i = 3; i < shortSize; i++)
	{
		//u16 topByte = ((u16)(wavsamples[i * 2]));
		//u16 lowerByte = ((u16)wavsamples[(i * 2) + 1]) << 8;
		samples[i] = (u16)(((u8)(.45f * (wavsamples[i])) + (u8)(.25f * (wavsamples[i - 1])) + (u8)(0.175f * (wavsamples[i - 2]) + (u8)(0.125f * (wavsamples[i - 3]))) - 0x80) << 8);
		//std::cout << samples[i] << std::endl;
		//samples[i] = (u16)(wavsamples[i] - 0x80) << 8;

	} */

	for (int i = 0; i < shortSize; i++)
	{
		//std::cout << samples[i] << std::endl;
	}

	u32 outsize = 0;

	u8* encoded = CreateVagSamples(samples, shortSize, &outsize, 0, 0, 0);

	std::ofstream outFile(".\\dude4.vag", std::ios::binary);

	VagFileHeader header{};
	memset(&header, '\0', sizeof(VagFileHeader));
	memcpy(header.magic, "VAGp", 4);
	header.version = _byteswap_ulong(32);
	header.sampleRate = _byteswap_ulong(sampleRate);
	header.dataLength = _byteswap_ulong(outsize);
	header.channels = file->header.num_channels;
	memcpy(header.filename, "dude.vag", 8);
	std::cout << file->header.num_channels << std::endl;

	CreateVagFile(outFile, &header, encoded, outsize);

	outFile.close();
	
	//free(samples);
	free(encoded);
	//free(rawsamples);
	//free(file);

	return 0;
}