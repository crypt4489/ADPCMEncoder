#pragma once

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <limits>
#include <vector>

#include "pcm24.hpp"

template <int N, typename T_SampleType>
struct FIR
{
	float coefficients[N];

	FIR(std::vector<float> _coef)
	{
		assert(N >= 2);
		assert(_coef.size() == N);
		for (int i = 0; i<N; i++) coefficients[i] = _coef[i];
	}

	T_SampleType CalculateNthSample(T_SampleType sample, int index)
	{
		assert(index < N);
		return static_cast<T_SampleType>(coefficients[index] * sample);
	}
};

template <typename T_SampleType>
class ConvertPCM16Data
{
public:
	uint64_t GetOutSize() const { return outSize; }

	int16_t *convert()
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

	virtual ~ConvertPCM16Data()
	{
		if (fir)
			delete fir;

	}

protected:
	static constexpr int FIRSIZE = 4;
	
	FIR<FIRSIZE, T_SampleType> *fir = nullptr;
	
	bool usefir = false;
	uint64_t sampleSize;
	uint8_t *inSamples;
	uint8_t bytesPerSample;
	uint64_t outSize;
	std::vector<int16_t>outSamples;
	std::vector<T_SampleType>convertSamples;

	ConvertPCM16Data(bool _use, uint64_t inSize, uint8_t *samples, 
	uint8_t bps, uint64_t _outSize, std::vector<float> coef) : 
	usefir(_use),												  
	sampleSize(inSize),
	inSamples(samples),
	bytesPerSample(bps),
	outSize(_outSize)

	{
		if (usefir)
		{
			fir = new (std::nothrow) FIR<FIRSIZE, T_SampleType>(coef);
		}
	}

	int16_t *convertwithfir()
	{
		
		for (int64_t i = (FIRSIZE-2) * bytesPerSample; i >= 0; i -= bytesPerSample)
		{
			outSamples.push_back(convertto16(i));
		}

		for (uint64_t i = (FIRSIZE-1) * bytesPerSample; i < sampleSize - 1; i += bytesPerSample)
		{
			T_SampleType sample = 0;

			for (int j = (FIRSIZE - 1) * bytesPerSample; j >= 0; j -= bytesPerSample)
			{
				sample += fir->CalculateNthSample(convertfirsample(i - j), j / bytesPerSample);
			}
			
			outSamples.push_back(convertto16(sample));
		}

		return outSamples.data();
	}

	int16_t *convertwithoutfir()
	{
		uint64_t outIndex = outSize - 1;

		for (int64_t i = sampleSize - bytesPerSample; i >= 0; i -= bytesPerSample)
		{
			outSamples.push_back (convertto16(i));
		}

		return outSamples.data();
	}

	virtual int16_t convertto16(int64_t index) = 0;
	virtual int16_t convertto16(T_SampleType val) = 0;
	virtual T_SampleType convertfirsample(int64_t index) = 0;
};

template <typename T_SampleType>
class ConvertPCM16 : public ConvertPCM16Data<T_SampleType>
{
public:
	ConvertPCM16() = delete;
	ConvertPCM16(bool _usefir,
		std::vector<float> coef,
		uint64_t inSize,
		uint8_t* samples) = delete;
	~ConvertPCM16() = delete;
	int16_t *convert() override { return nullptr; }

protected:
	int16_t convertto16(int64_t index) { return 0; }
	int16_t convertto16(T_SampleType val) { return 0; }
	T_SampleType convertfirsample(int64_t index) { return 0; }
};

template <>
class ConvertPCM16<uint8_t> : public ConvertPCM16Data<uint8_t>
{
public:
	ConvertPCM16() = delete;
	ConvertPCM16(bool _usefir,
				 std::vector<float> coef,
				 uint64_t inSize,
				 uint8_t *samples) : ConvertPCM16Data(_usefir, inSize, samples, 1, inSize, coef) {}

	~ConvertPCM16() = default;

protected:
	int16_t convertto16(int64_t index) override
	{
		return static_cast<int16_t>(inSamples[index] - 0x80) << 8;
	}

	int16_t convertto16(uint8_t val) override
	{
		return static_cast<int16_t>(val - 0x80) << 8;
	}

