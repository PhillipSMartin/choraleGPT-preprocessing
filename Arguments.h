#pragma once
#include <args.hxx>

class Arguments {
    public:
        enum XmlSourceType {
            FILE,
            URL
        };

    private:
        args::ArgumentParser parser_{"This program extracts parts from a music xml file", ""};
        args::HelpFlag help_{parser_, "help", "Display this help menu", {'h', "help"}};

        // Define positional argument
        args::Positional<std::string> xmlSource_{parser_, "source", "The music xml file name or url to process"};

        // Define optional switches
        args::Flag soprano_{parser_, "Soprano", "Parse the soprano part", {'s', "soprano"}};
        args::Flag alto_{parser_, "Alto", "Parse the alto part", {'a', "alto"}};
        args::Flag tenor_{parser_, "Tenor", "Parse the tenor part", {'t', "tenor"}};
        args::Flag bass_{parser_, "Bass", "Parse the bass part", {'b', "bass"}};

        // Store references to flags in vector
        std::vector<std::reference_wrapper<args::Flag>> flags_ { 
            soprano_, 
            alto_, 
            tenor_, 
            bass_ 
        };

    public:
        Arguments() {}
        bool parse_command_line(int argc, char** argv);

        const char* get_xml_source() { return args::get( xmlSource_ ).c_str(); }
        XmlSourceType get_xml_source_type() {
            return (args::get( xmlSource_ ).find("http") == 0) ? URL : FILE;
        }
        std::vector<std::string> get_parts_to_parse();
};