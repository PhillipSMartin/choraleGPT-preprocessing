 #include "CombinedPart.h"


std::ostream& CombinedPart::show_current_tokens( std::ostream& os ) const {
    os << "Current measure: " << currentMeasure_  << "." <<  nextTick_ << std::endl;
    for (auto& _part : parts_) {
        os << _part->part_name()  << ": ";
        if (_part->currentToken) {
            os << _part->currentToken->to_string();
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
    Encoding* _sopranoToken = nullptr; // save soprano token for compatibility check

    for (auto& _part : parts_) {
        if (_part->needNewToken) {
            if (_part->get_next_token()) {
                // don't need a new token unless this is a marker
                _part->needNewToken = _part->currentToken->is_marker();
            }
            else { 
                // if we have exhausted the tokens for this part, we have a problem
                _missingTokenPart = _part->part_name();  
            }
        }
        // check if we have an inconsistency
        // equality means tokens are compatible (not identical)
        if (!_sopranoToken) {
            _sopranoToken = _part->currentToken.get();
        }
        else if (*_sopranoToken != *_part->currentToken) {
            _incompatibleTokenPart = _part->part_name();
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

bool CombinedPart::reduce_duration(Note& note, const unsigned int reduction) {
    // calcuate number of sub-beats left
    int _newDuration = note.get_duration();
    _newDuration -= reduction;

    // if we are done with this note, get another next time
    if (_newDuration <= 0) {
        return true;
    }
    // otherwise next note is tied to this one
    else {
        note.set_duration( _newDuration );
        note.set_tied( true );
        return false;
    }
};

bool CombinedPart::process_marker( std::unique_ptr<Encoding>& token, bool verbose, bool noEOM  ) {
    if (token->is_EOM() && noEOM) {
        if (verbose) {
            std::cout << "Skipping EOM" << std::endl;
        }
        return false;
    }
    else {
        push_encoding( token );
        if (verbose) {
            std::cout << "Added marker: " 
                << location_to_string( get_last_encoding().get() ) 
                << ": " << get_last_encoding()->to_string() << std::endl;  
        } 
        return get_last_encoding()->is_EOC();
    }  
}

void CombinedPart::add_chord( bool verbose ) {
    // find the shortest duration
    unsigned int _shortestDuration = UINT_MAX;
    for ( auto& _part : parts_ ) {
        _shortestDuration = std::min( _shortestDuration, _part->currentToken->get_duration() );
    } 

    // reduce each note's duration to match shortest and save it in a vector
    std::vector<Note> _notes;
    for ( auto& _part : parts_ ) {
        Note& _note = dynamic_cast<Note&>( *_part->currentToken );
        _notes.emplace_back( _note );
        _part->needNewToken = reduce_duration( _note, _shortestDuration );
    }
    
    // build the chord and add it to the encoding stack
    auto chord = std::make_unique<Chord>( _notes, _shortestDuration );
    auto encoding = std::unique_ptr<Encoding>( chord.release() );
    push_encoding( encoding );

    if (verbose) {
        std::cout << "Added chord " 
            << location_to_string( get_last_encoding().get() ) 
            << ":  " << get_last_encoding()->to_string() << std::endl;
    }
}


bool CombinedPart::build( bool verbose, bool noEOM )  {           

    while (true) {
        // get new tokens if necessary
        if (!get_next_tokens()) {

            // we encountered some inconcistency
            // print out the current tokens and return false
            show_current_tokens( std::cerr );
            return false;
        }

        // if we have a Marker, add it to the combined parts unless it is EOM and noEOM was specified
        if (parts_[0]->currentToken->is_marker() ) {
            if (process_marker( parts_[0]->currentToken, verbose, noEOM )) {
                // if it's an EOC, we are done
                return true;
            }  
        }

        // if we have a Note or Rest, build a chord and add it to combined parts
        else {
            add_chord( verbose );
         }
    }
}