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
	uint64_t GetOutSize() const { return outsize; }

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
	uint64_t samplesize;
	uint8_t *insamples;
	uint8_t bytespersample;
	uint64_t outsize;
	uint16_t channels;
	std::vector<int16_t>outSamples;
	std::vector<T_SampleType>convertSamples;

	ConvertPCM16Data(bool _use, uint64_t inSize, uint8_t *samples, 
	uint8_t bps, uint64_t _outsize, uint16_t channels, std::vector<float> coef) : 
	usefir(_use),												  
	samplesize(inSize),
	insamples(samples),
	bytespersample(bps),
	outsize(_outsize),
	channels(channels)

	{
		if (usefir)
		{
			fir = new (std::nothrow) FIR<FIRSIZE, T_SampleType>(coef);
			if (!fir)
			{
				std::cerr << "Cannot allocate FIR sampling class\n";
				throw std::bad_alloc();
			}
		}
	}

	int16_t *convertwithfir()
	{
		
		for (int64_t i = (FIRSIZE-2) * bytespersample; i >= 0; i -= bytespersample)
		{
			outSamples.push_back(convertto16(i));
		}

		for (uint64_t i = (FIRSIZE-1) * bytespersample; i < samplesize - 1; i += bytespersample)
		{
			T_SampleType sample = 0;

			for (int j = (FIRSIZE - 1) * bytespersample; j >= 0; j -= bytespersample)
			{
				sample += fir->CalculateNthSample(convertfirsample(i - j), j / bytespersample);
			}
			
			outSamples.push_back(convertto16(sample));
		}

		if (channels == 2)
		{
			resample();
		}

		return outSamples.data();
	}

	int16_t *convertwithoutfir()
	{
		uint64_t outIndex = outsize - 1;

		for (int64_t i = samplesize - bytespersample; i >= 0; i -= bytespersample)
		{
			outSamples.push_back (convertto16(i));
		}

		if (channels == 2)
		{
			resample();
		}

		return outSamples.data();
	}

	void resample()
	{
		std::vector<int16_t> resampled;
		for (int i = 0; i < outsize; i += 2)
		{
			resampled.push_back((outSamples[i] + outSamples[i + 1]) / 2);
		}
		outsize = resampled.size();
		outSamples.swap(resampled);
	}

	virtual int16_t convertto16(int64_t index)
	{
		T_SampleType ret = bytepacker(index);
		return convertto16(ret);
	}

	virtual int16_t convertto16(T_SampleType val) = 0;

	virtual T_SampleType convertfirsample(int64_t index)
	{
		return bytepacker(index);
	}

	virtual T_SampleType bytepacker(int64_t index) = 0;
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
	T_SampleType bytepacker(int64_t index) { return 0;  }
};

template <>
class ConvertPCM16<uint8_t> : public ConvertPCM16Data<uint8_t>
{
public:

	ConvertPCM16() = delete;
	ConvertPCM16(bool _usefir,
				 std::vector<float> coef,
				 uint64_t inSize,
				 uint8_t *samples, uint16_t channels) : ConvertPCM16Data(_usefir, inSize, samples, 1, channels, inSize, coef) {}

	~ConvertPCM16() = default;

protected:

	int16_t convertto16(int64_t index) override
	{
		return static_cast<int16_t>(insamples[index] - 0x80) << 8;
	}

	int16_t convertto16(uint8_t val) override
	{
		return static_cast<int16_t>(val - 0x80) << 8;
	}

	uint8_t convertfirsample(int64_t index) override
	{
		return insamples[index];
	}

	uint8_t bytepacker(int64_t index) override
	{
		return 0;
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
		uint8_t* samples, uint16_t channels) : ConvertPCM16Data(_usefir, inSize, samples, 2, inSize >> 1, channels, coef) {}

	~ConvertPCM16() = default;

protected:

	int16_t convertto16(int64_t index) override
	{
		return bytepacker(index);
	}

	int16_t convertto16(int16_t val) override
	{
		return val;
	}

	int16_t convertfirsample(int64_t index) override
	{
		return bytepacker(index);
	}

