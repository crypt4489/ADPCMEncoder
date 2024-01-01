#include "program.hpp"
/*
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
*/
int main(int argc, char **argv)
{
	
	try
	{
		Program *program = new Program(argc, argv);
		program->Execute();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << '\n';
		return -1;
	}
}