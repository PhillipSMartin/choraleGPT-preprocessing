#include "Encoding.h"
#include "XmlUtils.h"

#include <map>
#include <sstream>

using namespace tinyxml2;

bool Encoding::is_SOC() const {
    if ( is_marker() ) {
        return dynamic_cast<const Marker&>(*this).get_marker_type() == Marker::MarkerType::SOC;
    }
    return false;
}

bool Encoding::is_EOC() const {
    if ( is_marker() ) {
        return dynamic_cast<const Marker&>(*this).get_marker_type() == Marker::MarkerType::EOC;
    }
    return false;
}

bool Encoding::is_EOM() const {
    if ( is_marker() ) {
        return dynamic_cast<const Marker&>(*this).get_marker_type() == Marker::MarkerType::EOM;
    }
    return false;
}


bool Encoding::is_EOP() const {
    if ( is_marker() ) {
        return dynamic_cast<const Marker&>(*this).get_marker_type() == Marker::MarkerType::EOP;
    }
    return false;
}

/**
 * Compares two Encoding objects for equality.
 * If both objects are of type Marker, the comparison is delegated to the Marker::operator== method.
 * Otherwise, the comparison is based on the tokenType_ member.
 */
bool Encoding::operator==( const Encoding& other ) const {
    if ((tokenType_ == MARKER) && (other.tokenType_ == MARKER)) {
        return dynamic_cast<const Marker&>( *this ).operator==( 
            dynamic_cast<const Marker&>( other ) );
    }
    else {
        return tokenType_ == other.tokenType_;
    }
}

std::string Marker::to_string() const {
    switch (markerType_) {
        case MarkerType::SOC:
            return SOC_STR;
        case MarkerType::EOM:
            return EOM_STR;
        case MarkerType::EOP:
            return EOP_STR;
        case MarkerType::EOC:
            return EOC_STR;
        default:
            return UNK_STR;
    }
}

/**
 * Parses the encoding string and updates the note's pitch, octave, and duration.
 * The encoding string is expected to be in the format "pitch.octave.duration".
 * This method is an implementation detail of the Note class.
 */
void Note::parse_encoding( const std::string& encoding ) {
    std::istringstream _is{encoding}; 

    std::string _inputPitch;
    std::string _inputOctave;
    std::string _inputDuration;
    std::getline( _is, _inputPitch, '.');  
    std::getline( _is, _inputOctave, '.');  
    std::getline( _is, _inputDuration);

    parse_pitch( _inputPitch );
    octave_ = std::stoi( _inputOctave );
    duration_ = std::stoi( _inputDuration );
}

/**
 * Parses the pitch string and updates the note's pitch and accidental properties.
 * The pitch string can optionally start with a '+' character to indicate the note is tied.
 * The pitch character is stored in the `pitch_` member, and the accidental value (if present) is stored in 
 *  the `accidental_` member.
 * This method is an implementation detail of the Note class.
 */
void Note::parse_pitch( const std::string& pitch ) {
    std::string _savePitch{ pitch }; 
    if (_savePitch[0] == '+') {
        tied_ = true;
        _savePitch = _savePitch.substr( 1 );
    }
    pitch_ = _savePitch[0];
    if (_savePitch.length() > 1) {
        accidental_ = std::stoi( _savePitch.substr( 1 ) );
    }
}

/**
 * Converts the note's pitch, accidental, and octave properties into a string representation.
 * The string will start with a '+' character if the note is tied, followed by the pitch character,
 * an optional accidental value, and the octave number separated by a period.
 * This method is an implementation detail of the Note class.
 */
std::string Note::pitch_to_string() const {
    std::ostringstream _os;
    if ( tied_ ) {
        _os << "+";
    }
    _os << pitch_;
    if ( accidental_) {
        _os << accidental_;
    }
    _os << '.' << octave_; 

    return _os.str();
}  

/**
 * Parses an XML note element and updates the note's properties accordingly.
 * This method is an implementation detail of the Note class.
 *
 * The method first checks if the note element has a "pitch" child element. If so, it extracts the
 * pitch, accidental, and octave information from the child elements and updates the corresponding
 * member variables of the Note object.
 *
 * The method also handles the case of a tied note by checking the "tie" child element and updating
 * the `tied_` and `tie_started_` member variables accordingly.
 *
 * If the note element has a "rest" child element, the method sets the `isValid_` member variable to
 * true, indicating that the note is a valid rest.
 *
 * Finally, the method calls the `Encoding::parse_xml()` function to extract the duration of the note,
 * which may in turn set the `isValid_` member variable to false if the duration is invalid.
 *
 * @param note The XML element representing the note to be parsed.
 * @return True if the note is valid, false otherwise.
 */
bool Note::parse_xml( XMLElement* note )  {
    isValid_ = false;
    XMLElement* _pitch = note ? note->FirstChildElement( "pitch" ) : nullptr;
    if (_pitch) {

        // get elements
        XMLElement* _step = XmlUtils::try_get_child( _pitch, "step" );
        XMLElement* _alter = _pitch->FirstChildElement( "alter" );
        XMLElement* _octave = XmlUtils::try_get_child( _pitch, "octave" );
        XMLElement* _tie = XmlUtils::try_get_child( note, "tie", /* verbose= */ false );

        if (_step && _octave) {
            pitch_ = _step->GetText()[0];
            accidental_ =  _alter ? std::stoi( _alter->GetText() ) : 0;
            octave_ = std::stoi( _octave->GetText() ); 
            isValid_ = true;   
        }

        // handle ties
        if (tie_started_) {
            tied_ = true;
        }
        
        if (_tie) {
            const char* _type = _tie->Attribute( "type" );
            if (_type && strcmp( _type, "start" ) == 0) {
                tie_started_ = true;
            }
            else if (_type && strcmp( _type, "stop" ) == 0) {
                tie_started_ = false;
            }
        }
    }
    else {
        if (note->FirstChildElement( "rest" )) {
            isValid_ = true;
        }
    }

    if (isValid_) {
        // get duration
        Encoding::parse_xml( note ); // may change isValid to false
    }
    return isValid_;
}

/**
 * Parses the XML element and extracts the duration value.
 *
 * @param element The XML element to be parsed.
 * @return True if the duration was successfully parsed, false otherwise.
 */
bool Encoding::parse_xml( XMLElement* element ) {
    XMLElement* _duration = XmlUtils::try_get_child( element, "duration" );
    if (_duration) {
        duration_ = atoi( _duration->GetText() );
        isValid_ = true;
    }
    else {
        isValid_ = false;
    }
    return isValid_;
}   

/**
 * Converts the Chord object to a string representation.
 * The string representation includes the pitch of each note in the chord,
 * separated by a period, followed by the duration of the chord.
 * 
 * @return A string representation of the Chord object.
 */
std::string Chord::to_string() const {
    std::ostringstream _os;
    for (auto& note : notes_) {
        _os << note.pitch_to_string() << '.';
    }
    _os << duration_;
    return _os.str();
}

/**
 * Transposes the pitch, octave, and accidental of the Note based on the provided transposition rules.
 *
 * @param rules A map of transposition rules, where the key is the current pitch and the value is a 
 *  TranspositionRule struct containing the new pitch, octave change, and accidental change.
 */
void Note::transpose( const std::map<char, TranspositionRule>& rules ) {
    auto rule = rules.find( pitch_ );
    if (rule != rules.end()) {
        pitch_ = rule->second.newPitch;
        octave_ += rule->second.octaveChange;
        accidental_ += rule->second.accidentalChange;
    }
}
