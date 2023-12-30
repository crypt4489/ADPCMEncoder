#include "types.h"

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <algorithm>
#include <iterator>
#ifdef _MSC_VER
#include <intrin.h>
#define BYTESWAP(x) _byteswap_ulong(x)
#else
#define BYTESWAP(x) __builtin_bswap32(x)
#endif
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
	int8_t shift;
	int8_t predict;
	uint8_t flags;
	uint8_t sample[14];
} EncBlock;

void CreateVagFile(std::ofstream &stream, VagFileHeader *header, uint8_t* buffer, uint32_t size)
{
	stream.write((char*)header, sizeof(VagFileHeader));
	char pad[16] = {0};
	stream.write(&pad[0], 16);
	stream.write((char*)buffer, size);
}


uint8_t* CreateVagSamples(int16_t* samples, uint32_t len, uint32_t* outSize, uint32_t loopStart, uint32_t loopEnd, uint32_t loopFlag)
{
	float _hist_1 = 0.0, _hist_2 = 0.0;
	float hist_1 = 0.0, hist_2 = 0.0;

	int fullChunks = len * 0.035714;
	int remaining = len - (fullChunks * 28);
	constexpr uint32_t sizeOfWrite = 16;
	if (remaining)
	{
		fullChunks++;
	}

	int sizeOfOut = fullChunks * sizeOfWrite;

	if (!loopFlag)
		sizeOfOut += sizeOfWrite;

	uint8_t* outBuffer = new (std::nothrow) uint8_t[sizeOfOut];

	uint8_t* ret = outBuffer;

	std::memset(outBuffer, '\0', sizeOfOut);

	uint32_t globalIndex = 0;
	uint32_t bytesRead = 0;
	EncBlock block{ 0, 0, 0, {0} };
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


		int16_t outBuf[28];
		uint32_t power2 = (1 << shift);
		for (int k = 0; k < 28; k++)
		{
			float s_double_trans = d_samples[k] + hist_1 * lut[predict][0] + hist_2 * lut[predict][1];
			float s_double = s_double_trans * power2;
			int sample = (int)(((int)s_double + 0x800) & 0xFFFFF000);

			if (sample > std::numeric_limits<int16_t>::max())
			{
				sample = std::numeric_limits<int16_t>::max();
			}
			if (sample < std::numeric_limits<int16_t>::min())
			{
				sample = std::numeric_limits<int16_t>::min();
			}

			outBuf[k] = static_cast<int16_t>(sample);

			sample >>= shift;
			hist_2 = hist_1;
			hist_1 = sample - s_double_trans;
		}

		for (int k = 0; k < 14; k++)
		{
			block.sample[k] = static_cast<uint8_t>((((outBuf[(k * 2) + 1] >> 8) & 0xf0) | ((outBuf[k * 2] >> 12) & 0xf)));
		}

		samples += 28;
		int8_t lastPredictAndShift = (((block.predict << 4) & 0xF0) | (block.shift & 0x0F));
		outBuffer[globalIndex++] = lastPredictAndShift;
		outBuffer[globalIndex++] = block.flags;
		for (int h = 0; h < 14; h++)
			outBuffer[globalIndex++] = block.sample[h];
		bytesRead += chunkSize;
	}

	if (!loopFlag)
	{
		EncBlock block{ 0, 0, VAGF_PLAYBACK_END, {0} };
		outBuffer[globalIndex++] = 0;
		outBuffer[globalIndex++] = block.flags;
		for (int h = 0; h < 14; h++)
			outBuffer[globalIndex++] = (uint8_t)block.sample[h];
	}

	*outSize = sizeOfOut;
	return ret;
}

