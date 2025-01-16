#include "Arguments.h"
#include "Chorale.h"
#include "Part.h"

#include <fstream>
#include <iostream>
#include <string>


/**
 * Reads a list of XML sources from a text file.
 *
 * @param xmlSource The path or URL to the text file containing the list of XML sources.
 * @param xmlSourceList The vector to store the read XML sources.
 * @return `true` if at least one XML source was read, `false` otherwise.
 */
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

/**
 * Retrieves a vector of XML sources based on the provided arguments.
 *
 * If the input source type is a text file, the function reads the list of XML sources from the file.
 * Otherwise, the function assumes the input source is a single file or URL and adds it to the vector.
 *
 * @param args The command-line arguments containing the input source information.
 * @return A vector of XML sources to be processed.
 */
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

/**
 * Prints the specified parts of a Chorale to the console.
 *
 * @param args The command-line arguments containing the parts to be printed.
 * @param chorale The Chorale object containing the parts to be printed.
 * @return `true` if the printing was successful, `false` otherwise.
 */
bool print_to_console( const Arguments& args, Chorale& chorale ) {
    // process each requested part
    for (std::string _partName : args.get_parts_to_parse() ) {
        if (auto& _part = chorale.get_part( _partName )) {
            std::cout << *_part << '\n';
        }
        else {
            std::cerr << "Part " << _partName << " not found for " << chorale.get_BWV() << std::endl;
            return 1;
        }
    }

    std::cout << std::endl;
    return true;
}

/**
 * Exports the specified parts of a Chorale to the provided output file.
 *
 * This function iterates through the parts to be exported, as specified in the provided Arguments object,
 * and writes each part to the output file. If a part is not found, an error message is printed to the
 * standard error stream.
 *
 * @param args The command-line arguments containing the parts to be exported.
 * @param chorale The Chorale object containing the parts to be exported.
 * @param outputFile The output file stream to write the parts to.
 * @return `true` if the export was successful, `false` otherwise.
 */
bool export_to_file( const Arguments& args, Chorale& chorale, std::ofstream& outputFile ) {
    // process each requested part
    for (std::string _partName : args.get_parts_to_parse() ) {
        if (auto& _part = chorale.get_part( _partName )) {
            outputFile << *_part;
        }
        else {
            std::cerr << "Part " << _partName << " not found for " << chorale.get_BWV() << std::endl;
            return 1;
        }
    }

    return true;
}

/**
 * The main entry point of the application. This function processes command-line arguments, reads and encodes
 *  MusicXML files, and either prints the encoded parts to the console or exports them to a file.
 *
 * @param argc The number of command-line arguments.
 * @param argv The array of command-line arguments.
 * @return 0 if the processing was successful, 1 otherwise.
 */
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
        unsigned int _successes{0};
        unsigned int _attempts{0};
        for (const std::string& _xmlSource : _xmlSources) {
            if (_xmlSource.empty() || _xmlSource.substr(0,2) == "//") {
                continue;
            }
            
            _attempts++;
            Chorale _chorale{ _xmlSource };

            // load xml for this chorale
            if (!_chorale.load_xml()) {
                std::cerr << "Failed to load xml source: " << _xmlSource << std::endl;
                continue;
            }

            // extract the parts and encode them
            _chorale.load_parts( _args.get_parts_to_parse() );
            if (!_chorale.encode_parts()) {
                std::cerr << "Failed to encode parts for " << _chorale.get_BWV() << std::endl;
                continue;
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

            _successes++;
            std::cout << "Encoded " << _chorale.get_BWV() << std::endl;
        }

        std::cout << "Successfully encoded " << _successes  
            << (_successes == 1 ? " chorale" : " chorales") << std::endl;
        if (_attempts > _successes) {
            std::cout << "Failed to encode " << _attempts - _successes 
                << ((_attempts - _successes) == 1 ? " chorale" : " chorales") << std::endl;
        }

        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}