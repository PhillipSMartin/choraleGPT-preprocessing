#include "Arguments.h"
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

    std::string _line;
    std::string _currentBWV;
    std::getline( _partEncodings, _line );

    Part _part{};
    _part.parse_encoding( _line );

 
    _partEncodings.close();
    return 0;
}