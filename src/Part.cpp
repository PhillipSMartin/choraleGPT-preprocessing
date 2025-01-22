#include "Part.h"
#include <iostream>
#include <numeric>
#include <sstream>

using namespace tinyxml2;

/**
 * Parses the XML representation of a musical part and constructs the corresponding internal data structures.
 *
 * This function is responsible for parsing the XML element representing a musical part, extracting the key, 
 *  time signature, and other attributes, and then parsing each measure within the part to construct the 
 *  corresponding encoding data.
 *
 * @param part The XML element representing the musical part to be parsed.
 * @return true if the parsing was successful, false otherwise.
 */
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

/**
 * Parses the XML attributes element and extracts the key, time signature, and other musical properties.
 *
 * This function is responsible for parsing the XML element representing the attributes of a musical part,
 *  including the key (fifths and mode), time signature (beats per measure and beat type), and divisions
 *  per quarter note. It populates the corresponding member variables of the Part class.
 *
 * @param attributes The XML element containing the attributes for the musical part.
 * @return true if the parsing was successful, false otherwise.
 */
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

/**
 * This function is responsible for parsing the XML element representing a measure of a musical part.
 * It iterates through the notes in the measure, ignoring any notes that are part of a chord, and
 * creates a Note object for each valid note. The Note objects are then added to the encodings_
 * vector of the Part object.
 *
 * If any errors occur during the parsing process, such as an invalid note or too many notes in the
 * measure, the function will log an error message and return false.
 *
 * @param measure The XML element representing the measure to be parsed.
 * @return true if the parsing was successful, false otherwise.
 */
