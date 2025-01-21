#pragma once
#include "Part.h"

#include <array>
#include <iostream>

// conatins a copy of a given part to allow us to pop tokens and combine them with other parts
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
        static const int NUM_PARTS = 4;
 
        // the parts that make up this combined part: Soprano, Alto, Tenor, Bass
        std::array<std::unique_ptr<PartWrapper>, NUM_PARTS> parts_; 

    public:
        CombinedPart( const std::unique_ptr<Part>& soprano,
            const std::unique_ptr<Part>& alto,
            const std::unique_ptr<Part>& tenor,
            const std::unique_ptr<Part>& bass ) : Part(
                soprano->get_id(), soprano->get_title(), "Combined" ) {

            // make copies of the parts, so we can pop encodings as we process them
            parts_[0] = std::make_unique<PartWrapper>( *soprano );
            parts_[1] = std::make_unique<PartWrapper>( *alto );
            parts_[2] = std::make_unique<PartWrapper>( *tenor );
            parts_[3] = std::make_unique<PartWrapper>( *bass );

            beatsPerMeasure_ = soprano->get_beats_per_measure();
            subBeatsPerBeat_ = soprano->get_sub_beats();
            key_ = soprano->get_key();
            mode_ = soprano->get_mode();
        }

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