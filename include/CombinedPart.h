#pragma once
#include "Part.h"

#include <array>
#include <iostream>

// contains a copy of a given part to allow us to pop tokens and combine them with other parts
struct PartWrapper {
    std::unique_ptr<Part> part;
    std::unique_ptr<Encoding> currentToken;
    bool needNewToken;

    PartWrapper(Part& inputPart) : needNewToken(true) {
        part = std::make_unique<Part>(inputPart);
    }

    std::string part_name() const {
        return part->get_part_name();
    }
    bool get_next_token() {
        currentToken = part->pop_encoding();
        return currentToken != nullptr;
    }
};

class CombinedPart : public Part {
    private:
        // the parts that make up this combined part: Soprano, Alto, Tenor, Bass
        std::vector<std::unique_ptr<PartWrapper>> parts_; 

    public:
        CombinedPart( const std::vector<std::unique_ptr<Part>>& parts );

        bool build( bool verbose );

    private:
        std::ostream& show_current_tokens( std::ostream& os ) const;

        // get the next token for each part if one is needed
        // possible errors:
        //     a part has no more tokens
        //     one part has a marker whereas another part has a rest or note
        // a message is printed and the function returns false if an error occur
        bool get_next_tokens();

        // reduce the duration of a token by the given number of ticks
        // returns true if we thereby exhaust the duration of this token and will need a new one next time
        bool reduce_duration( Note& note, const unsigned int reduction );

        // add marker to the combined parts
        // takes ownership of the token
        // returns true if the marker was EOC
        bool process_marker( std::unique_ptr<Encoding>& token,  bool verbose );

        // build chord from the current tokens
        void add_chord( bool verbose );
};