bool Part::parse_measure( tinyxml2::XMLElement* measure ) {
    XMLElement* _note = try_get_child( measure, "note" );
    while (_note) {

        // ignore note with a chord element
        if (try_get_child( _note, "chord", /*verbose - */ false )) {
            _note = _note->NextSiblingElement( "note" );
            continue;
        }

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

/**
 * Transposes the notes in the Part object to the specified key.
 *
 * This function iterates through all the encodings (notes) in the Part object and transposes
 * them up or down as necessary to reach the target key. It does this by calling the
 * transpose_up() or transpose_down() functions on each note, and then adjusting the key_
 * member variable one step closer to the target key.
 *
 * @param key The target key to transpose the Part to.
 * @return true if the transposition was successful, false otherwise.
 */
bool Part::transpose( int key ) {
    bool _byFifth = false; // first transposition is by a fourth

    while (key != key_) {
        for ( auto& _token : encodings_ ) {
            if (_token->is_note() && (key > key_)) { 
                transpose_up( static_cast<Note&>( *_token ), _byFifth ); 
            }
            else if (_token->is_note() && (key < key_)) {
                transpose_down( static_cast<Note&>( *_token ), _byFifth );
            }
        }

        // move one step closer to the target key
        if (key_ > key) {
            key_--;
        }
        else if (key_ < key) {
            key_++;
        }
        
        // alternate whether by fifth or by fourth to stay close to original key
        _byFifth = !_byFifth;
    }
    return true;
}

 /**
  * Sets the number of sub-beats per beat for the Part object.
  *
  * This function updates the `subBeatsPerBeat_` member variable and adjusts the duration and
  * tick number of all the encodings (notes) in the Part object to match the new sub-beat
  * count. This ensures that the timing and duration of the notes remain consistent when the
  * sub-beat count is changed.
  *
  * @param subBeats The new number of sub-beats per beat.
  */
 void Part::set_sub_beats( size_t subBeats ) {
    size_t _oldSubBeats{ subBeatsPerBeat_ };
    subBeatsPerBeat_ = subBeats;
    for (auto& _encoding : encodings_) {
        _encoding->set_duration( _encoding->get_duration() * subBeats / _oldSubBeats );
        _encoding->set_tick_number( (_encoding->get_tick_number() - 1) * subBeats / _oldSubBeats + 1 );
    }
 }

/**
 * Generates the header string for the Part object.
 * 
 * The header string includes the following information:
 * - ID: The unique identifier for the Part
 * - PART: The name of the Part
 * - KEY: The key of the Part
 * - BEATS: The number of beats per measure
 * - SUB_BEATS: The number of sub-beats per beat
 * 
 * This information is formatted and returned as a string.
 * 
 * @return The header string for the Part object.
 */
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

/**
 * Generates a string representation of the Part object, including its header information
 * and the string representations of all the encodings (notes) in the Part.
 *
 * The header information includes the Part's ID, name, key, beats per measure, and sub-beats
 * per beat. The encodings are separated by a space character.
 *
 * @return A string representation of the Part object.
 */
std::string Part::to_string() const {
    PartPrintOptions _opts;
    return to_string( _opts );
}

std::string Part::to_string( const PartPrintOptions& opts ) const {

    std::ostringstream _os;
    print_header( _os, opts );

    // keep track of position within the beat
    unsigned int _currentSubBeat = 0;
    unsigned int _nextSubBeat = 0;

    for (const auto& _encoding : encodings_) {
        if (_encoding->is_marker()) {
            print_marker( _os, opts, *_encoding );
        }
        else {
            _currentSubBeat = _nextSubBeat;
            _nextSubBeat = (_currentSubBeat + _encoding->get_duration()) % subBeatsPerBeat_;
            print_note( _os, opts, *_encoding, _currentSubBeat == 0, _nextSubBeat == 0 );
        }
    }

    return _os.str();
}


void Part::print_header( std::ostream& os, const PartPrintOptions& opts ) const {
    // don't print header unless requested
    if (opts.printHeader) {
        os << get_header() << " ";
    }
}

void Part::print_marker( std::ostream& os, const PartPrintOptions& opts, const Encoding& marker ) const {
    if (marker.is_EOM() && !opts.printEOM) {
        return;
    }

    if ((marker.is_SOC() || marker.is_EOC()) && opts.printEndTokensAsPeriod) {
        os << ".";
    }
    else {
        os << marker.to_string();
    }
    os << ( marker.is_EOC() ? "" : " " );
}

void Part::print_note( std::ostream& os, const PartPrintOptions& opts, const Encoding& note,
    bool startsBeat, bool endsBeat ) const {

    // if we are consolidating tokens withn the beat and have more coming, delimiter is a period
    char _delimiter = (opts.consolidateBeat && !endsBeat) ? '.' : ' ';

    // always print start of beat
    // ignore subsequent notes if requested
    if (startsBeat || !opts.printOnlyStartingTokenforEachBeat) {
        os << note.to_string( opts.printOnlyStartingTokenforEachBeat ) << _delimiter;
    }   
}




/**
 * Generates a string representation of the location of the given Encoding within the Part.
 * The location string includes the Part's ID, the measure number, and the beat number.
 * If the beat is subdivided, the sub-beat number is also included.
 *
 * @param encoding The Encoding object for which to generate the location string.
 * @return A string representation of the Encoding's location within the Part.
 */
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

/**
 * Attempts to retrieve a child XML element with the given name from the specified parent element.
 * If the child element is not found and the `verbose` flag is set, logs an error message.
 *
 * @param parent The parent XML element to search for the child.
 * @param childName The name of the child XML element to retrieve.
 * @param verbose Whether to log an error message if the child element is not found.
 * @return The child XML element if found, otherwise `nullptr`.
 */
XMLElement* Part::try_get_child( XMLElement* parent, const char* childName, bool verbose /* = true */ ) {
    XMLElement* _xmlElement = XmlUtils::try_get_child( parent, childName, verbose );
    if (verbose && !_xmlElement) { 
        std::cerr << "Unable to process " << partName_ << " for " <<  id_ << std::endl;
    }
    return _xmlElement;
}


/**
 * Parses a string representation of a part and extracts the header and encodings.
 *
 * This function first searches the input string for the end-of-header (EOH) marker. If the EOH marker is not found,
 * it logs an error message and returns `false`.
 *
 * If the EOH marker is found, the function calls `import_header()` to parse the header portion of the string, and
 * then calls `import_encodings()` to parse the encoding portion of the string.
 *
 * @param part The string representation of the part to parse.
 * @return `true` if the parsing was successful, `false` otherwise.
 */
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

/**
 * Parses a key string in the format "key-mode" and updates the `key_` and `mode_` member variables accordingly.
 *
 * The key string is expected to be in the format "key-mode", where "key" is a string representing the key 
 *  (e.g. "C", "G", "D") and "mode" is a string representing the mode (either "MAJOR" or "MINOR").
 *
 * This function first splits the input string on the "-" character to extract the key and mode. 
 *  It then looks up the key in the `circle_of_fifths_` array and sets the `key_` member variable based on the 
 *  index of the key in the array. If the mode is "MAJOR", the `mode_` member variable is set to `Mode::MAJOR`. 
 *  If the mode is "MINOR", the `mode_` member variable is set to `Mode::MINOR` and the `key_` member variable is 
 *  decremented by 3.
 *
 * @param keyString The key string in the format "key-mode".
 * @return `true` if the key string was successfully parsed, `false` otherwise.
 */
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

/**
 * Extracts the value of a specified key from a header string.
 *
 * This function searches the provided `header` string for the given `key`, and returns the value associated with 
 *  that key.
 * The key-value pairs in the header are expected to be delimited by the `DELIM` character, and the end of the 
 *  header is marked by the `EOH` string.
 *
 * If the key is not found in the header, the function prints an error message to `std::cerr` and returns an 
 *  empty string.
 *
 * @param header The header string to search.
 * @param key The key to search for in the header.
 * @return The value associated with the given key, or an empty string if the key is not found.
 */
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

/**
 * Imports header information from the provided string and updates the Part object's properties accordingly.
 *
 * This function extracts various values from the header string, such as the part ID, part name, beats per measure,
 * and sub-beats per beat. It then calls the `import_key()` function to update the key and mode properties based on
 * the key value found in the header.
 *
 * @param header The header string containing the part information.
 * @return `true` if the header information was successfully imported, `false` otherwise.
 */
bool Part::import_header( const std::string& header ) {
    id_ = find_header_value( header, ID );
    partName_ = find_header_value( header, PART );
    beatsPerMeasure_ = stoi( find_header_value( header, BEATS ) );
    subBeatsPerBeat_ = stoi( find_header_value( header, SUB_BEATS ) );
    return import_key( find_header_value( header, KEY ) ); // updates key_ and mode_
}

/**
 * Imports a series of encodings from the provided string and adds them to the Part object.
 *
 * This function takes a string containing one or more encoding values, separated by whitespace. It creates a
 * new `Encoding` object for each value using the `make_encoding()` function, and then adds each encoding to the
 * `encodings_` vector using the `push_encoding()` function.
 *
 * @param line The string containing the encoding values to import.
 * @return `true` if the encodings were successfully imported, `false` otherwise.
 */
bool Part::import_encodings( const std::string& line ) {
    std::istringstream _is{ line };
    std::string _token;
    while (_is >> _token) {
        std::unique_ptr<Encoding> _encoding = make_encoding( _token ); 
        push_encoding( _encoding );
    }
    return true;
}

/**
 * Creates a new `Encoding` object based on the provided encoding string.
 *
 * This function examines the encoding string and determines whether it represents a marker (such as start of
 * content, end of content, etc.) or a note. It then creates and returns the appropriate `Encoding` subclass
 * instance.
 *
 * @param encoding The string representation of the encoding to create.
 * @return A unique_ptr to the newly created `Encoding` object.
 */
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

/**
 * Removes and returns the first encoding from the Part's list of encodings.
 *
 * If the list of encodings is empty, this function will return a null pointer.
 * Otherwise, it will remove and return the first encoding in the list.
 *
 * @return A unique_ptr to the first encoding in the list, or nullptr if the list is empty.
 */
std::unique_ptr<Encoding> Part::pop_encoding() {
    if (encodings_.empty()) {
        return nullptr;
    }
    else {
        auto first = std::move( encodings_.front() );
        encodings_.erase(encodings_.begin());
        return first;
    }
}

/**
 * Adds an encoding to the part's list of encodings, updating the current measure and tick position as necessary.
 *
 * If the encoding is an "End of Measure" (EOM) marker, the function will increment the current measure number and
 * reset the next tick position to 1. It will also handle any upbeat logic if the current measure is the first full
 * measure.
 *
 * The function will then set the location of the encoding based on the current measure and tick position, and
 * update the next tick position by adding the duration of the encoding.
 *
 * Finally, the encoding is added to the list of encodings for the part.
 *
 * @param encoding A unique_ptr to the encoding to be added to the part.
 */
void Part::push_encoding( std::unique_ptr<Encoding>& encoding ) {

    // if the encoding is EOM, bump measure number and reset tick
    if (encoding->is_EOM()) {
        if (ticks_remaining() > 0) {
            if (currentMeasure_ == 1) {
                currentMeasure_ = 0;    // first full measure will be measure 1
                handle_upbeat();
            }
            // allow EOM for an incomplete measure after the first 
        }
        currentMeasure_++;
        nextTick_ = 1;
    }

    // save current location and bump tick
    encoding->set_location( currentMeasure_, nextTick_ );
    nextTick_ += encoding->get_duration();

    encodings_.push_back( std::move( encoding ) );
}

/**
 * Calculates the number of ticks remaining in the current measure.
 *
 * @return The number of ticks remaining in the current measure.
 */
int Part::ticks_remaining() const {
    int _ticksLeft = beatsPerMeasure_ * subBeatsPerBeat_;
    return _ticksLeft - nextTick_ + 1;
}

/**
 * Handles the upbeat logic for the part. This function iterates through all the encodings in the part and updates their
 * location to be at the beginning of the first full measure, with their tick number adjusted to account for the ticks
 * remaining in the upbeat measure.
 */
void Part::handle_upbeat() {
    for (auto& _encoding : encodings_) {
        _encoding->set_location( 0, _encoding->get_tick_number() + ticks_remaining() );
    }
}   

