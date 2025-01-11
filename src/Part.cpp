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
    XMLElement* _measure = try_get_child( part, "measure" );
    XMLElement* _attributes = _measure ? try_get_child( _measure, "attributes" ) : nullptr;
    if (!_attributes) {
        return false;
    }

    // Get key and durations
    if (!parse_attributes( _attributes )) {
        return false;
    }

    // Construct encodings_ from each measure
    std::unique_ptr<Encoding> _token = std::make_unique<Marker>( Marker::MarkerType::SOC );
    push_encoding( _token );
    while (_measure) {
        if (!parse_measure( _measure )) {
            return false;
        }
        _token = std::make_unique<Marker>( Marker::MarkerType::EOM );
        push_encoding( _token );
        _measure = _measure->NextSiblingElement( "measure" );
    }

    _token = std::make_unique<Marker>( Marker::MarkerType::EOC );
    push_encoding( _token );
    return true;
}

bool Part::parse_attributes( tinyxml2::XMLElement* attributes ) {
    // sample xml:
    //
    //   <attributes>
    //     <divisions>2</divisions>
    //     <key>
    //       <fifths>0</fifths>
    //       <mode>major</mode>
    //     </key>
    //     <time symbol="common">
    //       <beats>4</beats>
    //       <beat-type>4</beat-type>
    //     </time>
    //     ...
    //   </attributes>

    // get all elements of interest
    XMLElement* _key = try_get_child( attributes, "key" );
    XMLElement* _fifths = _key ? try_get_child( _key, "fifths" ) : nullptr; 
    XMLElement* _mode = _fifths ? try_get_child( _key, "mode" ) : nullptr;
    XMLElement* _time = _mode ?try_get_child( attributes, "time" ) : nullptr;
    XMLElement* _beats = _time ? try_get_child(_time, "beats" ) : nullptr;
    XMLElement* _beatType = _beats ? try_get_child(_time, "beat-type" ) : nullptr;
    XMLElement* _divisions = _beatType ? try_get_child( attributes, "divisions" ) : nullptr;
    if (!_divisions) {
        return false;
    }

    // key and mode
    key_ = atoi( _fifths->GetText() );   
    mode_ = (strcmp( _mode->GetText(), "major" ) == 0) ? Mode::MAJOR : Mode::MINOR;

    // time signature
    beatsPerMeasure_ = atoi( _beats->GetText() );
    int _beatTypeValue = atoi( _beatType->GetText() );

    // number of divisions per quarter note
    int _divisionsValue = atoi( _divisions->GetText() );
    subBeatsPerBeat_ = _divisionsValue * 4 / _beatTypeValue;

    return true;
}

bool Part::parse_measure( tinyxml2::XMLElement* measure ) {
    XMLElement* _note = try_get_child( measure, "note" );
    while (_note) {
        std::unique_ptr<Encoding> _token = std::make_unique<Note>( _note  );       
        if (!_token->is_valid()) {
            std::cerr << "Unable to process " << partName_ << " for " <<  id_ << std::endl;
            return false;
        }

        push_encoding( _token );
        _note = _note->NextSiblingElement( "note" );
    }

    if (ticks_remaining() < 0) {
        auto& _lastToken = get_last_encoding();
        std::cerr << location_to_string( _lastToken.get() ) << ": Too many notes in " << partName_  << std::endl;
        return false;
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

 void Part::set_sub_beats( size_t subBeats ) {
    size_t _oldSubBeats{ subBeatsPerBeat_ };
    subBeatsPerBeat_ = subBeats;
    for (auto& _encoding : encodings_) {
        _encoding->set_duration( _encoding->get_duration() * subBeats / _oldSubBeats );
        _encoding->set_tick_number( (_encoding->get_tick_number() - 1) * subBeats / _oldSubBeats + 1 );
    }
 }

std::string Part::get_header() const {
    std::ostringstream _os;
    _os << SOH << ID << id_ 
        << DELIM << PART << partName_  
        << DELIM << KEY + key_to_string() 
        << DELIM << BEATS  << beatsPerMeasure_ 
        << DELIM << SUB_BEATS << subBeatsPerBeat_
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

std::string Part::location_to_string( const Encoding* encoding ) const {
    if (encoding)
    {
        std::ostringstream _os;

        // display BWV
        _os << "[" << id_ 

        // display measure number and beat number
        << ", m. " << encoding->get_measure_number() 
            << ", b. " << tick_to_beat( encoding->get_tick_number() );

        // display sub-beat number only if beat is sub-divided
        if (tick_to_sub_beat( encoding->get_tick_number() ) == 1 &&
                (encoding->is_marker() || encoding->get_duration() >=  subBeatsPerBeat_)) {
            _os << ']';
        } 
        else  {
            _os << '.' << tick_to_sub_beat( encoding->get_tick_number() ) << "]";
        }
        return _os.str();
    }
    else {
        return "[ null ]";
    }
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
    subBeatsPerBeat_ = stoi( find_header_value( header, SUB_BEATS ) );
    return import_key( find_header_value( header, KEY ) ); // updates key_ and mode_
}

bool Part::import_encodings( const std::string& line ) {
    std::istringstream _is{ line };
    std::string _token;
    while (_is >> _token) {
        std::unique_ptr<Encoding> _encoding = make_encoding( _token ); 
        push_encoding( _encoding );
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

std::unique_ptr<Encoding> Part::pop_encoding() {
    if (encodings_.empty()) {
        return nullptr;
    }
    else {
        auto first = std::move(encodings_.front());
        encodings_.erase(encodings_.begin());
        return first;
    }
}
void Part::push_encoding( std::unique_ptr<Encoding>& encoding ) {

    // if the encoding is EOM, bump measure number and reset tick
    if (encoding->is_EOM()) {
        if (ticks_remaining() > 0) {
            if (currentMeasure_ == 1) {
                currentMeasure_ = 0;    // first full measure will be measure 1
                handle_upbeat();
            }
            else {
                return; // ignore EOM for an incomplete measure after the first
            }
        }
        currentMeasure_++;
        nextTick_ = 1;
    }

    // save current location and bump tick
    encoding->set_location( currentMeasure_, nextTick_ );
    nextTick_ += encoding->get_duration();

    encodings_.push_back( std::move(encoding) );
}

int Part::ticks_remaining() const {
    int _ticksLeft = beatsPerMeasure_ * subBeatsPerBeat_;
    return _ticksLeft - nextTick_ + 1;
}

void Part::handle_upbeat() {
    for (auto& _encoding : encodings_) {
        _encoding->set_location( 0, _encoding->get_tick_number() + ticks_remaining() );
        // std::cout << "Readjusted encoding: " 
        //     <<  location_to_string( _encoding.get() )
        //     << ": " << _encoding->to_string() << std::endl;
    }
}   

