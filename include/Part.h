#pragma once

#include "Encoding.h"
#include "TranspositionRule.h"
#include "XmlUtils.h"

#include <map>
#include <string>
#include <tinyxml2.h>
#include <vector>
#include <memory>

// class containing info about a single voice part
class Part {
    public:
        enum Mode {
            MAJOR,
            MINOR
        };

        // parameters for the header of the encoding
        static inline const std::string SOH = "[";
        static inline const std::string ID = "ID: ";  
        static inline const std::string PART = "PART: ";
        static inline const std::string KEY = "KEY: ";
        static inline const std::string BEATS = "BEATS: ";
        static inline const std::string SUB_BEATS = "SUB-BEATS: ";
        static inline const std::string EOH = "]";
        static inline const std::string DELIM = ", ";
        static inline const std::string MAJOR_STR = "Major";
        static inline const std::string MINOR_STR = "Minor";

    private:
        // for converting key_ to a displayable string
        static constexpr const char* circle_of_fifths_[] = {
           "Gb", "Db", "Ab", "Eb", "Bb", "F", "C", "G", "D", "A", "E", "B", "F#", "C#", "G#", "D#", "A#",
        };
        // the index of C in the above circle of fifths array (evaluated at compile time)
        static consteval size_t index_of_C() {
            for (size_t i = 0; i < sizeof(circle_of_fifths_)/sizeof(circle_of_fifths_[0]); i++) {
                if (circle_of_fifths_[i] == "C") 
                   return i;
            }
            return static_cast<size_t>( -1 ); 
        }

        std::string id_;            // identifier for piece this part belongs to, e.g. "BWV 10.1"
        std::string title_;         // title of piece if provided, e.g. "Jesu, meine Freude"
        std::string partName_;      // name of part within the piece, e.g. "Soprano"     

        // if we are in 4/4 time, beatsPerMeasure_ is 4
        // subBeats_ represents the granularity
        // if the shortest note in 4/4 time is an eighth note, subBeats_ is 2
        unsigned int beatsPerMeasure_ = 0;   
        unsigned int subBeats_ = 0;  

        // a negative number for key_ represents the number of flats in the key signature  
        // a positive number represents the number of sharps in the key signature
        // we normalize all parts so that key_ is 0
        int key_ = 0;   
        Mode mode_ = Mode::MAJOR;

        // encodings_ is a vector of words
        //  SOC: always the first word
        //  a note is presented in the format <pitch>.<octave>.<duration>
        //      <pitch> consists of a capital letter (A-G) or 'R' to indicate a rest,
        //            optionally followed by 1, 2, -1, or -2, indicating the number
        //            of half-steps above or below the given note
        //          if the pitch is tied from the previous beat, the token starts with a '+'
        //      <octave> is always 0 for a rest
        //      <duration> is a number indicating the number of subbeats the note or rest is held
        //  EOM: end of measure 
        //  EOP: end of phrase (follows EOM if a phrase ends at a measure)
        //  EOC: always the last word
        std::vector<std::unique_ptr<Encoding>> encodings_;

    public:
        Part() = default;
        Part(const std::string& id, const std::string& title, const std::string& partName) : id_{id}, title_{title}, partName_{partName} {}

        // these methods print an error to cerr and return false if they faile

        // parse the MusicXML 'Part' element  
        bool parse_xml( tinyxml2::XMLElement* part );
        // parse the encoding (performed on the musicXml in a previous run) 
        bool parse_encoding( const std::string& part );
        // transpose part to the key with given number of sharps (if plus) or flats (if minus)
        bool transpose( int key = 0 );       

        // accessors to private variables
        std::string get_id() const { return id_; }
        std::string get_part_name() const { return partName_; }
        std::string get_header() const;
        int get_beats_per_measure() const { return beatsPerMeasure_; }
        int get_sub_beats() const { return subBeats_; }
        int get_key() const { return key_; }
        Mode get_mode() const { return mode_; }
        std::vector<std::unique_ptr<Encoding>>& get_line() { return encodings_; }

        void set_sub_beats( unsigned int subBeats );

        // access encodings_
        std::unique_ptr<Encoding> pop_encoding() {
            if (encodings_.empty()) {
                return nullptr;
            }
            else {
                auto first = std::move(encodings_.front());
                encodings_.erase(encodings_.begin());
                return first;
            }
        }
        void push_encoding( std::unique_ptr<Encoding>& encoding ) {
            encodings_.push_back( std::move(encoding) );
        }
        std::unique_ptr<Encoding>& get_last_encoding() {
            return encodings_.back();
        }

        // conversions to facillitate printing
        std::string key_to_string() const {
            return static_cast<std::string>( circle_of_fifths_[key_ + index_of_C() + (mode_ == Mode::MINOR ? 3 : 0)] ) 
                + "-" 
                + mode_to_string();
        }
        std::string mode_to_string() const {
            return (mode_ == Mode::MAJOR) ? MAJOR_STR : MINOR_STR;
        }
        std::string to_string() const;

        friend std::ostream& operator <<( std::ostream& os, const Part& part );

    private:
        // helper functions for parse_xml

        // parse the MusicXML 'attributes' element, set variables accordingly
        bool parse_attributes( tinyxml2::XMLElement* attributes );
        // parse the MusicXML 'measure' element, append words to encodings_ accordingly
        bool parse_measure( tinyxml2::XMLElement* measure );

        // returns the specified child of a given XML element or nullptr
        tinyxml2::XMLElement* try_get_child( tinyxml2::XMLElement* parent, const char* childName );

        // helper functions for parse_encoding

        // save info from header
        bool import_header( const std::string& header );
        std::string find_header_value( const std::string& header, const std::string& key ) const;
        bool import_encodings( const std::string& line );
        std::unique_ptr<Encoding> make_encoding( const std::string& encoding ) const;
        bool import_key( const std::string& keyString );

        // helper functions for transpose function

        // transpose to a key one level higher in the circle of fifths
        void transpose_up( Note& note ) {
            note.transpose( transposeUpRules );
        }
        // transpose to a key one level lower in the circle of fifths
        void transpose_down( Note& note ) {
            note.transpose( transposeDownRules );
        }
};