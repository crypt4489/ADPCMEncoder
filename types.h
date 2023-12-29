#pragma once

typedef unsigned char u8;
typedef char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef int s32;



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
	u8* samples;
};


struct vagfile_header_t {
	u8 magic[4];
	u32 version;
	u32 reserved4;
	u32 dataLength;
	u32 sampleRate;
	u8 reserved10[10];
	u8 channels;
	u8 reservedbyte;
	s8 filename[16];
};

struct vagfile_holder_t {
	struct vagfile_header_t header;
	u8* samples;
};