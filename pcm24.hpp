#pragma once

#include <cstdint>

class PCM24
{
public:
	uint8_t bytes[3];
	int32_t integerVal;
	int32_t GetValue() const { return integerVal; }

};