#pragma once

#include <algorithm>
#include <cmath>
#include <fstream>
#ifdef _MSC_VER
#include <intrin.h>
#define BYTESWAP(x) _byteswap_ulong(x)
#else
#define BYTESWAP(x) __builtin_bswap32(x)
#endif
#include <iostream>
#include <iterator>
#include <limits>
#include <string>
#include <vector>


struct vagfile_header_t;
typedef struct vagfile_header_t VagFileHeader;
struct vagfile_holder_t;
typedef struct vagfile_holder_t VagFile;

static float enclut[5][2] = {{0.0, 0.0},
                             {-60.0 / 64.0, 0.0},
                             {-115.0 / 64.0, 52.0 / 64.0},
                             {-98.0 / 64.0, 55.0 / 64.0},
                             {-122.0 / 64.0, 60.0 / 64.0}};

enum VAGFlag
{
    VAGF_NOTHING = 0,          /* Nothing*/
    VAGF_LOOP_LAST_BLOCK = 1,  /* Last block to loop */
    VAGF_LOOP_REGION = 2,      /* Loop region*/
    VAGF_LOOP_END = 3,         /* Ending block of the loop */
    VAGF_LOOP_FIRST_BLOCK = 4, /* First block of looped data */
    VAGF_UNK = 5,              /* ?*/
    VAGF_LOOP_START = 6,       /* Starting block of the loop*/
    VAGF_PLAYBACK_END = 7      /* Playback ending position */
};

typedef struct enc_block_t
{
    int8_t shift;
    int8_t predict;
    uint8_t flags;
    uint8_t sample[14];
} EncBlock;

struct vagfile_header_t
{
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

struct vagfile_holder_t
{
    struct vagfile_header_t header{};
    std::vector<uint8_t> samples;

    vagfile_holder_t(uint32_t sampleRate, uint16_t channels, std::string filename)
    {
        header.magic[0] = 'V';
        header.magic[1] = 'A';
        header.magic[2] = 'G';
        header.magic[3] = 'p';
        header.version = BYTESWAP(32);
        header.sampleRate = BYTESWAP(sampleRate);
        header.channels = channels;
        auto endIter = (filename.size() <= 16) ? filename.end() : filename.begin() + 16;
        std::copy(filename.begin(), endIter, &header.filename[0]);
    }

    void WriteVagFile()
    {
        std::ofstream stream(reinterpret_cast<char*>(header.filename), std::ios::binary);
        size_t size = samples.size();
        stream.write(reinterpret_cast<char *>(&header), sizeof(VagFileHeader));
        char pad[16] = {0};
        stream.write(&pad[0], 16);
        stream.write(reinterpret_cast<char *>(samples.data()), size);
        stream.close();
    }

    void CreateVagSamples(int16_t *insamples, uint64_t len, uint32_t loopStart, uint32_t loopEnd, bool loopFlag, uint32_t channels)
    {
        float _hist_1 = 0.0, _hist_2 = 0.0;
        float hist_1 = 0.0, hist_2 = 0.0;

        int fullChunks = static_cast<int>(len / 28);
        int remaining = (fullChunks % 28);
        constexpr uint32_t sizeOfWrite = 16;
        if (remaining)
        {
            fullChunks++;
        }

        int sizeOfOut = fullChunks * sizeOfWrite;

        if (!loopFlag)
            sizeOfOut += sizeOfWrite;

        samples.resize(sizeOfOut);

        auto outBuffer = samples.begin();

        uint32_t bytesRead = 0;
        EncBlock block{0, 0, 0, {0}};
        for (int i = 0; i < fullChunks; i++)
        {

            int chunkSize = 28;
            if (i == fullChunks - 1)
            {
                chunkSize = remaining;
            }
            int predict = 0, shift;
            float min = 1e10;
            float s_1 = 0.0, s_2 = 0.0;
            float predictBuf[28][5];
            for (int j = 0; j < 5; j++)
            {
                float max = 0.0;

                s_1 = _hist_1;
                s_2 = _hist_2;

                for (int k = 0; k < chunkSize; k++)
                {
                    float sample = insamples[k];
                    if (sample > 30719.0)
                    {
                        sample = 30719.0;
                    }
                    if (sample < -30720.0)
                    {
                        sample = -30720.0;
                    }

                    float ds = sample + s_1 * enclut[j][0] + s_2 * enclut[j][1];

                    predictBuf[k][j] = ds;

                    if (fabs(ds) > max)
                    {
                        max = fabs(ds);
                    }

                    s_2 = s_1;
                    s_1 = sample;
                }
                if (max < min)
                {
                    min = max;
                    predict = j;
                }
                if (min <= 7)
                {
                    predict = 0;
                    break;
                }
            }
            
            _hist_1 = s_1;
            _hist_2 = s_2;

            float d_samples[28];
            for (int i = 0; i < 28; i++)
            {
                d_samples[i] = predictBuf[i][predict];
            }

            int min2 = static_cast<int>(min);
            int shift_mask = 0x4000;
            shift = 0;

            while (shift < 12)
            {
                if (shift_mask & (min2 + (shift_mask >> 3)))
                {
                    break;
                }
                shift++;
                shift_mask >>= 1;
            }
            block.predict = predict;
            block.shift = shift;

            if (len - bytesRead > 28)
            {
                block.flags = VAGF_NOTHING;
                if (loopFlag)
                {
                    block.flags = VAGF_LOOP_REGION;
                    if (i == loopStart)
                    {
                        block.flags = VAGF_LOOP_START;
                    }
                    if (i == loopEnd)
                    {
                        block.flags = VAGF_LOOP_END;
                    }
                }
            }
            else
            {
                block.flags = VAGF_LOOP_LAST_BLOCK;
                if (loopFlag)
                {
                    block.flags = VAGF_LOOP_END;
                }
            }

            int16_t outBuf[28];
            uint32_t power2 = (1 << shift);
            for (int k = 0; k < 28; k++)
            {
                float s_double_trans = d_samples[k] + hist_1 * enclut[predict][0] + hist_2 * enclut[predict][1];
                float s_double = s_double_trans * power2;
                int sample = (int)(((int)s_double + 0x800) & 0xFFFFF000);

                if (sample > std::numeric_limits<int16_t>::max())
                {
                    sample = std::numeric_limits<int16_t>::max();
                }
                if (sample < std::numeric_limits<int16_t>::min())
                {
                    sample = std::numeric_limits<int16_t>::min();
                }

                outBuf[k] = static_cast<int16_t>(sample);

                sample >>= shift;
                hist_2 = hist_1;
                hist_1 = sample - s_double_trans;
            }

            for (int k = 0; k < 14; k++)
            {
                block.sample[k] = static_cast<uint8_t>((((outBuf[(k * 2) + 1] >> 8) & 0xf0) | ((outBuf[k * 2] >> 12) & 0xf)));
            }

            insamples += 28;
            int8_t lastPredictAndShift = static_cast<int8_t>(((block.predict << 4) & 0xF0) | (block.shift & 0x0F));
            *outBuffer++ = lastPredictAndShift;
            *outBuffer++ = block.flags;
            for (int h = 0; h < 14; h++)
                *outBuffer++ = block.sample[h];
            bytesRead += chunkSize;
        }

        if (!loopFlag)
        {
            *outBuffer++ = 0;
            *outBuffer++ = VAGF_PLAYBACK_END;
            for (int h = 0; h < 14; h++)
                *outBuffer++ = 0;
        }

        header.dataLength = BYTESWAP(samples.size());
    }
};