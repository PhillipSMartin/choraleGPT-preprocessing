 #include "CombinedPart.h"


/**
 * Prints the current measure and the current tokens for each part in the CombinedPart.
 * @param os The output stream to write the information to.
 * @return The output stream after writing the information.
 */
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

/**
 * Retrieves the next set of tokens from the parts in the CombinedPart, ensuring that the tokens are compatible.
 * 
 * This function checks if any of the parts needs a new token, and if so, retrieves the next token for that part.
 * It then checks if all the tokens are compatible across the parts.
 * If any part is missing a token or has an incompatible token, an error message is printed to std::cerr and the 
 *  function returns false.
 * Otherwise, the function returns true, indicating that the next set of compatible tokens has been retrieved.
 *
 * @return true if the next set of compatible tokens has been retrieved, false otherwise.
 */
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

/**
 * Reduces the duration of a note by the specified amount.
 *
 * If the remaining duration is less than or equal to 0, the function returns true, indicating that a new note is needed.
 * Otherwise, the note's duration is updated and the function returns false, indicating that the note is tied to 
 *  the next one.
 *
 * @param note The note to have its duration reduced.
 * @param reduction The amount to reduce the note's duration by.
 * @return True if a new note is needed, false if the note is tied to the next one.
 */
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

/**
 * Processes a marker token, adding it to the encoding stack if it is not an EOM (End of Music) marker and the 
 *  noEOM flag is not set.
 *
 * @param token The marker token to process.
 * @param verbose If true, prints information about the added marker to the console.
 * @param noEOM If true, EOM markers will be skipped and not added to the encoding stack.
 * @return True if the processed marker is an EOC (End of Chord) marker, false otherwise.
 */
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

/**
 * Adds a chord to the combined parts by finding the shortest duration among the notes, reducing each note's duration
 * to match the shortest, creating a new Chord object with the reduced notes, and pushing it onto the encoding stack.
 * If the verbose flag is set, it will also print information about the added chord to the console.
 *
 * @param verbose If true, prints information about the added chord to the console.
 */
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


/**
 * Builds the combined parts by iteratively processing tokens from the individual parts. It adds markers to the 
 *  combined parts, unless the marker is an EOM (End of Music) and the noEOM flag is set. 
 *  If a Note or Rest is encountered, it builds a chord and adds it to the combined parts.
 *
 * @param verbose If true, prints information about the added markers and chords to the console.
 * @param noEOM If true, skips adding the EOM marker to the combined parts.
 * @return true if an EOC (End of Chord) marker is encountered, false otherwise.
 */
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