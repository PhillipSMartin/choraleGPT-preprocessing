 #include "CombinedPart.h"


std::ostream& CombinedPart::show_current_tokens( std::ostream& os ) const {
    for (size_t i = 0; i < NUM_PARTS; ++i) {
        os << parts_[i]->get_part_name()  << ": ";
        if (currentTokens_[i]) {
            os << currentTokens_[i]->to_string();
        }
        else {
            os << "<NULL>"; 
        }
        os << std::endl;
    }

    return os;
}

bool CombinedPart::get_next_tokens() {
    std::string _missingTokenPart{};
    std::string _incompatibleTokenPart{};

    // pop next token if needed
    for (size_t _i = 0; _i < NUM_PARTS; ++_i) {
        if (needNewToken_[_i]) {
            if (currentTokens_[_i] = parts_[_i]->pop_encoding()) {
                // don't need a new token unless this is a marker
                needNewToken_[_i] = currentTokens_[_i]->is_marker();
            }
            else { 
                // if we have exhausted the tokens for this part, we have a problem
                _missingTokenPart = parts_[_i]->get_part_name();  
            }
        }
        // check if we have an inconsistency
        // equality means tokens are compatible (not identical)
        if (currentTokens_[_i] != currentTokens_[0]) {
            _incompatibleTokenPart = parts_[_i]->get_part_name();    
        }
    }

    // check for errors
    if (!_missingTokenPart.empty()) {
        std::cerr << "Error: " << _missingTokenPart << " has no more tokens" << std::endl;
        return false;
    }
    else if (!_incompatibleTokenPart.empty()) {
        std::cerr << "Error: " << _incompatibleTokenPart << " has incompatible token" << std::endl;
        return false;
    }

    return true;
}

void CombinedPart::reduce_duration(Note& note, const unsigned int reduction, bool& needNewToken) {
    // calcuate number of sub-beats left
    int _newDuration = note.get_pitch();
    _newDuration -= reduction;

    // if we are done with this note, get another next time
    if (_newDuration <= 0) {
        needNewToken = true;
    }
    // otherwise next note is tied to this one
    else {
        note.set_duration( _newDuration );
        note.set_tied( true );
    }
};

void CombinedPart::process_marker( const std::unique_ptr<Encoding>& token, bool verbose, bool noEOM  ) {
    if (token->is_EOM() && noEOM) {
        if (verbose) {
            std::cout << "Skipping EOM" << std::endl;
        }
    }
    else {
        push_encoding( currentTokens_[0] );
        if (verbose) {
            std::cout << "Added marker: " 
                << location_to_string( get_last_encoding().get() ) 
                << ": " << get_last_encoding()->to_string() << std::endl;  
        } 
    }  
}

void CombinedPart::add_chord( bool verbose ) {
    // collect the notes from each part and find the shortest duration
    Note _notes[NUM_PARTS];
    unsigned int _shortestDuration = currentTokens_[0]->get_duration();

    for ( size_t _i = 0; _i < NUM_PARTS; _i++ ) {
        _notes[_i] = *dynamic_cast<Note*>( currentTokens_[_i].get() );
        if (_notes[_i].get_duration() < _shortestDuration) {
            _shortestDuration = _notes[_i].get_duration();
        }
    } 

    // buile the chord and add it to the encoding stack
    auto chord = std::make_unique<Chord>( _notes, _shortestDuration );
    auto encoding = std::unique_ptr<Encoding>( chord.release() );
    push_encoding( encoding );

    // reduce durations
    for ( size_t _i = 0; _i < NUM_PARTS; _i++ ) {
        reduce_duration( _notes[_i], _shortestDuration, needNewToken_[_i] );
    }

    if (verbose) {
        std::cout << "Added chord " 
            << location_to_string( get_last_encoding().get() ) 
            << ":  " << get_last_encoding()->to_string() << std::endl;
    }
}


bool CombinedPart::build( bool verbose, bool noEOM ) {
    while (true) {
        // get new tokens if necessary
        if (!get_next_tokens()) {
            show_current_tokens( std::cerr );
            return false;
        }

        // if we have a Marker, add it to the combined parts unless it is EOM and noEOM was specified
        if (currentTokens_[0]->is_marker() ) {
            process_marker( currentTokens_[0], verbose, noEOM );

             // if it is EOC, we are done
            if (currentTokens_[0]->is_EOC()) {
                return true;
            }  
        }

        // if we have a Note or Rest, build a chord and add it to combined parts
        else {
            add_chord( verbose );
         }
    }
}