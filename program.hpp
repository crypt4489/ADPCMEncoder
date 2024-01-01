#pragma once
#include <cctype>
#include <filesystem>
#include <iostream>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

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

    explicit Program(int argc, char **argv) :
    noisereduce(true),
    programtype(false),
    usehelp(false),
    type(UNKNOWNTYPE)
    {
        if (!ParseArguments(argc, argv))
        {
            PrintHelp();
            throw std::invalid_argument("Launching of program failed");
        }
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
    int Execute() {
        if (usehelp) {
            PrintHelp();
            return 0;
        }

        std::unique_ptr<WavFile> file = WavFile::LoadWavFile(GetFilePath());

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
		ConvertPCM16<int32_t>(GetNoiseReduce(), 
		coef, samplesSize, rawsamples));
		
		if (conversion == nullptr)
		{
			return -1;
		}

		std::unique_ptr<VagFile> vagFile(new VagFile(sampleRate, channels, GetOutputFile()));

		std::cout << conversion->GetOutSize() << std::endl;

		vagFile->CreateVagSamples(conversion->convert(), conversion->GetOutSize(), 0, 0, false);

		std::ofstream outFile(GetOutputFile(), std::ios::binary);

		std::cout << vagFile->samples.size() << "\n";

		vagFile->CreateVagFile(outFile);

		outFile.close();

        return 0;
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
    const std::unordered_map<std::string, FileType> filetypemap
    {
        { "wav", WAVTYPE }
    };

    bool ParseArguments(int argc, char **argv)
    {
        arguments.assign(&argv[1], argv+argc);
        std::transform(arguments.begin(), arguments.end(), arguments.begin(), [](std::string in) {
            std::string res;
            for (uint32_t i = 0; i<in.size(); i++)
                res.push_back(std::tolower(in[i]));
            return res;
         });
        for (auto it : arguments)
        {
            if (it.empty())
            {
                std::cerr << "Empty argument passed" << "\n";
                return false;
            }
            
            if (it == "-d" || it == "--decode")
                programtype = true;
            else if (it == "-nf" || it == "--nofir")
                noisereduce = false;
            else if (it == "-h" || it == "--help")
            {
                usehelp = true;
                return true;
            }
            else if (it == "-o" || it == "-output")
            {

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
        if (!std::regex_match(inputfile, std::regex("[A-Za-z0-9 _\\-/\\\\.]*\\.[A-Za-z0-9]+$")))
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

    void PrintHelp()
    {
        std::cout << "ADPCMEncoder - an application for Sony PS2 VAG file encoding/decoding\n\n";
        std::cout << "Usage: ADPCMEncoder [OPTIONS] [FILENAME]\n\n";
        std::cout << "Options:\n\n";
        std::cout << "-h, --help         Use cmdline help\n\n"; 
        std::cout << "-d, --decode       Decode a valid VAG file (encode is default)\n\n";  
        std::cout << "-nf, --no-fir      Don't use FIR sampling for noise (FIR usage is default)\n\n";
        std::cout << "Filename:\n\n";
        std::cout << "ADPCMEncoder accepts WAV files\n\n";  
    }
};