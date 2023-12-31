
#include <iostream>
#include <fstream>
#include <string>
#include <memory>

#include "convertpcm16.hpp"
#include "program.hpp"
#include "wav.hpp"
#include "vag.hpp"


uint8_t *LoadLmpFile(const char *name, int *size, int *samplerate)
{
	std::ifstream myfile(name, std::ios::binary | std::ios::ate);

	if (!myfile.is_open())
	{
		return NULL;
	}

	uint32_t fileSize = myfile.tellg();

	uint8_t *bufferLoad = (uint8_t *)malloc(fileSize);

	if (bufferLoad == NULL)
	{
		return NULL;
	}

	myfile.seekg(0, std::ios::beg);

	myfile.read((char *)bufferLoad, fileSize);

	*samplerate = 11025;

	*size = fileSize - 48;

	uint8_t *buffer = (uint8_t *)malloc(*size);

	memcpy(buffer, bufferLoad + 48, *size);

	free(bufferLoad);

	return buffer;
}

int main(int argc, char **argv)
{
	Program *program;
	try
	{
		program = new Program(argc, argv);

		std::unique_ptr<WavFile> file = WavFile::LoadWavFile(program->GetFilePath());

		if (file == nullptr)
		{
			return -1;
		}

		uint8_t *rawsamples = file->samples;

		uint32_t sampleRate = file->header.sample_rate;

		uint32_t samplesSize = file->header.data_length;

		uint16_t channels = file->header.num_channels;

		std::vector<float> coef = {.15f, .15f, .15f, .15f};

		std::cout << samplesSize << " " 
					<< channels << " "
					<< sampleRate << "\n";

		// ConvertPCM16<int16_t> *conversion = new (std::nothrow) ConvertPCM16<int16_t>(false, coef, file->header.data_length, rawsamples);

		// ConvertPCM16<uint8_t>* conversion = new (std::nothrow) ConvertPCM16<uint8_t>(true, coef, file->header.data_length, rawsamples);
		std::unique_ptr<ConvertPCM16<int32_t>> conversion(new (std::nothrow) 
		ConvertPCM16<int32_t>(program->GetNoiseReduce(), 
		coef, samplesSize, rawsamples));
		
		if (conversion == nullptr)
		{
			return -1;
		}

		std::unique_ptr<VagFile> vagFile(new VagFile(sampleRate, channels, program->GetOutputFile()));

		std::cout << conversion->GetOutSize() << std::endl;

		vagFile->CreateVagSamples(conversion->convert(), conversion->GetOutSize(), 0, 0, false);

		std::ofstream outFile(program->GetOutputFile(), std::ios::binary);

		std::cout << vagFile->samples.size() << "\n";

		vagFile->CreateVagFile(outFile);

		outFile.close();

		return 0;
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << '\n';
		return -1;
	}
}