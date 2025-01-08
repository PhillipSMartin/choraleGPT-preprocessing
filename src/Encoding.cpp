#include "Encoding.h"
#include "XmlUtils.h"

#include <map>
#include <sstream>

using namespace tinyxml2;

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

void Note::parse_pitch( const std::string& pitch ) {
    std::string _savePitch{ pitch }; 
    if (_savePitch[0] == '+') {
        tie_ = true;
        _savePitch = _savePitch.substr( 1 );
    }
    pitch_ = _savePitch[0];
    if (_savePitch.length() > 1) {
        accidental_ = std::stoi( _savePitch.substr( 1 ) );
    }
}

std::string Note::pitch_to_string() const {
    std::ostringstream _os;
    if ( tie_ ) {
        _os << "+";
    }
    _os << pitch_;
    if ( accidental_) {
        _os << accidental_;
    }
    _os << '.' << octave_; 

    return _os.str();
}  

bool Note::parse_xml( XMLElement* note )  {
    isValid_ = false;
    XMLElement* _pitch = note ? note->FirstChildElement( "pitch" ) : nullptr;
    if (_pitch) {
        XMLElement* _step = XmlUtils::try_get_child( _pitch, "step" );
        XMLElement* _alter = _pitch->FirstChildElement( "alter" );
        XMLElement* _octave = XmlUtils::try_get_child( _pitch, "octave" );
        if (_step && _octave) {
            pitch_ = _step->GetText()[0];
            accidental_ =  _alter ? std::stoi( _alter->GetText() ) : 0;
            octave_ = std::stoi( _octave->GetText() ); 
            isValid_ = true;   
        }
    }
    else {
        if (note->FirstChildElement( "rest" )) {
            isValid_ = true;
        }
    }

    if (isValid_) {
        Encoding::parse_xml( note ); // may change isValid to false
    }
    return isValid_;
}

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

std::string Chord::to_string() const {
    std::ostringstream _os;
    for (auto& note : notes_) {
        _os << note.pitch_to_string() << '.';
    }
    _os << duration_;
    return _os.str();
}

void Note::transpose( const std::map<char, TranspositionRule>& rules ) {
    auto rule = rules.find( pitch_ );
    if (rule != rules.end()) {
        pitch_ = rule->second.newPitch;
        octave_ += rule->second.octaveChange;
        accidental_ += rule->second.accidentalChange;
    }
}
