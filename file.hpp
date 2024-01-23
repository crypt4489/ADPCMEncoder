#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <fstream>
struct File
{
	uint8_t* samples{};
	uint32_t sampleRate{}, samplesSize{}, bps{};
	uint16_t channels{};
	bool isfloat = false;
	File() = default;
	virtual ~File() {
		if (samples)
			delete[] samples;
	};

	std::vector<uint8_t> LoadFile(std::string name)
	{
		std::ifstream filehandle(name, std::ios::binary | std::ios::ate);

		if (!filehandle.is_open())
			throw std::runtime_error("WAV file is unable to be opened");

		filehandle.seekg(0, std::ios_base::end);

		std::streampos filesize = filehandle.tellg();

		std::vector<uint8_t> filedata(filesize);

		filehandle.seekg(0, std::ios_base::beg);

		filehandle.read(reinterpret_cast<char*>(filedata.data()), filesize);

		filehandle.close();

		return filedata;
	}

	void u_law_decompression()
	{
		int16_t* decompressed = new (std::nothrow) int16_t[samplesSize];
		if (!decompressed)
		{
			std::cerr << "cannot allocate decompressed u_law samples\n";
			throw std::bad_alloc();
		}

		for (unsigned i = 0; i < samplesSize; i++)
		{
			constexpr uint16_t BIAS = 33;

			uint8_t input = ~samples[i];

			bool sign = false;

			if (input & 0x80)
			{
				sign = true;
				input &= 0x7f;
			}

			uint8_t quant = ((input & 0xF0) >> 4) + 5;

			int16_t decoded = ((1 << quant) | ((input & 0X0F) << (quant - 4)) | (1 << (quant - 5))) - BIAS;

			if (sign)
			{
				decoded = -decoded;
			}

			decompressed[i] = decoded;
		}

		delete[] samples;
		samples = (uint8_t*)decompressed;
		samplesSize *= 2;
		bps = 16;
	}

	void a_law_decompression()
	{
		int16_t* decompressed = new (std::nothrow) int16_t[samplesSize];
		if (!decompressed)
		{
			std::cerr << "cannot allocate decompressed a_law samples\n";
			throw std::bad_alloc();
		}

		for (unsigned i = 0; i < samplesSize; i++)
		{
			uint8_t input = 0x55 ^ samples[i];
			bool sign = false;
			if (input & 0x80)
			{
				sign = true;
				input &= 0x7f;
			}

			uint8_t quant = ((input & 0xF0) >> 4) + 4;
			int16_t decoded;

			if (quant != 4)
				decoded = ((1 << quant) | ((input & 0X0F) << (quant - 4)) | (1 << (quant - 5)));
			else
				decoded = (input << 1) | 1;

			if (sign)
			{
				decoded = -decoded;
			}

			decompressed[i] = decoded;
		}

		delete[] samples;
		samples = (uint8_t*)decompressed;
		samplesSize *= 2;
		bps = 16;
	}
};