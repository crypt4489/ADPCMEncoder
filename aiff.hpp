#include "file.hpp"
#include <string>
struct AIFFFile : public File
{
	AIFFFile() = delete;
	AIFFFile(std::string name)
	{
		
	}

	~AIFFFile() = default;
private:
	void LoadAIFFFile(std::string name)
	{
		auto filedata = LoadFile(name);
	}

};