	uint8_t convertfirsample(int64_t index) override
	{
		return inSamples[index];
	}
};

template <>
class ConvertPCM16<int16_t> : public ConvertPCM16Data<int16_t>
{
public:
	ConvertPCM16() = delete;
	ConvertPCM16(bool _usefir,
				 std::vector<float> coef,
				 uint64_t inSize,
				 uint8_t *samples) : ConvertPCM16Data(_usefir, inSize, samples, 2, inSize >> 1, coef) {}

	~ConvertPCM16() = default;

protected:
	int16_t convertto16(int64_t index) override
	{
		int16_t lowerByte = (int16_t)inSamples[index];
		int16_t topByte = ((int16_t)inSamples[index + 1]) << 8;
		return (topByte | lowerByte);
	}

	int16_t convertto16(int16_t val) override
	{
		return val;
	}

	int16_t convertfirsample(int64_t index) override
	{
		return convertto16(index);
	}
};


template <>
class ConvertPCM16<PCM24> : public ConvertPCM16Data<PCM24>
{
public:
	ConvertPCM16() = delete;
	ConvertPCM16(bool _usefir,
		std::vector<float> coef,
		uint64_t inSize,
		uint8_t* samples) : ConvertPCM16Data(_usefir, inSize, samples, 3, inSize / 3, coef) {}

	~ConvertPCM16() = default;

protected:


	int16_t convertto16(int64_t index) override
	{
		PCM24 ret = bytepacker24(index);
		return convertto16(ret);
	}

	int16_t convertto16(PCM24 val) override
	{
		constexpr uint32_t num = static_cast<uint32_t>(std::numeric_limits<int16_t>::max() - std::numeric_limits<int16_t>::min());

		constexpr uint64_t denom = static_cast<uint64_t>(PCM24::INT24_MAX - PCM24::INT24_MIN);

		int16_t output = num * static_cast<float>(val) / denom;

		return output;
	}

	PCM24 convertfirsample(int64_t index) override
	{
		return bytepacker24(index);
	}
private:

	PCM24 bytepacker24(int64_t index)
	{
		int32_t lowerByte = static_cast<int32_t>(inSamples[index]);
		int32_t middle1Byte = static_cast<int32_t>(inSamples[index + 1]) << 8;
		int32_t topByte = static_cast<int32_t>(inSamples[index + 2]) << 16;
		int32_t uppermost = 0;
		
		if (topByte & 0x00800000)
			uppermost = 0xff000000;
		
		return PCM24(uppermost | topByte | middle1Byte | lowerByte);
	}
};



template <>
class ConvertPCM16<int32_t> : public ConvertPCM16Data<int32_t>
{
public:

	ConvertPCM16() = delete;
	ConvertPCM16(bool _usefir,
				 std::vector<float> coef,
				 uint32_t inSize,
				 uint8_t *samples) : ConvertPCM16Data(_usefir, inSize, samples, 4, inSize >> 2, coef) {}

	~ConvertPCM16() = default;

protected:

	int16_t convertto16(int64_t index) override
	{
		int32_t ret = bytepacker32(index);
		return convertto16(ret);
	}

	int16_t convertto16(int32_t val) override
	{
		constexpr uint32_t num = static_cast<uint32_t>(std::numeric_limits<int16_t>::max() - std::numeric_limits<int16_t>::min());

		constexpr uint64_t denom = static_cast<uint64_t>(std::numeric_limits<int32_t>::max()) - std::numeric_limits<int32_t>::min();

		int16_t output = num * static_cast<float>(val) / denom;

		return output;
	}

	int32_t convertfirsample(int64_t index) override
	{
		return bytepacker32(index);
	}

private:

	int32_t bytepacker32(int64_t index)
	{
		int32_t lowerByte = static_cast<int32_t>(inSamples[index]);
		int32_t middle1Byte = static_cast<int32_t>(inSamples[index + 1]) << 8;
		int32_t middle2Byte = static_cast<int32_t>(inSamples[index + 2]) << 16;
		int32_t topByte = static_cast<int32_t>(inSamples[index + 3]) << 24;;
		return (topByte | middle2Byte | middle1Byte | lowerByte);
	}
};
