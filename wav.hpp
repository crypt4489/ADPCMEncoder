#include <cstdint>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <iterator>
#include <vector>
#include <memory>
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

struct wavfile_holder_t
{
    WavFileHeader header{};
    uint8_t *samples;
    wavfile_holder_t() = default;
    ~wavfile_holder_t()
    {
        if (samples)
            delete[] samples;
    };

    static std::unique_ptr<WavFile> LoadWavFile(std::string name)
    {
        std::ifstream filehandle(name, std::ios::binary | std::ios::ate);

        if (!filehandle.is_open())
        {
            std::cout << "FILE NOT FOUND" << std::endl;
            return nullptr;
        }

        filehandle.seekg(0, std::ios_base::end);

        std::streampos filesize = filehandle.tellg();

        std::vector<uint8_t> filedata(filesize);

        filehandle.seekg(0, std::ios_base::beg);

        filehandle.read(reinterpret_cast<char *>(filedata.data()), filesize);

        filehandle.close();

        std::unique_ptr<WavFile> wavFile(new (std::nothrow) WavFile());

        if (!wavFile)
        {
            return nullptr;
        }

        std::vector<uint8_t>::iterator buffer = filedata.begin();

        std::copy(buffer, buffer + 36, &wavFile->header.riff_tag[0]);

        std::string list(buffer, buffer + 4);

        buffer = filedata.begin() + 20 + wavFile->header.fmt_length;
        std::string data(buffer, buffer + 4);
        if (data == "data")
        {
            std::copy(data.begin(), data.end(), &wavFile->header.data_tag[0]);
        }
        else
        {
            std::cout << "Unable to read WAV file" << std::endl;
            return nullptr;
        }

        buffer += 4;

        std::copy(buffer, buffer + 4, reinterpret_cast<uint8_t *>(&wavFile->header.data_length));

        int dataSize = wavFile->header.data_length;

        buffer += 4;

        wavFile->samples = new (std::nothrow) uint8_t[dataSize];

        if (!wavFile->samples)
        {
            return nullptr;
        }

        std::copy(buffer, buffer + dataSize, wavFile->samples);

        return wavFile;
    }
};