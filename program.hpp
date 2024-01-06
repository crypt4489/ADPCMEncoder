#pragma once

#include <cctype>
#include <filesystem>
#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

#include "file.h"
#include "wav.hpp"
#include "vag.hpp"
#include "convertpcm16.hpp"

class Program
{
public:
    
    enum FileType
    {
        UNKNOWNTYPE = 0,
        VAGTYPE = 1,
        WAVTYPE = 2
    };

    Program() = delete;
    Program(const Program&) = delete;
    Program(const Program&&) = delete;

    explicit Program(int argc, char **argv) :
    noisereduce(true),
    programtype(false),
    usehelp(false),
    type(UNKNOWNTYPE),
    filepathregex(new std::regex("[\\:A-Za-z0-9 _\\-/\\\\.]*\\.[A-Za-z0-9]+$"))
    {

        if (!ParseArguments(argc, argv))
        {
            PrintHelp();
            throw std::invalid_argument("Launching of program failed");
        }
    }

    ~Program() {
        delete filepathregex;
    }

    bool GetNoiseReduce() const { return noisereduce; }
    
    bool GetProgramType() const { return programtype; }
    
    FileType GetFileType() const { return type; }
    
    std::string GetFileName() const { return filename; }

    std::string GetOutputFile() const { 
        if (outputfile.empty())
            return filename + ".vag";
        return outputfile;
    }

    std::string GetFilePath() const { return filepath; }

    void Execute() {
        if (usehelp) 
        {
            PrintHelp();
            return;
        }

        std::vector<float> coef = {.15f, .15f, .15f, .15f};

        std::unique_ptr<File> file;
            
        switch(type)
        {
            case WAVTYPE:
            {
                file =  std::make_unique<WavFile>(GetFilePath()) ;

		        if (!file)
			        throw std::runtime_error("Cannot create WAV file");

                break;
            }
            default:
                throw std::runtime_error("Invalid file type");
        }

		std::cout << file->samplesSize << " " 
					<< file->channels << " "
					<< file->sampleRate << " " << file->bps << "\n";

        int16_t* convertedsamplesptr{};
        uint64_t outsize{};
        
        using ConversionType = std::variant<std::unique_ptr<ConvertPCM16<uint8_t>>, std::unique_ptr<ConvertPCM16<int16_t>>,
            std::unique_ptr<ConvertPCM16<PCM24>>, std::unique_ptr<ConvertPCM16<int32_t>>>;

        ConversionType conversion;

        switch(file->bps)
        {
            case 8:
            {
                conversion = std::make_unique<ConvertPCM16<uint8_t>>(GetNoiseReduce(), coef, file->samplesSize, file->samples);
                break;
            }
            case 16:
            {
                conversion = std::make_unique<ConvertPCM16<int16_t>>(GetNoiseReduce(), coef, file->samplesSize, file->samples);
                break;
            }
            case 24:
            {
                conversion = std::make_unique<ConvertPCM16<PCM24>>(GetNoiseReduce(), coef, file->samplesSize, file->samples);
                break;
            }
            case 32:
            {
                conversion = std::make_unique<ConvertPCM16<int32_t>>(GetNoiseReduce(), coef, file->samplesSize, file->samples);
                break;
            }
            default:
                throw std::runtime_error("Unhandled bit rate or sample data type");
        }

        std::tie(outsize, convertedsamplesptr) = std::visit([](auto& conv) {
            if (!conv)
                throw std::runtime_error("Conversion pointer not created");
            
            return std::make_tuple<uint64_t, int16_t*>(conv->GetOutSize(), conv->convert());
            
            }, conversion);
		
        std::unique_ptr<VagFile> vagFile(new (std::nothrow) VagFile(file->sampleRate, file->channels, GetOutputFile()));

        if (!vagFile)
            throw std::runtime_error("Cannot create vagfile object");
        
		vagFile->CreateVagSamples(convertedsamplesptr, outsize, 0, 0, false);

        std::cout << GetOutputFile() << std::endl;

		vagFile->WriteVagFile(GetOutputFile());
    }

private:
    bool noisereduce; //use fir = true, don't use = false
    bool programtype; //encode = false, decode = true
    bool usehelp; //passed help command
    std::string filepath;
    std::string filename;
    std::string outputfile;
    FileType type;
    std::vector<std::string> arguments; 
    std::regex *filepathregex;
    const std::unordered_map<std::string, FileType> filetypemap
    {
        { "wav", WAVTYPE }
    };

