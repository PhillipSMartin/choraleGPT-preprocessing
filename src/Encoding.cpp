#include "Encoding.h"
#include "XmlUtils.h"

#include <map>
#include <sstream>

using namespace tinyxml2;


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

Note::Note( const std::string& encoding ) {
    std::string _inputNote;
    std::string _inputOctave;
    std::string _inputDuration;
    std::istringstream _is{encoding}; 
    std::getline( _is, _inputNote, '.');  
    std::getline( _is, _inputOctave, '.');
    std::getline( _is, _inputDuration);

    pitch_ = _inputNote[0];
    octave_ = std::stoi( _inputOctave );
    duration_ = std::stoi( _inputDuration );
    if (_inputNote.length() > 1) {
        accidental_ = std::stoi( _inputNote.substr( 1 ) );
    }
}

std::string Note::pitch_to_string() const {
    std::ostringstream _os;
    _os << pitch_;
    if ( accidental_) {
        _os << accidental_;
    }
    _os << '.' << octave_; 

    return _os.str();
}  

bool Note::parse_xml( XMLElement* note )  {
    isValid = false;
    XMLElement* _pitch = note ? note->FirstChildElement( "pitch" ) : nullptr;
    if (_pitch) {
        XMLElement* _step = XmlUtils::try_get_child( _pitch, "step" );
        XMLElement* _alter = _pitch->FirstChildElement( "alter" );
        XMLElement* _octave = XmlUtils::try_get_child( _pitch, "octave" );
        if (_step && _octave) {
            pitch_ = _step->GetText()[0];
            accidental_ =  _alter ? std::stoi( _alter->GetText() ) : 0;
            octave_ = std::stoi( _octave->GetText() ); 
            isValid = true;   
        }
    }
    else {
        if (note->FirstChildElement( "rest" )) {
            isValid = true;
        }
    }

    if (isValid) {
        Encoding::parse_xml( note ); // may change isValid to false
    }
    return isValid;
}

bool Encoding::parse_xml( XMLElement* element ) {
    XMLElement* _duration = XmlUtils::try_get_child( element, "duration" );
    if (_duration) {
        duration_ = atoi( _duration->GetText() );
        isValid = true;
    }
    else {
        isValid = false;
    }
    return isValid;
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
