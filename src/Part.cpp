#include "Part.h"
#include <iostream>
#include <numeric>
#include <sstream>

using namespace tinyxml2;

bool Part::parse_xml( tinyxml2::XMLElement* part ) {
    if (!part) {
        std::cerr << "part element is null" << std::endl;
        return false;
    }

    // Get the first measure
    XMLElement* measure = try_get_child( part, "measure" );
    if (!measure) {
        return false;
    }

    // Get key and durations
    if (!parse_attributes( try_get_child( measure, "attributes" ) )) {
        return false;
    }

    // Construct line_ from each measure
    line_.emplace_back( Marker{Marker::MarkerType::SOC} );
    while (measure) {
        if (!parse_measure( measure )) {
            return false;
        }
        line_.emplace_back( Marker{Marker::MarkerType::EOM} );
        measure = measure->NextSiblingElement( "measure" );
    }

    line_.emplace_back( Marker{Marker::MarkerType::EOC} );
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
    XMLElement* _note = try_get_child( measure, "note" );
    while (_note) {
        line_.emplace_back( Note{ _note } );
        if (!line_.back().is_valid()) {
            std::cerr << "Unable to process " << partName_ << " for " <<  id_ << std::endl;
            return false;
        }
        _note = _note->NextSiblingElement( "note" );
    }
    return true;
}

bool Part::transpose( int key ) {
    while (key != key_) {
        for ( Encoding _token : line_ ) {
            if (_token.is_note() && (key > key_)) { 
                transpose_up( static_cast<Note&>( _token ) ); 
            }
            else if (_token.is_note() && (key < key_)) {
                transpose_down( static_cast<Note&>( _token ) );
            }
        }

        // move one step closer to the target key
        if (key_ > key) {
            key_--;
        }
        else if (key_ < key) {
            key_++;
        }
    }
    return true;
}

 void Part::set_sub_beats( unsigned int subBeats ) {
    unsigned int _oldSubBeats{ subBeats };
    for (Encoding& _encoding : line_) {
        _encoding.set_duration( _encoding.get_duration() * subBeats / _oldSubBeats );
    }
 }

std::string Part::get_header() const {
    std::ostringstream _os;
    _os << SOH << ID << id_ 
        << DELIM << PART << partName_  
        << DELIM << KEY + key_to_string() 
        << DELIM << BEATS  << beatsPerMeasure_ 
        << DELIM << SUB_BEATS << subBeats_
        << EOH;
    return _os.str();
}

std::string Part::to_string() const {
    std::ostringstream _os;
    _os << get_header();
    
    for (const auto& _encoding : line_) {
        _os << " ";
        _os << _encoding;
    }
    return _os.str();
}

std::ostream& operator <<( std::ostream& os, const Part& part) { 
    os << part.to_string() << std::endl;
    return os;
}

XMLElement* Part::try_get_child( XMLElement* parent, const char* childName ) {
    XMLElement* _xmlElement = XmlUtils::try_get_child( parent, childName );
    if (!_xmlElement) { 
        std::cerr << "Unable to process " << partName_ << " for " <<  id_ << std::endl;
    }
    return _xmlElement;
}

bool Part::parse_encoding( const std::string& part ) {
    auto _it = part.find(']');
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
    // auto _it = header.find( ID );
    // if (_it == std::string::npos) {
    //     std::cerr << "No ID found. Header = " << header;
    //     return false;
    // }
    // int _cursor = _it + ID.length();
    // auto _delim = header.find_first_of( ",]", _cursor );
    // id_ = header.substr( _cursor, _delim - _cursor);

    return true;
}

bool Part::import_line( const std::string& line ) {
    std::istringstream _is{ line };
    std::string _word1, _word2;
    _is >> _word1 >> _word2;
    std::cout << "word1:" << _word1 << ", word2:" << _word2 << std::endl;
    return true;
}

