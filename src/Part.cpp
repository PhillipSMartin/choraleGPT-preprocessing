#include "Part.h"
#include <iostream>
#include <numeric>
#include <sstream>

using namespace tinyxml2;

bool Part::parse_musicXml( tinyxml2::XMLElement* part ) {
    if (!part) {
        std::cerr << "part element is null" << std::endl;
        return false;
    }

    // Get the first measure
    XMLElement* measure = try_get_child( part, "measure" );

    // Get key and durations
    if (!parse_attributes( try_get_child( measure, "attributes" ) )) {
        return false;
    }

    // Construct line_ from each measure
    line_.push_back( SOC );
    while (measure) {
        if (!parse_measure( measure )) {
            return false;
        }
        line_.push_back( EOM );
        measure = measure->NextSiblingElement( "measure" );
    }

    line_.push_back( EOC );
    return true;
}

bool Part::parse_attributes( tinyxml2::XMLElement* attributes ) {
    // // for debugging
    // XmlUtils::printElement( attributes );

    XMLElement* key = try_get_child( attributes, "key" );
    XMLElement* fifths = try_get_child( key, "fifths" ); 
    if (fifths) {
        key_ = atoi( fifths->GetText() );   
    }

    XMLElement* mode = try_get_child( key, "mode" );
    if (mode) {
        mode_ = (strcmp( mode->GetText(), "major" ) == 0) ? Mode::MAJOR : Mode::MINOR;
    }
    
    XMLElement* time = try_get_child( attributes, "time" );
    XMLElement* beats = try_get_child(time, "beats" );
    if (beats) {
        beatsPerMeasure_ = atoi( beats->GetText() );
    }

    XMLElement* divisions = try_get_child( attributes, "divisions" );
    if (divisions) {
        subBeats_ = atoi( divisions->GetText() );
    }

    return fifths && mode && beats && divisions;
}

bool Part::parse_measure( tinyxml2::XMLElement* measure ) {
    XMLElement* note = try_get_child( measure, "note" );
    while (note) {
        if (!parse_note( note ))
            return false;
        note = note->NextSiblingElement( "note" );
    }
    return true;
}

bool Part::parse_note( tinyxml2::XMLElement* note ) {
    std::string _word{};
    XMLElement* _pitch = note ? note->FirstChildElement( "pitch" ) : nullptr;
    if (_pitch) {
        XMLElement* step = try_get_child( _pitch, "step" );
        XMLElement* alter = _pitch->FirstChildElement( "alter" );
        XMLElement* octave = try_get_child( _pitch, "octave" );
        if (step && octave) {
            _word.append( step->GetText() );
            _word.append( alter ? alter->GetText() : "" );
            _word.append( "." );
            _word.append( octave->GetText() );
            _word.append( "." );       
        }
    }
    else {
        _pitch = note ? note->FirstChildElement( "rest" ) : nullptr;
        if (_pitch) {
            _word.append( "R.0." );
        }
    }

    XMLElement* _duration = try_get_child( note, "duration" );
    if (_duration) {
        int _beats = atoi( _duration->GetText() ) * MIN_SUBBEATS / subBeats_;
        _word.append( std::to_string( _beats ) );
    }

    line_.push_back( _word );
    return _pitch && _duration;
}

bool Part::transpose( int key ) {
    while (key != key_) {
        for ( std::string& _word : line_ ) {
            if (key > key_) {
              _word = transpose_up( _word ); 
            }
            else {
              _word = transpose_down( _word );
            }
        }
        key_ += (key > key_) ? 1 : -1;
    }
    return false;
}

std::string Part::transpose_note( const std::string& word, const std::map<char, TranspositionRule>& rules ) const {
    std::string _word{ word };
    char _pitch = _word[0];
    if (_pitch == 'R') {
        return _word;
    }

    std::size_t _dot1 = _word.find(".");
    if (_dot1 == std::string::npos) {
        return _word;
    }   
    std::size_t _dot2 = _word.find(".", _dot1 + 1);
    if (_dot2 == std::string::npos) {
        return _word;
    }  

    int _alter = (_dot1 > 1) ? std::stoi( _word.substr( 1, _dot1 ) ) : 0;
    int _octave = std::stoi( _word.substr( _dot1 + 1, _dot2 - _dot1 - 1 ) );
    int _duration = std::stoi( _word.substr( _dot2 + 1 ) );

    auto rule = rules.find( _pitch );
    if (rule != rules.end()) {
        _pitch = rule->second.newPitch;
        _octave += rule->second.octaveChange;
        _alter += rule->second.alterChange;
    }

    std::ostringstream _os;
    _os << _pitch;
    if (_alter) 
        _os << _alter;
    _os << "." << _octave << "." << _duration;
    return _os.str();
}

std::string Part::transpose_up( const std::string& word ) const {
    static const std::map<char, TranspositionRule> upRules = {
        {'C', {'G', -1, 0}},
        {'D', {'A', -1, 0}},
        {'E', {'B', -1, 0}},
        {'F', {'C', 0, 0}},
        {'G', {'D', 0, 0}},
        {'A', {'E', 0, 0}},
        {'B', {'F', 0, 1}}
    };
    return transpose_note(word, upRules);
}

std::string Part::transpose_down( const std::string& word ) const {
    static const std::map<char, TranspositionRule> downRules = {
        {'C', {'F', 0, 0}},
        {'D', {'G', 0, 0}},
        {'E', {'A', 0, 0}},
        {'F', {'B', 0, -1}},
        {'G', {'C', 1, 0}},
        {'A', {'D', 1, 0}},
        {'B', {'E', 1, 0}}
    };
    return transpose_note( word, downRules );
}

std::string Part::get_header() const {
    return SOH + ID + id_ +
        DELIM + PART + partName_ + 
        DELIM + KEY + key_to_string() + 
        DELIM + BEATS  + std::to_string( beatsPerMeasure_ ) +
        DELIM + SUB_BEATS + std::to_string( MIN_SUBBEATS ) +
        EOH;
}

std::string Part::to_string() const {
    std::ostringstream _os;
    _os << get_header();
    
    for (const auto& word : line_) {
        _os << " ";
        _os << word;
    }
    return _os.str();
}

std::ostream& operator <<( std::ostream& os, const Part& part) { 
    os << part.to_string() << std::endl;
    return os;
}

tinyxml2::XMLElement* Part::try_get_child( tinyxml2::XMLElement* parent, const char* childName ) {
    tinyxml2::XMLElement* _xmlElement = XmlUtils::try_get_child( parent, childName );
    if (!_xmlElement) { 
        std::cerr << "Unable to process " << partName_ << " for " <<  id_ << std::endl;
    }
    return _xmlElement;
}

bool Part::parse_encoding( const std::string& part ) {
    auto _it = part.find(EOH);
    if (_it == std::string::npos) {
        std::cerr << "No header found. Line = " << part;
        return false;
    }

    if (!import_header( part.substr( 0, _it + 1 ))) {
        return false;
    }
    return import_line( part.substr( _it + 1 ) );
}

bool Part::import_header( const std::string& header ) {
    auto _it = header.find( ID );
    if (_it == std::string::npos) {
        std::cerr << "No ID found. Header = " << header;
        return false;
    }
    int _cursor = _it + ID.length();
    auto _delim = header.find_first_of( ",]", _cursor );
    id_ = header.substr( _cursor, _delim - _cursor);

    return true;
}

bool Part::import_line( const std::string& line ) {
    std::istringstream _is{ line };
    std::string _word1, _word2;
    _is >> _word1 >> _word2;
    std::cout << "word1:" << _word1 << ", word2:" << _word2 << std::endl;
    return true;
}

