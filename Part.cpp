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
        divisionsPerBeat_ = atoi( divisions->GetText() );
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
    XMLElement* pitch = note ? note->FirstChildElement( "pitch" ) : nullptr;
    if (pitch) {
        XMLElement* step = try_get_child( pitch, "step" );
        XMLElement* alter = pitch->FirstChildElement( "alter" );
        XMLElement* octave = try_get_child( pitch, "octave" );
        if (step && octave) {
            _word.append( step->GetText() );
            _word.append( alter ? alter->GetText() : "" );
            _word.append( "." );
            _word.append( octave->GetText() );
            _word.append( "." );       
        }
    }
    else {
        pitch = note ? note->FirstChildElement( "rest" ) : nullptr;
        if (pitch) {
            _word.append( "R.0." );
        }
    }

    XMLElement* duration = try_get_child( note, "duration" );
    if (duration) {
        _word.append( duration->GetText() );
    }

    line_.push_back( _word );
    return pitch && duration;
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
    return ID + id_ +
        PART + partName_ + 
        KEY + key_to_string() + 
        BEATS  + std::to_string( beatsPerMeasure_ ) +
        SUB_BEATS + std::to_string( divisionsPerBeat_ ) +
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

