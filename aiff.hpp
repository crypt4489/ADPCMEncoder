#include "file.hpp"
#include "pcm24.hpp"
#include <numeric>
typedef struct form_chunk_t
{
	char FORM[4];
	uint32_t formSize;
	char AIFF[4];
} FormChunk;


typedef struct common_chunk_t
{
	char COMM[4];
	uint32_t size;
	uint16_t channels;
	uint32_t numFrames;
	uint16_t bps;
	unsigned long sampleRate;
} CommonChunk;

typedef struct sound_chunk_t
{
	uint32_t len;
	uint32_t offset;
	uint32_t blockSize;
} SoundChunk;


struct AIFFFile : public File
{
	FormChunk form;
	CommonChunk common;
	SoundChunk snd;
	AIFFFile() = delete;
	AIFFFile(std::string name)
	{
		LoadAIFFFile(name);
	}

	~AIFFFile() = default;
private:
	void LoadAIFFFile(std::string name)
	{
		auto filedata = LoadFile(name);
		auto buffer = filedata.begin();
		int32_t stride = 0;
		while (buffer != filedata.end())
		{
			std::string chunkID(buffer, buffer + 4);
			if (chunkID == "FORM")
			{
				std::copy(buffer, buffer + 4, &form.FORM[0]);
				form.formSize = ConvertBigEndian<uint32_t>(buffer+4);
				std::copy(buffer+8, buffer + 12, &form.AIFF[0]);
				stride = 12;
			}
			else if (chunkID == "COMM")
			{
				std::copy(buffer, buffer + 4, &common.COMM[0]);
				common.size = ConvertBigEndian<uint32_t>(buffer+4);
				common.channels = ConvertBigEndian<uint16_t>(buffer + 8);
				common.numFrames = ConvertBigEndian<uint32_t>(buffer + 10);
				common.bps = ConvertBigEndian<uint16_t>(buffer + 14);
				common.sampleRate = convert80bitto32bit(buffer+16);
				stride = common.size + 8;
			}
			else if (chunkID == "SSND")
			{
				samplessize = snd.len = ConvertBigEndian<int32_t>(buffer + 4)-8;
				samples = new (std::nothrow)uint8_t[samplessize];
				snd.offset = ConvertBigEndian<int32_t>(buffer + 8);
				snd.blockSize = ConvertBigEndian<int32_t>(buffer + 12);
				switch (common.bps)
				{
				case 16:
					SwapSamples<int16_t>(buffer + 12, buffer + 12 + samplessize);
					break;
				case 24:
					SwapSamples<PCM24>(buffer + 12, buffer + 12 + samplessize);
					break;
				case 32:
					SwapSamples<int32_t>(buffer + 12, buffer + 12 + samplessize);
					break;
				}
				
				std::copy(buffer + 12, buffer + 12 + samplessize, samples);
				stride = 16 + samplessize;
			}
			else if (chunkID == "ANNO" || chunkID == "NAME" || chunkID == "AUTH" || chunkID == "(c) ")
			{
				//pstring
				uint32_t skip = ConvertBigEndian<uint32_t>(buffer + 4);
				if (skip % 2) skip += 1;
				stride = skip+8;
			}
			else 
			{
				uint32_t skip = ConvertBigEndian<uint32_t>(buffer + 4);
				stride = skip + 8;
			}

			buffer += stride;
		}

		samplerate = common.sampleRate;
		channels = common.channels;
		bps = common.bps;
	}

	template<typename IntType> IntType ConvertBigEndian(std::vector<uint8_t>::iterator buffer)
	{
		int stride;
		if constexpr (std::is_same_v<IntType, PCM24>)
		{
			stride = 3;
		} 
		else
		{
			stride = sizeof(IntType);
		}
		return std::accumulate(buffer, buffer + stride, IntType{},
			[](IntType acc, uint8_t byte) {
				return (acc << 8) | static_cast<IntType>(byte);
			});
	}

	unsigned long long convert80bitto32bit(std::vector<uint8_t>::iterator buffer)
	{
		uint8_t exp = 30 - *(buffer + 1);
		unsigned long fraction = ConvertBigEndian<unsigned long>(buffer + 2);
		unsigned long last = 0;
		while(exp--)
		{ 
			last = fraction;
			fraction >>= 1;
		}
		if (last & 0x00000001) {
			fraction++;
		}
		return fraction;
	}

	template <typename T_Sample> void SwapSamples(std::vector<uint8_t>::iterator begin, std::vector<uint8_t>::iterator end)
	{
		int stride;
		if constexpr (std::is_same_v<T_Sample, PCM24>)
		{
			stride = 3;
		}
		else
		{
			stride = sizeof(T_Sample);
		}
		while (begin != end)
		{
			*reinterpret_cast<T_Sample*>(&(*begin)) = ConvertBigEndian<T_Sample>(begin);
			begin += stride;
		}
	}

};