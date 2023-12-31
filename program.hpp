#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
class Program
{
public:
    enum FileType
    {
        UNKNOWNTYPE = 0,
        VAGTYPE = 1,
        WAVTYPE = 2
    };

    explicit Program(int argc, char **argv) :
    noisereduce(true),
    programtype(false),
    type(UNKNOWNTYPE)
    {
        if (!ParseArguments(argc, argv))
        {
            throw std::invalid_argument("Launching of program failed");
        }
    }

    bool GetNoiseReduce() const { return noisereduce; }
    bool GetProgramType() const { return programtype; }
    FileType GetFileType() const { return type; }
    std::string GetFileName() const { return filename; }
    std::string GetOutputFile() const { return filename + ".vag"; }
    std::string GetFilePath() const { return filepath; }
private:
    bool noisereduce; //use fir = true, don't use = false
    bool programtype; //encode = false, decode = true
    std::string filepath;
    std::string filename;
    FileType type;
    std::vector<std::string> arguments; 
    const std::unordered_map<std::string, FileType> filetypemap
    {
        { "wav", WAVTYPE }
    };

    bool ParseArguments(int argc, char **argv)
    {
        arguments.assign(&argv[1], argv+argc);
        for (std::vector<std::string>::iterator it = arguments.begin(); 
            it != arguments.end();
            it++)
        {
            if ((*it).empty())
            {
                std::cerr << "Empty argument passed" << "\n";
                return false;
            }
            
            if (*it == "-d")
            {
                programtype = true;
            } 
            else if (*it == "-nf")
            {
                noisereduce = false;
            }
            else if (ParseInputFile(*it))
            {
                filepath = *it;
            }
            else
            {
                std::cerr << "Illegal argument passed " <<  *it << "\n";
                return false;
            }
        }

        if (filename.empty() || filepath.empty())
        {
            std::cerr << "No input file found" << "\n";
            return false;
        }

        return true;
    }

    bool ParseInputFile(std::string inputfile)
    {
#ifdef _MSC_VER
        std::string splitter = "\\";
#else 
        std::string splitter = "/";
#endif
        size_t slashpos = inputfile.rfind(splitter);

        if (slashpos != std::string::npos)
        {
            inputfile = inputfile.substr(slashpos+1);
        }

        size_t dotpos = inputfile.rfind(".");

        if (dotpos == std::string::npos)
        {
            std::cerr << "No file extension found, cannot determine type" << "\n";
            return false;
        }

        std::string ext = inputfile.substr(dotpos+1);

        filename = inputfile.substr(0, dotpos);

        std::cout << filename << " " << ext << std::endl;

        auto keyIter = filetypemap.find(ext);

        if (keyIter != filetypemap.end())
        {
            type = keyIter->second;
            return true;
        } 

        std::cerr << "Unsupported file extension" << "\n";

        return false;
    }
};