	int16_t bytepacker(int64_t index) override
	{
		int16_t lowerByte = (int16_t)insamples[index];
		int16_t topByte = ((int16_t)insamples[index + 1]) << 8;
		return (topByte | lowerByte);
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
		uint8_t* samples, uint16_t channels) : ConvertPCM16Data(_usefir, inSize, samples, 3, inSize / 3, channels, coef) {}

	~ConvertPCM16() = default;

protected:

	int16_t convertto16(PCM24 val) override
	{
		constexpr uint32_t num = static_cast<uint32_t>(std::numeric_limits<int16_t>::max() - std::numeric_limits<int16_t>::min());

		constexpr uint32_t denom = static_cast<uint32_t>(PCM24::INT24_MAX - PCM24::INT24_MIN);

		int16_t output = num * static_cast<float>(val) / denom;

		return output;
	}

	PCM24 bytepacker(int64_t index) override
	{
		int32_t lowerByte = static_cast<int32_t>(insamples[index]);
		int32_t middleByte = static_cast<int32_t>(insamples[index + 1]) << 8;
		int32_t topByte = static_cast<int32_t>(insamples[index + 2]) << 16;
		int32_t excess = 0;
		
		if (topByte & 0x00800000)
			excess = 0xff000000;
		
		return PCM24(excess | topByte | middleByte | lowerByte);
	}
};



template <>
class ConvertPCM16<int32_t> : public ConvertPCM16Data<int32_t>
{
public:

	ConvertPCM16() = delete;
	ConvertPCM16(bool _usefir,
		std::vector<float> coef,
		uint64_t inSize,
		uint8_t* samples, uint16_t channels) : ConvertPCM16Data(_usefir, inSize, samples, 4, inSize >> 2, channels, coef) {}

	~ConvertPCM16() = default;

protected:

	
	int16_t convertto16(int32_t val) override
	{
		constexpr uint32_t num = static_cast<uint32_t>(std::numeric_limits<int16_t>::max() - std::numeric_limits<int16_t>::min());

		constexpr uint64_t denom = static_cast<uint64_t>(std::numeric_limits<int32_t>::max()) - std::numeric_limits<int32_t>::min();

		int16_t output = num * static_cast<float>(val) / denom;

		return output;
	}

	int32_t bytepacker(int64_t index) override
	{
		int32_t out = 0;
		for (int i = 0; i < bytespersample; i++)
		{
			out |= static_cast<int32_t>(insamples[index + i]) << (8 * i);
		}
		return out;
	}
};

template <>
class ConvertPCM16<float> : public ConvertPCM16Data<float>
{
public:

	ConvertPCM16() = delete;
	ConvertPCM16(bool _usefir,
		std::vector<float> coef,
		uint64_t inSize,
		uint8_t* samples, uint16_t channels) : ConvertPCM16Data(_usefir, inSize, samples, 4, inSize >> 2, channels, coef) {}

	~ConvertPCM16() = default;

protected:


	int16_t convertto16(float val) override
	{
		constexpr uint32_t num = std::numeric_limits<int16_t>::max();

		int16_t output = num * val;

		//std::cout << output << std::endl;

		return output;
	}

	float bytepacker(int64_t index) override
	{
		union 
		{
			float val;
			int32_t intVal;
		} out{};

		for (int i = 0; i < bytespersample; i++)
		{
			out.intVal |= static_cast<int32_t>(insamples[index + i]) << (8 * i);
		}

		return out.val;
	}
};

template <>
class ConvertPCM16<double> : public ConvertPCM16Data<double>
{
public:

	ConvertPCM16() = delete;
	ConvertPCM16(bool _usefir,
		std::vector<float> coef,
		uint64_t inSize,
		uint8_t* samples, uint16_t channels) : ConvertPCM16Data(_usefir, inSize, samples, 8, inSize >> 3, channels, coef) {}

	~ConvertPCM16() = default;

protected:

	int16_t convertto16(double val) override
	{
		constexpr uint32_t num = std::numeric_limits<int16_t>::max();

		int16_t output = num * val;

		return output;
	}

	double bytepacker(int64_t index) override
	{
		union
		{
			double val;
			int64_t intVal;
		} out{};
		
		for (int i = 0; i < bytespersample; i++)
		{
			out.intVal |= static_cast<int64_t>(insamples[index + i]) << (8 * i);
		}

		return out.val;
	}
};

