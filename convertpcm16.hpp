
#include <vector>
#include <cstdint>
#include <cassert>
#include <limits>

template <int N, typename T_SampleType> struct FIR
{
	float coefficients[N];

	FIR(std::vector<float> _coef)
	{
		assert(_coef.size() == N);
		memcpy(coefficients, _coef.data(), sizeof(float) * N);
	}

	T_SampleType CalculateNthSample(T_SampleType sample, int index)
	{
		assert(index < N);
		return static_cast<T_SampleType>(coefficients[index] * sample);
	}
};

template <typename T_SampleType> class ConvertPCM16Data
{

public:
	uint32_t GetOutSize() const { return outSize; }

protected:
	static constexpr int FIRSIZE = 4;
	FIR<FIRSIZE, T_SampleType>* fir = nullptr;
	bool usefir = false;
	int16_t* outSamples;
	uint32_t sampleSize;
	uint32_t outSize;
	uint8_t* inSamples;
	T_SampleType *convertSamples;
	uint8_t bytesPerSample;

	ConvertPCM16Data(bool _use, uint32_t inSize, uint8_t *samples, uint8_t bps, uint32_t _outSize) : 
		usefir(_use), 
		sampleSize(inSize), 
		inSamples(samples),
		outSamples(nullptr),
		convertSamples(nullptr),
		bytesPerSample(bps), 
		outSize(_outSize)

	{}

	virtual ~ConvertPCM16Data()
	{
		if (fir) delete fir;
		if (outSamples) delete outSamples;
		if (convertSamples) delete convertSamples;
	}
	
};

template <typename T_SampleType> class ConvertPCM16 : public ConvertPCM16Data<T_SampleType>
{
public:
	ConvertPCM16() = delete;
	int16_t* convert()
	{
		return nullptr;
	}
};


template <> class ConvertPCM16<uint8_t> : public ConvertPCM16Data<uint8_t>
{
public:
	ConvertPCM16() = default;
	ConvertPCM16(bool _usefir, 
		std::vector<float> coef,
		uint32_t inSize,
		uint8_t *samples) : ConvertPCM16Data(_usefir, inSize, samples, 1, inSize) 
	{
		if (usefir) {
			fir = new (std::nothrow) FIR<FIRSIZE, uint8_t>(coef);
		}

		outSamples = new (std::nothrow) int16_t[outSize];
	}

	~ConvertPCM16()
	{
		
	}

	int16_t* convert()
	{
		if (usefir)
		{
			return convertwithfir();
		} 
		else
		{
			return convertwithoutfir();
		}
	}

private:
	int16_t* convertwithfir()
	{
		for (int i = FIRSIZE-2; i >= 0; i--)
		{
			outSamples[i] = static_cast<int16_t>(inSamples[i] - 0x80) << 8;
		}

		for (int i = FIRSIZE-1; i < sampleSize; i++)
		{
			uint8_t sample = 0;
			for (int j = FIRSIZE-1; j >= 0; j--)
			{
				sample += fir->CalculateNthSample(inSamples[i - j], j);
			}
			outSamples[i] = static_cast<int16_t>(sample - 0x80) << 8;
		}

		return outSamples;
	}

	int16_t* convertwithoutfir()
	{
		for (int i = sampleSize-1; i >= 0; i--)
		{
			outSamples[i] = static_cast<int16_t>(inSamples[i] - 0x80) << 8;
		}

		return outSamples;
	}	
};

