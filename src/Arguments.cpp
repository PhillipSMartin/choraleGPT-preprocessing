#include "Arguments.h"

bool Arguments::parse_command_line(int argc, char** argv)
{
    try {
        parser_.ParseCLI(argc, argv);
        inputSource_ = args::get( inputSourceParm_ );
        outputFile_ = trim_leading_whitespace( args::get( outputFileParm_ ) );
    } 
    catch (args::Help&) {
        std::cout << parser_;
        return false;
    } 
    catch (args::ParseError& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser_;
        return false;
    } 
    catch (args::ValidationError& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser_;
        return false;
    }

    return true;
}

std::vector<std::string> Arguments::get_parts_to_parse() const
{
    // Build vector of selected parts
    std::vector<std::string> _selectedParts;
    for (const auto& _flag : flags_) {
        if (_flag.get())
            _selectedParts.push_back( _flag.get().Name() );
    }
    return _selectedParts;
}