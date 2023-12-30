#pragma once

struct wavfile_header_t;
typedef struct wavfile_header_t WavFileHeader;
struct wavfile_holder_t;
typedef struct wavfile_holder_t WavFile;

struct vagfile_header_t;
typedef struct vagfile_header_t VagFileHeader;
struct vagfile_holder_t;
typedef struct vagfile_holder_t VagFile;

struct wavfile_header_t {
	char	riff_tag[4];
	int	riff_length;
	char	wav_tag[4];
	char	fmt_tag[4];
	int	fmt_length;
	short	audio_format;
	short	num_channels;
	int	sample_rate;
	int	byte_rate;
	short	block_align;
	short	bits_per_sample;
	char	data_tag[4];
	int	data_length;
};

struct wavfile_holder_t
{
	WavFileHeader header;
	unsigned char* samples;
};


struct vagfile_header_t {
	unsigned char magic[4];
	unsigned int version;
	unsigned int reserved4;
	unsigned int dataLength;
	unsigned int sampleRate;
	unsigned char reserved10[10];
	unsigned char channels;
	unsigned char reservedbyte;
	char filename[16];
};

struct vagfile_holder_t {
	struct vagfile_header_t header;
	unsigned char* samples;
};