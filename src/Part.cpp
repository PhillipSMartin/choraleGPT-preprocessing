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

    // Construct encodings_ from each measure
    encodings_.emplace_back( std::make_unique<Marker>( Marker::MarkerType::SOC ) );
    while (measure) {
        if (!parse_measure( measure )) {
            return false;
        }
        encodings_.emplace_back( std::make_unique<Marker>( Marker::MarkerType::EOM ) );
        measure = measure->NextSiblingElement( "measure" );
    }

    encodings_.emplace_back(std::make_unique<Marker>( Marker::MarkerType::EOC ) );
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
        encodings_.emplace_back( std::make_unique<Note>( _note  ) );
        if (!encodings_.back()->is_valid()) {
            std::cerr << "Unable to process " << partName_ << " for " <<  id_ << std::endl;
            return false;
        }
        _note = _note->NextSiblingElement( "note" );
    }
    return true;
}

bool Part::transpose( int key ) {
    while (key != key_) {
        for ( auto& _token : encodings_ ) {
            if (_token->is_note() && (key > key_)) { 
                transpose_up( static_cast<Note&>( *_token ) ); 
            }
            else if (_token->is_note() && (key < key_)) {
                transpose_down( static_cast<Note&>( *_token ) );
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
    unsigned int _oldSubBeats{ subBeats_ };
    subBeats_ = subBeats;
    for (auto& _encoding : encodings_) {
        _encoding->set_duration( _encoding->get_duration() * subBeats / _oldSubBeats );
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
    
    for (const auto& _encoding : encodings_) {
        _os << " ";
        _os << _encoding->to_string();
    }
    return _os.str();
}

std::ostream& operator <<( std::ostream& os, const Part& part) { 
    os << part.to_string() << '\n';
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
    auto _it = part.find(EOH);
    if (_it == std::string::npos) {
        std::cerr << "No header found. Line = " << part;
        return false;
    }

    if (!import_header( part.substr( 0, _it + 1 ))) {
        return false;
    }
    return import_encodings( part.substr( _it + 1 ) );
}

bool Part::import_key( const std::string& keyString )  {
    std::string _inputKey; 
    std::string _inputMode;
    std::istringstream _is{keyString}; 
    std::getline( _is, _inputKey, '-' );  
    std::getline( _is, _inputMode );

    auto it = std::find(std::begin(circle_of_fifths_), std::end(circle_of_fifths_), _inputKey);
    if (it != std::end(circle_of_fifths_)) {
        key_ = std::distance(std::begin(circle_of_fifths_), it) - index_of_C();
    }

    if (_inputMode == MAJOR_STR) {
        mode_ = Mode::MAJOR;
    }
    else if (_inputMode == MINOR_STR) {
        mode_ = Mode::MINOR;
        key_ -= 3;
    }
    else {
        return false;
    }

    return true;
}

std::string Part::find_header_value( const std::string& header, const std::string& key ) const {
    auto _it = header.find( key );
    if (_it == std::string::npos) {
        std::cerr << "No " << key << " found in header: " << header;
        return "";
    }

    size_t _cursor = _it + key.length();
    auto _delim = header.find(DELIM, _cursor);
    if (_delim == std::string::npos) {
        _delim = header.find(EOH, _cursor); // guaranteed to be there by caller
    }

    return header.substr( _cursor, _delim - _cursor );
}

bool Part::import_header( const std::string& header ) {
    id_ = find_header_value( header, ID );
    partName_ = find_header_value( header, PART );
    beatsPerMeasure_ = stoi( find_header_value( header, BEATS ) );
    subBeats_ = stoi( find_header_value( header, SUB_BEATS ) );
    return import_key( find_header_value( header, KEY ) ); // updates key_ and mode_
}

bool Part::import_encodings( const std::string& line ) {
    std::istringstream _is{ line };
    std::string _token;
    while (_is >> _token) {
        encodings_.emplace_back( make_encoding( _token ) );
    }
    return true;
}

std::unique_ptr<Encoding> Part::make_encoding( const std::string& encoding ) const {
    if (encoding == Marker::SOC_STR) {   
        return std::make_unique<Marker>( Marker::MarkerType::SOC );
    }
    else if (encoding == Marker::EOC_STR) {
        return std::make_unique<Marker>( Marker::MarkerType::EOC );
    }
    else if (encoding == Marker::EOP_STR) {
        return std::make_unique<Marker>( Marker::MarkerType::EOP );
    }
    else if (encoding == Marker::EOM_STR) {
        return std::make_unique<Marker>( Marker::MarkerType::EOM );
    }

    // if it's not a marker, it must be a note
    return std::make_unique<Note>( encoding );
}

