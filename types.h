#pragma once

#include <cstdint>

struct wavfile_header_t;
typedef struct wavfile_header_t WavFileHeader;
struct wavfile_holder_t;
typedef struct wavfile_holder_t WavFile;

struct vagfile_header_t;
typedef struct vagfile_header_t VagFileHeader;
struct vagfile_holder_t;
typedef struct vagfile_holder_t VagFile;

struct wavfile_header_t {
	int8_t	riff_tag[4];
	uint32_t	riff_length;
	int8_t	wav_tag[4];
	int8_t	fmt_tag[4];
	uint32_t	fmt_length;
	int16_t	audio_format;
	int16_t	num_channels;
	uint32_t	sample_rate;
	uint32_t	byte_rate;
	int16_t	block_align;
	int16_t	bits_per_sample;
	int8_t	data_tag[4];
	uint32_t	data_length;
};

struct wavfile_holder_t
{
	WavFileHeader header;
	uint8_t* samples;
};


struct vagfile_header_t {
	uint8_t magic[4];
	uint32_t version;
	uint32_t reserved4;
	uint32_t dataLength;
	uint32_t sampleRate;
	uint8_t reserved10[10];
	uint8_t channels;
	uint8_t reservedbyte;
	int8_t filename[16];
};

struct vagfile_holder_t {
	struct vagfile_header_t header;
	uint8_t* samples;
};