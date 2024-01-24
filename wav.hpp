#pragma once

#include <algorithm>
#include <iostream>
#include <iterator>
#include <memory>

#include "file.hpp"
struct wavfile_header_t;
typedef struct wavfile_header_t WavFileHeader;
struct wavfile_holder_t;
typedef struct wavfile_holder_t WavFile;

struct wavfile_header_t
{
    int8_t riff_tag[4];
    uint32_t riff_length;
    int8_t wav_tag[4];
    int8_t fmt_tag[4];
    uint32_t fmt_length;
    int16_t audio_format;
    int16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    int16_t block_align;
    int16_t bits_per_sample;
    int8_t data_tag[4];
    uint32_t data_length;
};

struct wavfile_holder_t : public File
{
    WavFileHeader header{};
    wavfile_holder_t() = delete;
    wavfile_holder_t(std::string name)
    {
        LoadWavFile(name);
    }

    ~wavfile_holder_t() = default;

    enum AudioFormatCode
    {
        PCM = 1,
        MSADPCM = 2,
        Float = 3,
        ALaw = 6,
        ULaw = 7,
        Extensible = 0xFFFE
    };

private:
    void LoadWavFile(std::string name)
    {
        auto filedata = LoadFile(name);

        std::vector<uint8_t>::iterator buffer = filedata.begin();

        std::copy(buffer, buffer + 36, &header.riff_tag[0]);

        buffer = filedata.begin() + 20 + header.fmt_length;

        std::string data(buffer, buffer + 4);
        
        if (data == "data")
            std::copy(data.begin(), data.end(), &header.data_tag[0]);
        else
             throw std::runtime_error("WAV file does not have DATA tag");   

        buffer += 4;

        std::copy(buffer, buffer + 4, reinterpret_cast<uint8_t *>(&header.data_length));

        int dataSize = header.data_length;

        buffer += 4;

        samples = new (std::nothrow) uint8_t[dataSize];

        if (!samples)
        {   
            std::cerr << "Unable to create WAV samples buffer\n";
            throw std::bad_alloc();
        }

        std::copy(buffer, buffer + dataSize, samples);

        channels = header.num_channels;
        samplerate = header.sample_rate;
        samplessize = dataSize;
        bps = header.bits_per_sample;

        switch (header.audio_format)
        {
            case PCM:
                break;
            case MSADPCM:
                break;
            case Float:
                isfloat = true;
                break;
            case ALaw:
                ALawDecompression();
                break;
            case ULaw:
                ULawDecompression();
                break;
            case Extensible:
                break;
            default:
                break;
        }
    }
};