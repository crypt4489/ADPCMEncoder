#pragma once

#include <cstdint>
#include <cassert>
class PCM24
{
public:
	static constexpr int32_t INT24_MIN = -8388608;
	static constexpr int32_t INT24_MAX = 8388607;
	int32_t integerval;
	
	PCM24() = default;

	PCM24(int val)
	{
		assert(val >= INT24_MIN && val <= INT24_MAX);
		integerval = val;
	}

	operator float() const
	{
		return static_cast<float>(integerval);
	}

	operator int() const
	{
		return integerval;
	}

	PCM24 operator +(const PCM24& val) const
	{
		return PCM24(this->integerval + val.integerval);
	}

	PCM24 operator -(const PCM24& val) const
	{
		return PCM24(this->integerval - val.integerval);
	}

	PCM24 operator *(const PCM24& val) const
	{
		return PCM24(this->integerval * val.integerval);
	}

	PCM24 operator /(const PCM24& val) const
	{
		return PCM24(this->integerval / val.integerval);
	}

	PCM24 operator |(const PCM24& val) const
	{
		return PCM24(this->integerval | val.integerval);
	}

	PCM24 operator <<(const int shift) const
	{
		return PCM24(this->integerval << shift);
	}


	PCM24& operator=(int val)
	{
		assert(val <= INT24_MAX && val >= INT24_MIN);
		integerval = val;
		return *this;
	}

	PCM24& operator+=(PCM24& val)
	{
		*this = *this + val;
		return *this;
	}

	PCM24& operator+=(PCM24 val)
	{
		*this = *this + val;
		return *this;
	}

	PCM24& operator-=(PCM24& val)
	{
		*this = *this - val;
		return *this;
	}

	PCM24& operator/=(PCM24& val)
	{
		*this = *this / val;
		return *this;
	}

	PCM24& operator*=(PCM24& val)
	{
		*this = *this * val;
		return *this;
	}

	friend PCM24 operator *(const float val, const PCM24& rhs)
	{
		return PCM24(rhs.integerval * val);
	}

};