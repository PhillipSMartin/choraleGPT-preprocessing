#include "Part.h"
#include <iostream>

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
    line_.append( SOC ).append( " " );
    while (measure) {
        if (!parse_measure( measure )) {
            return false;
        }
        line_.append( EOM ).append( " " );
        measure = measure->NextSiblingElement( "measure" );
    }

    line_.append( EOC );
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
    XMLElement* pitch = note ? note->FirstChildElement( "pitch" ) : nullptr;
    if (pitch) {
        XMLElement* step = try_get_child( pitch, "step" );
        XMLElement* alter = pitch->FirstChildElement( "alter" );
        XMLElement* octave = try_get_child( pitch, "octave" );
        if (step && octave) {
            line_.append( step->GetText() );
            line_.append( octave->GetText() );
            line_.append( alter ? alter->GetText() : "" );
            line_.append( "." );
        }
    }
    else {
        pitch = note ? note->FirstChildElement( "rest" ) : nullptr;
        if (pitch) {
            line_.append( "R" );
        }
    }

    XMLElement* duration = try_get_child( note, "duration" );
    if (duration) {
        line_.append( duration->GetText() ).append( " " );
    }

    return pitch && duration;
}

std::ostream& operator <<( std::ostream& os, const Part& part)
{ 
    os << part.partName_ << ": " 
        << part.key_to_string() << " "
        << part.mode_to_string() << " - "
        << part.beatsPerMeasure_ << " beats per measure, ";

    if (part.divisionsPerBeat_ > 1) {
        os << part.divisionsPerBeat_ << " sub-divisions per beat\n";
    }
    else {
        os << "not sub-divided\n";
    }

    os << part.line_ << std::endl;
    return os;
}

