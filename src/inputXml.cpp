#include "Arguments.h"
#include "Chorale.h"
#include "Part.h"

#include <fstream>
#include <iostream>
#include <string>

// no parts for BWVs 41.6 140.7 171.6 253.0

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

std::vector<std::string> get_xml_sources( const Arguments& args ) {
    std::vector<std::string> _xmlSources;
    switch (args.get_input_source_type( args.get_input_source() )) {

        // if xml source is a txt file it contains a list of files or urls
        case Arguments::TXT:
            read_xml_source_list( args.get_input_source(), _xmlSources );
            break;

        // otherwise, xml source is a single file or url
        default:
           _xmlSources.push_back(args.get_input_source());
            break;
    }
    return _xmlSources;
}

bool print_to_console( const Arguments& args, const std::vector<std::string>& xmlSources ) {
    // process each music xml source in list
    for (const std::string& _xmlSource : xmlSources) {
        Chorale _chorale{_xmlSource};
        if (!_chorale.load_xml()) {
            std::cerr << "Failed to load xml source: " << _xmlSource << std::endl;
            return 1;
        }

        if (_chorale.build_endcoded_parts( args.get_parts_to_parse() )) {
            for (std::string _part : args.get_parts_to_parse() ) {
                std::cout << _chorale.get_encoded_part( _part ) << "\n\n";
            }
        }
        else {
            std::cerr << "Failed to encode parts for BWV " << _chorale.get_BWV() << std::endl;
            return 1;
        } 
    }        

    std::cout << std::endl;
    return true;
}

bool export_to_file( const Arguments& args, const std::vector<std::string>& xmlSources ) {
    std::ofstream _outputFile{args.get_output_file(), std::ios::out};
    if (!_outputFile) {
        std::cerr << "Failed to open output file: " << args.get_output_file() << std::endl;
        return false;
    }

    // process each music xml source in list
    for (const std::string& _xmlSource : xmlSources) {
        Chorale _chorale{_xmlSource};
        if (!_chorale.load_xml()) {
            std::cerr << "Failed to load xml source: " << _xmlSource << std::endl;
            return false;
        }

        if (_chorale.build_endcoded_parts( args.get_parts_to_parse() )) {
            for (std::string _part : args.get_parts_to_parse() ) {
                _outputFile << _chorale.get_encoded_part( _part ) << '\n';
            }
        }
        else {
            std::cerr << "Failed to encode parts for BWV " << _chorale.get_BWV() << std::endl;
            return 1;
        } 
    }

    _outputFile.close();
    return true;
}

int main( int argc, char** argv ) { 
    Arguments args;
    if (!args.parse_command_line( argc, argv )) {
        return 1;
    }

    // build list of music xml files to read
    std::vector<std::string> _xmlSources = get_xml_sources( args );
    if (_xmlSources.empty()) {
        std::cerr << "No music xml sources to process" << std::endl;
        return 1;
    }

    if (args.has_output_file()) {
        if (!export_to_file( args, _xmlSources )) {
            return 1;
        }
    }
    else {
        if (!print_to_console( args, _xmlSources )) {
            return 1;
        }
    }

    return 0;
}