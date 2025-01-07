#include "Arguments.h"
#include "Chorale.h"
#include "Part.h"

#include <fstream>
#include <iostream>
#include <sstream>

int main( int argc, char** argv )
{
    Arguments _args;
    if (!_args.parse_command_line( argc, argv )) {
        return 1;
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
            _parts.back()->parse_encoding( _line );
        } 

        if (!_done) {
            Chorale _chorale{ "", _parts.back()->get_id()} ; 
            _chorale.load_parts( _parts );   

            for (std::string _partName : _args.get_parts_to_parse() ) {
                if (auto& _part = _chorale.get_part( _partName )) {
                    std::cout << *_part << '\n';
                }
                else {
                    std::cerr << "Part " << _partName << " not found for " << _chorale.get_BWV() << std::endl;
                    return 1;
                }
            }
        }
    }

    _partEncodings.close();
    return 0;
}