WavFile* LoadWavFile(const char* name)
{
	std::ifstream filehandle(name, std::ios::binary | std::ios::ate);

	if (!filehandle.is_open())
	{
		std::cout << "FILE NOT FOUND" << std::endl;
		return nullptr;
	}

	filehandle.seekg(0, std::ios_base::end);

    std::streampos filesize = filehandle.tellg();

	std::vector<uint8_t> filedata(filesize);

	filehandle.seekg(0, std::ios_base::beg);

	filehandle.read(reinterpret_cast<char*>(filedata.data()), filesize);

	filehandle.close();

	std::cout << filedata.size() << std::endl;

	WavFile *wavFile = new (std::nothrow) WavFile();

	if (!wavFile)
	{
		return wavFile;
	}

	std::vector<uint8_t>::iterator buffer = filedata.begin();

	std::copy(buffer, buffer+36, &wavFile->header.riff_tag[0]);

	buffer += 36;

	std::string list(buffer, buffer+4);

	if (list == "data")
	{
		//memcpy(&wavFile->header.data_tag[0], list, 4);
		std::copy(&list[0], &list[4], &wavFile->header.data_tag[0]);
	}
	else if (list == "list")
	{
		int skip;
		buffer += 4;
		std::copy(buffer, buffer+4, reinterpret_cast<uint8_t*>(&skip));
		buffer += (skip + 4);
		std::copy(buffer, buffer+4, &wavFile->header.data_tag[0]);
	}
	else if (list[0] == 0 && list[1] == 0)
	{
		buffer += 6;
		int skip;
		std::copy(buffer, buffer+4, reinterpret_cast<uint8_t*>(&skip));
		buffer += (skip + 4);
		std::copy(buffer, buffer+4, &wavFile->header.data_tag[0]);
	} else {
		//search for a bit for the data tag.
		int i = 0;
		
		for (; i<32; i++)
		{
			buffer += 4;
			std::string data(buffer, buffer+4);
			if (data == "data")
			{
				std::copy(buffer, buffer+4, &wavFile->header.data_tag[0]);
				break;
			}
		}

		if (i == 32)
		{
			std::cout << "Unable to read WAV file" << std::endl;
			delete wavFile;
			return nullptr;
		}
	}

	buffer += 4;

	std::copy(buffer, buffer+4, reinterpret_cast<uint8_t*>(&wavFile->header.data_length));

	int dataSize = wavFile->header.data_length;

	buffer += 4;

	wavFile->samples = new(std::nothrow) uint8_t[dataSize];

	if (!wavFile->samples)
	{
		delete wavFile;
		return nullptr;
	}

	std::copy(buffer, buffer+dataSize, wavFile->samples);

	return wavFile;
}

uint8_t* LoadLmpFile(const char* name, int *size, int *samplerate)
{
	std::ifstream myfile(name, std::ios::binary | std::ios::ate);

	if (!myfile.is_open())
	{
		return NULL;
	}

	uint32_t fileSize = myfile.tellg();

	uint8_t* bufferLoad = (uint8_t*)malloc(fileSize);

	if (bufferLoad == NULL)
	{
		return NULL;
	}

	myfile.seekg(0, std::ios::beg);

	myfile.read((char*)bufferLoad, fileSize);

	*samplerate = 11025;

	*size = fileSize - 48;

	uint8_t* buffer = (uint8_t*)malloc(*size);

	memcpy(buffer, bufferLoad + 48, *size);

	free(bufferLoad);

	return buffer;
}

int main(int argc, char **argv)
{
	int sampleRate = 0;
	WavFile* file = LoadWavFile("./laugh.wav");

	if (file == NULL)
	{
		return -1;
	}

	uint8_t* rawsamples = file->samples;
	
	sampleRate = file->header.sample_rate;
	
	std::cout << file->header.num_channels << " " << file->header.bits_per_sample << " " << file->header.data_length << std::endl;
	
	int16_t* samples; 

	std::vector<float> coef = { .15f, .15f, .15f, .15f };
	
	//ConvertPCM16<int16_t> *conversion = new (std::nothrow) ConvertPCM16<int16_t>(false, coef, file->header.data_length, rawsamples);

	//ConvertPCM16<uint8_t>* conversion = new (std::nothrow) ConvertPCM16<uint8_t>(true, coef, file->header.data_length, rawsamples);
	ConvertPCM16<int32_t>* conversion = new (std::nothrow) ConvertPCM16<int32_t>(false, coef, file->header.data_length, rawsamples);
	if (conversion == nullptr)
	{
		free(file);
		return -1;
	}

	samples = conversion->convert();
	uint32_t shortSize = conversion->GetOutSize();

	uint32_t outsize = 0;

	uint8_t* encoded = CreateVagSamples(samples, shortSize, &outsize, 0, 0, 0);

	std::ofstream outFile(".\\dude4.vag", std::ios::binary);

	std::cout << outsize << std::endl;

	VagFileHeader header{};
	std::string headerString("VAGp");
	std::copy(headerString.begin(), headerString.end(), &header.magic[0]);
	header.version = BYTESWAP(32);
	header.sampleRate = BYTESWAP(sampleRate);
	header.dataLength = BYTESWAP(outsize);
	header.channels = file->header.num_channels;
	std::string output("dude.vag");
	std::copy(output.begin(), output.end(), &header.filename[0]);
	std::cout << file->header.num_channels << std::endl;

	CreateVagFile(outFile, &header, encoded, outsize);

	outFile.close();
	
	//free(samples);
	free(encoded);
	//free(rawsamples);
	//free(file);
	delete file;

	return 0;
}