#include "Arguments.h"
#include "Chorale.h"
#include "Part.h"

#include <fstream>
#include <iostream>
#include <string>

bool read_xml_source_list( const std::string& xmlSource, std::vector<std::string>& xmlSourceList ) {
    std::ifstream _xmlSourceListFile{xmlSource};
    if (!_xmlSourceListFile) {
        std::cerr << "Error opening xml source list file: " << xmlSource << std::endl;
        return false;
    }

    for (std::string _xmlSource; std::getline(_xmlSourceListFile, _xmlSource); ) {
        if (!_xmlSource.empty()) {
            xmlSourceList.push_back(_xmlSource);
        }
    }
    return xmlSourceList.size() > 0;
}

int main( int argc, char** argv ) { 
    Arguments args;
    if (!args.parse_command_line(argc, argv)) {
        return 1;
    }

    std::ofstream _outputFile;
    if (args.has_output_file()) {
        _outputFile.open(args.get_output_file(), std::ios::out);
        if (!_outputFile) {
            std::cerr << "Failed to open output file: " << args.get_output_file() << std::endl;
            return 1;
        }
    }

    // build list of music xml files to read
    std::vector<std::string> _musicXmlSources;
    switch (args.get_xml_source_type( args.get_xml_source() )) {

        // if xml source is a txt file it contains a list of files or urls
        case Arguments::TXT:
            if (!read_xml_source_list( args.get_xml_source(), _musicXmlSources )) {
                return 1;
            }
            break;

        // otherwise, xml source is a single file or url
        default:
           _musicXmlSources.push_back(args.get_xml_source());
            break;
    }

    // process each music xml source in list
    for (const std::string& _musicXmlSource : _musicXmlSources) {

        // Initialze Chorale
        Chorale _chorale{_musicXmlSource};

        bool _loaded = false;
        switch (args.get_xml_source_type( _musicXmlSource )) {
            case Arguments::FILE:
                _loaded = _chorale.load_from_file( _musicXmlSource);
                break;
            case Arguments::URL:
                _loaded = _chorale.load_from_url( _musicXmlSource );
                break;
            default:
                std::cerr << "Invalid xml source type: " << _musicXmlSource << std::endl;
                break;
        }
        if (!_loaded) {
            return 1;
        }

        if (args.has_output_file()) {
            for (std::string _part : _chorale.encode_parts( args.get_parts_to_parse() ) ) {
                _outputFile << _part << '\n';
            }
        }

        else {
            std::cout << _chorale.get_title() << '\n' << std::endl;
            for (std::string _part : _chorale.encode_parts( args.get_parts_to_parse() ) ) {
                std::cout << _part << "\n\n";
            }      
        }
    }

    if (args.has_output_file()) {
        _outputFile.close();
    }
    else {
        std::cout << std::endl;
    }

    return 0;
}