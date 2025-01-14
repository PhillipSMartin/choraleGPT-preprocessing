#include "Part.h"

#include <iostream>

class CombinedPart : public Part {
    private:
        static const int NUM_PARTS = 4;
 
        // the parts that make up this combined part: Soprano, Alto, Tenor, Bass
        std::unique_ptr<Part> parts_[NUM_PARTS]; 

        // the tokens currently being processed for each part
        std::unique_ptr<Encoding> currentTokens_[NUM_PARTS]; 

        // bool to indicate if we need a new token for each part
        bool needNewToken_[NUM_PARTS] = { true };

    public:
        CombinedPart( const std::unique_ptr<Part>& soprano,
            const std::unique_ptr<Part>& alto,
            const std::unique_ptr<Part>& tenor,
            const std::unique_ptr<Part>& bass ) : Part(
                soprano->get_id(), soprano->get_title(), "Combined" ) {

            // make copies of the parts, so we can pop encodings as we process them
            parts_[0] = std::make_unique<Part>( *soprano );
            parts_[1] = std::make_unique<Part>( *alto );
            parts_[2] = std::make_unique<Part>( *tenor );
            parts_[3] = std::make_unique<Part>( *bass );
        }

        bool build( bool verbose, bool noEOM );

    private:
        std::ostream& show_current_tokens( std::ostream& os ) const;

        // get the next token for each part if one is needed
        // possible errors:
        //     a part has no more tokens
        //     one part has a marker whereas another part has a rest or note
        // a message is printed and the function returns false if an error occur
        bool get_next_tokens();

        // reduce the duration of a token by the given number of ticks
        // if we have exhausted the duration of this token, set needNewToken to true
        void reduce_duration(Note& note, const unsigned int reduction, bool& needNewToken);

        // add marker to the combined parts unless it is EOM and noEOM was specified
        void process_marker( const std::unique_ptr<Encoding>& token,  bool verbose, bool noEOM  );

        // build chord from the current tokens
        void add_chord( bool verbose );
};