    bool ParseArguments(int argc, char **argv)
    {
        auto tolowercase = [](std::string in) {
            std::string res{};
            for (auto i : in)
                res.push_back(std::tolower(i));
            return res;
        };
        arguments.assign(&argv[1], argv+argc);
        for (auto it : arguments)
        {
            if (it.empty())
            {
                std::cerr << "Empty argument passed" << "\n";
                return false;
            }
            
            if (tolowercase(it) == "-d" || tolowercase(it) == "--decode")
                programtype = true;
            else if (tolowercase(it) == "-nf" || tolowercase(it) == "--nofir")
                noisereduce = false;
            else if (tolowercase(it) == "-h" || tolowercase(it) == "--help")
            {
                usehelp = true;
                return true;
            }
            else if (tolowercase(it.substr(0, 2)) == "-o" || tolowercase(it.substr(0, 8)) == "--output")
            {
                if (!ParseOutputFile(it))
                    return false;
            }
            else if (ParseInputFile(it))
                filepath = it;
            else
            {
                std::cerr << "Illegal argument passed " <<  it << "\n";
                return false;
            }
        }

        if (filename.empty() || filepath.empty() || type == UNKNOWNTYPE)
        {
            std::cerr << "No input file found" << "\n";
            return false;
        }

        return true;
    }

    bool ParseInputFile(std::string inputfile)
    {
        if (!std::regex_match(inputfile, *filepathregex))
        {
            return false;
        }

        std::filesystem::path path{inputfile};

        if (!path.has_parent_path())
            path = std::filesystem::current_path() / path;
        

        if (!std::filesystem::exists(path))
        {
            std::cerr << "File does not exist " << path.string() << "\n";
            return false;
        }

        std::regex regex("[./\\\\]");
        std::sregex_token_iterator first{inputfile.begin(), inputfile.end(), regex, -1}, last;//the '-1' is what makes the regex split (-1 := what was not matched)
        std::vector<std::string> tokens{first, last};
        
        auto size = tokens.size();

        filename =  tokens[size-2];
        
        auto typesearch = filetypemap.find(tokens[size-1]);

        if (typesearch == filetypemap.end())
        {
            std::cerr << "Unsupported file type extension" << "\n";
            return false;
        }

        type = typesearch->second;

        return true;
    }

    bool ParseOutputFile(std::string arg)
    {
        size_t split = arg.rfind("=");

        if (split == std::string::npos)
        {
            std::cerr << "Incorrect output argument format " << arg << " missing = delimiter\n";
            return false;
        }

        outputfile = arg.substr(split+1);
        
        if (!std::regex_match(outputfile, *filepathregex))
        {
            std::cerr << "Incorrect output file format " << outputfile << "\n";
            return false;
        } 
        else if (outputfile.substr(outputfile.size()-4) != ".vag")
        {
            std::cerr << "Incorrect extension, should be .vag, not " << arg.substr(arg.size()-4) << "\n";
            return false;
        }
        return true;
    }

    void PrintHelp()
    {
        std::cout << "\nADPCMEncoder - an application for Sony PS2 VAG file encoding/decoding\n\n";
        std::cout << "Usage: ADPCMEncoder [OPTIONS] [FILENAME]\n\n";
        std::cout << "Options:\n\n";
        std::cout << "-h, --help                    Use cmdline help\n\n"; 
        std::cout << "-d, --decode                  Decode a valid VAG file (encode is default)\n\n";  
        std::cout << "-nf, --no-fir                 Don't use FIR sampling for noise (FIR usage is default)\n\n";
        std::cout << "-o=[FILE], --output=[FILE]    Output file name (Input file name is default)\n\n";
        std::cout << "All Options are case insensitive for alpha characters\n\n"; 
        std::cout << "Filename:\n\n";
        std::cout << "ADPCMEncoder accepts WAV files\n\n";  
    }
};