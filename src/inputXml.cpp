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

bool print_to_console( const Arguments& args, Chorale& chorale ) {
    // process each requested part
    for (std::string _partName : args.get_parts_to_parse() ) {
        if (auto _part = chorale.get_part( _partName )) {
            std::cout << _part.value().get() << '\n';
        }
        else {
            std::cerr << "Part " << _partName << " not found for BWV " << chorale.get_BWV() << std::endl;
            return 1;
        }
    }

    std::cout << std::endl;
    return true;
}

bool export_to_file( const Arguments& args, Chorale& chorale, std::ofstream& outputFile ) {
    // process each requested part
    for (std::string _partName : args.get_parts_to_parse() ) {
        if (auto _part = chorale.get_part( _partName )) {
            outputFile << _part.value().get();
        }
        else {
            std::cerr << "Part " << _partName << " not found for BWV " << chorale.get_BWV() << std::endl;
            return 1;
        }
    }

    return true;
}

int main( int argc, char** argv ) { 
    try {
        Arguments _args;
        if (!_args.parse_command_line( argc, argv )) {
            return 1;
        }

        // open output file if we have one
        std::ofstream _outputFile{};
        if (_args.has_output_file()) {
            _outputFile.open( _args.get_output_file(), std::ios::out );
            if (!_outputFile) {
                std::cerr << "Failed to open output file: " << _args.get_output_file() << std::endl;
                return 1;
            }
        }

        // build list of musicXml files to read
        std::vector<std::string> _xmlSources = get_xml_sources( _args );
        if (_xmlSources.empty()) {
            std::cerr << "No musicXml sources to process" << std::endl;
            return 1;
        }

        // process each musicXml source in list
        for (const std::string& _xmlSource : _xmlSources) {
            Chorale _chorale{ _xmlSource };

            // load xml for this chorale
            if (!_chorale.load_xml()) {
                std::cerr << "Failed to load xml source: " << _xmlSource << std::endl;
                return 1;
            }

            // extract the parts and encode them
            _chorale.load_parts( _args.get_parts_to_parse() );
            if (!_chorale.encode_parts()) {
                std::cerr << "Failed to encode parts for BWV " << _chorale.get_BWV() << std::endl;
                return 1;
            }

            // print or save results
            if (_args.has_output_file()) {
                if (!export_to_file( _args, _chorale, _outputFile )) {
                    return 1;
                }
            }
            else {
                if (!print_to_console( _args, _chorale )) {
                    return 1;
                }
            }
        }

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}