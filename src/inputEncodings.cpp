#include "Arguments.h"
#include "Chorale.h"
#include "Part.h"

#include <fstream>
#include <iostream>
#include <sstream>

/**
 * The main entry point of the application. This program reads the tokens generated by the inputXml program 
 *  and combines all the parts into a single stream of tokens, using chords instead of individual notes. 
 *  These tokens can then be used to train a transformer model.
 *
 * @param argc The number of command-line arguments, including the program name.
 * @param argv An array of C-style strings containing the command-line arguments.
 * @return 0 on successful completion, 1 on error.
 */
int main( int argc, char** argv )
{
    Arguments _args;
    if (!_args.parse_command_line( argc, argv )) {
        return 1;
    }
    PartPrintOptions _printOptions{ _args};

    // open output file if we have one
    std::ofstream _outputFile{};
    if (_args.has_output_file()) {
        _outputFile.open( _args.get_output_file(), std::ios::out );
        if (!_outputFile) {
            std::cerr << "Failed to open output file: " << _args.get_output_file() << std::endl;
            return 1;
        }
    }

    // read part encodings
    std::ifstream _partEncodings{_args.get_input_source()};
    if (!_partEncodings) {
        std::cerr << "Error opening input file: " << _args.get_input_source() << std::endl;
        return 1;
    }

    bool _done = false;
    std::string _line;
    std::vector<std::unique_ptr<Part>> _parts;

    unsigned int _successes{0};
    unsigned int _attempts{0};
    while (!_done) {
         _parts.clear();

        for (size_t _i = 0; _i < 4; _i++) {
            if (!std::getline(_partEncodings, _line)) {
                // if we haven't started a new chorale, we're done
                if ( _i == 0 ) {
                    _done = true;
                    break;
                }
                /// otherwise, we have a chorale with fewer than four voices
                else {
                    std::cerr << "Missing voices for chorale " << _parts.back()->get_id() << std::endl;
                    break;
                }
            }
            _parts.push_back( std::make_unique<Part>() );

            // build Part object from the input line
            _parts.back()->parse_encoding( _line );
        } 

        if (!_done) {
            _attempts++;

            // create a Chorale object from the parts
            Chorale _chorale{ "", _parts.back()->get_id()} ; 
            _chorale.load_parts( _parts );  

            // combine the parts into chords
            if (_chorale.combine_parts( _args.get_parts_to_parse(), _args.verbose() )) {

                // print the combined part to the output file
                if (_args.has_output_file()) {
                    if (auto& _part = _chorale.get_combined_part()) {
                        _outputFile << _part->to_string( _printOptions ) << std::endl;
                    }
                    else {
                        std::cerr << "Combined parts not found for " << _chorale.get_BWV() << std::endl;
                        return 1;
                    }
                }

                _successes++;
            }
            else {
                std::cerr << "Failed to process " << _chorale.get_BWV() << std::endl;
            }
        }
    }

    std::cout << "Successfully processed " << _successes  
        << (_successes == 1 ? " chorale" : " chorales") << std::endl;
    if (_attempts > _successes) {
        std::cout << "Failed to process " << _attempts - _successes 
            << ((_attempts - _successes) == 1 ? " chorale" : " chorales") << std::endl;
    } 

    _partEncodings.close();
    _outputFile.close();
    return 0;
}