template <> class ConvertPCM16<int16_t> : public ConvertPCM16Data<int16_t>
{
public:
	ConvertPCM16() = default;
	ConvertPCM16(bool _usefir,
		std::vector<float> coef,
		uint32_t inSize,
		uint8_t* samples) : ConvertPCM16Data(_usefir, inSize, samples, 2, inSize >> 1)
	{
		if (usefir) {
			fir = new (std::nothrow) FIR<FIRSIZE, int16_t>(coef);
		}
		outSamples = new (std::nothrow) int16_t[outSize];
	}

	~ConvertPCM16()
	{
	
	}

	int16_t* convert()
	{
		if (usefir)
		{
			return convertwithfir();
		}
		else
		{
			return convertwithoutfir();
		}
	}

private:
	int16_t* convertwithfir()
	{
		int outIndex = 0;

		for (int i = (FIRSIZE - 2) * bytesPerSample; i >= 0; i-=bytesPerSample)
		{
			outSamples[outIndex++] = convertBytesToShort(i);
		}

		for (int i = (FIRSIZE-1 * bytesPerSample); i < sampleSize-1; i+=bytesPerSample)
		{
			int16_t sample = 0;

			for (int j = (FIRSIZE - 1) * bytesPerSample; j >= 0; j-= bytesPerSample)
			{
				sample += fir->CalculateNthSample(convertBytesToShort(i-j), j / bytesPerSample);
			}
			
			outSamples[outIndex++] = sample;
		}

		return outSamples;
	}

	int16_t* convertwithoutfir()
	{
		int outIndex = outSize-1;

		for (int i = sampleSize- bytesPerSample; i >= 0; i-= bytesPerSample)
		{
			outSamples[outIndex--] = convertBytesToShort(i);
		}

		return outSamples;
	}

	int16_t convertBytesToShort(int index)
	{
		int16_t lowerByte = (int16_t)inSamples[index];
		int16_t topByte = ((int16_t)inSamples[index + 1]) << 8;
		return static_cast<int16_t>(topByte | lowerByte);
	}
};

template <> class ConvertPCM16<int32_t> : public ConvertPCM16Data<int32_t>
{
public:
	ConvertPCM16() = default;
	ConvertPCM16(bool _usefir,
		std::vector<float> coef,
		uint32_t inSize,
		uint8_t* samples) : ConvertPCM16Data(_usefir, inSize, samples, 4, inSize >> 2)
	{
		if (usefir) {
			fir = new (std::nothrow) FIR<FIRSIZE, int32_t>(coef);
		}
		outSamples = new (std::nothrow) int16_t[outSize];
	}

	~ConvertPCM16()
	{

	}

	int16_t* convert()
	{
		if (usefir)
		{
			return convertwithfir();
		}
		else
		{
			return convertwithoutfir();
		}
	}

private:
	int16_t* convertwithfir()
	{
		int outIndex = 0;

		for (int i = (FIRSIZE - 2) * bytesPerSample; i >= 0; i -= bytesPerSample)
		{
			outSamples[outIndex++] = convertBytesToInt(i);
		}

		for (int i = (FIRSIZE - 1 * bytesPerSample); i < sampleSize - 1; i += bytesPerSample)
		{
			int16_t sample = 0;

			for (int j = (FIRSIZE - 1) * bytesPerSample; j >= 0; j -= bytesPerSample)
			{
				sample += fir->CalculateNthSample(convertBytesToInt(i - j), j / bytesPerSample);
			}

			outSamples[outIndex++] = sample;
		}

		return outSamples;
	}

	int16_t* convertwithoutfir()
	{
		int outIndex = outSize - 1;

		for (int i = sampleSize - bytesPerSample; i >= 0; i -= bytesPerSample)
		{
			outSamples[outIndex--] = convertIntToShort(convertBytesToInt(i));
			printf("%x\n", outSamples[outIndex + 1]);
		}

		return outSamples;
	}

	int32_t convertBytesToInt(int index)
	{
		int32_t lowerByte = (int32_t)inSamples[index];
		int32_t middle1Byte = ((int32_t)inSamples[index + 1]) << 8;
		int32_t middle2Byte = ((int32_t)inSamples[index + 2]) << 16;
		int32_t topByte = ((int32_t)inSamples[index + 3]) << 24;
		return static_cast<int32_t>(topByte | middle2Byte | middle1Byte | lowerByte);
	}

	int16_t convertIntToShort(int32_t sample)
	{
		printf("%x heere\n", sample);
		return (std::numeric_limits<int16_t>::max() - std::numeric_limits<int16_t>::min()) * (float)(sample / (uint64_t)(std::numeric_limits<int32_t>::max() - std::numeric_limits<int32_t>::min()));
	}
};