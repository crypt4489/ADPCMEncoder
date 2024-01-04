#pragma once
#include <cstdint>

struct File
{
	uint8_t* samples{};
	uint32_t sampleRate{}, samplesSize{}, bps{};
	uint16_t channels{};
};