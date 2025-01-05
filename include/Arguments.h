#pragma once
#include <args.hxx>

class Arguments {
    public:
        enum XmlSourceType {
            FILE,   // a music xml file on the local filesystem
            URL,    // a music xml file obtainable via a url
            TXT,     // a text file containing a list of music xml file names or urls
            UNKNOWN
        };

    private:
        std::string inputSource_; // the string passed by the positional argument inputSourceParm_
        std::string outputFile_; // the string passed by the optional argument outputFileParm_

        args::ArgumentParser parser_{"This program extracts parts from a music xml file", ""};
        args::HelpFlag help_{parser_, "help", "Display this help menu", {'h', "help"}};

        // Define positional argument
        args::Positional<std::string> inputSourceParm_{parser_, "source", "The file name or url to process"};

        // Define optional switches
        args::Flag soprano_{parser_, "Soprano", "Parse the soprano part", {'s', "soprano"}};
        args::Flag soprano1_{parser_, "Soprano 1", "Parse the soprano 1 part", {'1', "soprano1"}};
        args::Flag soprano2_{parser_, "Soprano 2", "Parse the soprano 2 part", {'2', "soprano2"}};
        args::Flag alto_{parser_, "Alto", "Parse the alto part", {'a', "alto"}};
        args::Flag tenor_{parser_, "Tenor", "Parse the tenor part", {'t', "tenor"}};
        args::Flag bass_{parser_, "Bass", "Parse the bass part", {'b', "bass"}};
        args::ValueFlag<std::string> outputFileParm_{parser_, "output", "Output file path", {'f', "file"}};

        // Store references to flags in vector
        std::vector<std::reference_wrapper<args::Flag>> flags_ { 
            soprano_, 
            soprano1_,
            soprano2_,
            alto_, 
            tenor_, 
            bass_ 
        };

        static std::string trim_leading_whitespace(const std::string& str) { 
             auto it = std::find_if( str.begin(), str.end(), [](char ch) { 
                return !std::isspace(static_cast<unsigned char>(ch)); 
            } ); 
            return std::string{it, str.end()};
        }

    public:
        Arguments() {}
        bool parse_command_line(int argc, char** argv);

        std::string get_input_source() const { return inputSource_; }
        static XmlSourceType get_input_source_type( const std::string& xmlSource ) {
            if (xmlSource.find("http://") == 0 || xmlSource.find("https://") == 0) {
                return Arguments::URL;
            }
            else if (xmlSource.find(".txt") != std::string::npos) {
                return Arguments::TXT;
            }
            else  if (xmlSource.find(".xml") != std::string::npos) {
                return Arguments::FILE;
            }
            else {
                return Arguments::UNKNOWN;
            }
        }
        std::vector<std::string> get_parts_to_parse() const;
        bool has_output_file() const { return outputFileParm_.Matched(); }
        std::string get_output_file() const { return outputFile_; };
};